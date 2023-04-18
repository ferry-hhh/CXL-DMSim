#include "base/trace.hh"
#include "dev/storage/cxl_memory.hh"
#include "debug/CxlMemory.hh"

namespace gem5
{

CxlMemory::CxlMemory(const Param &p)
    : PciDevice(p),
    mem_(RangeSize(p.BAR0->addr(), p.BAR0->size()), *this),
    latency_(p.latency),
    cxl_mem_latency_(p.cxl_mem_latency) {}

Tick CxlMemory::read(PacketPtr pkt) {
    DPRINTF(CxlMemory, "read address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    DPRINTF(CxlMemory, "read latency = %llu\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}

Tick CxlMemory::write(PacketPtr pkt) {
    DPRINTF(CxlMemory, "write address : (%lx, %lx)\n", pkt->getAddr(),
            pkt->getSize());
    Tick cxl_latency = resolve_cxl_mem(pkt);
    mem_.access(pkt);
    DPRINTF(CxlMemory, "write latency = %llu\n", latency_ + cxl_latency);
    return latency_ + cxl_latency;
}

AddrRangeList CxlMemory::getAddrRanges() const {
    return PciDevice::getAddrRanges();
}

Tick CxlMemory::resolve_cxl_mem(PacketPtr pkt) {
    // if (pkt->cmd == MemCmd::M2SReq) {
    if (pkt->cmd == MemCmd::ReadReq) {
        assert(pkt->isRead());
        assert(pkt->needsResponse());
    // } else if (pkt->cmd == MemCmd::M2SRwD) {
    } else if (pkt->cmd == MemCmd::WriteReq) {
        assert(pkt->isWrite());
        assert(pkt->needsResponse());
    }
    return cxl_mem_latency_;
}

CxlMemory::Memory::Memory(const AddrRange& range, CxlMemory& owner)
    : range(range),
    owner(owner) {
    pmemAddr = new uint8_t[range.size()];
    DPRINTF(CxlMemory, "initial range start=0x%lx, range size=0x%lx\n", range.start(), range.size());
}

void CxlMemory::Memory::access(PacketPtr pkt) {
    PciBar *bar = owner.BARs[0];
    range = RangeSize(bar->addr(), bar->size());
    DPRINTF(CxlMemory, "final range start=0x%lx, range size=0x%lx\n", range.start(), range.size());
    // range = AddrRange(0x100000000, 0x100000000 + 0x100000000); // 0x8000000=128MiB 0x100000000=4GiB
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
    DPRINTF(CxlMemory, "host_addr = %p, pkt->getAddr = 0x%lx, pmemAddr = %p, range.start = 0x%lx\n", 
        *host_addr, pkt->getAddr(), *pmemAddr, range.start());
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
            pkt->writeData(&overwrite_val[0]);  // Write the data of the pkt to the vector
            pkt->setData(host_addr);            // Write the data of host_addr to pkt

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
            DPRINTF(CxlMemory, "%s read due to %s\n", __func__, pkt->print());
            // std::cout << "read data = " << pkt->getPtr<uint8_t>() << "\n";
        }
    } else if (pkt->isInvalidate() || pkt->isClean()) {
        assert(!pkt->isWrite());
        // in a fastmem system invalidating and/or cleaning packets
        // can be seen due to cache maintenance requests

        // no need to do anything
    } else if (pkt->isWrite()) {
        if (pmemAddr) {
            pkt->writeData(host_addr);
            DPRINTF(CxlMemory, "%s write due to %s\n", __func__, pkt->print());
            // std::cout << "write data = " << pkt->getPtr<uint8_t>() << "\n";
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
