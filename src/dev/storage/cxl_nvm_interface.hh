#ifndef _CXL_NVM_INTERFACE_HH_
#define _CXL_NVM_INTERFACE_HH_

#include "mem/nvm_interface.hh"
#include "params/CXLNVMInterface.hh"

namespace gem5
{

class CXLNVMInterface : public memory::NVMInterface
{
    public:
        CXLNVMInterface(const CXLNVMInterfaceParams &p);
        
        inline uint8_t * toHostAddr(Addr addr) const override
        {
            return pmemAddr + addr - range.start();
        }
        
};

}

#endif