#ifndef __CXL_MEM_CTRL_HH__
#define __CXL_MEM_CTRL_HH__

#include "mem/mem_ctrl.hh"
#include "params/CXLMemCtrl.hh"
#include "dev/io_device.hh"

namespace gem5
{

namespace memory
{
  class MemInterface;
  class DRAMInterface;
  class NVMInterface;
}

typedef std::deque<memory::MemPacket*> MemPacketQueue;

class CXLMemCtrl : public memory::MemCtrl
{
  public:
    CXLMemCtrl(const CXLMemCtrlParams &p);
    PioPort<PioDevice>* pioPort;
    void setPioPort(PioPort<PioDevice>* _pioPort);

    void init() override;

    Tick recvAtomic(PacketPtr pkt) override;


  protected:
    void processRespondEvent(memory::MemInterface* mem_intr,
                        MemPacketQueue& queue,
                        EventFunctionWrapper& resp_event,
                        bool& retry_rd_req) override;
    
    void accessAndRespond(PacketPtr pkt, Tick static_latency,
                          memory::MemInterface* mem_intr) override;
    
    void processNextReqEvent(memory::MemInterface* mem_intr,
                          MemPacketQueue& resp_queue,
                          EventFunctionWrapper& resp_event,
                          EventFunctionWrapper& next_req_event,
                          bool& retry_wr_req) override;

};

}

#endif