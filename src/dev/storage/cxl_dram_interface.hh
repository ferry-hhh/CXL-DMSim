#ifndef _CXL_DRAM_INTERFACE_HH_
#define _CXL_DRAM_INTERFACE_HH_

#include "mem/dram_interface.hh"
#include "params/CXLDRAMInterface.hh"

namespace gem5
{

class CXLDRAMInterface : public memory::DRAMInterface
{
    public:
        CXLDRAMInterface(const CXLDRAMInterfaceParams &p);
        
        inline uint8_t * toHostAddr(Addr addr) const override
        {
            return pmemAddr + addr - range.start();
        }
        
};

}

#endif