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
#include <iostream>
#include <opae/cxx/core/shared_buffer.h>
#include "perf_counters.h"

namespace intel
{
namespace fpga
{
namespace nlb
{

enum class dsm_version
{
    nlb_classic,
    cmdq_batch
};

class nlb_stats
{
public:
    nlb_stats(opae::fpga::types::shared_buffer::ptr_t dsm,
              dsm_version dsm_v,
              uint32_t cachelines,
              const fpga_cache_counters &cache_counters,
              const fpga_fabric_counters &fabric_counters,
              uint32_t clock_freq,
              bool continuous=false,
              bool suppress_hdr=false,
              bool csv=false);
    nlb_stats(opae::fpga::types::shared_buffer::ptr_t dsm,
              uint32_t cachelines,
              const fpga_cache_counters &cache_counters,
              const fpga_fabric_counters &fabric_counters,
              uint32_t clock_freq,
              bool continuous=false,
              bool suppress_hdr=false,
              bool csv=false);

friend std::ostream & operator << (std::ostream &os, const nlb_stats &stats);

private:
    opae::fpga::types::shared_buffer::ptr_t dsm_;
    dsm_version dsm_version_;
    uint32_t cachelines_;
    const fpga_cache_counters &cache_counters_;
    const fpga_fabric_counters &fabric_counters_;
    uint32_t clock_freq_;
    bool continuous_;
    bool suppress_hdr_;
    bool csv_;

    std::string normalized_freq() const;
    std::string read_bandwidth() const;
    std::string write_bandwidth() const;
};

class dsm_tuple
{
public:

    dsm_tuple(dsm_version v = dsm_version::nlb_classic);
    dsm_tuple(opae::fpga::types::shared_buffer::ptr_t dsm, dsm_version v = dsm_version::nlb_classic);
    dsm_tuple & operator += (const dsm_tuple &rhs);
    void put(opae::fpga::types::shared_buffer::ptr_t dsm);
    void get(opae::fpga::types::shared_buffer::ptr_t dsm);

    uint64_t raw_ticks()
    {
        return raw_ticks_;
    }

    uint32_t start_overhead()
    {
        return start_overhead_;
    }

    uint32_t end_overhead()
    {
        return end_overhead_;
    }

    uint64_t num_reads()
    {
        return num_reads_;
    }

    uint64_t num_writes()
    {
        return num_writes_;
    }


protected:
    uint64_t raw_ticks_;
    uint32_t start_overhead_;
    uint32_t end_overhead_;
    uint64_t num_reads_;
    uint64_t num_writes_;

    dsm_tuple(uint64_t raw_ticks,
              uint32_t start_overhead,
              uint32_t end_overhead,
              uint64_t num_reads,
              uint64_t num_writes);

    friend dsm_tuple operator + (const dsm_tuple &lhs, const dsm_tuple &rhs);
private:
    dsm_version version_;
};

} // end of namespace nlb
} // end of namespace fpga
} // end of namespace intel

