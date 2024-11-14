# Copyright (c) 2012 ARM Limited
# Copyright (c) 2020 Barkhausen Institut
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from m5.defines import buildEnv
from m5.objects import *

from gem5.isas import ISA

# Base implementations of L1, L2, IO and TLB-walker caches. There are
# used in the regressions and also as base components in the
# system-configuration scripts. The values are meant to serve as a
# starting point, and specific parameters can be overridden in the
# specific instantiations.


class L1Cache(Cache):
    assoc = 8
    tag_latency = 4
    data_latency = 4
    response_latency = 4
    mshrs = 20
    write_buffers = 20
    tgts_per_mshr = 20


class L1_ICache(L1Cache):
    assoc = 6
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 20
    is_read_only = True
    # Writeback clean lines as well
    writeback_clean = True
    prefetcher = IndirectMemoryPrefetcher()

class L1_DCache(L1Cache):
    assoc = 8
    tag_latency = 3
    data_latency = 3
    response_latency = 2
    mshrs = 16
    write_buffers = 16
    tgts_per_mshr = 20
    writeback_clean = False
    prefetcher = IndirectMemoryPrefetcher()

class L2Cache(Cache):
    assoc = 16
    tag_latency = 7
    data_latency = 7
    response_latency = 5
    mshrs = 20
    tgts_per_mshr = 12
    write_buffers = 20
    writeback_clean = True
    clusivity = "mostly_incl"
    prefetcher = L2MultiPrefetcher()

class L3Cache(Cache):
    assoc = 48
    tag_latency = 96
    data_latency = 96
    response_latency = 48
    mshrs = 384
    tgts_per_mshr = 32
    write_buffers = 256
    writeback_clean = False
    clusivity = "mostly_excl"
    prefetcher = L2MultiPrefetcher()

class IOCache(Cache):
    assoc = 8
    tag_latency = 50
    data_latency = 50
    response_latency = 50
    mshrs = 20
    size = "1kB"
    tgts_per_mshr = 12


class PageTableWalkerCache(Cache):
    assoc = 4
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 10
    size = "1kB"
    tgts_per_mshr = 12
    is_read_only = False
