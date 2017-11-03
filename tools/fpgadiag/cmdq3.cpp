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
#include "cmdq3.h"
#include "fpga_app/fpga_common.h"
#include "option.h"
#include "nlb.h"
#include "nlb_stats.h"
#include "nlb_cache_prime.h"

#define CACHE_TEMP 1

using namespace std::chrono;
using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

cmdq3::cmdq3()
: name_("cmdq3")
, config_("cmdq3.json")
, target_("fpga")
, mode_("trput")
, allocations_(1)
, cachelines_(1)
, cont_(false)
, timeout_usec_(0)
, timeout_msec_(0)
, timeout_sec_(1)
, timeout_min_(0)
, timeout_hour_(0)
, freq_(MHZ(400))
, multi_cl_(1)
, read_vc_("auto")
, write_vc_("auto")
, wrfence_vc_("auto")
, cool_fpga_cache_(false)
, warm_fpga_cache_(false)
, guid_("F7DF405C-BD7A-CF72-22F1-44B0B93ACD18")
, dsm_size_(MB(4))
, timeout_(0)
, poll_sleep_ns_(100)
, src_address_(nullptr)
, dst_address_(nullptr)
, sw_valid_(nullptr)
, assignments_(0)
, iterations_(0)
, errors_(0)
, suppress_header_(false)
, csv_format_(false)
, batch_mode_(false)
{
    options_.add_option<bool>("help",                 'h', intel::utils::option::no_argument,   "Show help", false);
    options_.add_option<std::string>("config",        'c', intel::utils::option::with_argument, "Path to test config file", config_);
    options_.add_option<std::string>("target",        't', intel::utils::option::with_argument, "one of { fpga, ase }", target_);
    options_.add_option<std::string>("mode",          'k', intel::utils::option::with_argument, "one of { read, write, trput }", mode_);
    options_.add_option<std::uint32_t>("allocations", 'a', intel::utils::option::with_argument, "Number of FIFO entries",  allocations_);
    options_.add_option<std::uint32_t>("cachelines",  'l', intel::utils::option::with_argument, "Size of each FIFO entry", cachelines_);
    options_.add_option<bool>("cont",                 'L', intel::utils::option::no_argument,   "Enable continuous mode", cont_);
    options_.add_option<uint32_t>("timeout-usec",     'O', intel::utils::option::with_argument, "Timeout (usec) for cont mode", timeout_usec_);
    options_.add_option<uint32_t>("timeout-msec",     'Q', intel::utils::option::with_argument, "Timeout (msec) for cont mode", timeout_msec_);
    options_.add_option<uint32_t>("timeout-sec",      'X', intel::utils::option::with_argument, "Timeout (sec) for cont mode", timeout_sec_);
    options_.add_option<uint32_t>("timeout-min",      'Y', intel::utils::option::with_argument, "Timeout (minutes) for cont mode", timeout_min_);
    options_.add_option<uint32_t>("timeout-hour",     'Z', intel::utils::option::with_argument, "Timeout (hours) for cont mode", timeout_hour_);
    options_.add_option<uint32_t>("freq",             'T', intel::utils::option::no_argument,   "clock frequency", freq_);
    options_.add_option<uint32_t>("multi-cl",         'u', intel::utils::option::with_argument, "one of { 1, 2, 4 }", multi_cl_);
    options_.add_option<std::string>("read-vc",       'r', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1, random }", read_vc_);
    options_.add_option<std::string>("write-vc",      'w', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1, random }", write_vc_);
    options_.add_option<std::string>("wrfence-vc",    'f', intel::utils::option::with_argument, "one of { auto, vl0, vh0, vh1 }", wrfence_vc_);
#if CACHE_TEMP
    options_.add_option<bool>("cool-fpga-cache",      'M', intel::utils::option::no_argument,   "cool down fpga cache", cool_fpga_cache_);
    options_.add_option<bool>("warm-fpga-cache",      'H', intel::utils::option::no_argument,   "warm up fpga cache", warm_fpga_cache_);
#endif
    options_.add_option<uint8_t>("bus-number",        'B', intel::utils::option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",            'D', intel::utils::option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",          'F', intel::utils::option::with_argument, "Function number of PCIe device");
    options_.add_option<uint8_t>("socket-id",         's', intel::utils::option::with_argument, "Socket id encoded in BBS");
    options_.add_option<std::string>("guid",          'g', intel::utils::option::with_argument, "accelerator id to enumerate", guid_);
    options_.add_option<bool>("suppress-hdr",         'S', intel::utils::option::no_argument,   "Suppress column headers", suppress_header_);
    options_.add_option<bool>("csv",                  'V', intel::utils::option::no_argument,   "Comma separated value format", csv_format_);
    options_.add_option<bool>("batch",                     intel::utils::option::no_argument,   "Run in batch mode", batch_mode_);
    options_.add_option<uint64_t>("poll-sleep",            intel::utils::option::with_argument, "Amount of time (ns) to sleep in between polling. Default is 100ns", poll_sleep_ns_);
}

cmdq3::~cmdq3()
{
}

void cmdq3::show_help(std::ostream &os)
{
    os << "Usage: cmdq3" << std::endl
       << std::endl;

    for (const auto &it : options_)
    {
        it->show_help(os);
    }
}

bool cmdq3::setup()
{
    options_.get_value<bool>("batch", batch_mode_);
    options_.get_value<std::string>("target", target_);
    if ((target_ != "fpga") && (target_ != "ase"))
    {
        std::cerr << "Invalid --target: " << target_ << std::endl;
        return false;
    }

    if (target_ == "fpga")
    {
        src_address_ = reinterpret_cast<volatile uint64_t*>(accelerator_->mmio_pointer(static_cast<uint32_t>(nlb0_csr::src_addr)));
        dst_address_ = reinterpret_cast<volatile uint64_t*>(accelerator_->mmio_pointer(static_cast<uint32_t>(nlb0_csr::dst_addr)));
        sw_valid_    = reinterpret_cast<volatile uint32_t*>(accelerator_->mmio_pointer(static_cast<uint32_t>(nlb0_csr::cmdq_sw)));
    }

    options_.get_value<std::string>("mode", mode_);
    if ((mode_ != "read") && (mode_ != "write") && (mode_ != "trput"))
    {
        std::cerr << "Invalid --mode: " << mode_ << std::endl;
        return false;
    }
    if (mode_ == "read")
    {
        cfg_ = nlb_mode::read;
    }
    else if (mode_ == "write")
    {
        cfg_ = nlb_mode::write;
    }
    else
    {
        cfg_ = nlb_mode::throughput;
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

#if CACHE_TEMP
    options_.get_value<bool>("cool-fpga-cache", cool_fpga_cache_);
    options_.get_value<bool>("warm-fpga-cache", warm_fpga_cache_);
#endif

    if (cool_fpga_cache_ && warm_fpga_cache_)
    {
        std::cerr << "Specify only one of --cool-fpga-cache and --warm-fpga-cache." << std::endl;
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

bool cmdq3::run()
{
    uint32_t buffer_size = CL(cachelines_);
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
    else if(warm_fpga_cache_)
    {
        if (mode_ == "read")
        {
            cmdq_entry_t e = fifo1_.front();
            nlb_read_cache_warm warmer(target_, accelerator_, dsm_, e.first, e.second, true);
            if (!warmer.warm())
            {
                std::cerr << "fpga cache warming (read) timed out." << std::endl;
                return false;
            }
        }
        else if (mode_ == "write")
        {
            cmdq_entry_t e = fifo1_.front();
            nlb_write_cache_warm warmer(target_, accelerator_, dsm_, e.first, e.second, true);
            if (!warmer.warm())
            {
                std::cerr << "fpga cache warming (write) timed out." << std::endl;
                return false;
            }
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

    // set number of cache lines for test
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::num_lines), cachelines_);
    // spawn another thread to check for HWVALID being set to 0
    // when HWVALID is 0, then check the buffers for the test mode
    // eg. test mode is 0, then check output buffers match input buffers

    cancel_ = false;
    errors_ = 0;

    std::thread sw_thread;
    std::thread hw_thread;
    swvalid_next_ = 0;
    hwvalid_next_ = 0;
    if (cont_)
    {
        sw_thread = std::thread(&cmdq3::cont_swvalid_thr, this);
        if (!batch_mode_)
        {
            hw_thread = std::thread(&cmdq3::cont_hwvalid_thr, this);
        }

        std::this_thread::sleep_for(duration<double, std::micro>(timeout_));
        cancel_ = true;
        sw_thread.join();

        if (!batch_mode_)
        {
            hw_thread.join();
        }
    }
    else
    {
        sw_thread = std::thread(&cmdq3::swvalid_thr, this);
        if (!batch_mode_)
        {
            hw_thread = std::thread(&cmdq3::hwvalid_thr, this);
        }
        sw_thread.join();
        if (!batch_mode_)
        {
            hw_thread.join();
        }
    }

    // stop the device
    if (!batch_mode_)
    {
        accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
    }

    bool res = true;

    // Wait for NLB to finish, but not more than 1 second.

    cancel_ = false;
    std::future<bool> done = std::async(std::launch::async,
                &cmdq3::wait_for_done, this, assignments_);
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
        if (batch_mode_)
        {
            end_cache_ctrs_ = accelerator_->cache_counters();
            end_fabric_ctrs_ = accelerator_->fabric_counters();
        }

        auto dsm_v = batch_mode_ ? dsm_version::cmdq_batch : dsm_version::nlb_classic;

        std::cout << intel::fpga::nlb::nlb_stats(dsm_,
                                                 dsm_v,
                                                 assignments_ * cachelines_,
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
        std::cerr << "swvalid iters: " << assignments_ << " hwvalid iters: " << iterations_ << std::endl;
    }

    return res && (0 == errors_);
}

bool cmdq3::apply(const cmdq_entry_t &entry)
{
    dma_buffer::ptr_t inp = entry.first;
    dma_buffer::ptr_t out = entry.second;

    uint64_t src_address = CACHELINE_ALIGNED_ADDR(inp->iova());
    uint64_t dst_address = CACHELINE_ALIGNED_ADDR(out->iova());

    const uint32_t max_loops = 100000;
    uint32_t loops = 0;
    uint32_t sw = 1;
    accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
    while ((sw & 0x1) != 0)
    {
        ++loops;
        if (loops >= max_loops)
        {
            return false;
        }

        accelerator_->read_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), sw);
    }
    // set input workspace address
    accelerator_->write_mmio64(static_cast<uint32_t>(nlb0_csr::src_addr), src_address);
    // set output workspace address
    accelerator_->write_mmio64(static_cast<uint32_t>(nlb0_csr::dst_addr), dst_address);

    // set swvalid bit to 1
    accelerator_->write_mmio32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), 1);
    assignments_++;
    return true;
}

bool cmdq3::apply_ptr(const cmdq_entry_t &entry)
{
    dma_buffer::ptr_t inp = entry.first;
    dma_buffer::ptr_t out = entry.second;

    uint64_t src_address = CACHELINE_ALIGNED_ADDR(inp->iova());
    uint64_t dst_address = CACHELINE_ALIGNED_ADDR(out->iova());

    const uint32_t max_loops = 100000;
    uint32_t loops = 0;
    while(*sw_valid_ & 0x1 == 1 && loops++ <= max_loops)
    {
        if (poll_sleep_ns_)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(poll_sleep_ns_));
        }
    }

    if (loops >= max_loops)
    {
        return false;
    }
    // set input workspace address
    *src_address_ = src_address;
    // set output workspace address
    *dst_address_ = dst_address;

    // set swvalid bit to 1
    *sw_valid_ = 1;
    assignments_++;
    return true;
}

void cmdq3::swvalid_thr()
{
    start_cache_ctrs_ = accelerator_->cache_counters();
    start_fabric_ctrs_ = accelerator_->fabric_counters();
    while (swvalid_next_ < allocations_)
    {
        if (!(target_ == "fpga" ? apply_ptr(fifo1_[swvalid_next_++]) :
                                  apply(fifo1_[swvalid_next_++])))
        {
            cancel_ = true;
            std::lock_guard<std::mutex> g(print_lock_);
            ++errors_;
            std::cerr << "swvalid_thr: Timeout waiting for cmdq_sw to be 0." << std::endl;
            return;
        }
    }
}

void cmdq3::cont_swvalid_thr()
{
    start_cache_ctrs_ = accelerator_->cache_counters();
    start_fabric_ctrs_ = accelerator_->fabric_counters();

    while (!cancel_)
    {
        if (!(target_ == "fpga" ? apply_ptr(fifo1_[swvalid_next_]) :
                                  apply(fifo1_[swvalid_next_])))
        {
            cancel_ = true;
            std::lock_guard<std::mutex> g(print_lock_);
            ++errors_;
            std::cerr << "cont_swvalid_thr: Timeout waiting for cmdq_sw to be 0." << std::endl;
            return;
        }
        swvalid_next_ = (swvalid_next_ + 1) % allocations_;
    }
}

void cmdq3::hwvalid_thr()
{
    uint32_t i;

    for(i = 0 ; i < allocations_ ; ++i)
    {
        // DSM status space will alternate on even/odd iterations between
        // the DSM base address + 0x40 and DSM base address + DSM length + test_complete offset
        // Adjust according to iteration number (even vs odd)
        uint32_t dsm_offset = (uint32_t)nlb0_dsm::test_complete +
                (i % 2) * (uint32_t)nlb0_dsm::test_complete;

        volatile uint32_t *dsm_status_addr = (volatile uint32_t *)(dsm_->address() + dsm_offset);

        while(!cancel_ && (hwvalid_next_ == swvalid_next_));
        if (cancel_) break;

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
    }

out:
    end_cache_ctrs_ = accelerator_->cache_counters();
    end_fabric_ctrs_ = accelerator_->fabric_counters();
    dsm_tuple_.put(dsm_);
}

void cmdq3::cont_hwvalid_thr()
{
    uint32_t i = 0;

    while (!cancel_)
    {
        // DSM status space will alternate on even/odd iterations between
        // the DSM base address + 0x40 and DSM base address + DSM length + test_complete offset
        // Adjust according to iteration number (even vs odd)
        uint32_t dsm_offset = (uint32_t)nlb0_dsm::test_complete +
                (i % 2) * (uint32_t)nlb0_dsm::test_complete;

        volatile uint32_t *dsm_status_addr = (volatile uint32_t *)(dsm_->address() + dsm_offset);

        while(!cancel_ && (hwvalid_next_ == swvalid_next_));
        if (cancel_) break;

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

        cmdq_entry_t entry = fifo1_[hwvalid_next_];
        hwvalid_next_ = (hwvalid_next_+1)/fifo1_.size();

        ++i;
    }

out:
    end_cache_ctrs_ = accelerator_->cache_counters();
    end_fabric_ctrs_ = accelerator_->fabric_counters();
    dsm_tuple_.put(dsm_);
}

bool cmdq3::wait_for_done(uint32_t allocations)
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

