#ifndef __DEV_STORAGE_CXL_MEMORY_HH__
#define __DEV_STORAGE_CXL_MEMORY_HH__

#include <deque>

#include "base/addr_range.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "dev/pci/device.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "mem/port.hh"
#include "params/CXLMemory.hh"
#include "sim/clocked_object.hh"


namespace gem5
{

class CXLMemory : public PciDevice 
{
    protected:

        /**
        * A deferred packet stores a packet along with its scheduled
        * transmission time
        */
        class DeferredPacket
        {
        public:
            const Tick tick;
            const PacketPtr pkt;

            DeferredPacket(PacketPtr _pkt, Tick _tick) : tick(_tick), pkt(_pkt)
            { }
        };

        // Forward declaration to allow the response port to have a pointer
        class CXLRequestPort;

        /**
        * The port on the side that receives requests and sends
        * responses. The response port also has a buffer for the
        * responses not yet sent.
        */
        class CXLResponsePort : public ResponsePort
        {
            private:

                /** The CXLMemory to which this port belongs. */
                CXLMemory& cxlMemory;

                /**
                * Request port on which CXLMemory sends requests to the back-end memory media.
                */
                CXLRequestPort& memReqPort;

                /** Latency in protocol processing by CXLMemory. */
                const Cycles protoProcLat;

                /** Address ranges to pass through the CXLMemory */
                const AddrRange cxlMemRange;

                /**
                * Response packet queue. Response packets are held in this
                * queue for a specified delay to model the processing delay
                * of the CXLMemory.
                */
                std::deque<DeferredPacket> transmitList;

                /** Counter to track the outstanding responses. */
                unsigned int outstandingResponses;

                /** If we should send a retry when space becomes available. */
                bool retryReq;

                /** Max queue size for reserved responses. */
                unsigned int respQueueLimit;

                /**
                * Upstream caches need this packet until true is returned, so
                * hold it for deletion until a subsequent call
                */
                std::unique_ptr<Packet> pendingDelete;

                /**
                * Is this side blocked from accepting new response packets.
                *
                * @return true if the reserved space has reached the set limit
                */
                bool respQueueFull() const;

                /**
                * Handle send event, scheduled when the packet at the head of
                * the response queue is ready to transmit (for timing
                * accesses only).
                */
                void trySendTiming();

                /** Send event for the response queue. */
                EventFunctionWrapper sendEvent;

            public:
                /**
                * Constructor for the CXLResponsePort.
                *
                * @param _name the port name including the owner
                * @param _cxlMemory the structural owner
                * @param _memReqPort the request port of CXLMemory
                * @param _protoProcLat the delay in cycles from receiving to sending
                * @param _resp_limit the size of the response queue
                * @param _cxlMemRange the address range of the CXLMemory
                */
                CXLResponsePort(const std::string& _name, CXLMemory& _cxlMemory,
                                CXLRequestPort& _memReqPort, Cycles _protoProcLat,
                                int _resp_limit, AddrRange _cxlMemRange);

                /**
                * Queue a response packet to be sent out later and also schedule
                * a send if necessary.
                *
                * @param pkt a response to send out after a delay
                * @param when tick when response packet should be sent
                */
                void schedTimingResp(PacketPtr pkt, Tick when);

                /**
                * Retry any stalled request that we have failed to accept at
                * an earlier point in time. This call will do nothing if no
                * request is waiting.
                */
                void retryStalledReq();

            // protected:
                /** When receiving a timing request from the Host,
                    pass it to the back-end memory media. */
                bool recvTimingReq(PacketPtr pkt) override;

                /** When receiving a retry request from the Host,
                    pass it to the back-end memory media. */
                void recvRespRetry() override;

                /** When receiving an Atomic request from the Host,
                    pass it to the back-end memory media. */
                Tick recvAtomic(PacketPtr pkt) override;

                Tick recvAtomicBackdoor(
                    PacketPtr pkt, MemBackdoorPtr &backdoor) override;

                void recvMemBackdoorReq(
                    const MemBackdoorReq &req, MemBackdoorPtr &backdoor) override {};

                void recvFunctional(PacketPtr pkt) override {};

                /** When receiving a address range request the Host,
                    pass it to the back-end memory media. */
                AddrRangeList getAddrRanges() const override;

                Cycles processCXLMem(PacketPtr ptk);
        };


        /**
        * Port on the side that forwards requests to and receives 
        * responses from back-end memory media. The request port 
        * has a buffer for the requests not yet sent.
        */
        class CXLRequestPort : public RequestPort
        {
            private:
                /** The CXLMemory to which this port belongs. */
                CXLMemory& cxlMemory;

                /**
                * The response port on the other side of the CXLMemory.
                */
                CXLResponsePort& cxlRspPort;

                /** Latency in protocol processing by CXLMemory. */
                const Cycles protoProcLat;

                /**
                * Request packet queue. Request packets are held in this
                * queue for a specified delay to model the processing delay
                * of the CXLMemory.
                */
                std::deque<DeferredPacket> transmitList;

                /** Max queue size for request packets */
                const unsigned int reqQueueLimit;

                /**
                * Handle send event, scheduled when the packet at the head of
                * the outbound queue is ready to transmit (for timing
                * accesses only).
                */
                void trySendTiming();

                /** Send event for the request queue. */
                EventFunctionWrapper sendEvent;

            public:
                /**
                * Constructor for the CXLRequestPort.
                *
                * @param _name the port name including the owner
                * @param _cxlMemory the structural owner
                * @param _cxlRspPort the response port of CXLMemory
                * @param _protoProcLat the delay in cycles from receiving to sending
                * @param _req_limit the size of the request queue
                */
                CXLRequestPort(const std::string& _name, CXLMemory& _cxlMemory,
                                CXLResponsePort& _cxlRspPort, Cycles _protoProcLat,
                                int _req_limit);

                /**
                * Is this side blocked from accepting new request packets.
                *
                * @return true if the occupied space has reached the set limit
                */
                bool reqQueueFull() const;

                /**
                * Queue a request packet to be sent out later and also schedule
                * a send if necessary.
                *
                * @param pkt a request to send out after a delay
                * @param when tick when response packet should be sent
                */
                void schedTimingReq(PacketPtr pkt, Tick when);

            protected:
                /** When receiving a timing request from the back-end memory media,
                    pass it to the Host. */
                bool recvTimingResp(PacketPtr pkt) override;

                /** When receiving a retry request from the back-end memory media,
                    pass it to the Host. */
                void recvReqRetry() override;
        };

        /** Response port of the CXLMemory. */
        CXLResponsePort cxlRspPort;

        /** Request port of the CXLMemory. */
        CXLRequestPort memReqPort;

    public:
        Tick read(PacketPtr pkt) override {
            return cxlRspPort.recvAtomic(pkt);
        }
        Tick write(PacketPtr pkt) override {
            return cxlRspPort.recvAtomic(pkt);
        }
        Port &getPort(const std::string &if_name,
            PortID idx=InvalidPortID) override;

        void init() override;

        AddrRangeList getAddrRanges() const override;

        PARAMS(CXLMemory);
        CXLMemory(const Params &p);
};

} // namespace gem5

#endif // __DEV_STORAGE_CXL_MEMORY_HH__