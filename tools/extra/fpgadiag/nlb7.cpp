// Copyright(c) 2017-2019, Intel Corporation
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

#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "nlb7.h"
#include "fpga_app/fpga_common.h"
#include "option.h"
#include "nlb.h"
#include "nlb_stats.h"
#include "safe_string/safe_string.h"
#include "diag_utils.h"


#define USE_UMSG 0
using namespace opae::fpga::types;
using namespace std::chrono;
using namespace intel::utils;
using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

static uint32_t max_cl = 65535;

nlb7::nlb7()
: name_("nlb7")
, config_("nlb7.json")
, target_("fpga")
, afu_id_("7BAF4DEA-A57C-E91E-168A-455D9BDA88A3")
, dsm_size_(MB(2))
, step_(1)
, begin_(1)
, end_(1)
, frequency_(DEFAULT_FREQ)
, notice_(nlb7_notice::poll)
, dsm_timeout_(FPGA_DSM_TIMEOUT)
, suppress_headers_(false)
, csv_format_(false)
, suppress_stats_(false)
, cachelines_(0)
{
    options_.add_option<bool>("help",                'h', option::no_argument,   "Show help", false);
    options_.add_option<bool>("version",             'v', option::no_argument,   "Show version", false);
    options_.add_option<std::string>("config",       'c', option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",       't', option::with_argument, "one of {fpga, ase}", target_);
    options_.add_option<uint32_t>("begin",           'b', option::with_argument, "where 1 <= <value> <= 65535", begin_);
    options_.add_option<uint32_t>("end",             'e', option::with_argument, "where 1 <= <value> <= 65535", end_);
    options_.add_option<std::string>("cache-policy", 'p', option::with_argument, "one of {wrline-I, wrline-M, wrpush-I}", "wrline-M");
    options_.add_option<std::string>("cache-hint",   'i', option::with_argument, "one of {rdline-I, rdline-S}", "rdline-I");
    options_.add_option<std::string>("read-vc",      'r', option::with_argument, "one of {auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<std::string>("write-vc",     'w', option::with_argument, "one of {auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<std::string>("wrfence-vc",   'f', option::with_argument, "one of {auto, vl0, vh0, vh1}", "auto");
    options_.add_option<std::string>("notice",       'N', option::with_argument, "one of {poll, csr-write, umsg-data, umsg-hint}", "poll");
    options_.add_option<uint64_t>("dsm-timeout-usec",     option::with_argument, "Timeout for test completion", dsm_timeout_.count());
    options_.add_option<uint8_t>("bus",              'B', option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",           'D', option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",         'F', option::with_argument, "Function number of PCIe device");
    options_.add_option<uint8_t>("socket-id",        'S', option::with_argument, "Socket id encoded in BBS");
    options_.add_option<std::string>("guid",         'G', option::with_argument, "accelerator id to enumerate", afu_id_);
    options_.add_option<uint32_t>("clock-freq",      'T', option::with_argument, "Clock frequency (used for bw measurements)", frequency_);
    options_.add_option<bool>("suppress-hdr",             option::no_argument,   "Suppress column headers", suppress_headers_);
    options_.add_option<bool>("csv",                 'V', option::no_argument,   "Comma separated value format", csv_format_);
    options_.add_option<bool>("suppress-stats",           option::no_argument,   "Show stats at end", suppress_stats_);
}

nlb7::~nlb7()
{
}

void nlb7::show_help(std::ostream &os)
{
    os << "Usage: fpgadiag --mode sw [options]:" << std::endl
       << std::endl;

    for (const auto &it : options_)
    {
        it->show_help(os);
    }
}

bool nlb7::setup()
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

    // begin, end
    options_.get_value<uint32_t>("begin", begin_);
    options_.get_value<uint32_t>("end", end_);
    auto end_opt = options_.find("end");


    if (begin_ > max_cl)
    {
        log_.error("nlb7") << "begin: " << begin_ << " is greater than max: " << max_cl << std::endl;
        return false;
    }

    if (end_opt && !end_opt->is_set())
    {
        end_ = begin_;
    }
    else
    {
        if (end_ < begin_)
        {
            log_.warn("nlb7") << "end: " << end_ << " is less than begin: " << begin_ << std::endl;
            end_ = begin_;
        }
    }

    cfg_ = nlb_mode::sw;

    // wrline-I, default=wrline-M, wrpush-I
    std::string cache_policy;
    options_.get_value<std::string>("cache-policy", cache_policy);
    if (cache_policy == "wrpush-I")
    {
        cfg_ |= nlb0_ctl::wrpush_i;
    }
    else if (cache_policy == "wrline-I")
    {
        cfg_ |= nlb0_ctl::wrline_i;
    }
    else if (cache_policy == "wrline-M")
    {
        cfg_ |= nlb0_ctl::wrline_m;
    }
    else
    {
        std::cerr << "Invalid --cache-policy: " << cache_policy << std::endl;
        return false;
    }

    // default=rdline-I, rdline-S
    std::string cache_hint;
    options_.get_value<std::string>("cache-hint", cache_hint);
    if (cache_hint == "rdline-I")
    {
        cfg_ |= nlb0_ctl::rdi;
    }
    else if (cache_hint == "rdline-S")
    {
        cfg_ |= nlb0_ctl::rds;
    }
    else
    {
        std::cerr << "Invalid --cache-hint: " << cache_hint << std::endl;
        return false;
    }

    // set the read channel
    // default=auto, vl0, vh0, vh1, random
    std::string rd_channel;
    options_.get_value<std::string>("read-vc", rd_channel);
    if (rd_channel == "vl0")
    {
        cfg_ |= nlb0_ctl::read_vl0;
    }
    else if (rd_channel == "vh0")
    {
        cfg_ |= nlb0_ctl::read_vh0;
    }
    else if (rd_channel == "vh1")
    {
        cfg_ |= nlb0_ctl::read_vh1;
    }
    else if (rd_channel == "random")
    {
        cfg_ |= nlb0_ctl::read_vr;
    }
    else if (rd_channel == "auto")
    {
        ;
    }
    else
    {
        std::cerr << "Invalid --read-vc: " << rd_channel << std::endl;
        return false;
    }

    // set the write fence channel
    // default=auto, vl0, vh0, vh1
    std::string wrfence = "auto";
    auto wrfence_opt = options_.find("wrfence-vc");
    options_.get_value<std::string>("wrfence-vc", wrfence);
    if (wrfence == "auto")
    {
        cfg_ |= nlb0_ctl::wrfence_va;
    }
    else if (wrfence == "vl0")
    {
        cfg_ |= nlb0_ctl::wrfence_vl0;
    }
    else if (wrfence == "vh0")
    {
        cfg_ |= nlb0_ctl::wrfence_vh0;
    }
    else if (wrfence == "vh1")
    {
        cfg_ |= nlb0_ctl::wrfence_vh1;
    }
    else
    {
        std::cerr << "Invalid --wrfence-vc: " << wrfence << std::endl;
        return false;
    }
    bool wrfence_set = wrfence_opt && wrfence_opt->is_set();

    // set the write channel
    // default=auto, vl0, vh0, vh1, random
    std::string wr_channel;
    options_.get_value<std::string>("write-vc", wr_channel);
    if (wr_channel == "vl0")
    {
        cfg_ |= nlb0_ctl::write_vl0;
        if (!wrfence_set)
        {
            cfg_ |= nlb0_ctl::wrfence_vl0;
        }
    }
    else if (wr_channel == "vh0")
    {
        cfg_ |= nlb0_ctl::write_vh0;
        if (!wrfence_set)
        {
            cfg_ |= nlb0_ctl::wrfence_vh0;
        }
    }
    else if (wr_channel == "vh1")
    {
        cfg_ |= nlb0_ctl::write_vh1;
        if (!wrfence_set)
        {
            cfg_ |= nlb0_ctl::wrfence_vh1;
        }
    }
    else if (wr_channel == "random")
    {
        cfg_ |= nlb0_ctl::write_vr;
    }
    else if (wr_channel == "auto")
    {
        ;
    }
    else
    {
        std::cerr << "Invalid --write-vc: " << wr_channel << std::endl;
        return false;
    }

    // default=poll, csr-write, umsg-data, umsg-hint
    std::string notice;
    options_.get_value<std::string>("notice", notice);
    if (notice == "poll")
    {
        notice_ = nlb7_notice::poll;
    }
    else if (notice == "csr-write")
    {
        notice_ = nlb7_notice::csr_write;
	cfg_ |= nlb0_ctl::csr_write;
    }
#if USE_UMSG
    else if (notice == "umsg-data")
    {
        notice_ = nlb7_notice::umsg_data;
    }
    else if (notice == "umsg-hint")
    {
        notice_ = nlb7_notice::umsg_hint;
    }
#endif // USE_UMSG
    else
    {
        std::cerr << "Invalid --notice: " << notice << std::endl;
        return false;
    }

    options_.get_value<uint32_t>("freq", frequency_);
    options_.get_value<bool>("suppress-hdr", suppress_headers_);
    options_.get_value<bool>("csv", csv_format_);

    // FIXME: use actual size for dsm size
    dsm_ = shared_buffer::allocate(accelerator_, dsm_size_);
    if (!dsm_) {
        log_.error("nlb7") << "failed to allocate DSM workspace." << std::endl;
        return false;
    }
    return true;
}

#define HIGH 0xffffffff
#define LOW  0

bool nlb7::run()
{
    bool res = true;
    const std::chrono::microseconds one_msec(1000);
    shared_buffer::ptr_t inout; // shared workspace, if possible
    shared_buffer::ptr_t inp;   // input workspace
    shared_buffer::ptr_t out;   // output workspace

    std::size_t buf_size = CL(end_ + 1);  // size of input and output buffer (each)

    // Allocate the smallest possible workspaces for DSM, Input and Output
    // buffers.

    if (buf_size <= KB(2) || (buf_size > KB(4) && buf_size <= MB(1)) ||
                             (buf_size > MB(2) && buf_size < MB(512))) {  // split
        inout = shared_buffer::allocate(accelerator_, buf_size * 2);
        if (!inout) {
            log_.error("nlb7") << "failed to allocate input/output buffers." << std::endl;
            return false;
        }
        std::vector<shared_buffer::ptr_t> bufs = split_buffer::split(inout, {buf_size, buf_size});
        inp = bufs[0];
        out = bufs[1];
    } else {
        inp = shared_buffer::allocate(accelerator_, buf_size);
        out = shared_buffer::allocate(accelerator_, buf_size);
        if (!inp || !out) {
            log_.error("nlb7") << "failed to allocate input/output buffers." << std::endl;
            return false;
        }
    }

    if (!inp) {
        log_.error("nlb7") << "failed to allocate input workspace." << std::endl;
        return false;
    }
    if (!out) {
        log_.error("nlb7") << "failed to allocate output workspace." << std::endl;
        return false;
    }

    umsg_set_mask(accelerator_, LOW);

#if USE_UMSG
    volatile uint8_t *pUMsgUsrVirt = (volatile uint8_t *) accelerator_->umsg_get_ptr();
    size_t pagesize = (size_t) sysconf(_SC_PAGESIZE);
    size_t UMsgBufSize = pagesize * (size_t) umsg_num(accelerator_);
#else
    volatile uint8_t *pUMsgUsrVirt = (volatile uint8_t *) NULL;
    size_t UMsgBufSize = 0;
#endif
    if (pUMsgUsrVirt &&
        ((nlb7_notice::umsg_data == notice_) ||
         (nlb7_notice::umsg_hint == notice_)))
    {
       if (nlb7_notice::umsg_data == notice_)
       {
           umsg_set_mask(accelerator_, LOW);
       }
       else
       {
           umsg_set_mask(accelerator_, HIGH);
       }
    }

    accelerator_->reset();

    // set dsm base, high then low
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->io_address()));
    // assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 0);
    // de-assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 1);
    // set input workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb7_csr::src_addr), CACHELINE_ALIGNED_ADDR(inp->io_address()));
    // set output workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb7_csr::dst_addr), CACHELINE_ALIGNED_ADDR(out->io_address()));

    accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::cfg), cfg_.value());

    uint32_t sz = CL(begin_);
    auto fme_token = get_parent_token(accelerator_);
    // Read perf counters.
    fpga_cache_counters  start_cache_ctrs  = fpga_cache_counters(fme_token);
    fpga_fabric_counters start_fabric_ctrs = fpga_fabric_counters(fme_token);

    while (sz <= CL(end_))
    {
        size_t MaxPoll = 1000;
        if (target_ == "ase")
            MaxPoll *= 100000;

        // Clear the UMsg address space.
        if (pUMsgUsrVirt)
            memset_s((uint8_t *)pUMsgUsrVirt, UMsgBufSize, 0);

        // Zero the output buffer.
        out->fill(0);

        // Re-initialize the input buffer.
        const uint32_t InputData = 0xdecafbad;
        for (size_t i = 0; i < inp->size()/sizeof(uint32_t); ++i)
            inp->write(InputData, i);

        // assert AFU reset.
        accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 0);
        // clear the DSM.
        dsm_->fill(0);
        // de-assert afu reset
        accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 1);
        // set number of cache lines for test
        accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::num_lines), sz / CL(1));
        // start the test
        accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 3);

        // Test flow
        // 1. CPU polls on address N+1
        bool poll_result = buffer_poll<uint32_t>(out, sz, dsm_timeout_, HIGH, HIGH);
        if (!poll_result)
        {
            std::cerr << "Maximum timeout for CPU poll on Address N+1 was exceeded" << std::endl;
            res = false;
            break;
        }

        // 2. CPU copies dest to src
        errno_t e;
        e = memcpy_s((void *)inp->c_type(), sz,
			(void *)out->c_type(), sz);
        if (EOK != e) {
            std::cerr << "memcpy_s failed" << std::endl;
            res = false;
            break;
        }

        // fence operation
        __sync_synchronize();

        // 3. CPU to FPGA message. Select notice type.
        if (nlb7_notice::csr_write == notice_)
        {
            accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::sw_notice), 0x10101010);
        }
        else if (pUMsgUsrVirt && ((nlb7_notice::umsg_data == notice_) ||
                 (nlb7_notice::umsg_hint == notice_)))
        {
            *(uint32_t *)pUMsgUsrVirt = HIGH;
        }
        else
        { // poll
            inp->write<uint32_t>(HIGH, sz);
            inp->write<uint32_t>(HIGH, sz+4);
            inp->write<uint32_t>(HIGH, sz+8);
        }

        // Wait for test completion
        poll_result = buffer_poll<uint32_t>(dsm_, (size_t)nlb0_dsm::test_complete,
                                          dsm_timeout_,
                                          (uint32_t)0x1,
                                          (uint32_t)1);
        if (!poll_result)
        {
            std::cerr << "Maximum Timeout for test complete was exceeded." << std::endl;
            res = false;
            break;
        }

        // stop the device
        accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 7);

        while ((0 == dsm_->read<uint32_t>((size_t)nlb0_dsm::test_complete)) &&
               MaxPoll)
        {
            --MaxPoll;
            buffer_poll<uint32_t>(dsm_, (size_t)nlb0_dsm::test_complete,
                                one_msec,
                                (uint32_t)0x1,
                                (uint32_t)1);
        }

        // Read Perf Counters
        fpga_cache_counters  end_cache_ctrs  = fpga_cache_counters(fme_token);
        fpga_fabric_counters end_fabric_ctrs = fpga_fabric_counters(fme_token);

        if (!MaxPoll)
        {
            std::cerr << "Maximum timeout for test stop was exceeded." << std::endl;
            res = false;
            std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                     sz/CL(1),
                                                     end_cache_ctrs - start_cache_ctrs,
                                                     end_fabric_ctrs - start_fabric_ctrs,
                                                     frequency_,
                                                     false,
                                                     suppress_headers_,
                                                     csv_format_);
            break;
        }

        auto test_error = dsm_->read<uint32_t>((size_t)nlb0_dsm::test_error);
        if (0 != test_error)
        {
            std::cerr << "Error bit set in DSM." << std::endl;
            std::cerr << "DSM Test Error: 0x" << std::hex << test_error << std::endl;

            if (test_error & 0x1)
            {
                std::cerr << "Unexpected Read or Write response" << std::endl;
            }
            else if(test_error & 0x4)
            {
                std::cerr << "Write FIFO overflow" << std::endl;
            }
            else if(test_error & 0x8)
            {
                std::cerr << "Read data not as expected when FPGA read back N lines" << std::endl;
            }

            std::cerr << "Mode error vector:" << std::endl;

            for (int i=0 ; i < 8 ; ++i)
            {
                std::cerr << "[" << i << "]: 0x" << std::hex <<
                        dsm_->read<uint32_t>((size_t)nlb7_dsm::mode_error + i*sizeof(uint32_t)) <<
                        std::endl;

                if (0 == i)
                {
                    std::cerr << " ; Read Data";
                }
                else if(1 == i)
                {
                    std::cerr << " ; Read response address";
                }
                else if(2 == i)
                {
                    std::cerr << " ; Read response header";
                }
                else if(3 == i)
                {
                    std::cerr << " ; Num of read responses";
                }

                std::cerr << std::endl;
            }

            std::cerr << std::dec << std::endl;

            res = false;

            std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                     sz/CL(1),
                                                     end_cache_ctrs - start_cache_ctrs,
                                                     end_fabric_ctrs - start_fabric_ctrs,
                                                     frequency_,
                                                     false,
                                                     suppress_headers_,
                                                     csv_format_);
            break;
        }

        // Check for num_clocks underflow.
        auto num_clocks = dsm_->read<uint32_t>((size_t)nlb7_dsm::num_clocks);
        auto start_overhead = dsm_->read<uint32_t>((size_t)nlb7_dsm::start_overhead);
        auto end_overhead = dsm_->read<uint32_t>((size_t)nlb7_dsm::end_overhead);
        if (num_clocks < start_overhead + end_overhead)
        {
            std::cerr << "Number of Clocks underflow." << std::endl;
            res = false;
            std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                     sz/CL(1),
                                                     end_cache_ctrs - start_cache_ctrs,
                                                     end_fabric_ctrs - start_fabric_ctrs,
                                                     frequency_,
                                                     false,
                                                     suppress_headers_,
                                                     csv_format_);
            break;
        }

        std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                 sz/CL(1),
                                                 end_cache_ctrs - start_cache_ctrs,
                                                 end_fabric_ctrs - start_fabric_ctrs,
                                                 frequency_,
                                                 false,
                                                 suppress_headers_,
                                                 csv_format_);

        // Save Perf Monitors
        start_cache_ctrs  = end_cache_ctrs;
        start_fabric_ctrs = end_fabric_ctrs;
        cachelines_ += sz/CL(1);
        sz += CL(1);
    }

    accelerator_->write_csr32(static_cast<uint32_t>(nlb7_csr::ctl), 0);

    accelerator_->reset();

    dsm_.reset();

    return res;
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

