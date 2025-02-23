#ifndef __DEV_STORAGE_CXL_MEMORY_HH__
#define __DEV_STORAGE_CXL_MEMORY_HH__

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
    protected:
        CXLPort<CXLMemory> cxlPort;

    private:
        CXLMemCtrl* ctrl;
        Tick deviceProtoProcLat;
        AddrRange cxlMemRange;

    public:
        Tick read(PacketPtr pkt) override;
        Tick write(PacketPtr pkt) override;
        bool read_timing(PacketPtr pkt);
        bool write_timing(PacketPtr pkt);

        AddrRangeList getAddrRanges() const override;
        Port &getPort(const std::string &if_name, PortID idx=InvalidPortID) override;
        void init() override;

        Tick processCXLMem(PacketPtr ptk);
        using Param = CXLMemoryParams;
        CXLMemory(const Param &p);
};

} // namespace gem5

#endif // __DEV_STORAGE_CXL_MEMORY_HH__