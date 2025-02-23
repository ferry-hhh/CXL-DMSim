#include "base/trace.hh"
#include "dev/storage/cxl_memory.hh"
#include "debug/CXLMemory.hh"

namespace gem5
{

CXLMemory::CXLMemory(const Param &p)
    : PciDevice(p),
    cxlPort(this),
    ctrl(p.ctrl),
    deviceProtoProcLat(ticksToCycles(p.device_proto_proc_lat)),
    cxlMemRange(p.cxl_mem_range)
    {
        ctrl->setCXLPort(&cxlPort);
        DPRINTF(CXLMemory, "BAR0_addr:0x%lx, BAR0_size:0x%lx\n", p.BAR0->addr(),
            p.BAR0->size());
    }

Tick CXLMemory::read(PacketPtr pkt) {
    DPRINTF(CXLMemory, "read address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = processCXLMem(pkt);
    Tick mem_latency = ctrl->publicRecvAtomic(pkt);
    Tick read_latency = cxl_latency * this->clockPeriod() + mem_latency;
    // DPRINTF(CXLMemory, "read latency = %llu\n", read_latency);
    return read_latency;
}

bool CXLMemory::read_timing(PacketPtr pkt) {
    DPRINTF(CXLMemory, "read timing address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    return ctrl->publicRecvTimingReq(pkt);
}

Tick CXLMemory::write(PacketPtr pkt) {
    DPRINTF(CXLMemory, "write address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = processCXLMem(pkt);
    Tick mem_latency = ctrl->publicRecvAtomic(pkt);
    Tick write_latency = cxl_latency * this->clockPeriod() + mem_latency;
    // DPRINTF(CXLMemory, "write latency = %llu\n", write_latency);
    return write_latency;
}

bool CXLMemory::write_timing(PacketPtr pkt) {
    DPRINTF(CXLMemory, "write timing address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    return ctrl->publicRecvTimingReq(pkt);
}

AddrRangeList CXLMemory::getAddrRanges() const {
    AddrRangeList ranges = PciDevice::getAddrRanges();
    ranges.push_back(cxlMemRange);
    return ranges;
}

Port & CXLMemory::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "pio") {
        return cxlPort;
    }
    else if (if_name == "dma") {
        return dmaPort;
    }
    return PioDevice::getPort(if_name, idx);
}

void
CXLMemory::init()
{
    if (!cxlPort.isConnected())
        panic("CXL port of %s not connected to anything!", name());
    cxlPort.sendRangeChange();
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
