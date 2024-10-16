from m5.objects.CXLMemCtrl import CXLMemCtrl
from m5.objects.DRAMInterface import *

class CXLDRAMInterface(DRAMInterface):
    type = "CXLDRAMInterface"
    cxx_header = "dev/storage/cxl_dram_interface.hh"
    cxx_class = "gem5::CXLDRAMInterface"
    
    def controller(self):
        controller = CXLMemCtrl()
        controller.dram = self
        return controller
