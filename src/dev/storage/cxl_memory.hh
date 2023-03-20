#include "base/addr_range.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/CxlMemory.hh"
#include "dev/pci/device.hh"

namespace gem5
{
    class CxlMemory : public PciDevice {
        private:
        class Memory {
            private:
            AddrRange range;
            uint8_t* pmemAddr = nullptr;
            bool inAddrMap = true;
            const std::string name_ = "CxlMemory::Memory";
            CxlMemory& owner;
            public:

        public:
        Memory(const AddrRange& range, CxlMemory& owner);
        inline uint8_t* toHostAddr(Addr addr) const { return pmemAddr + addr - range.start(); } // 这里计算的地址其实是要读/写的pmem的地址
        const std::string& name() const { return name_; }
        uint64_t size() const { return range.size(); }
        Addr start() const { return range.start(); }
        bool isInAddrMap() const { return inAddrMap; }
        void access(PacketPtr pkt);

        Memory(const Memory& other) = delete;
        Memory& operator=(const Memory& other) = delete;

        ~Memory() { delete pmemAddr; }
        };

        Memory mem_;

        Tick latency_;

        Tick cxl_mem_latency_;
        
        public:
        Tick read(PacketPtr pkt) override;
        Tick write(PacketPtr pkt) override;

        AddrRangeList getAddrRanges() const override;

        Tick resolve_cxl_mem(PacketPtr ptk);
        using Param = CxlMemoryParams;
        CxlMemory(const Param &p);
    };
} // namespace gem5
