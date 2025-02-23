#include "dev/storage/cxl_mem_ctrl.hh"
#include "mem/dram_interface.hh"
#include "mem/mem_interface.hh"
#include "mem/nvm_interface.hh"
#include "debug/Drain.hh"
#include "debug/CXLMemCtrl.hh"

namespace gem5
{

CXLMemCtrl::CXLMemCtrl(const CXLMemCtrlParams &p) :
    memory::MemCtrl::MemCtrl(p)
{
    DPRINTF(CXLMemCtrl, "Setting up cxl memory controller");
    port.unbind();
    dram->setCtrl(this, commandWindow);
}

void CXLMemCtrl::setCXLPort(CXLPort<CXLMemory>* _cxlPort)
{
    cxlPort = _cxlPort;
}

void
CXLMemCtrl::init()
{
    DPRINTF(CXLMemCtrl, "cxl memory controller init");
}

void
CXLMemCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency,
                                                memory::MemInterface* mem_intr)
{
    DPRINTF(CXLMemCtrl, "Responding to Address %#x.. \n", pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    if (!mem_intr->getAddrRange().contains(pkt->getAddr())) {
        panic("Can't handle address range for packet %s\n", pkt->print());
    }
    // panic_if(!mem_intr->getAddrRange().contains(pkt->getAddr()),
    //          "Can't handle address range for packet %s\n", pkt->print());
    mem_intr->access(pkt);

    // turn packet around to go back to requestor if response expected
    if (needsResponse) {
        // access already turned the packet into a response
        assert(pkt->isResponse());
        // response_time consumes the static latency and is charged also
        // with headerDelay that takes into account the delay provided by
        // the xbar and also the payloadDelay that takes into account the
        // number of data beats.

        Tick response_time = curTick() + static_latency + pkt->headerDelay +
                             pkt->payloadDelay;
        // Here we reset the timing of the packet before sending it out.
        pkt->headerDelay = pkt->payloadDelay = 0;
        
        DPRINTF(CXLMemCtrl, "Responding packet to time %#x.. %llu\n", pkt->getAddr(), response_time);
        // queue the packet in the response queue to be sent out after
        // the static latency has passed
        cxlPort->schedTimingResp(pkt, response_time);
        // warn("cxl cannot send response");
    } else {
        // @todo the packet is going to be deleted, and the MemPacket
        // is still having a pointer to it
        pendingDelete.reset(pkt);
    }

    DPRINTF(CXLMemCtrl, "Done\n");

    return;
}


void
CXLMemCtrl::processRespondEvent(memory::MemInterface* mem_intr,
                        MemPacketQueue& queue,
                        EventFunctionWrapper& resp_event,
                        bool& retry_rd_req)
{

    DPRINTF(CXLMemCtrl,
            "processRespondEvent(): Some req has reached its readyTime\n");

    memory::MemPacket* mem_pkt = queue.front();

    // media specific checks and functions when read response is complete
    // DRAM only
    mem_intr->respondEvent(mem_pkt->rank);

    if (mem_pkt->burstHelper) {
        // it is a split packet
        mem_pkt->burstHelper->burstsServiced++;
        if (mem_pkt->burstHelper->burstsServiced ==
            mem_pkt->burstHelper->burstCount) {
            // we have now serviced all children packets of a system packet
            // so we can now respond to the requestor
            // @todo we probably want to have a different front end and back
            // end latency for split packets
            accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency,
                             mem_intr);
            delete mem_pkt->burstHelper;
            mem_pkt->burstHelper = NULL;
        }
    } else {
        // it is not a split packet
        accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency,
                         mem_intr);
    }

    queue.pop_front();

    if (!queue.empty()) {
        assert(queue.front()->readyTime >= curTick());
        assert(!resp_event.scheduled());
        schedule(resp_event, queue.front()->readyTime);
    } else {
        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !totalWriteQueueSize && !totalReadQueueSize &&
            allIntfDrained()) {

            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            // DRAM only
            mem_intr->checkRefreshState(mem_pkt->rank);
        }
    }

    delete mem_pkt;

    // We have made a location in the queue available at this point,
    // so if there is a read that was forced to wait, retry now
    if (retry_rd_req) {
        retry_rd_req = false;
        // warn("cxl cannot send response");
        cxlPort->sendRetryReq();
    }
}

void
CXLMemCtrl::processNextReqEvent(memory::MemInterface* mem_intr,
                        MemPacketQueue& resp_queue,
                        EventFunctionWrapper& resp_event,
                        EventFunctionWrapper& next_req_event,
                        bool& retry_wr_req) {
    // transition is handled by QoS algorithm if enabled
    if (turnPolicy) {
        // select bus state - only done if QoS algorithms are in use
        busStateNext = selectNextBusState();
    }

    // detect bus state change
    bool switched_cmd_type = (mem_intr->busState != mem_intr->busStateNext);
    // record stats
    recordTurnaroundStats(mem_intr->busState, mem_intr->busStateNext);

    DPRINTF(CXLMemCtrl, "QoS Turnarounds selected state %s %s\n",
            (mem_intr->busState==memory::MemCtrl::READ)?"READ":"WRITE",
            switched_cmd_type?"[turnaround triggered]":"");
    
    if (switched_cmd_type) {
        if (mem_intr->busState == memory::MemCtrl::READ) {
            DPRINTF(CXLMemCtrl,
            "Switching to writes after %d reads with %d reads "
            "waiting\n", mem_intr->readsThisTime, mem_intr->readQueueSize);
            stats.rdPerTurnAround.sample(mem_intr->readsThisTime);
            mem_intr->readsThisTime = 0;
        } else {
            DPRINTF(CXLMemCtrl,
            "Switching to reads after %d writes with %d writes "
            "waiting\n", mem_intr->writesThisTime, mem_intr->writeQueueSize);
            stats.wrPerTurnAround.sample(mem_intr->writesThisTime);
            mem_intr->writesThisTime = 0;
        }
    }

    if (drainState() == DrainState::Draining && !totalWriteQueueSize &&
        !totalReadQueueSize && respQEmpty() && allIntfDrained()) {

        DPRINTF(Drain, "MemCtrl controller done draining\n");
        signalDrainDone();
    }

    // updates current state
    mem_intr->busState = mem_intr->busStateNext;

    nonDetermReads(mem_intr);

    if (memBusy(mem_intr)) {
        return;
    }

    // when we get here it is either a read or a write
    if (mem_intr->busState == READ) {

        // track if we should switch or not
        bool switch_to_writes = false;

        if (mem_intr->readQueueSize == 0) {
            // In the case there is no read request to go next,
            // trigger writes if we have passed the low threshold (or
            // if we are draining)
            if (!(mem_intr->writeQueueSize == 0) &&
                (drainState() == DrainState::Draining ||
                 mem_intr->writeQueueSize > writeLowThreshold)) {

                DPRINTF(CXLMemCtrl,
                        "Switching to writes due to read queue empty\n");
                switch_to_writes = true;
            } else {
                // check if we are drained
                // not done draining until in PWR_IDLE state
                // ensuring all banks are closed and
                // have exited low power states
                if (drainState() == DrainState::Draining &&
                    respQEmpty() && allIntfDrained()) {

                    DPRINTF(Drain, "cxl MemCtrl controller done draining\n");
                    signalDrainDone();
                }

                // nothing to do, not even any point in scheduling an
                // event for the next request
                return;
            }
        } else {

            bool read_found = false;
            MemPacketQueue::iterator to_read;
            uint8_t prio = numPriorities();

            for (auto queue = readQueue.rbegin();
                 queue != readQueue.rend(); ++queue) {

                prio--;

                DPRINTF(QOS,
                        "Checking READ queue [%d] priority [%d elements]\n",
                        prio, queue->size());

                // Figure out which read request goes next
                // If we are changing command type, incorporate the minimum
                // bus turnaround delay which will be rank to rank delay
                to_read = chooseNext((*queue), switched_cmd_type ?
                                     minWriteToReadDataGap() : 0, mem_intr);

                if (to_read != queue->end()) {
                    // candidate read found
                    read_found = true;
                    break;
                }
            }

            // if no read to an available rank is found then return
            // at this point. There could be writes to the available ranks
            // which are above the required threshold. However, to
            // avoid adding more complexity to the code, return and wait
            // for a refresh event to kick things into action again.
            if (!read_found) {
                DPRINTF(CXLMemCtrl, "No Reads Found - exiting\n");
                return;
            }

            auto mem_pkt = *to_read;

            Tick cmd_at = doBurstAccess(mem_pkt, mem_intr);

            DPRINTF(CXLMemCtrl,
            "Command for %#x, issued at %lld.\n", mem_pkt->addr, cmd_at);

            // sanity check
            assert(pktSizeCheck(mem_pkt, mem_intr));
            assert(mem_pkt->readyTime >= curTick());

            // log the response
            logResponse(memory::MemCtrl::READ, (*to_read)->requestorId(),
                        mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                        mem_pkt->readyTime - mem_pkt->entryTime);

            mem_intr->readQueueSize--;

            // Insert into response queue. It will be sent back to the
            // requestor at its readyTime
            if (resp_queue.empty()) {
                assert(!resp_event.scheduled());
                schedule(resp_event, mem_pkt->readyTime);
            } else {
                assert(resp_queue.back()->readyTime <= mem_pkt->readyTime);
                assert(resp_event.scheduled());
            }

            resp_queue.push_back(mem_pkt);

            // we have so many writes that we have to transition
            // don't transition if the writeRespQueue is full and
            // there are no other writes that can issue
            // Also ensure that we've issued a minimum defined number
            // of reads before switching, or have emptied the readQ
            if ((mem_intr->writeQueueSize > writeHighThreshold) &&
               (mem_intr->readsThisTime >= minReadsPerSwitch ||
               mem_intr->readQueueSize == 0)
               && !(nvmWriteBlock(mem_intr))) {
                switch_to_writes = true;
            }

            // remove the request from the queue
            // the iterator is no longer valid .
            readQueue[mem_pkt->qosValue()].erase(to_read);
        }

        // switching to writes, either because the read queue is empty
        // and the writes have passed the low threshold (or we are
        // draining), or because the writes hit the hight threshold
        if (switch_to_writes) {
            // transition to writing
            mem_intr->busStateNext = WRITE;
        }
    } else {

        bool write_found = false;
        MemPacketQueue::iterator to_write;
        uint8_t prio = numPriorities();

        for (auto queue = writeQueue.rbegin();
             queue != writeQueue.rend(); ++queue) {

            prio--;

            DPRINTF(QOS,
                    "Checking WRITE queue [%d] priority [%d elements]\n",
                    prio, queue->size());

            // If we are changing command type, incorporate the minimum
            // bus turnaround delay
            to_write = chooseNext((*queue),
                    switched_cmd_type ? minReadToWriteDataGap() : 0, mem_intr);

            if (to_write != queue->end()) {
                write_found = true;
                break;
            }
        }

        // if there are no writes to a rank that is available to service
        // requests (i.e. rank is in refresh idle state) are found then
        // return. There could be reads to the available ranks. However, to
        // avoid adding more complexity to the code, return at this point and
        // wait for a refresh event to kick things into action again.
        if (!write_found) {
            DPRINTF(CXLMemCtrl, "No Writes Found - exiting\n");
            return;
        }

        auto mem_pkt = *to_write;

        // sanity check
        assert(pktSizeCheck(mem_pkt, mem_intr));

        Tick cmd_at = doBurstAccess(mem_pkt, mem_intr);
        DPRINTF(CXLMemCtrl,
        "Command for %#x, issued at %lld.\n", mem_pkt->addr, cmd_at);

        isInWriteQueue.erase(burstAlign(mem_pkt->addr, mem_intr));

        // log the response
        logResponse(memory::MemCtrl::WRITE, mem_pkt->requestorId(),
                    mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                    mem_pkt->readyTime - mem_pkt->entryTime);

        mem_intr->writeQueueSize--;

        // remove the request from the queue - the iterator is no longer valid
        writeQueue[mem_pkt->qosValue()].erase(to_write);

        delete mem_pkt;

        // If we emptied the write queue, or got sufficiently below the
        // threshold (using the minWritesPerSwitch as the hysteresis) and
        // are not draining, or we have reads waiting and have done enough
        // writes, then switch to reads.
        // If we are interfacing to NVM and have filled the writeRespQueue,
        // with only NVM writes in Q, then switch to reads
        bool below_threshold =
            mem_intr->writeQueueSize + minWritesPerSwitch < writeLowThreshold;

        if (mem_intr->writeQueueSize == 0 ||
            (below_threshold && drainState() != DrainState::Draining) ||
            (mem_intr->readQueueSize && mem_intr->writesThisTime >= minWritesPerSwitch) ||
            (mem_intr->readQueueSize && (nvmWriteBlock(mem_intr)))) {

            // turn the bus back around for reads again
            mem_intr->busStateNext = memory::MemCtrl::READ;

            // note that the we switch back to reads also in the idle
            // case, which eventually will check for any draining and
            // also pause any further scheduling if there is really
            // nothing to do
        }
    }
    // It is possible that a refresh to another rank kicks things back into
    // action before reaching this point.
    if (!next_req_event.scheduled())
        schedule(next_req_event, std::max(mem_intr->nextReqTime, curTick()));

    if (retry_wr_req && mem_intr->writeQueueSize < writeBufferSize) {
        retry_wr_req = false;
        cxlPort->sendRetryReq();
        // warn("cxl cannot send response");
    }
}

}