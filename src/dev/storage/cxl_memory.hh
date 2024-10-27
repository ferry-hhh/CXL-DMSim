#include "base/addr_range.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/CXLMemory.hh"
#include "dev/pci/device.hh"
#include "dev/storage/cxl_mem_ctrl.hh"

namespace gem5
{
    class CXLMemory : public PciDevice {
        private:
            CXLMemCtrl* ctrl;
            Tick deviceProtoProcLat;
            AddrRange cxlMemRange;


        public:
            Tick read(PacketPtr pkt) override;
            Tick write(PacketPtr pkt) override;

            AddrRangeList getAddrRanges() const override;

            Tick processCXLMem(PacketPtr ptk);
            using Param = CXLMemoryParams;
            CXLMemory(const Param &p);
    };
} // namespace gem5
