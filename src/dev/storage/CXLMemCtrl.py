from m5.params import *
from m5.objects.MemCtrl import *


class CXLMemCtrl(MemCtrl):
    type = 'CXLMemCtrl'
    cxx_header = 'dev/storage/cxl_mem_ctrl.hh'
    cxx_class = 'gem5::CXLMemCtrl'
