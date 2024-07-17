from m5.params import *
from m5.objects.PciDevice import *


class CXLMemory(PciDevice):
    type = 'CXLMemory'
    cxx_header = "dev/storage/cxl_memory.hh"
    cxx_class = 'gem5::CXLMemory'
    latency = Param.Latency('50ns', "cxl-memory device's latency for mem access")
    cxl_mem_latency = Param.Latency('30ns', "cxl.mem protocol processing's latency for device")
    cxl_mem_range = Param.AddrRange(AddrRange(Addr("4GB"), size="2GB"), "cxl.mem protocol processing's address range for device")
    numa_flag = Param.Bool(True, "whether the device is numa node or not")

    VendorID = 0x8086
    DeviceID = 0X7890
    Command = 0x0
    Status = 0x280
    Revision = 0x0
    ClassCode = 0x05
    SubClassCode = 0x00
    ProgIF = 0x00
    InterruptLine = 0x1f
    InterruptPin = 0x01

    # Primary
    BAR0 = PciMemBar(size='4GB')
    BAR1 = PciMemUpperBar()
