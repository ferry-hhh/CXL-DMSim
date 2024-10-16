#include "dev/storage/cxl_dram_interface.hh"
#include "mem/dram_interface.hh"

namespace gem5
{

CXLDRAMInterface::CXLDRAMInterface(const CXLDRAMInterfaceParams &p):
    memory::DRAMInterface::DRAMInterface(p)
{

}
}