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
#include <future>
#include "cmdq7.h"
#include "fpga_app/fpga_common.h"
#include "option.h"
#include "nlb.h"
#include "nlb_cache_prime.h"
#include "safe_string/safe_string.h"

#define COOL_FPGA_CACHE 0
#define USE_UMSG        0

using namespace std::chrono;
using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

cmdq7::cmdq7()
: name_("cmdq7")
, config_("cmdq7.json")
, guid_("7BAF4DEA-A57C-E91E-168A-455D9BDA88A3")
, target_("fpga")
, allocations_(1)
, cachelines_(1)
, cont_(false)
, timeout_usec_(0)
, timeout_msec_(0)
, timeout_sec_(1)
, timeout_min_(0)
, timeout_hour_(0)
, freq_(MHZ(400))
, notice_(poll)
, cache_policy_("wrline-M")
, cache_hint_("rdline-I")
, multi_cl_(1)
, read_vc_("auto")
, write_vc_("auto")
, wrfence_vc_("auto")
, cool_fpga_cache_(false)
, dsm_size_(MB(4))
, timeout_(0)
, errors_(0)
, umsg_virt_(nullptr)
, umsg_size_(0)
, suppress_header_(false)
, csv_format_(false)
{
    options_.add_option<bool>("help",                 'h', intel::utils::option::no_argument,   "Show help", false);
    options_.add_option<std::string>("config",        'c', intel::utils::option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",        't', intel::utils::option::with_argument, "one of { fpga, ase }", target_);
    options_.add_option<std::uint32_t>("allocations", 'a', intel::utils::option::with_argument, "Number of FIFO entries",  allocations_);
    options_.add_option<std::uint32_t>("cachelines",  'l', intel::utils::option::with_argument, "Size of each FIFO entry", cachelines_);
    options_.add_option<bool>("cont",                 'L', intel::utils::option::no_argument,   "Enable continuous mode", cont_);
    options_.add_option<uint32_t>("timeout-usec",     'O', intel::utils::option::with_argument, "Timeout (usec) for cont mode", timeout_usec_);
    options_.add_option<uint32_t>("timeout-msec",     'Q', intel::utils::option::with_argument, "Timeout (msec) for cont mode", timeout_msec_);
    options_.add_option<uint32_t>("timeout-sec",      'X', intel::utils::option::with_argument, "Timeout (sec) for cont mode", timeout_sec_);
    options_.add_option<uint32_t>("timeout-min",      'Y', intel::utils::option::with_argument, "Timeout (minutes) for cont mode", timeout_min_);
    options_.add_option<uint32_t>("timeout-hour",     'Z', intel::utils::option::with_argument, "Timeout (hours) for cont mode", timeout_hour_);
    options_.add_option<uint32_t>("freq",             'T', intel::utils::option::no_argument,   "clock frequency", freq_);
#if USE_UMSG
    options_.add_option<std::string>("notice",        'N', intel::utils::option::with_argument, "one of { poll, csr-write, umsg-data, umsg-hint }", "poll");
#else
    options_.add_option<std::string>("notice",        'N', intel::utils::option::with_argument, "one of { poll, csr-write }", "poll");
#endif // USE_UMSG
    options_.add_option<std::string>("cache-policy",  'p', intel::utils::option::with_argument, "one of { wrline-I, wrline-M, wrpush-I }", cache_policy_);
    options_.add_option<std::string>("cache-hint",    'i', intel::utils::option::with_argument, "one of { rdline-I, rdline-S }", cache_hint_);
    options_.add_option<uint32_t>("multi-cl",         'u', intel::utils::option::with_argument, "one of { 1, 2, 4 }", multi_cl_);
    options_.add_option<std::string>("read-vc",       'r', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1, random }", read_vc_);
    options_.add_option<std::string>("write-vc",      'w', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1, random }", write_vc_);
    options_.add_option<std::string>("wrfence-vc",    'f', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1 }", wrfence_vc_);
#if COOL_FPGA_CACHE
    options_.add_option<bool>("cool-fpga-cache",      'M', intel::utils::option::no_argument,   "cool down fpga cache", cool_fpga_cache_);
#endif
    options_.add_option<uint8_t>("bus-number",        'B', intel::utils::option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",            'D', intel::utils::option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",          'F', intel::utils::option::with_argument, "Function number of PCIe device");
    options_.add_option<uint8_t>("socket-id",         's', intel::utils::option::with_argument, "Socket id encoded in BBS");
    options_.add_option<std::string>("guid",          'g', intel::utils::option::with_argument, "accelerator id to enumerate", guid_);
    options_.add_option<bool>("suppress-hdr",         'S', intel::utils::option::no_argument,   "Suppress column headers", suppress_header_);
    options_.add_option<bool>("csv",                  'V', intel::utils::option::no_argument,   "Comma separated value format", csv_format_);
}

cmdq7::~cmdq7()
{
}

void cmdq7::show_help(std::ostream &os)
{
    os << "Usage: cmdq7" << std::endl
       << std::endl;

    for (const auto &it : options_)
    {
        it->show_help(os);
    }
}

bool cmdq7::setup()
{
    options_.get_value<std::string>("target", target_);
    if ((target_ != "fpga") && (target_ != "ase"))
    {
        std::cerr << "Invalid --target: " << target_ << std::endl;
        return false;
    }

    options_.get_value<uint32_t>("allocations", allocations_);
    options_.get_value<uint32_t>("cachelines", cachelines_);

    options_.get_value<bool>("cont", cont_);
    options_.get_value<uint32_t>("timeout-usec", timeout_usec_);
    options_.get_value<uint32_t>("timeout-msec", timeout_msec_);
    options_.get_value<uint32_t>("timeout-sec", timeout_sec_);
    options_.get_value<uint32_t>("timeout-min", timeout_min_);
    options_.get_value<uint32_t>("timeout-hour", timeout_hour_);

    timeout_ = (double)timeout_usec_ +
               ((double)timeout_msec_ * 1000.0) +
               ((double)timeout_sec_  * 1000.0*1000.0) +
               ((double)timeout_min_  * 60.0*1000.0*1000.0) +
               ((double)timeout_hour_ * 3600.0*1000.0*1000.0);

    options_.get_value<uint32_t>("freq", freq_);
#if COOL_FPGA_CACHE
    options_.get_value<bool>("cool-fpga-cache", cool_fpga_cache_);
#endif

    std::string notice;

    options_.get_value<std::string>("notice", notice);
    if (notice == "poll")
    {
        notice_ = poll;
    }
    else if (notice == "csr-write")
    {
        notice_ = csr_write;
    }
#if USE_UMSG
    else if (notice == "umsg-data")
    {
        notice_ = umsg_data;
    }
    else if (notice == "umsg-hint")
    {
        notice_ = umsg_hint;
    }
#endif // USE_UMSG
    else
    {
        std::cerr << "Invalid --notice: " << notice << std::endl;
        return false;
    }

    cfg_ = nlb_mode::sw;

    options_.get_value<std::string>("cache-policy", cache_policy_);
    if (cache_policy_ == "wrpush-I")
    {
        cfg_ |= nlb0_ctl::wrpush_i;
    }
    else if (cache_policy_ == "wrline-I")
    {
        cfg_ |= nlb0_ctl::wrline_i;
    }
    else if (cache_policy_ == "wrline-M")
    {
        cfg_ |= nlb0_ctl::wrline_m;
    }
    else
    {
        std::cerr << "Invalid --cache-policy: " << cache_policy_ << std::endl;
        return false;
    }

    options_.get_value<std::string>("cache-hint", cache_hint_);
    if (cache_hint_ == "rdline-I")
    {
        cfg_ |= nlb0_ctl::rdi;
    }
    else if (cache_hint_ == "rdline-S")
    {
        cfg_ |= nlb0_ctl::rds;
    }
    else
    {
        std::cerr << "Invalid --cache-hint: " << cache_hint_ << std::endl;
        return false;
    }

    options_.get_value<uint32_t>("multi-cl", multi_cl_);
    if (multi_cl_ == 2)
    {
        cfg_ |= nlb0_ctl::mcl2;
    }
    else if (multi_cl_ == 4)
    {
        cfg_ |= nlb0_ctl::mcl4;
    }
    else if (multi_cl_ != 1)
    {
        std::cerr << "Invalid --multi-cl: " << multi_cl_ << std::endl;
        return false;
    }

    options_.get_value<std::string>("read-vc", read_vc_);
    if (read_vc_ == "auto")
    {
        cfg_ |= nlb0_ctl::va;
    }
    else if (read_vc_ == "vl0")
    {
        cfg_ |= nlb0_ctl::read_vl0;
    }
    else if (read_vc_ == "vh0")
    {
        cfg_ |= nlb0_ctl::read_vh0;
    }
    else if (read_vc_ == "vh1")
    {
        cfg_ |= nlb0_ctl::read_vh1;
    }
    else if (read_vc_ == "random")
    {
        cfg_ |= nlb0_ctl::read_vr;
    }
    else
    {
        std::cerr << "Invalid --read-vc: " << read_vc_ << std::endl;
        return false;
    }

    options_.get_value<std::string>("write-vc", write_vc_);
    if (write_vc_ == "auto")
    {
        cfg_ |= nlb0_ctl::va;
    }
    else if (write_vc_ == "vl0")
    {
        cfg_ |= nlb0_ctl::write_vl0;
    }
    else if (write_vc_ == "vh0")
    {
        cfg_ |= nlb0_ctl::write_vh0;
    }
    else if (write_vc_ == "vh1")
    {
        cfg_ |= nlb0_ctl::write_vh1;
    }
    else if (write_vc_ == "random")
    {
        cfg_ |= nlb0_ctl::write_vr;
    }
    else
    {
        std::cerr << "Invalid --write-vc: " << write_vc_ << std::endl;
        return false;
    }

    options_.get_value<std::string>("wrfence-vc", wrfence_vc_);
    if (wrfence_vc_ == "auto")
    {
        cfg_ |= nlb0_ctl::wrfence_va;
    }
    else if (wrfence_vc_ == "vl0")
    {
        cfg_ |= nlb0_ctl::wrfence_vl0;
    }
    else if (wrfence_vc_ == "vh0")
    {
        cfg_ |= nlb0_ctl::wrfence_vh0;
    }
    else if (wrfence_vc_ == "vh1")
    {
        cfg_ |= nlb0_ctl::wrfence_vh1;
    }
    else
    {
        std::cerr << "Invalid --wrfence-vc: " << wrfence_vc_ << std::endl;
        return false;
    }

    options_.get_value<bool>("suppress-hdr", suppress_header_);
    options_.get_value<bool>("csv", csv_format_);

    return true;
}

#define HIGH 0xffffffff
#define LOW  0

bool cmdq7::run()
{
    uint32_t buffer_size = CL(cachelines_ + 1);
    uint32_t alloc_size = 2 * allocations_ * buffer_size;

    if (alloc_size < GB(1))
    {
        alloc_size = GB(1);
    }

    wkspc_ = accelerator_->allocate_buffer(alloc_size);
    if (!wkspc_)
    {
        std::cerr << "failed to allocate workspace." << std::endl;
        return false;
    }

    // Split the DSM from the workspace.
    std::vector<dma_buffer::ptr_t> bufs = dma_buffer::split(wkspc_,
            { dsm_size_, nlb_cache_cool::fpga_cache_cool_size,
              alloc_size - (dsm_size_ + nlb_cache_cool::fpga_cache_cool_size) });

    // Create and enqueue the input/output buffer pairs.
    dsm_ = bufs[0];
    dma_buffer::ptr_t cool_buf = bufs[1];
    dma_buffer::ptr_t avail = bufs[2];
    uint32_t avail_size = avail->size();

    for (uint32_t i = 0 ; i < allocations_ ; ++i)
    {
        avail_size -= 2 * buffer_size;
        bufs = dma_buffer::split(avail, { buffer_size, buffer_size, avail_size });

        bufs[0]->fill(0xaf);
        bufs[1]->fill(0xbe);

        fifo1_.push_back(std::make_pair(bufs[0], bufs[1]));

        avail = bufs[2];
    }

    accelerator_->umsg_set_mask(LOW);

#if USE_UMSG
    umsg_virt_ = (volatile uint8_t *) accelerator_->umsg_get_ptr();
    size_t pagesize = (size_t) sysconf(_SC_PAGESIZE);
    umsg_size_ = pagesize * (size_t) accelerator_->umsg_num();
#else
    umsg_virt_ = (volatile uint8_t *) NULL;
    umsg_size_ = 0;
#endif

    if (umsg_virt_ &&
        ((nlb7_notice::umsg_data == notice_) ||
         (nlb7_notice::umsg_hint == notice_)))
    {
        if (nlb7_notice::umsg_data == notice_)
        {
            accelerator_->umsg_set_mask(LOW);
        }
        else
        {
            accelerator_->umsg_set_mask(HIGH);
        }
    }

    if (!accelerator_->reset())
    {
        std::cerr << "accelerator reset failed." << std::endl;
        return false;
    }

    if (cool_fpga_cache_)
    {
        nlb_cache_cool cooler(target_, accelerator_, dsm_, cool_buf, true);
        if (!cooler.cool())
        {
            std::cerr << "fpga cache cooling timed out." << std::endl;
            return false;
        }
    }

    // set dsm base, high then low
    accelerator_->write_mmio64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->iova()));
    // assert afu reset
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::ctl), 1);
    // set the test mode
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::cfg), cfg_.value());

    // spawn another thread to check for HWVALID being set to 0
    // when HWVALID is 0, then check the buffers for the test mode
    // eg. test mode is 0, then check output buffers match input buffers

    cancel_ = false;
    errors_ = 0;

    std::future<uint32_t> swvalid;
    std::future<uint32_t> hwvalid;

    if (cont_)
    {
        swvalid = std::async(std::launch::async,
                             &cmdq7::cont_swvalid_thr,
                             this,
                             std::ref(fifo1_),
                             std::ref(fifo2_));

        hwvalid = std::async(std::launch::async,
                             &cmdq7::cont_hwvalid_thr,
                             this,
                             std::ref(fifo1_),
                             std::ref(fifo2_));

        std::this_thread::sleep_for(duration<double, std::micro>(timeout_));
        cancel_ = true;
        swvalid.wait();
        hwvalid.wait();
    }
    else
    {
        swvalid = std::async(std::launch::async,
                             &cmdq7::swvalid_thr,
                             this,
                             std::ref(fifo1_),
                             std::ref(fifo2_));

        hwvalid = std::async(std::launch::async,
                             &cmdq7::hwvalid_thr,
                             this,
                             std::ref(fifo1_),
                             std::ref(fifo2_));

        swvalid.wait();
        hwvalid.wait();
    }

    uint32_t allocations = swvalid.get();
    uint32_t iterations = hwvalid.get();

    // stop the device
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::ctl), 7);

    bool res = true;

    // Wait for NLB to finish, but not more than 1 second.

    cancel_ = false;
    std::future<bool> done = std::async(std::launch::async,
            &cmdq7::wait_for_done, this, allocations);
    const auto fs = done.wait_for(seconds(1));


    if (fs != std::future_status::ready)
    {
        std::lock_guard<std::mutex> g(print_lock_);
        std::cerr << "Timeout waiting for cmdq_sw and cmdq_hw to be 0." << std::endl;
        cancel_ = true;
        res = false;
    }
    else
    {
        res = done.get();

        std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                 allocations_ * cachelines_,
                                                 end_cache_ctrs_ - start_cache_ctrs_,
                                                 end_fabric_ctrs_ - start_fabric_ctrs_,
                                                 freq_,
                                                 cont_,
                                                 suppress_header_,
                                                 csv_format_);
    }

    if (errors_ > 0)
    {
        std::lock_guard<std::mutex> g(print_lock_);
        std::cerr << "swvalid iters: " << allocations << " hwvalid iters: " << iterations << std::endl;
    }

    return res && (0 == errors_);
}

void cmdq7::apply(accelerator::ptr_t accelerator, const cmdq_entry_t &entry)
{
    dma_buffer::ptr_t inp = entry.first;
    dma_buffer::ptr_t out = entry.second;

    // Zero the output buffer.
    out->fill(0);

    // Re-initialize the input buffer.
    const uint32_t InputData = 0xdecafbad;
    for(size_t i = 0 ; i < inp->size()/sizeof(uint32_t) ; ++i)
        inp->write(InputData, i);

    // set input workspace address
    accelerator->write_mmio64(static_cast<uint32_t>(nlb0_csr::src_addr), CACHELINE_ALIGNED_ADDR(inp->iova()));
    // set output workspace address
    accelerator->write_mmio64(static_cast<uint32_t>(nlb0_csr::dst_addr), CACHELINE_ALIGNED_ADDR(out->iova()));
    // set number of cache lines for test
    accelerator->write_mmio32(static_cast<uint32_t>(nlb0_csr::num_lines), (inp->size() / CL(1)) - 1);

    // set swvalid bit to 1
    accelerator->write_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), 1);
}

bool cmdq7::verify(const cmdq_entry_t &entry)
{
    dma_buffer::ptr_t inp = entry.first;
    dma_buffer::ptr_t out = entry.second;
    return 0 == ::memcmp((void *)inp->address(), (void *)out->address(), inp->size());
}

void cmdq7::show_mismatch(const cmdq_entry_t &entry)
{
    dma_buffer::ptr_t inp = entry.first;
    dma_buffer::ptr_t out = entry.second;

    uint32_t offset;
    uint32_t i;
    uint32_t j;

    volatile uint32_t *pin = (volatile uint32_t *) inp->address();
    volatile uint32_t *pEndin = pin + (inp->size() / sizeof(uint32_t));
    volatile uint32_t *pout = (volatile uint32_t *) out->address();
    volatile uint32_t *pEndout = pout + (out->size() / sizeof(uint32_t));

    while (pin < pEndin)
    {
        if (*pin != *pout)
        {
            goto print;
        }

        ++pin;
        ++pout;
    }

    return;

print:

    offset = ((uint8_t *)pin - inp->address());

    std::cerr << std::endl;
    std::cerr << "offset: " << offset << " bytes into buffer" << std::endl;
    std::cerr << " src:" << (void *)(pin + (offset / sizeof(uint32_t))) << ' ';
    std::cerr << " src phys:0x" << std::hex << std::setfill('0') <<
                (inp->iova() + offset) << std::endl;

    for (i = 0 ; i < 2 ; ++i)
    {
        for (j = 0 ; j < 8 ; ++j)
        {
            if (pin >= pEndin)
            {
                goto print_output;
            }

            std::cerr << std::hex << std::setfill('0') << std::setw(8) << *pin++ << ' ';
        }
        std::cerr << std::endl;
    }

print_output:
    std::cerr << std::endl;
    std::cerr << "dest:" << (void *)(pout + (offset / sizeof(uint32_t))) << ' ';
    std::cerr << "dest phys:0x" << std::hex << std::setfill('0') <<
                (out->iova() + offset) << std::endl;

    for (i = 0 ; i < 2 ; ++i)
    {
        for (j = 0 ; j < 8 ; ++j)
        {
            if (pout >= pEndout)
            {
                goto print_done;
            }

            std::cerr << std::hex << std::setfill('0') << std::setw(8) << *pout++ << ' ';
        }
        std::cerr << std::endl;
    }

print_done:
    std::cerr << std::dec << std::setfill(' ');
}

uint32_t cmdq7::swvalid_thr(cmdq_t &fifo1, cmdq_t &fifo2)
{
    uint32_t i;
    uint32_t loops;
    const uint32_t max_loops = 100000;
    errno_t e;

    dma_buffer::microseconds_t timeout(1000000);
    if (target_ == "ase")
        timeout *= 100000;

    start_cache_ctrs_ = accelerator_->cache_counters();
    start_fabric_ctrs_ = accelerator_->fabric_counters();

    for (i = 0 ; i < allocations_ ; ++i)
    {
        // Clear the UMsg address space
        if (umsg_virt_)
            memset((void *)umsg_virt_, 0, umsg_size_);

        cmdq_entry_t entry = fifo1_pop();
        apply(accelerator_, entry);

        // Test flow
        // 1. CPU polls on address N+1

        dma_buffer::ptr_t inp = entry.first;
        dma_buffer::ptr_t out = entry.second;

        if (!out->poll<uint32_t>(out->size() - CL(1),
                                 timeout,
                                 HIGH,
                                 HIGH))
        {
            std::lock_guard<std::mutex> g(print_lock_);
            cancel_ = true;
            ++errors_;

            size_t offset = out->size() - CL(1);

            std::cerr << std::endl;
            std::cerr << "Error: Maximum timeout for CPU poll on Address N+1 was exceeded" << std::endl
                      << "virt: " << (void *)(out->address() + offset)
                      << " phys: 0x" << std::hex << std::setfill('0') << (out->iova() + offset)
                      << " cacheline: 0x" << std::hex << std::setfill('0')
                      << CACHELINE_ALIGNED_ADDR(out->iova() + offset);
            std::cerr << std::dec << std::setfill(' ') << std::endl;

            return i;
        }

        // 2. CPU copies dest to src
        e = memcpy_s((void *)inp->address(), inp->size(),
			(void *)out->address(), inp->size());
	if (EOK != e) {
		std::cerr << "memcpy_s failed" << std::endl;
                ++errors_;
	}

        // fence operation
        __sync_synchronize();

        // 3. CPU to FPGA message. Select notice type.
        if (nlb7_notice::csr_write == notice_)
        {
            accelerator_->write_mmio32(static_cast<uint32_t>(nlb7_csr::sw_notice), 0x10101010);
        }
        else if (umsg_virt_ && ((nlb7_notice::umsg_data == notice_) ||
                 (nlb7_notice::umsg_hint == notice_)))
        {
            *(volatile uint32_t *)umsg_virt_ = HIGH;
        }
        else
        { // poll
            inp->write<uint32_t>(HIGH, inp->size());
            inp->write<uint32_t>(HIGH, inp->size()+4);
            inp->write<uint32_t>(HIGH, inp->size()+8);
        }

        fifo2_push(entry);

        loops = 0;
        uint32_t sw = 1;
        accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
        while ((sw & 0x1) != 0)
        {
            ++loops;
            if (loops >= max_loops)
            {
                std::lock_guard<std::mutex> g(print_lock_);
                cancel_ = true;
                ++errors_;
                std::cerr << "swvalid_thr: Timeout waiting for cmdq_sw to be 0." << std::endl;
                return i;
            }

            std::this_thread::sleep_for(microseconds(10));
            accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
        }
    }

    return i;
}

uint32_t cmdq7::cont_swvalid_thr(cmdq_t &fifo1, cmdq_t &fifo2)
{
    uint32_t allocations = 0;
    uint32_t sw;
    uint32_t loops;
    const uint32_t max_loops = 1000000;
    errno_t e;

    dma_buffer::microseconds_t timeout(1000000);
    if (target_ == "ase")
        timeout *= 100000;

    start_cache_ctrs_ = accelerator_->cache_counters();
    start_fabric_ctrs_ = accelerator_->fabric_counters();

    while (!cancel_)
    {
        // Clear the UMsg address space
        if (umsg_virt_)
            memset((void *)umsg_virt_, 0, umsg_size_);

        cmdq_entry_t entry = fifo1_pop();
        apply(accelerator_, entry);
        ++allocations;

        // Test flow
        // 1. CPU polls on address N+1

        dma_buffer::ptr_t inp = entry.first;
        dma_buffer::ptr_t out = entry.second;

        if (!out->poll<uint32_t>(out->size() - CL(1),
                                 timeout,
                                 HIGH,
                                 HIGH))
        {
            std::lock_guard<std::mutex> g(print_lock_);
            cancel_ = true;
            ++errors_;

            size_t offset = out->size() - CL(1);

            std::cerr << std::endl;
            std::cerr << "Error: Maximum timeout for CPU poll on Address N+1 was exceeded" << std::endl
                      << "virt: " << (void *)(out->address() + offset)
                      << " phys: 0x" << std::hex << std::setfill('0') << (out->iova() + offset)
                      << " cacheline: 0x" << std::hex << std::setfill('0')
                      << CACHELINE_ALIGNED_ADDR(out->iova() + offset);
            std::cerr << std::dec << std::setfill(' ') << std::endl;

            return allocations;
        }

        // 2. CPU copies dest to src
        e = memcpy_s((void *)inp->address(), inp->size(),
			(void *)out->address(), inp->size());
	if (EOK != e) {
		std::cerr << "memcpy_s failed" << std::endl;
                ++errors_;
	}

        // fence operation
        __sync_synchronize();

        // 3. CPU to FPGA message. Select notice type.
        if (nlb7_notice::csr_write == notice_)
        {
            accelerator_->write_mmio32(static_cast<uint32_t>(nlb7_csr::sw_notice), 0x10101010);
        }
        else if (umsg_virt_ && ((nlb7_notice::umsg_data == notice_) ||
                 (nlb7_notice::umsg_hint == notice_)))
        {
            *(volatile uint32_t *)umsg_virt_ = HIGH;
        }
        else
        { // poll
            inp->write<uint32_t>(HIGH, inp->size());
            inp->write<uint32_t>(HIGH, inp->size()+4);
            inp->write<uint32_t>(HIGH, inp->size()+8);
        }

        fifo2_push(entry);

        loops = 0;
        sw = 1;
        accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
        while ((sw & 0x1) != 0)
        {
            ++loops;
            if (loops >= max_loops)
            {
                std::lock_guard<std::mutex> g(print_lock_);
                cancel_ = true;
                ++errors_;
                std::cerr << "cont_swvalid_thr: Timeout waiting for cmdq_sw to be 0." << std::endl;
            }

            std::this_thread::sleep_for(microseconds(10));
            accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
            if (cancel_)
            {
                return allocations;
            }
        }

        loops = 0;
        while (fifo1_is_empty())
        {
            ++loops;
            if (loops >= max_loops)
            {
                std::lock_guard<std::mutex> g(print_lock_);
                cancel_ = true;
                ++errors_;
                std::cerr << "cont_swvalid_thr: Timeout waiting for queue items." << std::endl;
            }

            std::this_thread::sleep_for(microseconds(10));
            if (cancel_)
            {
                return allocations;
            }
        }
    }

    return allocations;
}

uint32_t cmdq7::hwvalid_thr(cmdq_t &fifo1, cmdq_t &fifo2)
{
    uint32_t hw = 0;
    uint32_t i;

    for(i = 0 ; i < allocations_ ; ++i)
    {
        // DSM status space will alternate on even/odd iterations between
        // the DSM base address + 0x40 and DSM base address + DSM length + test_complete offset
        // Adjust according to iteration number (even vs odd)
        uint32_t dsm_offset = (uint32_t)nlb0_dsm::test_complete +
                (i % 2) * (uint32_t)nlb0_dsm::test_complete;

        volatile uint32_t *dsm_status_addr = (volatile uint32_t *)(dsm_->address() + dsm_offset);

        if (i == 0)
        {
            // Wait for status bit to be set to 1
            // Because NLB uses dsm number as zero based,
            // for the first iteration, wait until the status is 1

            while (0 == ((*dsm_status_addr) & 0x1))
            {
                std::this_thread::sleep_for(microseconds(10));
                if(cancel_)
                {
                    goto out;
                }
            }

            dsm_tuple_ = dsm_tuple(dsm_);
        }
        else
        {
            // Otherwise, wait for NLB to write index to dsm.
            // The hardware counter is 15 bits wide.
            while ((i % max_cmdq_counter) > ((*dsm_status_addr) >> 1))
            {
                std::this_thread::sleep_for(microseconds(10));
                if(cancel_)
                {
                    goto out;
                }
            }

            dsm_tuple_ += dsm_tuple(dsm_);
        }

        if (fifo2_is_empty())
        {
            goto out;
        }

        cmdq_entry_t entry(fifo2_pop());
        bool passed = verify(entry);
        fifo1_push(entry);

        if (!passed)
        {
            std::lock_guard<std::mutex> g(print_lock_);
            cancel_ = true;
            ++errors_;
            std::cerr << "hwvalid_thr: buffer mismatch at iteration " << i << "." << std::endl;
            show_mismatch(entry);
            goto out;
        }

    }

out:
    end_cache_ctrs_ = accelerator_->cache_counters();
    end_fabric_ctrs_ = accelerator_->fabric_counters();
    dsm_tuple_.put(dsm_);
    return i;
}

uint32_t cmdq7::cont_hwvalid_thr(cmdq_t &fifo1, cmdq_t &fifo2)
{
    uint32_t i = 0;
    uint32_t hw;

    while (!cancel_)
    {
        // DSM status space will alternate on even/odd iterations between
        // the DSM base address + 0x40 and DSM base address + DSM length + test_complete offset
        // Adjust according to iteration number (even vs odd)
        uint32_t dsm_offset = (uint32_t)nlb0_dsm::test_complete +
                (i % 2) * (uint32_t)nlb0_dsm::test_complete;

        volatile uint32_t *dsm_status_addr = (volatile uint32_t *)(dsm_->address() + dsm_offset);

        if (i == 0)
        {
            // Wait for status bit to be set to 1
            // Because NLB uses dsm number as zero based,
            // for the first iteration, wait until the status is 1

            while (0 == ((*dsm_status_addr) & 0x1))
            {
                if (cancel_)
                {
                    goto out;
                }
                std::this_thread::sleep_for(microseconds(10));
            }

            dsm_tuple_ = dsm_tuple(dsm_);
        }
        else
        {
            // Otherwise, wait for NLB to write index to dsm.
            // The hardware counter is 15 bits wide.
            while ((i % max_cmdq_counter) > ((*dsm_status_addr) >> 1))
            {
                if (cancel_)
                {
                    goto out;
                }
                std::this_thread::sleep_for(microseconds(10));
            }

            dsm_tuple_ += dsm_tuple(dsm_);
        }

        while (fifo2_is_empty())
        {
            std::this_thread::sleep_for(microseconds(10));
            if (cancel_)
            {
                goto out;
            }
        }

        cmdq_entry_t entry(fifo2_pop());
        bool passed = verify(entry);
        fifo1_push(entry);

        if (!passed)
        {
            std::lock_guard<std::mutex> g(print_lock_);
            cancel_ = true;
            ++errors_;
            std::cerr << "cont_hwvalid_thr: buffer mismatch at iteration " << i << "." << std::endl;
            show_mismatch(entry);
            goto out;
        }

        ++i;
    }

out:
    end_cache_ctrs_ = accelerator_->cache_counters();
    end_fabric_ctrs_ = accelerator_->fabric_counters();
    dsm_tuple_.put(dsm_);
    return i;
}

bool cmdq7::wait_for_done(uint32_t allocations)
{
    const uint32_t max_loops = 10000;
    uint32_t loops = 0;

    while (true)
    {
        ++loops;
        if (loops >= max_loops)
        {
            return false;
        }

        uint32_t sw = 1;
        uint32_t hw = 1;

        accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
        accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_hw), hw);

        if ((sw & 0x1) == 0 && (hw & 0x1) == 0)
        {
            return true;
        }

        std::this_thread::sleep_for(microseconds(10));
    }
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

