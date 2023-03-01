from m5.params import *
from m5.objects.PciDevice import *

class CxlMemory(PciDevice):
    type = 'CxlMemory'
    cxx_header = "dev/storage/cxl_memory.hh"
    cxx_class = 'gem5::CxlMemory'
    latency = Param.Latency('50ns', "cxl-memory device's latency for mem access")
    cxl_mem_latency = Param.Latency('2ns', "cxl.mem protocol processing's latency for device")

    VendorID = 0x8086
    DeviceID = 0X7890
    Command = 0x0
    # Status = 0x280
    Revision = 0x0
    ClassCode = 0x01
    SubClassCode = 0x01
    ProgIF = 0x85
    InterruptLine = 0x1f
    InterruptPin = 0x01

    # Primary
    BAR0 = PciMemBar(size='128MiB')
    BAR1 = PciMemUpperBar()
