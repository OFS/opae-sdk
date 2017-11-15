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

#include "mtnlb.h"
#include "nlb.h"
#include "fpga_app/fpga_common.h"
#include <chrono>
#include <thread>
#include "option.h"
#include "nlb_stats.h"
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace std::chrono;
using namespace intel::utils;
using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

mtnlb::mtnlb(const std::string & afu_id)
: accelerator_app()
, afu_id_(afu_id)
, name_()
, dsm_size_(MB(4))
, inp_size_(MB(4))
, out_size_(MB(4))
, frequency_(DEFAULT_FREQ)
, thread_count_(0)
, count_(0)
, stride_(1)
, mode7_args_(0)
, mode_("mt7")
, config_("mtnlb7.json")
, target_("fpga")
, dsm_timeout_(FPGA_DSM_TIMEOUT)
, suppress_header_(false)
, csv_format_(false)
, suppress_stats_(false)
, cachelines_(0)
{
    define_options();
}

mtnlb::mtnlb(const std::string & afu_id, const std::string & name)
: accelerator_app()
, afu_id_(afu_id)
, name_(name)
, dsm_size_(MB(1))
, inp_size_(MB(2))
, out_size_(MB(2))
, frequency_(DEFAULT_FREQ)
, thread_count_(0)
, count_(0)
, stride_(1)
, mode7_args_(0)
, mode_("mt7")
, config_("mtnlb7.json")
, target_("fpga")
, dsm_timeout_(FPGA_DSM_TIMEOUT)
, suppress_header_(false)
, csv_format_(false)
, suppress_stats_(false)
, cachelines_(0)
{
    define_options();
}


void mtnlb::define_options()
{
    options_.add_option<bool>("help",                'h', option::no_argument,   "Show help message", false);
    options_.add_option<std::string>("mode",         'm', option::with_argument, "mtnlb mode (mt7 or mt8)", mode_);
    options_.add_option<std::string>("config",       'c', option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",       't', option::with_argument, "one of { fpga, ase }", target_);
    options_.add_option<uint32_t>("count",           'C', option::with_argument, "number of iterations", 1);
    options_.add_option<uint32_t>("threads",         'T', option::with_argument, "number of threads to spawn - default is 64", 64);
    options_.add_option<uint32_t>("stride",          'e', option::with_argument, "stride number", stride_);
    options_.add_option<std::string>("cache-policy", 'p', option::with_argument, "one of { wrline-I, wrline-M wrpush-I", "wrline-M");
    options_.add_option<std::string>("cache-hint",   'i', option::with_argument, "one of { rdline-I, rdline-S}", "rdline-I");
    options_.add_option<std::string>("read-vc",      'r', option::with_argument, "one of { auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<std::string>("write-vc",     'w', option::with_argument, "one of { auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<uint64_t>("dsm-timeout-usec",     option::with_argument, "Timeout for test completion", dsm_timeout_.count());
    options_.add_option<uint32_t>("timeout-usec",         option::with_argument, "Timeout for continuous mode (microseconds portion)", 0);
    options_.add_option<uint32_t>("timeout-msec",         option::with_argument, "Timeout for continuous mode (milliseconds portion)", 0);
    options_.add_option<uint32_t>("timeout-sec",          option::with_argument, "Timeout for continuous mode (seconds portion)", 1);
    options_.add_option<uint32_t>("timeout-min",          option::with_argument, "Timeout for continuous mode (minutes portion)", 0);
    options_.add_option<uint32_t>("timeout-hour",         option::with_argument, "Timeout for continuous mode (hours portion)", 0);
    options_.add_option<uint32_t>("frequency",       'k', option::with_argument, "Clock frequency (used for bw measurements)", frequency_);
    options_.add_option<bool>("suppress-hdr",        'S', option::no_argument,   "Suppress column headers", suppress_header_);
    options_.add_option<bool>("csv",                 'V', option::no_argument,   "Comma separated value format", csv_format_);
    options_.add_option<bool>("suppress-stats",           option::no_argument,   "Show stats", suppress_stats_);
}

mtnlb::~mtnlb()
{

}

bool mtnlb::setup()
{
    options_.get_value<std::string>("target", target_);
    options_.get_value<bool>("suppress-stats", suppress_stats_);
    if (target_ == "fpga")
    {
        dsm_timeout_ = FPGA_DSM_TIMEOUT;
    }
    else if (target_ == "ase")
    {
        dsm_timeout_ = ASE_DSM_TIMEOUT;
    }
    else
    {
        std::cerr << "Invalid --target: " << target_ << std::endl;
        return false;
    }

    uint64_t dsm_timeout_usec = 0;
    if (options_.get_value<uint64_t>("dsm-timeout-usec", dsm_timeout_usec))
    {
        dsm_timeout_ = microseconds(dsm_timeout_usec);
    }

    cfg_ = nlb_mode::mt;

    std::string cache_policy;

    options_.get_value<std::string>("cache-policy", cache_policy);
    if (cache_policy == "wrpush-I")
    {
        cfg_ |= mtnlb_ctl::wrpush_i;
    }
    else if (cache_policy == "wrline-I")
    {
        cfg_ |= mtnlb_ctl::wrline_i;
    }

    std::string cache_hint;

    options_.get_value<std::string>("cache-hint", cache_hint);
    if (cache_hint == "rdline-I")
    {
        cfg_ |= mtnlb_ctl::rdi;
    }
    else if (cache_hint == "rdline-S")
    {
        cfg_ |= mtnlb_ctl::rds;
    }

    // set the read channel
    //{ auto, vl0, vh0, vh1, random}", "auto")
    std::string rd_channel;
    options_.get_value<std::string>("read-vc", rd_channel);
    if (rd_channel == "vl0")
    {
        cfg_ |= mtnlb_ctl::read_vl0;
    }
    else if (rd_channel == "vh0")
    {
        cfg_ |= mtnlb_ctl::read_vh0;
    }
    else if (rd_channel == "vh1")
    {
        cfg_ |= mtnlb_ctl::read_vh1;
    }
    else if (rd_channel == "random")
    {
        cfg_ |= mtnlb_ctl::read_vr;
    }

    // set the write channel
    //{ auto, vl0, vh0, vh1, random}", "auto")
    std::string wr_channel;
    options_.get_value<std::string>("write-vc", wr_channel);
    if (wr_channel == "vl0")
    {
        cfg_ |= mtnlb_ctl::write_vl0;
    }
    else if (wr_channel == "vh0")
    {
        cfg_ |= mtnlb_ctl::write_vh0;
    }
    else if (wr_channel == "vh1")
    {
        cfg_ |= mtnlb_ctl::write_vh1;
    }
    else if (wr_channel == "random")
    {
        cfg_ |= mtnlb_ctl::write_vr;
    }

    options_.get_value<std::string>("mode", mode_);
    options_.get_value<uint32_t>("threads", thread_count_);
    options_.get_value<uint32_t>("count", count_);
    options_.get_value<uint32_t>("stride", stride_);
    log_.set_level(logger::level::level_info);
    if (thread_count_ > mtnlb_max_threads)
    {
        log_.error(mode_) << "thread count is bigger than max: " << mtnlb_max_threads << std::endl;
        return false;
    }

    if (count_ > mtnlb_max_count)
    {
        log_.error(mode_) << "count is bigger than max: " << mtnlb_max_count << std::endl;
        return false;
    }

    if (stride_ > mtnlb_max_stride)
    {
        log_.error(mode_) << "stride is bigger than max: " << mtnlb_max_stride << std::endl;
        return false;
    }

    double log_stride = std::log2(static_cast<double>(stride_));

    cachelines_ = count_*thread_count_*stride_;
    if (thread_count_*stride_*cacheline_size > inp_size_)
    {
        log_.warn() << "Thread count and stride not compatible with max buffer size of " << inp_size_ << std::endl;
        return false;
    }


    dsm_ = accelerator_->allocate_buffer(dsm_size_);
    inp_ = accelerator_->allocate_buffer(inp_size_);
    out_ = accelerator_->allocate_buffer(out_size_);
    if (!dsm_ || !inp_ || !out_) {
        log_.error(mode_) << "failed to allocate workspace and input/output buffers." << std::endl;
        return false;
    }

    if (mode_ == "mt7")
    {
        inp_->fill(0);
    }

    options_.get_value<bool>("suppress-hdr", suppress_header_);
    options_.get_value<bool>("csv", csv_format_);

    mode7_args_ = (static_cast<uint64_t>(log_stride) << 32) | (count_ << 11) | thread_count_;
    return true;
}

bool mtnlb::run()
{
    out_->fill(0);
    stop_ = false;

    // set dsm base, high then low
    accelerator_->write_mmio64(static_cast<uint32_t>(mtnlb_dsm::basel), reinterpret_cast<uint64_t>(dsm_->iova()));
    // assert afu reset
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::ctl), 0);
    // clear the dsm status fields
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::ctl), 1);

    if (mode_ == "mt7")
    {
        // set input workspace address
        accelerator_->write_mmio64(static_cast<uint32_t>(mtnlb_csr::src_addr),
            CACHELINE_ALIGNED_ADDR(inp_->iova()));
    }

    // set output workspace address
    accelerator_->write_mmio64(static_cast<uint32_t>(mtnlb_csr::dst_addr),
            CACHELINE_ALIGNED_ADDR(out_->iova()));
    // set number of cache lines for test
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::num_lines),
            inp_size_/CL(1));

    // set the test mode
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::cfg), cfg_.value());

    accelerator_->write_mmio64(static_cast<uint64_t>(mtnlb_csr::mode7_args), mode7_args_);

    // start the test
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::ctl), 3);

    std::vector<std::thread> threads;
    // set the atomic ready bool to flalse, then create the threads
    // each thread will busy wait on the ready variable until it is true
    ready_ = false;
    cancel_ = false;
    auto thread_fn = [this](uint64_t tid, uint64_t iter, uint64_t stride)
    {
        this->work(tid, iter, stride);
    };

    for (uint64_t i = 0; i < thread_count_; ++i)
    {
        threads.push_back(std::thread(thread_fn, i, count_, stride_));
    }
    // Read perf counters.
    fpga_cache_counters  start_cache_ctrs ;
    fpga_fabric_counters start_fabric_ctrs;
    if (!suppress_stats_)
    {
        start_cache_ctrs  = accelerator_->cache_counters();
        start_fabric_ctrs = accelerator_->fabric_counters();
    }
    // start the threads
    ready_ = true;
    for( std::thread & t : threads )
    {
        t.join();
    }

    // Read perf counters
    fpga_cache_counters  end_cache_ctrs  = accelerator_->cache_counters();
    fpga_fabric_counters end_fabric_ctrs = accelerator_->fabric_counters();

    std::this_thread::sleep_for(microseconds(100));
    bool result = !cancel_;

    log_.debug(mode_) << "All threads complete, sending stop command" << std::endl;
    // stop the test
    accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::ctl), 7);
    cancel_ = false;
    // wait until NLB writes the allocation index (0 based)
    log_.debug(mode_) << "Waiting for dsm status bit" << std::endl;

    result &= dsm_->wait(static_cast<size_t>(mtnlb_dsm::test_complete),
                                             dma_buffer::microseconds_t(10),
                                             dsm_timeout_,
                                             0x1,
                                             0x1);

    if (result)
    {
        log_.debug(mode_) << "DSM Status complete. Stopping test" << std::endl;

        // stop the test
        accelerator_->write_mmio32(static_cast<uint32_t>(mtnlb_csr::ctl), 7);
    }
    else
    {
        log_.warn(mode_) << "Timed out waiting for dsm status bit" << std::endl;
        show_pending(-1);
    }

    if (!suppress_stats_)
    {
        log_.info(name()) << std::endl
                          << intel::fpga::nlb::nlb_stats(dsm_,
                                                         0,
                                                         end_cache_ctrs - start_cache_ctrs,
                                                         end_fabric_ctrs -start_fabric_ctrs,
                                                         frequency_,
                                                         false,
                                                         suppress_header_,
                                                         csv_format_);
    }

    return result;
}

std::string mtnlb::show_rw()
{
    uint64_t num_rw;
    if (accelerator_->read_mmio64(static_cast<uint32_t>(mtnlb_csr::num_rw), num_rw))
    {
        std::stringstream ss;
        uint32_t rd, wr;
        rd = (num_rw >> 32);
        wr = static_cast<uint32_t>(num_rw);
        ss << std::endl << "Num_Rd  Num_Wr" << std::endl
           << std::setw(6) << rd << " " << std::setw(6) << wr << std::endl;
        return ss.str();
    }
    return "";
}

void mtnlb::show_pending(uint32_t thread_id)
{
    uint64_t pending;
    if (accelerator_->read_mmio64(static_cast<uint32_t>(mtnlb_csr::io_pending), pending))
    {
        uint32_t rd, wr;
        rd = (pending >> 32);
        wr = static_cast<uint32_t>(pending);
        log_.error(mode_) << "thread [" << thread_id << "]: RD pending " << rd << ", WR pending " << wr << std::endl;
    }
}


} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel





