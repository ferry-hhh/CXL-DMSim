#include "base/trace.hh"
#include "dev/storage/cxl_memory.hh"
#include "debug/CxlMemory.hh"

namespace gem5
{

CxlMemory::CxlMemory(const Param &p)
    : PciDevice(p),
    mem_(RangeSize(p.BAR0->addr(), p.BAR0->size())),
    latency_(p.latency),
    cxl_mem_latency_(p.cxl_mem_latency) {}

Tick CxlMemory::read(PacketPtr pkt) {
    DPRINTF(CxlMemory, "read address : (%lx, %lx)", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    return latency_ + cxl_latency;
}

Tick CxlMemory::write(PacketPtr pkt) {
    DPRINTF(CxlMemory, "write address : (%lx, %lx)", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    return latency_ + cxl_latency;
}

AddrRangeList CxlMemory::getAddrRanges() const {
    return PciDevice::getAddrRanges();
}

Tick CxlMemory::resolve_cxl_mem(PacketPtr pkt) {
    if (pkt->cmd == MemCmd::ReadReq) {
        assert(pkt->isRead());
        assert(pkt->needsResponse());
    } else if (pkt->cmd == MemCmd::WriteReq) {
        assert(pkt->isWrite());
        assert(pkt->needsResponse());
    }
    return cxl_mem_latency_;
}

CxlMemory::Memory::Memory(const AddrRange& range) : range(range) {
    pmemAddr = new uint8_t[range.size()];
}

void CxlMemory::Memory::access(PacketPtr pkt) {
    range = AddrRange(0x100000000, 0x100000000 + 0x7000000);
    if (pkt->cacheResponding()) {
        DPRINTF(CxlMemory, "Cache responding to %#llx: not responding\n", pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(CxlMemory, "CleanEvict  on 0x%x: not responding\n", pkt->getAddr());
        return;
    }
    
    assert(pkt->getAddrRange().isSubset(range));

    uint8_t* host_addr = toHostAddr(pkt->getAddr());

    if (pkt->cmd == MemCmd::SwapReq) {

        if (pkt->isAtomicOp()) {
            if (pmemAddr) {
                pkt->setData(host_addr);
                (*(pkt->getAtomicOp()))(host_addr);
            }
        } else {
            std::vector<uint8_t> overwrite_val(pkt->getSize());
            uint64_t condition_val64;
            uint32_t condition_val32;

            panic_if(!pmemAddr,
                     "Swap only works if there is real memory "
                     "(i.e. null=False)");

            bool overwrite_mem = true;
            // keep a copy of our possible write value, and copy what is at the
            // memory address into the packet
            pkt->writeData(&overwrite_val[0]);
            pkt->setData(host_addr);

            if (pkt->req->isCondSwap()) {
                if (pkt->getSize() == sizeof(uint64_t)) {
                    condition_val64 = pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val64, host_addr, sizeof(uint64_t));
                } else if (pkt->getSize() == sizeof(uint32_t)) {
                    condition_val32 = (uint32_t)pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val32, host_addr, sizeof(uint32_t));
                } else
                    panic("Invalid size for conditional read/write\n");
            }

            if (overwrite_mem)
                std::memcpy(host_addr, &overwrite_val[0], pkt->getSize());

            assert(!pkt->req->isInstFetch());
        }
    } else if (pkt->isRead()) {
        assert(!pkt->isWrite());
        if (pmemAddr) {
            pkt->setData(host_addr);
        }
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        assert(!pkt->isWrite());
        // in a fastmem system invalidating and/or cleaning packets
        // can be seen due to cache maintenance requests

        // no need to do anything
    } else if (pkt->isWrite()) {
        printf("access run 125\n");
        if (pmemAddr) {
            pkt->writeData(host_addr);
            DPRINTF(CxlMemory, "%s write due to %s\n", __func__, pkt->print());
        }
        assert(!pkt->req->isInstFetch());
    } else {
        panic("Unexpected packet %s", pkt->print());
    }
    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
}

} // namespace gem5
