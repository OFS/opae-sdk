// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include <cstdint>

namespace intel
{
namespace fpga
{
namespace nlb
{

const uint64_t cacheline_size = 64;
const uint64_t max_cpu_cache_size = 100 * 1024*1024;
const uint64_t max_cachelines = 65535;
const uint32_t max_cmdq_counter = 32768;
const uint32_t mtnlb_max_threads = 2047;  // 11 bits
const uint32_t mtnlb_max_count = 1048575; // 20 bits
const uint32_t mtnlb_max_stride = 4294967295; // 32 bit

/// @brief test mode constants to use for
/// NLB config register
enum class nlb_mode : uint32_t
{
    loopback   = 0x0000,
    cont       = 0x0002,
    read       = 0x0004,
    write      = 0x0008,
    throughput = 0x000c,
    sw         = 0x001c,
    mt         = 0x0180
};

enum class nlb0_csr : uint32_t
{
    scratchpad0 = 0x0100,
    ctl         = 0x0138,
    cfg         = 0x0140,
    num_lines   = 0x0130,
    src_addr    = 0x0120,
    dst_addr    = 0x0128,
    status2     = 0x0170,
    mode7_args  = 0x0180,
    cmdq_sw     = 0x0190,
    cmdq_hw     = 0x0198
};

enum class nlb0_ctl : uint32_t
{
    lpbk1    	 = 0x000,
    wrline_m  	 = 0x000,	//Write-back,
    wrline_i  	 = 0x001,	//Write-through,
    cont     	 = 0x002,
    read     	 = 0x004,
    write    	 = 0x008,
    trput    	 = 0x00c,
    atomic    	 = 0x014,
    lpbk3    	 = 0x018,
    sw           = 0x01c,
    mask     	 = 0x01c,
    rds      	 = 0x000,
    rdi      	 = 0x200,
    umsg_poll	 = 0x0000000,
    csr_write    = 0x4000000,
    umsg_data    = 0x8000000,
    umsg_hint    = 0xc000000,
    va           = 0x0000,
    read_vl0	 = 0x1000,
    read_vh0	 = 0x2000,
    read_vh1	 = 0x3000,
    read_vr      = 0x4000,
    alt_wr_prn   = 0x8000,
    mcl2         = 0x0020,
    mcl4         = 0x0060,
    wrpush_i     = 0x10000,
    write_vl0    = 0x20000,
    write_vh0    = 0x40000,
    write_vh1    = 0x60000,
    write_vr     = 0x80000,
    wrfence_va   = 0x0,
    wrfence_vl0  = 0x40000000,
    wrfence_vh0  = 0x80000000,
    wrfence_vh1  = 0xc0000000
};

enum class nlb0_dsm : uint32_t
{
    basel           = 0x0110,
    baseh           = 0x0114,
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c
};

enum class nlb3_csr : uint32_t
{
    scratchpad0 = 0x0100,
    ctl         = 0x0138,
    cfg         = 0x0140,
    num_lines   = 0x0130,
    src_addr    = 0x0120,
    dst_addr    = 0x0128,
    status2     = 0x0170,
    strided_acs = 0x0178,
    mode7_args  = 0x0180,
    cmdq_sw     = 0x0190,
    cmdq_hw     = 0x0198
};

enum class nlb3_ctl : uint32_t
{
    lpbk1    	 = 0x000,
    wrline_m  	 = 0x000,	//Write-back,
    wrline_i  	 = 0x001,	//Write-through,
    cont     	 = 0x002,
    read     	 = 0x004,
    write    	 = 0x008,
    trput    	 = 0x00c,
    atomic    	 = 0x014,
    lpbk3    	 = 0x018,
    sw           = 0x01c,
    mask     	 = 0x01c,
    rds      	 = 0x000,
    rdi      	 = 0x200,
    umsg_poll	 = 0x0000000,
    csr_write    = 0x4000000,
    umsg_data    = 0x8000000,
    umsg_hint    = 0xc000000,
    va           = 0x0000,
    read_vl0	 = 0x1000,
    read_vh0	 = 0x2000,
    read_vh1	 = 0x3000,
    read_vr      = 0x4000,
    alt_wr_prn   = 0x8000,
    mcl2         = 0x0020,
    mcl4         = 0x0060,
    wrpush_i     = 0x10000,
    write_vl0    = 0x20000,
    write_vh0    = 0x40000,
    write_vh1    = 0x60000,
    write_vr     = 0x80000,
    wrfence_va   = 0x0,
    wrfence_vl0  = 0x40000000,
    wrfence_vh0  = 0x80000000,
    wrfence_vh1  = 0xc0000000
};

enum class nlb3_dsm : uint32_t
{
    basel           = 0x0110,
    baseh           = 0x0114,
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c
};

enum class mtnlb_csr : uint32_t
{
    scratchpad0 = 0x0100,
    ctl         = 0x0138,
    cfg         = 0x0140,
    num_lines   = 0x0130,
    src_addr    = 0x0120,
    dst_addr    = 0x0128,
    num_rw      = 0x0160,
    io_pending  = 0x0168,
    strided_acs = 0x0178,
    mode7_args  = 0x0180,
    cmdq_sw     = 0x0190,
    cmdq_hw     = 0x0198
};

enum class mtnlb_ctl : uint32_t
{
    lpbk1    	 = 0x000,
    wrline_m  	 = 0x000,	//Write-back,
    wrline_i  	 = 0x001,	//Write-through,
    cont     	 = 0x002,
    read     	 = 0x004,
    write    	 = 0x008,
    trput    	 = 0x00c,
    atomic    	 = 0x014,
    lpbk3    	 = 0x018,
    sw           = 0x01c,
    mask     	 = 0x01c,
    rds      	 = 0x000,
    rdi      	 = 0x200,
    umsg_poll	 = 0x0000000,
    csr_write    = 0x4000000,
    umsg_data    = 0x8000000,
    umsg_hint    = 0xc000000,
    va           = 0x0000,
    read_vl0	 = 0x1000,
    read_vh0	 = 0x2000,
    read_vh1	 = 0x3000,
    read_vr      = 0x4000,
    alt_wr_prn   = 0x8000,
    mcl2         = 0x0020,
    mcl4         = 0x0060,
    wrpush_i     = 0x10000,
    write_vl0    = 0x20000,
    write_vh0    = 0x40000,
    write_vh1    = 0x60000,
    write_vr     = 0x80000,
    wrfence_va   = 0x0,
    wrfence_vl0  = 0x40000000,
    wrfence_vh0  = 0x80000000,
    wrfence_vh1  = 0xc0000000
};

enum class mtnlb_dsm : uint32_t
{
    basel           = 0x0110,
    baseh           = 0x0114,
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c
};

enum class nlb7_csr : uint32_t
{
//    scratchpad0 = 0x0100,
    ctl         = 0x0138,
    cfg         = 0x0140,
    num_lines   = 0x0130,
    src_addr    = 0x0120,
    dst_addr    = 0x0128,
    sw_notice   = 0x0158,
//    strided_acs = 0x0178,
//    mode7_args  = 0x0180,
//    cmdq_sw     = 0x0190,
//    cmdq_hw     = 0x0198
};

enum class nlb7_dsm : uint32_t
{
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c,
    mode_error      = 0x0060
};

enum class mb1_csr : uint32_t
{
    basel       = 0x0110,
    baseh       = 0x0114,
    src_addr    = 0x0120,
    dst_addr    = 0x0128,
    num_lines   = 0x0130,
    ctl         = 0x0138,
    cfg         = 0x0140,
    sw_ctl      = 0x0178,
};

enum class mb1_ctl : uint32_t
{
    // cache-hint
    rds      	 = 0x000,
    rdi      	 = 0x200,
    // read-vc
    va           = 0x0000,
    read_vl0	 = 0x1000,
    read_vh0	 = 0x2000,
    read_vh1	 = 0x3000,
    read_vr      = 0x4000,
};

enum class mb1_dsm : uint32_t
{
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c,
    mode_error      = 0x0060
};

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

