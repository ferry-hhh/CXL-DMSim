#include "dev/storage/cxl_nvm_interface.hh"
#include "mem/nvm_interface.hh"

namespace gem5
{

CXLNVMInterface::CXLNVMInterface(const CXLNVMInterfaceParams &p):
    memory::NVMInterface::NVMInterface(p)
{

}
}