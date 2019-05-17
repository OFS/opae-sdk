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

#include "nlb0.h"
#include "fpga_app/fpga_common.h"
#include "nlb_stats.h"
#include <uuid/uuid.h>
#include "diag_utils.h"
#include <chrono>
#include <thread>
#include <unistd.h>
//#include "perf_counters.h"
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

nlb0::nlb0()
: name_("nlb0")
, config_("nlb0.json")
, target_("fpga")
, afu_id_("D8424DC4-A4A3-C413-F89E-433683F9040B")
, nlb0_id_("D8424DC4-A4A3-C413-F89E-433683F9040B")
, dsm_size_(MB(2))
, step_(1)
, begin_(1)
, end_(1)
, frequency_(DEFAULT_FREQ)
, cont_(false)
, dsm_timeout_(FPGA_DSM_TIMEOUT)
, suppress_header_(false)
, csv_format_(false)
, suppress_stats_(false)
, cachelines_(0)
, offset_(0)
{
    options_.add_option<bool>("help",                'h', option::no_argument,   "Show help", false);
    options_.add_option<std::string>("config",       'c', option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",       't', option::with_argument, "one of {fpga, ase}", target_);
    options_.add_option<uint32_t>("begin",           'b', option::with_argument, "where 1 <= <value> <= 65535", begin_);
    options_.add_option<uint32_t>("end",             'e', option::with_argument, "where 1 <= <value> <= 65535", end_);
    options_.add_option<uint32_t>("multi-cl",        'u', option::with_argument, "one of {1, 2, 4}", 1);
    options_.add_option<bool>("cont",                'L', option::no_argument,   "Continuous mode", cont_);
    options_.add_option<std::string>("cache-policy", 'p', option::with_argument, "one of {wrline-I, wrline-M wrpush-I}", "wrline-M");
    options_.add_option<std::string>("cache-hint",   'i', option::with_argument, "one of {rdline-I, rdline-S}", "rdline-I");
    options_.add_option<std::string>("read-vc",      'r', option::with_argument, "one of {auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<std::string>("write-vc",     'w', option::with_argument, "one of {auto, vl0, vh0, vh1, random}", "auto");
    options_.add_option<std::string>("wrfence-vc",   'f', option::with_argument, "one of {auto, vl0, vh0, vh1}", "auto");
    options_.add_option<uint64_t>("dsm-timeout-usec",     option::with_argument, "Timeout for test completion", dsm_timeout_.count());
    options_.add_option<uint32_t>("timeout-usec",         option::with_argument, "Timeout for continuous mode (microseconds portion)", 0);
    options_.add_option<uint32_t>("timeout-msec",         option::with_argument, "Timeout for continuous mode (milliseconds portion)", 0);
    options_.add_option<uint32_t>("timeout-sec",          option::with_argument, "Timeout for continuous mode (seconds portion)", 1);
    options_.add_option<uint32_t>("timeout-min",          option::with_argument, "Timeout for continuous mode (minutes portion)", 0);
    options_.add_option<uint32_t>("timeout-hour",         option::with_argument, "Timeout for continuous mode (hours portion)", 0);
    options_.add_option<uint8_t>("socket-id",        'S', option::with_argument, "Socket id encoded in BBS");
    options_.add_option<uint8_t>("bus",              'B', option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",           'D', option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",         'F', option::with_argument, "Function number of PCIe device");
    options_.add_option<std::string>("guid",         'G', option::with_argument, "accelerator id to enumerate", afu_id_);
    options_.add_option<std::string>("id",           'I', option::with_argument, "NLB0 id to enumerate", nlb0_id_);
    options_.add_option<uint32_t>("freq",            'T', option::with_argument, "Clock frequency (used for bw measurements)", frequency_);
    options_.add_option<bool>("suppress-hdr",             option::no_argument,   "Suppress column headers", suppress_header_);
    options_.add_option<bool>("csv",                 'V', option::no_argument,   "Comma separated value format", csv_format_);
    options_.add_option<bool>("suppress-stats",           option::no_argument,   "Show stas at end", suppress_stats_);
}

nlb0::~nlb0()
{

}

bool nlb0::setup()
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
        // Statistics aren't available in ASE (no driver)
        suppress_stats_ = true;
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

    cfg_ = nlb_mode::loopback;
    if (options_.get_value<bool>("cont", cont_) && cont_)
    {
        cfg_ |= nlb0_ctl::cont;
        cont_ = true;
    }

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

    // set the read channel
    //{ auto, vl0, vh0, vh1, random}", "auto")
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

    // set the write fence channel
    //{ auto, vl0, vh0, vh1}", "wrfence-vc")
    std::string wrfence = "auto";
    auto wrfrence_opt = options_.find("wrfence-vc");
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

    bool wrfence_set = wrfrence_opt && wrfrence_opt->is_set();
    // set the write channel
    //{ auto, vl0, vh0, vh1, random}", "auto")
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

    uint32_t multi_cl = 1;
    if (options_.get_value<uint32_t>("multi-cl", multi_cl))
    {
        switch(multi_cl)
        {
            case 2 :
                cfg_ |= nlb0_ctl::mcl2;
                step_ = 2;
                break;
            case 4 :
                cfg_ |= nlb0_ctl::mcl4;
                step_ = 4;
                break;
        }
    }

    // begin, end
    options_.get_value<uint32_t>("begin", begin_);
    options_.get_value<uint32_t>("end", end_);
    auto end_opt = options_.find("end");

    if (begin_ > MAX_CL)
    {
        log_.error("nlb0") << "begin: " << begin_ << " is greater than max: " << MAX_CL << std::endl;
        return false;
    }

    if (begin_ % step_ > 0)
    {
        log_.error("nlb0") << "begin: " << begin_ << " is not a multiple of mcl: " << step_ << std::endl;
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
            log_.warn("nlb0") << "end: " << end_ << " is less than begin: " << begin_ << std::endl;
            end_ = begin_;
        }
    }

    if (cont_)
    {
        cont_timeout_ = duration<double>(0.0);
        uint32_t timeout_usec = 0;
        if (options_.get_value<uint32_t>("timeout-usec", timeout_usec))
        {
            cont_timeout_ += microseconds(timeout_usec);
        }
        uint32_t timeout_msec = 0;
        if (options_.get_value<uint32_t>("timeout-msec", timeout_msec))
        {
            cont_timeout_ += milliseconds(timeout_msec);
        }
        uint32_t timeout_sec = 0;
        if (options_.get_value<uint32_t>("timeout-sec", timeout_sec))
        {
            cont_timeout_ += seconds(timeout_sec);
        }
        uint32_t timeout_min = 0;
        if (options_.get_value<uint32_t>("timeout-min", timeout_min))
        {
            cont_timeout_ += minutes(timeout_min);
        }
        uint32_t timeout_hour = 0;
        if (options_.get_value<uint32_t>("timeout-hour", timeout_hour))
        {
            cont_timeout_ += hours(timeout_hour);
        }
    }

    options_.get_value<uint32_t>("freq", frequency_);
    options_.get_value<bool>("suppress-hdr", suppress_header_);
    options_.get_value<bool>("csv", csv_format_);

    options_.get_value<std::string>("id", nlb0_id_);
    fpga_guid nlb0_id;

    if (uuid_parse(nlb0_id_.c_str(), nlb0_id)<0) {
        std::cerr << "Invalid NLB0 id:" << nlb0_id_ << std::endl;
        return false;
    }

    uint32_t offset = 0;
    uint64_t dfh = 0;

    uint8_t* n = static_cast<uint8_t *>(&nlb0_id)
    //uint8_t* n = (uint8_t *)&nlb0_id;
    uint64_t nlb0_hi = bswap_64(*n);
    uint64_t nlb0_lo = bswap_64(*(n+8));

    // enumerate dfh if input id arguement 
    if (options_.find("id")) {

        bool found = false;

        do {
             uint64_t feature_uuid_lo, feature_uuid_hi;
             // Read the next feature header
             dfh = accelerator_->read_csr64(static_cast<uint32_t>(offset));
             feature_uuid_lo = accelerator_->read_csr64(static_cast<uint32_t>(offset + 8));
             feature_uuid_hi = accelerator_->read_csr64(static_cast<uint32_t>(offset + 16));
             if ((nlb0_lo == feature_uuid_lo) && (nlb0_hi == feature_uuid_hi)) {
                  offset_ = offset;
                  found = true;
                  printf("found the NLB offset=0x%x\n", offset);
                  break;
             }

            offset += NEXT_DFH_OFFSET(dfh);
        } while (!DFH_EOL(dfh));

        if (!found) {
            std::cerr << "Not found NLB:" << nlb0_id_ << std::endl;
            return false;
        }

    }

    // TODO: Infer pclock from the device id
    // For now, get the pclock frequency from status2 register
    // that frequency (MHz) is encoded in bits [47:32]
    uint64_t s2 = accelerator_->read_csr64(static_cast<uint32_t>(nlb0_csr::status2));
    uint32_t freq = (s2 >> 32) & 0xffff;
    if (freq > 0) {
         // frequency_ is in Hz
         frequency_ = freq * 1E6;
    }

    // FIXME: use actual size for dsm size
    dsm_ = shared_buffer::allocate(accelerator_, dsm_size_);
    if (!dsm_) {
        log_.error("nlb0") << "failed to allocate DSM workspace." << std::endl;
        return false;
    }
    return true;
}

bool nlb0::run()
{
    auto fme_token = get_parent_token(accelerator_);
    shared_buffer::ptr_t inout; // shared workspace, if possible
    shared_buffer::ptr_t inp;   // input workspace
    shared_buffer::ptr_t out;   // output workspace

    std::size_t buf_size = CL(end_);  // size of input and output buffer (each)

    // Allocate the smallest possible workspaces for DSM, Input and Output
    // buffers.

    if (buf_size <= KB(2) || (buf_size > KB(4) && buf_size <= MB(1)) ||
                             (buf_size > MB(2) && buf_size < MB(512))) {  // split
        inout = shared_buffer::allocate(accelerator_, buf_size * 2);
        if (!inout) {
            log_.error("nlb0") << "failed to allocate input/output buffers." << std::endl;
            return false;
        }
        std::vector<shared_buffer::ptr_t> bufs = split_buffer::split(inout, {buf_size, buf_size});
        inp = bufs[0];
        out = bufs[1];
    } else {
        inp = shared_buffer::allocate(accelerator_, buf_size);
        out = shared_buffer::allocate(accelerator_, buf_size);
        if (!inp || !out) {
            log_.error("nlb0") << "failed to allocate input/output buffers." << std::endl;
            return false;
        }
    }

    if (!inp) {
        log_.error("nlb0") << "failed to allocate input workspace." << std::endl;
        return false;
    }
    if (!out) {
        log_.error("nlb0") << "failed to allocate output workspace." << std::endl;
        return false;
    }

    inp->write<uint8_t>(0xA, 15);

    accelerator_->reset();

    // set dsm base, high then low
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->io_address()));
    // assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
    // de-assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 1);
    // set input workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::src_addr), CACHELINE_ALIGNED_ADDR(inp->io_address()));
    // set output workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::dst_addr), CACHELINE_ALIGNED_ADDR(out->io_address()));
    // set the test mode
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cfg), cfg_.value());

    for (size_t i = 0; i < inp->size(); ++i)
    {
        inp->write(i, i);
    }

    dsm_tuple dsm_tpl;
    for (uint32_t i = begin_; i <= end_; i+=step_)
    {
        dsm_->fill(0);
        out->fill(0);

        // assert afu reset
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
        // de-assert afu reset
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 1);

        // set number of cache lines for test
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::num_lines), i);

        // Read perf counters.
        fpga_cache_counters  start_cache_ctrs;
        fpga_fabric_counters start_fabric_ctrs;
        if (!suppress_stats_)
        {
            start_cache_ctrs  = fpga_cache_counters(fme_token);
            start_fabric_ctrs = fpga_fabric_counters(fme_token);
        }
        // start the test
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 3);

        if (cont_)
        {
            std::this_thread::sleep_for(cont_timeout_);
            // stop the device
            accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
            if (!buffer_wait(dsm_, static_cast<size_t>(nlb0_dsm::test_complete),
                           std::chrono::microseconds(10), dsm_timeout_, 0x1, 1))
            {
                log_.error("nlb0") << "test timeout at "
                                   << i << " cachelines." << std::endl;
                return false;
            }
        }
        else
        {
            if (!buffer_wait(dsm_, static_cast<size_t>(nlb0_dsm::test_complete),
                        std::chrono::microseconds(10), dsm_timeout_, 0x1, 1))
            {
                log_.error("nlb0") << "test timeout at "
                                   << i << " cachelines." << std::endl;
                return false;
            }
            // stop the device
            accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
        }
        cachelines_ += i;
        // if we don't suppress stats then we show them at the end of each iteration
	if (!suppress_stats_)
        {
            // Read Perf Counters
            fpga_cache_counters  end_cache_ctrs  = fpga_cache_counters(fme_token);
            fpga_fabric_counters end_fabric_ctrs = fpga_fabric_counters(fme_token);
            std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                     i,
                                                     end_cache_ctrs - start_cache_ctrs,
                                                     end_fabric_ctrs - start_fabric_ctrs,
                                                     frequency_,
                                                     cont_,
                                                     suppress_header_,
                                                     csv_format_);
        }
        else
        {
            // if we suppress stats, add the current dsm stats to the rolling tuple
            dsm_tpl += dsm_tuple(dsm_);
        }
        // verify in and out
        if (inp->compare(out, i * cacheline_size))
        {
            // put the tuple back into the dsm buffer
            dsm_tpl.put(dsm_);
            std::cerr << "Input and output buffer mismatch when testing on "
                      << i << " cache lines" << std::endl;
            return false;
        }

        // Wait for the AFU's read/write traffic to complete. Give up after 100
        // tries.
        uint32_t afu_traffic_trips = 0;
        while (afu_traffic_trips < 100)
        {
            // CSR_STATUS1 holds two 32 bit values: num pending reads and writes.
            // Wait for it to be 0.
            uint64_t s1 = accelerator_->read_csr64(static_cast<uint32_t>(nlb0_csr::status1));
            if (s1 == 0)
            {
                break;
            }

            afu_traffic_trips += 1;
            usleep(1000);
        }
    }
    // put the tuple back into the dsm buffer
    dsm_tpl.put(dsm_);

    dsm_.reset();

    return true;
}


void nlb0::show_help(std::ostream &os)
{
    os << "Usage: fpgadiag --mode lpbk1 [options]:" << std::endl
       << std::endl;

    for (const auto & it : options_)
    {
        it->show_help(os);
    }
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel





