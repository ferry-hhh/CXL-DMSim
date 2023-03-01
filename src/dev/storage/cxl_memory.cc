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
    printf("start read\n");
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    printf("read latency_ + cxl_latency=%ld\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}

Tick CxlMemory::write(PacketPtr pkt) {
    printf("start write\n");
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    printf("read latency_ + cxl_latency=%ld\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}

AddrRangeList CxlMemory::getAddrRanges() const {
    printf("getAddrRanges……\n");
    return PciDevice::getAddrRanges();
}

Tick CxlMemory::resolve_cxl_mem(PacketPtr pkt) {
    if (pkt->cmd == MemCmd::M2SReq) {
        assert(pkt->isRead());
        assert(pkt->needsResponse());
    } else if (pkt->cmd == MemCmd::M2SRwD) {
        assert(pkt->isWrite());
        assert(pkt->needsResponse());
    }
    return cxl_mem_latency_;
}

CxlMemory::Memory::Memory(const AddrRange& range) : range(range) {
    printf("CXLMemory::Memory init start!\n");
    printf("range start:%lx, size:%lx, end:%lx\n", range.start(), range.size(), range.end());
    try
    {
        pmemAddr = new uint8_t[range.size()];
    }
    catch(const std::bad_alloc& e)
    {
        printf("memory error");
        std::cerr << e.what() << '\n';
    }
    printf("CXLMemory::Memory init finish!\n");
}

void CxlMemory::Memory::access(PacketPtr pkt) {
    printf("access start\n");
    if (pkt->cacheResponding()) {
        DPRINTF(CxlMemory, "Cache responding to %#llx: not responding\n", pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(CxlMemory, "CleanEvict  on 0x%x: not responding\n", pkt->getAddr());
        return;
    }
    
    assert(pkt->getAddrRange().isSubset(range));
    printf("access run 70\n");
    uint8_t* host_addr = toHostAddr(pkt->getAddr());
    printf("access run 72\n");
    if (pkt->cmd == MemCmd::SwapReq) {
        printf("access run 74\n");
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
        printf("access run 112\n");
        assert(!pkt->isWrite());
        if (pmemAddr) {
            pkt->setData(host_addr);
        }
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        printf("access run 118\n");
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
        printf("access run 132\n");
        panic("Unexpected packet %s", pkt->print());
    }
    printf("access run 135\n");
    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
    printf("access end\n");
    
}

} // namespace gem5
