// Copyright(c) 2017-2018, Intel Corporation
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

#include <iostream>
#include <fstream>
#include <cstdint>
#include "perf_counters.h"
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/sysobject.h>

using namespace opae::fpga::types;

namespace intel
{
namespace fpga
{

fpga_cache_counters::fpga_cache_counters()
: fme_()
, perf_feature_rev_(-1)
{
}

fpga_cache_counters::fpga_cache_counters(token::ptr_t fme)
: fme_(fme)
, perf_feature_rev_(-1)
{
    auto rev = sysobject::get(fme_, "*perf/revision", FPGA_OBJECT_GLOB);
    if (rev) {
        perf_feature_rev_ = rev->read64();
        ctr_map_ = read_counters();
    }

}

fpga_cache_counters::fpga_cache_counters(const fpga_cache_counters &other)
: fme_(other.fme_)
, perf_feature_rev_(other.perf_feature_rev_)
, ctr_map_(other.ctr_map_)
{
}

fpga_cache_counters & fpga_cache_counters::operator = (const fpga_cache_counters &other)
{
    if (&other != this)
    {
        fme_ = other.fme_;
        ctr_map_ = other.ctr_map_;
    }
    return *this;
}

uint64_t fpga_cache_counters::operator [] (fpga_cache_counters::ctr_t c) const
{
    const_ctr_map_iter_t iter = ctr_map_.find(c);
    if (ctr_map_.end() == iter)
        return (uint64_t)-1;
    return iter->second;
}

std::string fpga_cache_counters::name(fpga_cache_counters::ctr_t c) const
{
#define CASE(x) case x : return #x
    switch(c)
    {
        CASE(read_hit);
        CASE(write_hit);
        CASE(read_miss);
        CASE(write_miss);
        CASE(hold_request);
        CASE(data_write_port_contention);
        CASE(tag_write_port_contention);
        CASE(tx_req_stall);
        CASE(rx_req_stall);
        CASE(rx_eviction);
        default: return "";
    }
#undef CASE
}

fpga_cache_counters operator - (const fpga_cache_counters &l,
                                const fpga_cache_counters &r)
{
    fpga_cache_counters ctrs;
    fpga_cache_counters::ctr_t ctr_ts[] =
    {
        fpga_cache_counters::ctr_t::read_hit,
        fpga_cache_counters::ctr_t::write_hit,
        fpga_cache_counters::ctr_t::read_miss,
        fpga_cache_counters::ctr_t::write_miss,
        fpga_cache_counters::ctr_t::hold_request,
        fpga_cache_counters::ctr_t::data_write_port_contention,
        fpga_cache_counters::ctr_t::tag_write_port_contention,
        fpga_cache_counters::ctr_t::tx_req_stall,
        fpga_cache_counters::ctr_t::rx_req_stall,
        fpga_cache_counters::ctr_t::rx_eviction,
    };

    uint64_t left;
    uint64_t right;
    uint64_t diff;

    for (auto it : ctr_ts) {
        left  = l[it];
        right = r[it];
        diff  = (left < right) ? (UINT64_MAX - right) + left : left - right;
        ctrs.ctr_map_.insert(std::make_pair(it, diff));
    }

    return ctrs;
}

fpga_cache_counters::ctr_map_t fpga_cache_counters::read_counters()
{
    ctr_t c;
    ctr_map_t m;
    auto handle = handle::open(fme_, FPGA_OPEN_SHARED);

    if (handle) {
        auto cache = sysobject::get(handle, "*perf/cache", FPGA_OBJECT_GLOB);

        if (cache) {
            auto freeze = cache->get("freeze");
            freeze->write64(1);

            c = ctr_t::read_hit;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::write_hit;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::read_miss;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::write_miss;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::hold_request;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::data_write_port_contention;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::tag_write_port_contention;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::tx_req_stall;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::rx_req_stall;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::rx_eviction;
            m.insert(std::make_pair(c, read_counter(c)));

            freeze->write64(0);
        }

        handle->close();
    }
    return m;
}

uint64_t fpga_cache_counters::read_counter(fpga_cache_counters::ctr_t c)
{
    std::string ctr("*perf/cache/");

#define CASE(x) case x : ctr += #x; break
    switch(c)
    {
        CASE(read_hit);
        CASE(write_hit);
        CASE(read_miss);
        CASE(write_miss);
        CASE(hold_request);
        CASE(data_write_port_contention);
        CASE(tag_write_port_contention);
        CASE(tx_req_stall);
        CASE(rx_req_stall);
        CASE(rx_eviction);
        default : return (uint64_t)-1;
    }
#undef CASE
    try {
        auto counter = sysobject::get(fme_, ctr, FPGA_OBJECT_GLOB);
        if (counter) {
          return counter->read64();
        }
    }catch(not_found &err) {
       return 0;
    }

    return 0;
}


fpga_fabric_counters::fpga_fabric_counters()
: fme_()
, perf_feature_rev_(-1)
{
}

fpga_fabric_counters::fpga_fabric_counters(token::ptr_t fme)
: fme_(fme)
, perf_feature_rev_(-1)
{
    auto rev = sysobject::get(fme_, "*perf/revision", FPGA_OBJECT_GLOB);
    if (rev) {
        perf_feature_rev_ = rev->read64();
        ctr_map_ = read_counters();
    }
}

fpga_fabric_counters::fpga_fabric_counters(const fpga_fabric_counters &other)
: fme_(other.fme_)
, perf_feature_rev_(other.perf_feature_rev_)
, ctr_map_(other.ctr_map_)
{
}

fpga_fabric_counters & fpga_fabric_counters::operator = (const fpga_fabric_counters &other)
{
    if (&other != this)
    {
        fme_ = other.fme_;
        perf_feature_rev_ = other.perf_feature_rev_;
        ctr_map_ = other.ctr_map_;
    }
    return *this;
}

uint64_t fpga_fabric_counters::operator [] (fpga_fabric_counters::ctr_t c) const
{
    const_ctr_map_iter_t iter = ctr_map_.find(c);
    if (ctr_map_.end() == iter)
        return (uint64_t)-1;
    return iter->second;
}

std::string fpga_fabric_counters::name(fpga_fabric_counters::ctr_t c) const
{
#define CASE(x) case x : return #x
    switch(c)
    {
        CASE(mmio_read);
        CASE(mmio_write);
        CASE(pcie0_read);
        CASE(pcie0_write);
        CASE(pcie1_read);
        CASE(pcie1_write);
        CASE(upi_read);
        CASE(upi_write);
        default: return "";
    }
#undef CASE
}

fpga_fabric_counters operator - (const fpga_fabric_counters &l,
                                const fpga_fabric_counters &r)
{
    fpga_fabric_counters ctrs;
    fpga_fabric_counters::ctr_t ctr_ts[] =
    {
        fpga_fabric_counters::ctr_t::mmio_read,
        fpga_fabric_counters::ctr_t::mmio_write,
        fpga_fabric_counters::ctr_t::pcie0_read,
        fpga_fabric_counters::ctr_t::pcie0_write,
        fpga_fabric_counters::ctr_t::pcie1_read,
        fpga_fabric_counters::ctr_t::pcie1_write,
        fpga_fabric_counters::ctr_t::upi_read,
        fpga_fabric_counters::ctr_t::upi_write,
    };

    uint64_t left;
    uint64_t right;
    uint64_t diff;

    for (auto it : ctr_ts) {
        left  = l[it];
        right = r[it];
        diff  = (left < right) ? (UINT64_MAX - right) + left : left - right;
        ctrs.ctr_map_.insert(std::make_pair(it, diff));
    }

    return ctrs;
}

fpga_fabric_counters::ctr_map_t fpga_fabric_counters::read_counters()
{
    ctr_t c;
    ctr_map_t m;
    auto handle = handle::open(fme_, FPGA_OPEN_SHARED);

    if (handle) {
        auto fabric = sysobject::get(handle, "*perf/fabric", FPGA_OBJECT_GLOB);

        if (fabric) {
            auto freeze = fabric->get("freeze");
            freeze->write64(1);

            c = ctr_t::mmio_read;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::mmio_write;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::pcie0_read;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::pcie0_write;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::pcie1_read;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::pcie1_write;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::upi_read;
            m.insert(std::make_pair(c, read_counter(c)));
            c = ctr_t::upi_write;
            m.insert(std::make_pair(c, read_counter(c)));

            freeze->write64(0);
        }
        handle->close();
    }

    return m;
}

uint64_t fpga_fabric_counters::read_counter(fpga_fabric_counters::ctr_t c)
{
    std::string ctr("*perf/fabric/");

#define CASE(x) case x : ctr += #x; break
    switch(c)
    {
        CASE(mmio_read);
        CASE(mmio_write);
        CASE(pcie0_read);
        CASE(pcie0_write);
        CASE(pcie1_read);
        CASE(pcie1_write);
        CASE(upi_read);
        CASE(upi_write);
        default : return (uint64_t)-1;
    }
#undef CASE
    try {
        auto counter = sysobject::get(fme_, ctr, FPGA_OBJECT_GLOB);
        if (counter) {
          return counter->read64();
        }
    }catch(not_found &err) {
       return 0;
    }

    return 0;
}

} // end of namespace fpga
} // end of namespace intel

