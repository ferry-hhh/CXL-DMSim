#include "base/trace.hh"
#include "dev/storage/cxl_memory.hh"
#include "debug/CXLMemory.hh"

namespace gem5
{

CXLMemory::CXLMemory(const Param &p)
    : PciDevice(p),
    ctrl(p.ctrl),
    deviceProtoProcLat(ticksToCycles(p.device_proto_proc_lat)),
    cxlMemRange(p.cxl_mem_range)
    {
        ctrl->setPioPort(&pioPort);
        DPRINTF(CXLMemory, "BAR0_addr:0x%lx, BAR0_size:0x%lx\n", p.BAR0->addr(),
            p.BAR0->size());
    }

Tick CXLMemory::read(PacketPtr pkt) {
    DPRINTF(CXLMemory, "read address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = processCXLMem(pkt);
    Tick mem_latency = ctrl->recvAtomic(pkt);
    Tick read_latency = cxl_latency * this->clockPeriod() + mem_latency;
    DPRINTF(CXLMemory, "read latency = %llu\n", read_latency);
    return read_latency;
}

Tick CXLMemory::write(PacketPtr pkt) {
    DPRINTF(CXLMemory, "write address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = processCXLMem(pkt);
    Tick mem_latency = ctrl->recvAtomic(pkt);
    Tick write_latency = cxl_latency * this->clockPeriod() + mem_latency;
    DPRINTF(CXLMemory, "write latency = %llu\n", write_latency);
    return write_latency;
}

AddrRangeList CXLMemory::getAddrRanges() const {
    AddrRangeList ranges = PciDevice::getAddrRanges();
    ranges.push_back(cxlMemRange);
    return ranges;
}

Tick CXLMemory::processCXLMem(PacketPtr pkt) {
    if (pkt->cxl_cmd == MemCmd::M2SReq) {
        assert(pkt->isRead());
    } else if (pkt->cxl_cmd == MemCmd::M2SRwD) {
        assert(pkt->isWrite());
    }
    return deviceProtoProcLat * 2;
}

} // namespace gem5
