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
#include <string>
#include <map>
#include <opae/cxx/core/token.h>

namespace intel
{
namespace fpga
{

class fpga_cache_counters
{
public:
    enum ctr_t
    {
       read_hit = 0,
       write_hit,
       read_miss,
       write_miss,
       // 4 is reserved
       hold_request = 5,
       data_write_port_contention,
       tag_write_port_contention,
       tx_req_stall,
       rx_req_stall,
       rx_eviction
    };

    fpga_cache_counters();
    fpga_cache_counters(opae::fpga::types::token::ptr_t fme);
    fpga_cache_counters(const fpga_cache_counters &other);
    fpga_cache_counters & operator = (const fpga_cache_counters &other);

    uint64_t operator [] (ctr_t c) const;
    std::string name(ctr_t c) const;

    friend fpga_cache_counters operator - (const fpga_cache_counters &l,
                                           const fpga_cache_counters &r);

protected:
    typedef std::map<ctr_t, uint64_t> ctr_map_t;
    typedef ctr_map_t::iterator ctr_map_iter_t;
    typedef ctr_map_t::const_iterator const_ctr_map_iter_t;

    void freeze(bool f);

    ctr_map_t read_counters();
    uint64_t read_counter(ctr_t c);

private:
    opae::fpga::types::token::ptr_t fme_;
    uint64_t perf_feature_rev_;
    ctr_map_t ctr_map_;
};

class fpga_fabric_counters
{
public:
    enum ctr_t
    {
       mmio_read = 0,
       mmio_write,
       pcie0_read,
       pcie0_write,
       pcie1_read,
       pcie1_write,
       upi_read,
       upi_write
    };

    fpga_fabric_counters();
    fpga_fabric_counters(opae::fpga::types::token::ptr_t fme);
    fpga_fabric_counters(const fpga_fabric_counters &other);
    fpga_fabric_counters & operator = (const fpga_fabric_counters &other);

    uint64_t operator [] (ctr_t c) const;
    std::string name(ctr_t c) const;

    friend fpga_fabric_counters operator - (const fpga_fabric_counters &l,
                                            const fpga_fabric_counters &r);

protected:
    typedef std::map<ctr_t, uint64_t> ctr_map_t;
    typedef ctr_map_t::iterator ctr_map_iter_t;
    typedef ctr_map_t::const_iterator const_ctr_map_iter_t;

    void freeze(bool f);

    ctr_map_t read_counters();
    uint64_t read_counter(ctr_t c);

private:
    opae::fpga::types::token::ptr_t fme_;
    uint64_t perf_feature_rev_;
    ctr_map_t ctr_map_;
};

} // end of namespace fpga
} // end of namespace intel

