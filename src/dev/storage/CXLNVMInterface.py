from m5.objects.CXLMemCtrl import CXLMemCtrl
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

class CXLNVMInterface(NVMInterface):
    type = "CXLNVMInterface"
    cxx_header = "dev/storage/cxl_nvm_interface.hh"
    cxx_class = "gem5::CXLNVMInterface"
    
    def controller(self):
        controller = CXLMemCtrl()
        controller.dram = self
        return controller

class CXL_NVM_2400_1x64(CXLNVMInterface):
    write_buffer_size = 128
    read_buffer_size = 64

    max_pending_writes = 128
    max_pending_reads = 64

    device_rowbuffer_size = "256B"

    # 8X capacity compared to DDR4 x4 DIMM with 8Gb devices
    device_size = "512GiB"
    # Mimic 64-bit media agnostic DIMM interface
    device_bus_width = 64
    devices_per_rank = 1
    ranks_per_channel = 1
    banks_per_rank = 16

    burst_length = 8

    two_cycle_rdwr = True

    # 1200 MHz
    tCK = "0.833ns"

    tREAD = "150ns"
    tWRITE = "500ns"
    tSEND = "14.16ns"
    tBURST = "3.332ns"

    # Default all bus turnaround and rank bus delay to 2 cycles
    # With DDR data bus, clock = 1200 MHz = 1.666 ns
    tWTR = "1.666ns"
    tRTW = "1.666ns"
    tCS = "1.666ns"
