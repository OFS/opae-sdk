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

#include <sstream>
#include <iomanip>
#include "nlb_stats.h"

using namespace opae::fpga::types;

namespace intel
{
namespace fpga
{
namespace nlb
{

enum class nlb_dsm : uint32_t
{
    test_complete   = 0x0040,
    test_error      = 0x0044,
    num_clocks      = 0x0048,
    num_reads       = 0x0050,
    num_writes      = 0x0054,
    start_overhead  = 0x0058,
    end_overhead    = 0x005c
};

enum class cmdqbatch_dsm : uint32_t
{
    test_complete   = 0x0040,
    test_error      = 0x0042,
    num_clocks      = 0x0046,
    num_reads       = 0x004e,
    num_writes      = 0x0056,
    start_overhead  = 0x005e,
    end_overhead    = 0x005f
};

nlb_stats::nlb_stats(shared_buffer::ptr_t dsm,
                     uint32_t cachelines,
                     const fpga_cache_counters &cache_counters,
                     const fpga_fabric_counters &fabric_counters,
                     uint32_t clock_freq,
                     bool continuous,
                     bool suppress_hdr,
                     bool csv)
: dsm_(dsm)
, dsm_version_(dsm_version::nlb_classic)
, cachelines_(cachelines)
, cache_counters_(cache_counters)
, fabric_counters_(fabric_counters)
, clock_freq_(clock_freq)
, continuous_(continuous)
, suppress_hdr_(suppress_hdr)
, csv_(csv)
{

}


nlb_stats::nlb_stats(shared_buffer::ptr_t dsm,
                     dsm_version dsm_v,
                     uint32_t cachelines,
                     const fpga_cache_counters &cache_counters,
                     const fpga_fabric_counters &fabric_counters,
                     uint32_t clock_freq,
                     bool continuous,
                     bool suppress_hdr,
                     bool csv)
: dsm_(dsm)
, dsm_version_(dsm_v)
, cachelines_(cachelines)
, cache_counters_(cache_counters)
, fabric_counters_(fabric_counters)
, clock_freq_(clock_freq)
, continuous_(continuous)
, suppress_hdr_(suppress_hdr)
, csv_(csv)
{

}

std::ostream & operator << (std::ostream &os, const nlb_stats &stats)
{
    auto header = !stats.suppress_hdr_;
    auto csv = stats.csv_;

    uint64_t ticks;
    dsm_tuple dsm(stats.dsm_, stats.dsm_version_);
    auto rawticks = dsm.raw_ticks();
    auto startpenalty = dsm.start_overhead();
    auto endpenalty = dsm.end_overhead();

    if (stats.continuous_)
    {
        ticks = rawticks - startpenalty;
    }
    else
    {
        ticks = rawticks - (startpenalty + endpenalty);
    }

    auto num_reads = dsm.num_reads();
    auto num_writes = dsm.num_writes();

    if (csv)
    {
        if (header)
        {
            os << "Cachelines,Read_Count,Write_Count,Cache_Rd_Hit,Cache_Wr_Hit,Cache_Rd_Miss,Cache_Wr_Miss,Eviction,'Clocks(@"
               << stats.normalized_freq() << ")',Rd_Bandwidth,Wr_Bandwidth,VH0_Rd_Count,VH0_Wr_Count,VH1_Rd_Count,VH1_Wr_Count,VL0_Rd_Count,VL0_Wr_Count" << std::endl;
        }

        os << stats.cachelines_                                         << ','
           << num_reads                                                 << ','
           << num_writes                                                << ','
           << stats.cache_counters_[fpga_cache_counters::read_hit]      << ','
           << stats.cache_counters_[fpga_cache_counters::write_hit]     << ','
           << stats.cache_counters_[fpga_cache_counters::read_miss]     << ','
           << stats.cache_counters_[fpga_cache_counters::write_miss]    << ','
           << stats.cache_counters_[fpga_cache_counters::rx_eviction]   << ','
           << ticks                                                     << ','
           << stats.read_bandwidth()                                    << ','
           << stats.write_bandwidth()                                   << ','
           << stats.fabric_counters_[fpga_fabric_counters::pcie0_read]  << ','
           << stats.fabric_counters_[fpga_fabric_counters::pcie0_write] << ','
           << stats.fabric_counters_[fpga_fabric_counters::pcie1_read]  << ','
           << stats.fabric_counters_[fpga_fabric_counters::pcie1_write] << ','
           << stats.fabric_counters_[fpga_fabric_counters::upi_read]    << ','
           << stats.fabric_counters_[fpga_fabric_counters::upi_write]   << std::endl;
    }
    else
    {
        os << std::endl;

        if (header)
        {
                 //0123456789 0123456789 01234567890 012345678901 012345678901 0123456789012 0123456789012 0123456789 0123456789012
            os << "Cachelines Read_Count Write_Count Cache_Rd_Hit Cache_Wr_Hit Cache_Rd_Miss Cache_Wr_Miss   Eviction 'Clocks(@"
               << stats.normalized_freq() << ")'"
                   // 01234567890123 01234567890123
               << "   Rd_Bandwidth   Wr_Bandwidth" << std::endl;
        }

        os << std::setw(10) << stats.cachelines_                                       << ' '
           << std::setw(10) << num_reads                                               << ' '
           << std::setw(11) << num_writes                                              << ' '
           << std::setw(12) << stats.cache_counters_[fpga_cache_counters::read_hit]    << ' '
           << std::setw(12) << stats.cache_counters_[fpga_cache_counters::write_hit]   << ' '
           << std::setw(13) << stats.cache_counters_[fpga_cache_counters::read_miss]   << ' '
           << std::setw(13) << stats.cache_counters_[fpga_cache_counters::write_miss]  << ' '
           << std::setw(10) << stats.cache_counters_[fpga_cache_counters::rx_eviction] << ' '
           << std::setw(16) << ticks                                                   << ' '
           << std::setw(14) << stats.read_bandwidth()                                  << ' '
           << std::setw(14) << stats.write_bandwidth()
           << std::endl     << std::endl;

        if (header)
        {
                // 012345678901 012345678901 012345678901 012345678901 012345678901 012345678901
            os << "VH0_Rd_Count VH0_Wr_Count VH1_Rd_Count VH1_Wr_Count VL0_Rd_Count VL0_Wr_Count " << std::endl;
        }

        os << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::pcie0_read]  << ' '
           << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::pcie0_write] << ' '
           << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::pcie1_read]  << ' '
           << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::pcie1_write] << ' '
           << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::upi_read]    << ' '
           << std::setw(12) << stats.fabric_counters_[fpga_fabric_counters::upi_write]   << ' '
           << std::endl     << std::endl;
    }

    return os;
}

#define CL(x)  ((x) * 64)
#define GHZ(x) ((x) * 1000000000ULL)
#define MHZ(x) ((x) * 1000000ULL)
#define KHZ(x) ((x) * 1000ULL)

std::string nlb_stats::normalized_freq() const
{
    auto freq = clock_freq_;
    std::ostringstream oss;

    if (freq >= GHZ(1))
    {
        oss << (freq / GHZ(1)) << " GHz";
        return oss.str();
    }
    if (freq >= MHZ(1))
    {
        oss << (freq / MHZ(1)) << " MHz";
        return oss.str();
    }
    if (freq >= KHZ(1))
    {
        oss << (freq / KHZ(1)) << " KHz";
        return oss.str();
    }
    oss << freq << " Hz";
    return oss.str();
}

std::string nlb_stats::read_bandwidth() const
{
    auto clockfreq = clock_freq_;
    const double giga = 1000.0 * 1000.0 * 1000.0;

    dsm_tuple dsm(dsm_, dsm_version_);
    auto rawticks = dsm.raw_ticks();
    auto startpenalty = dsm.start_overhead();
    auto endpenalty = dsm.end_overhead();
    auto rds = dsm.num_reads();

    uint64_t ticks;

    if (continuous_)
    {
        ticks = rawticks - startpenalty;
    }
    else
    {
        ticks = rawticks - (startpenalty + endpenalty);
    }

    const double Rds   = (double)rds;
    const double Ticks = (double)ticks;
    const double Hz    = (double)clockfreq;

    double bw = (Rds * (CL(1) * Hz)) / Ticks;

    bw /= giga;

    std::ostringstream oss;

    oss.precision(3);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << bw;
    if (!(csv_ && suppress_hdr_))
    {
        oss << " GB/s";
    }

    return oss.str();
}

std::string nlb_stats::write_bandwidth() const
{
    auto clockfreq = clock_freq_;
    const double giga = 1000.0 * 1000.0 * 1000.0;

    dsm_tuple dsm(dsm_, dsm_version_);
    auto rawticks = dsm.raw_ticks();
    auto startpenalty = dsm.start_overhead();
    auto endpenalty = dsm.end_overhead();
    auto wrs = dsm.num_writes();

    uint64_t ticks;

    if (continuous_)
    {
        ticks = rawticks - startpenalty;
    }
    else
    {
        ticks = rawticks - (startpenalty + endpenalty);
    }

    const double Wrs   = (double)wrs;
    const double Ticks = (double)ticks;
    const double Hz    = (double)clockfreq;

    double bw = (Wrs * (CL(1) * Hz)) / Ticks;

    bw /= giga;

    std::ostringstream oss;

    oss.precision(3);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << bw;
    if (!(csv_ && suppress_hdr_))
    {
        oss << " GB/s";
    }

    return oss.str();
}

dsm_tuple::dsm_tuple(dsm_version v)
: raw_ticks_(0)
, start_overhead_(0)
, end_overhead_(0)
, num_reads_(0)
, num_writes_(0)
, version_(v)
{
}

dsm_tuple::dsm_tuple(shared_buffer::ptr_t dsm, dsm_version v)
: dsm_tuple(v)
{
    get(dsm);
}

dsm_tuple::dsm_tuple(uint64_t raw_ticks,
                     uint32_t start_overhead,
                     uint32_t end_overhead,
                     uint64_t num_reads,
                     uint64_t num_writes)
: raw_ticks_(raw_ticks)
, start_overhead_(start_overhead)
, end_overhead_(end_overhead)
, num_reads_(num_reads)
, num_writes_(num_writes)
, version_(dsm_version::nlb_classic)
{
}


dsm_tuple & dsm_tuple::operator += (const dsm_tuple &rhs)
{
    raw_ticks_ += rhs.raw_ticks_;
    start_overhead_ += rhs.start_overhead_;
    end_overhead_ += rhs.end_overhead_;
    num_reads_ += rhs.num_reads_;
    num_writes_ += rhs.num_writes_;
    return *this;
}

void dsm_tuple::get(shared_buffer::ptr_t dsm)
{
    switch(version_)
    {
        case dsm_version::nlb_classic:
            raw_ticks_ = dsm->read<uint64_t>((size_t)nlb_dsm::num_clocks);
            start_overhead_ = dsm->read<uint32_t>((size_t)nlb_dsm::start_overhead);
            end_overhead_ = dsm->read<uint32_t>((size_t)nlb_dsm::end_overhead);
            num_reads_ = dsm->read<uint32_t>((size_t)nlb_dsm::num_reads);
            num_writes_ = dsm->read<uint32_t>((size_t)nlb_dsm::num_writes);
            break;
        case dsm_version::cmdq_batch:
            raw_ticks_ = dsm->read<uint64_t>((size_t)cmdqbatch_dsm::num_clocks);
            start_overhead_ = dsm->read<uint8_t>((size_t)cmdqbatch_dsm::start_overhead);
            end_overhead_ = dsm->read<uint8_t>((size_t)cmdqbatch_dsm::end_overhead);
            num_reads_ = dsm->read<uint64_t>((size_t)cmdqbatch_dsm::num_reads);
            num_writes_ = dsm->read<uint64_t>((size_t)cmdqbatch_dsm::num_writes);
            break;
        default:
            std::cerr << "Unrecognized DSM version\r";
    }
}

void dsm_tuple::put(shared_buffer::ptr_t dsm)
{
    switch(version_)
    {
        case dsm_version::nlb_classic:
            dsm->write<uint64_t>(raw_ticks_, (size_t)nlb_dsm::num_clocks);
            dsm->write<uint32_t>(start_overhead_, (size_t)nlb_dsm::start_overhead);
            dsm->write<uint32_t>(end_overhead_, (size_t)nlb_dsm::end_overhead);
            dsm->write<uint32_t>(num_reads_, (size_t)nlb_dsm::num_reads);
            dsm->write<uint32_t>(num_writes_, (size_t)nlb_dsm::num_writes);
            break;
        case dsm_version::cmdq_batch:
            dsm->write<uint64_t>(raw_ticks_, (size_t)cmdqbatch_dsm::num_clocks);
            dsm->write<uint8_t>(start_overhead_, (size_t)cmdqbatch_dsm::start_overhead);
            dsm->write<uint8_t>(end_overhead_, (size_t)cmdqbatch_dsm::end_overhead);
            dsm->write<uint64_t>(num_reads_, (size_t)cmdqbatch_dsm::num_reads);
            dsm->write<uint64_t>(num_writes_, (size_t)cmdqbatch_dsm::num_writes);
            break;
        default:
            std::cerr << "Unrecognized DSM version\r";
    }
}

dsm_tuple operator + (const dsm_tuple &lhs, const dsm_tuple &rhs)
{
    return dsm_tuple(lhs.raw_ticks_ + rhs.raw_ticks_,
                     lhs.start_overhead_ + rhs.start_overhead_,
                     lhs.end_overhead_ + rhs.end_overhead_,
                     lhs.num_reads_ + rhs.num_reads_,
                     lhs.num_writes_ + rhs.num_writes_);
}

} // end of namespace nlb
} // end of namespace fpga
} // end of namespace intel

