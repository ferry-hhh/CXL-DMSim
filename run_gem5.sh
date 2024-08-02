#!/bin/bash

build/X86/gem5.opt -d "fs_lmbench_dram" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd lmbench_dram.sh
# Note: If using the O3 CPU to run the lmbench test takes too long, you can also use the following command to start the TIMING CPU for the lmbench test.
# The test results are consistent between the two CPU types, but the TIMING CPU will be much faster.
# build/X86/gem5.opt -d "fs_lmbench_dram" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd lmbench_dram.sh --cpu_type TIMING
# build/X86/gem5.opt -d "fs_lmbench_cxl_ASIC" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd lmbench_cxl.sh --cpu_type TIMING
# build/X86/gem5.opt -d "fs_lmbench_cxl_FPGA" configs/example/gem5_library/x86-cxl-run.py --is_asic False --test_cmd lmbench_cxl.sh --cpu_type TIMING

build/X86/gem5.opt -d "fs_lmbench_cxl_ASIC" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd lmbench_cxl.sh

build/X86/gem5.opt -d "fs_lmbench_cxl_FPGA" configs/example/gem5_library/x86-cxl-run.py --is_asic False --test_cmd lmbench_cxl.sh

build/X86/gem5.opt -d "fs_merci_dram" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd merci_dram.sh --num_cpus 48

build/X86/gem5.opt -d "fs_merci_cxl_ASIC" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd merci_cxl.sh --num_cpus 48

build/X86/gem5.opt -d "fs_merci_dram+cxl_ASIC" configs/example/gem5_library/x86-cxl-run.py --is_asic True --test_cmd merci_dram+cxl.sh --num_cpus 48
