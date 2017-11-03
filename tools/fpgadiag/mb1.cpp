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

#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "mb1.h"
#include "fpga_app/fpga_common.h"
#include "option.h"
#include "nlb_stats.h"
#include "nlb.h"

using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

struct cacheline_t
{
    uint64_t data[8];
};

mb1::mb1()
: name_("mb1")
, config_("mb1.json")
, target_("fpga")
, cachelines_(1024)
, num_sw_repeat_(1)
, read_vc_("auto")
, cache_hint_("rdline-I")
, cool_cpu_cache_(false)
, guid_("C000C966-0D82-4272-9AEF-FE5F84570612")
, frequency_(MHZ(400))
, wkspc_size_(GB(1))
, dsm_size_(MB(2))
, inp_size_(CL(65536))
, out_size_(CL(65536))
, cool_cache_size_(MB(200))
, suppress_header_(false)
, csv_format_(false)
{
    options_.add_option<bool>("help",              'h', intel::utils::option::no_argument,   "Show help", false);
    options_.add_option<std::string>("config",     'c', intel::utils::option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",     't', intel::utils::option::with_argument, "one of { fpga, ase }", target_);
    options_.add_option<uint32_t>("num-lines",     'l', intel::utils::option::with_argument, "Number of cache lines", cachelines_);
    options_.add_option<uint32_t>("num-sw-repeat", 'R', intel::utils::option::with_argument, "Number of test repetitions", num_sw_repeat_);
    options_.add_option<std::string>("read-vc",    'r', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1, random }", read_vc_);
    options_.add_option<std::string>("cache-hint", 'i', intel::utils::option::with_argument, "one of { rdline-I, rdline-S }", cache_hint_);
    options_.add_option<bool>("cool-cpu-cache",    'C', intel::utils::option::no_argument,   "cool down cpu cache", cool_cpu_cache_);
    options_.add_option<uint8_t>("bus-number",     'B', intel::utils::option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",         'D', intel::utils::option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",       'F', intel::utils::option::with_argument, "Function number of PCIe device");
    options_.add_option<uint8_t>("socket-id",      's', intel::utils::option::with_argument, "Socket id encoded in BBS");
    options_.add_option<std::string>("guid",       'g', intel::utils::option::with_argument, "accelerator id to enumerate", guid_);
    options_.add_option<uint32_t>("clock-freq",    'T', intel::utils::option::with_argument, "Clock frequency (used for bw measurements)", frequency_);
    options_.add_option<bool>("suppress-hdr",      'S', intel::utils::option::no_argument,   "Suppress column headers", suppress_header_);
    options_.add_option<bool>("csv",               'V', intel::utils::option::no_argument,   "Comma separated value format", csv_format_);
}

mb1::~mb1()
{
}

void mb1::show_help(std::ostream &os)
{
    os << "Usage: mb1 [options]" << std::endl
       << std::endl;

    os << "   options are chosen from the following and may appear in the JSON" << std::endl
       << "   file specified by --config." << std::endl
       << std::endl;

    intel::utils::option::ptr_t o;

    o = options_["config"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<std::string>() << std::endl;

    o = options_["target"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<std::string>() << std::endl;

    o = options_["num-lines"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<uint32_t>() << std::endl;

    o = options_["num-sw-repeat"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<uint32_t>() << std::endl;

    o = options_["read-vc"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<std::string>() << std::endl;

    o = options_["cache-hint"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<std::string>() << std::endl;

    o = options_["cool-cpu-cache"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<bool>() << std::endl;

    o = options_["bus-number"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << "." << std::endl;

    o = options_["device"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << "." << std::endl;

    o = options_["function"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << "." << std::endl;

    o = options_["socket-id"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << "." << std::endl;

    o = options_["guid"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<std::string>() << std::endl;

    o = options_["clock-freq"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<uint32_t>() << std::endl;

    o = options_["suppress-hdr"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<bool>() << std::endl;

    o = options_["csv"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help()
       << ". Default=" << o->value<bool>() << std::endl;

    o = options_["help"];
    os << "    --" << o->name() << ",-" << o->short_opt() << " " << o->help() << std::endl;
}

bool mb1::setup()
{
    options_.get_value<std::string>("target", target_);
    if ((target_ != "fpga") && (target_ != "ase"))
    {
        std::cerr << "Invalid --target: " << target_ << std::endl;
        return false;
    }

    options_.get_value<uint32_t>("num-lines", cachelines_);
    if (0 == cachelines_)
    {
        std::cerr << "Invalid --num-lines: " << cachelines_ << std::endl;
        return false;
    }

    options_.get_value<uint32_t>("num-sw-repeat", num_sw_repeat_);
    if (0 == num_sw_repeat_)
    {
        std::cerr << "Invalid --num-sw-repeat: " << num_sw_repeat_ << std::endl;
        return false;
    }

    cfg_ = 0;

    // set the read channel
    // default=auto, vl0, vh0, vh1, random
    std::string read_vc_;
    if (options_.get_value<std::string>("read-vc", read_vc_))
    {
        if (read_vc_ == "vl0")
        {
            cfg_ |= mb1_ctl::read_vl0;
        }
        else if (read_vc_ == "vh0")
        {
            cfg_ |= mb1_ctl::read_vh0;
        }
        else if (read_vc_ == "vh1")
        {
            cfg_ |= mb1_ctl::read_vh1;
        }
        else if (read_vc_ == "random")
        {
            cfg_ |= mb1_ctl::read_vr;
        }
        else if (read_vc_ == "auto")
        {
            cfg_ |= mb1_ctl::va;
        }
        else
        {
            std::cerr << "Invalid --read-vc: " << read_vc_ << std::endl;
            return false;
        }
    }

    // default=rdline-I, rdline-S
    options_.get_value<std::string>("cache-hint", cache_hint_);
    if (cache_hint_ == "rdline-I")
    {
        cfg_ |= mb1_ctl::rdi;
    }
    else if (cache_hint_ == "rdline-S")
    {
        cfg_ |= mb1_ctl::rds;
    }
    else
    {
        std::cerr << "Invalid --cache-hint: " << cache_hint_ << std::endl;
        return false;
    }

    options_.get_value<bool>("cool-cpu-cache", cool_cpu_cache_);
    options_.get_value<std::string>("guid", guid_);
    options_.get_value<uint32_t>("clock-freq", frequency_);

    options_.get_value<bool>("suppress-hdr", suppress_header_);
    options_.get_value<bool>("csv", csv_format_);

    return true;
}

bool mb1::run()
{
    // Allocate one large workspace, then carve it up amongst
    // DSM, Input, and Output buffers.

    bool res = true;

    dma_buffer::ptr_t dsm = accelerator_->allocate_buffer(dsm_size_);
    if (!dsm) {
        log_.error("mb1") << "failed to allocate DSM workspace." << std::endl;
        return false;
    }


    dma_buffer::ptr_t inout; // shared workspace, if possible
    dma_buffer::ptr_t inp;   // input workspace
    dma_buffer::ptr_t out;   // output workspace

    std::size_t buf_size = CL(cachelines_);
    if (buf_size <= KB(2) || (buf_size > KB(4) && buf_size <= MB(1)) ||
                             (buf_size > MB(2) && buf_size < MB(512))) {  // split
        inout = accelerator_->allocate_buffer(buf_size * 2);
        std::vector<dma_buffer::ptr_t> bufs = dma_buffer::split(inout, {buf_size, buf_size});
        inp = bufs[0];
        out = bufs[1];
    } else {
        inp = accelerator_->allocate_buffer(buf_size);
        out = accelerator_->allocate_buffer(buf_size);
    }

    inp->fill(0);
    volatile cacheline_t *cl_ptr = reinterpret_cast<volatile cacheline_t*>(inp->address());
    for (size_t i = 0; i < cachelines_; ++i, ++cl_ptr)
    {
        cl_ptr->data[0] = (i + 1)%cachelines_;
    }

    // Initialize the output buffer
    out->fill(0);

    if (!accelerator_->reset())
    {
        std::cerr << "accelerator reset failed." << std::endl;
        return false;
    }

    // set dsm base, high then low
    accelerator_->write_mmio64(static_cast<uint32_t>(mb1_csr::basel), reinterpret_cast<uint64_t>(dsm_->iova()));
    // assert AFU reset.
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::ctl), 0);
    // clear the DSM.
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::ctl), 1);

    if (target_ == "ase")
    { // give ASE time to catch up.
        sleep(5);
    }

    // set input workspace address
    accelerator_->write_mmio64(static_cast<uint32_t>(mb1_csr::src_addr), CACHELINE_ALIGNED_ADDR(inp->iova()));
    // set output workspace address
    accelerator_->write_mmio64(static_cast<uint32_t>(mb1_csr::dst_addr), CACHELINE_ALIGNED_ADDR(out->iova()));
    // set number of cache lines for test
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::num_lines), cachelines_);
    // Set Control Parameters for sw test (No. of times each instance has to run)
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::sw_ctl), num_sw_repeat_);
    // set the test config
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::cfg), cfg_.value());

    std::vector<uint32_t> cpu_cool_buffer(0);
    if (cool_cpu_cache_)
    {
        cpu_cool_buffer.resize(cool_cache_size_/sizeof(uint32_t));
        uint32_t  i = 0;
        for (auto & r32 : cpu_cool_buffer)
            r32 = i++;
    }

    // Read perf counters.
    fpga_cache_counters  start_cache_ctrs = accelerator_->cache_counters();
    fpga_fabric_counters start_fabric_ctrs = accelerator_->fabric_counters();

    // start the test
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::ctl), 3);

    dma_buffer::microseconds_t timeout(3000000);

    bool poll_result = dsm_->poll<uint32_t>((size_t)mb1_dsm::test_complete,
                                           timeout,
                                           (uint32_t)0x1,
                                           (uint32_t)1);

    // Read Perf Counters
    fpga_cache_counters  end_cache_ctrs  = accelerator_->cache_counters();
    fpga_fabric_counters end_fabric_ctrs = accelerator_->fabric_counters();

    if (!poll_result)
    {
        std::cerr << "Timeout waiting for test complete." << std::endl;
        res = false;
    }

    std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                             cachelines_,
                                             end_cache_ctrs - start_cache_ctrs,
                                             end_fabric_ctrs - start_fabric_ctrs,
                                             frequency_,
                                             false,
                                             suppress_header_,
                                             csv_format_);

    // stop the device
    accelerator_->write_mmio32(static_cast<uint32_t>(mb1_csr::ctl), 7);

    return res;
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

