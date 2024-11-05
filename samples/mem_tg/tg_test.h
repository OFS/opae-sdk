// Copyright(c) 2022-2023, Intel Corporation
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

#include <unistd.h>
#include <thread>
#include <iostream>
#include <vector>
#include <future>
#include <string>
#include <condition_variable> 
#include <mutex>

#include "afu_test.h"
#include "mem_tg.h"

using test_afu = opae::afu_test::afu;
using opae::fpga::types::token;

namespace mem_tg {

// Shared static variables for synchornization
std::mutex tg_print_mutex;
std::mutex tg_start_write_mutex;
std::condition_variable tg_cv;
std::atomic<int> tg_waiting_threads_counter;
int tg_num_threads = -1; // Written to once in run() then read-only

// Returns the number of 1 bits in the value of x.
// C++20 provides std::popcount.
static uint64_t popcount64(uint64_t x) {
  uint64_t n = 0;
  while (x) {
    ++n;
    x &= x - 1ULL;
  }
  return n;
}

// Returns the number of bits needed to store the value x.
// C++20 provides std::bit_width.
static uint64_t bit_width64(uint64_t x) {
  uint64_t n = 0;
  while (x >>= 1) ++n;
  return n;
}

// Calculates read/write address offset for each channel, when one or
// multiple Network-on-Chip (NoC) are present to facilitate data movement
// between the FPGA core logic and memory. In this case, memory requests
// from multiple channels through a single NoC must be non-overlapping
// for maximum bandwidth. The TG feature register provides the number
// of NoC and the total addressable memory per NoC, which may be
// evenly divided into non-overlapping ranges for each channel.
//
// NoC support was added in TG version 1.1.
class tg_channel_address_offset {
private:
  // read/write address offset per channel in multiples of 256 bytes
  uint64_t offset_ = 0;
  // number of channels after which offset wraps around to 0
  uint64_t count_ = 1;

public:
  tg_channel_address_offset(mem_tg *tg_exe_) {
    if (tg_exe_->version_ < tg_exe_->version_code(1, 1))
      return;

    const uint64_t mem_tg_feat = tg_exe_->read64(MEM_TG_FEAT);
    const uint64_t noc_addr_width = mem_tg_feat & 0xfff; // [11:0]
    const uint64_t num_noc = (mem_tg_feat >> 12) & 0xf;  // [15:12]

    if (num_noc == 0)
      return;

    const uint64_t mem_ctrl = tg_exe_->read64(MEM_TG_CTRL);
    const uint64_t num_channels = popcount64(mem_ctrl);

    tg_exe_->logger_->debug("number of channels: {}", num_channels);
    tg_exe_->logger_->debug("number of NoC: {}", num_noc);
    tg_exe_->logger_->debug("address width per NoC: {}", noc_addr_width);

    const uint64_t num_channels_bits = bit_width64(num_channels);
    const uint64_t num_noc_bits = bit_width64(num_noc);

    assert(num_channels == (1ULL << num_channels_bits));
    assert(num_noc == (1ULL << num_noc_bits));
    assert(num_channels_bits >= num_noc_bits);

    const uint64_t channels_per_noc_bits = num_channels_bits - num_noc_bits;

    offset_ = 1ULL << (noc_addr_width - (channels_per_noc_bits + 8));
    count_ = 1ULL << channels_per_noc_bits;

    tg_exe_->logger_->debug("address offset per channel: {}", offset_);
    tg_exe_->logger_->debug("channels per NoC: {}", count_);
  }

  uint64_t offset_for_channel(uint32_t c) const {
    return (c % count_) * offset_;
  }
};

class tg_test : public test_command
{
public:
    tg_test()
        : tg_offset_(0x0)
        , tg_exe_(NULL)
    {}

    virtual ~tg_test() {}

    virtual const char *name() const override
    {
        return "tg_test";
    }

    virtual const char *description() const override
    {
      return "configure & run mem traffic generator test";
    }

    virtual const char *afu_id() const override
    {
      return AFU_ID;
    }

    // Convert number of transactions to bandwidth (GB/s)
    double bw_calc(uint64_t xfer_bytes, uint64_t num_ticks) const
    {
        return (double)(xfer_bytes) / ((1000.0 / (double)tg_exe_->mem_speed_ * (double)num_ticks));
    }

    void tg_perf (mem_tg *tg_exe_) const
    {
      // Lock mutex before printing so print statements don't collide between threads.
      std::unique_lock<std::mutex> print_lock(tg_print_mutex);
      std::cout << "Channel " << std::stoi(tg_exe_->mem_ch_[0]) << ":" << std::endl;

      if (tg_exe_->status_ == TG_STATUS_TIMEOUT) {
        std::cout << "TG TIMEOUT" << std::endl;
      } else if (tg_exe_->status_ == (uint32_t)(-1)) {
	std::cout << "Error: Timed out in TG_STATUS_ACTIVE state. Consider increasing MEM_TG_TEST_TIMEOUT." << std::endl; 
      } else if (tg_exe_->status_ == TG_STATUS_ERROR) {
        uint32_t tg_fail_exp;
        uint32_t tg_fail_act;
        uint64_t tg_fail_addr;
        std::cout << "TG ERROR" << std::endl;
        tg_fail_addr = tg_exe_->read64(tg_exe_->tg_offset_ + TG_FIRST_FAIL_ADDR_L);
        tg_fail_exp  = tg_exe_->read64(tg_exe_->tg_offset_ + TG_FAIL_EXPECTED_DATA);
        tg_fail_act  = tg_exe_->read64(tg_exe_->tg_offset_ + TG_FAIL_READ_DATA);
        std::cout << "Failed at address 0x" << std::hex << tg_fail_addr << " exp=0x" << tg_fail_exp << " act=0x" << tg_fail_act << std::endl;
      } else {
        std::cout << "TG PASS" << std::endl;
      }

      uint32_t mem_ch_offset = (std::stoi(tg_exe_->mem_ch_[0])) << 0x3;
      uint64_t num_ticks = tg_exe_->read64(MEM_TG_CLOCKS + mem_ch_offset);
      std::cout << "Mem Clock Cycles: " << std::dec << num_ticks << std::endl;
	
      std::cout << "DEBUG: wcnt_ " << std::dec << tg_exe_->wcnt_ << std::endl;
      std::cout << "DEBUG: rcnt_ " << std::dec << tg_exe_->rcnt_ << std::endl;
      std::cout << "DEBUG: waddr_ " << std::dec << tg_exe_->waddr_ << std::endl;
      std::cout << "DEBUG: raddr_ " << std::dec << tg_exe_->raddr_ << std::endl;
      std::cout << "DEBUG: bcnt_ " << std::dec << tg_exe_->bcnt_ << std::endl;
      std::cout << "DEBUG: loop_ " << std::dec << tg_exe_->loop_ << std::endl;
      std::cout << "DEBUG: num_ticks " << std::dec << num_ticks << std::endl;

      uint64_t write_bytes = 64 * (tg_exe_->loop_*tg_exe_->wcnt_*tg_exe_->bcnt_);
      uint64_t read_bytes  = 64 * (tg_exe_->loop_*tg_exe_->rcnt_*tg_exe_->bcnt_);
      std::cout << "Write BW: " << bw_calc(write_bytes,num_ticks) << " GB/s" << std::endl;
      std::cout << "Read BW: "  << bw_calc(read_bytes,num_ticks)  << " GB/s\n" << std::endl;

      print_lock.unlock();
    }
  
    bool tg_wait_test_completion (mem_tg *tg_exe_) const
    {
        /* Wait for test completion */
        uint32_t timeout = MEM_TG_TEST_TIMEOUT * tg_exe_->loop_ * tg_exe_->bcnt_;
        // poll while active bit is set (channel status = {pass,fail,timeout,active})
        uint32_t tg_status = 0x1;
        tg_status = 0xF & (tg_exe_->read64(MEM_TG_STAT) >> (0x4*(std::stoi(tg_exe_->mem_ch_[0]))));
        while (tg_status == TG_STATUS_ACTIVE) {
          tg_status = 0xF & (tg_exe_->read64(MEM_TG_STAT) >> (0x4*(std::stoi(tg_exe_->mem_ch_[0]))));
          std::this_thread::yield();
          if (--timeout == 0) {
            tg_exe_->status_ = -1;
            std::cout << "DEBUG: timeout error" << std::endl;
	    return false;
          }
        }
      	if (tg_status == TG_STATUS_TIMEOUT) {
          tg_exe_->status_ = tg_status;
	  std::cout << "DEBUG: status timeout" << std::endl;
          return false;
        }

        if (tg_status == TG_STATUS_ERROR) {
          tg_exe_->status_ = tg_status;
	  std::cout << "DEBUG: status error" << std::endl;
          return false;
        }
        tg_exe_->status_ = tg_status;
        return true;
    }

    int config_input_options(mem_tg *tg_exe_) const
    {
        if (!tg_exe_)
          return -1;
        uint64_t mem_capability = tg_exe_->read64(MEM_TG_CTRL);
    	if ((mem_capability & (0x1 << std::stoi(tg_exe_->mem_ch_[0]))) == 0) { 
          std::cerr << "No traffic generator for mem[" << std::stoi(tg_exe_->mem_ch_[0]) << "]" << std::endl;
          return -1;
        }

        tg_exe_->tg_offset_ = AFU_DFH + (MEM_TG_CFG_OFFSET * (1+std::stoi(tg_exe_->mem_ch_[0])));

        tg_exe_->write32(tg_exe_->tg_offset_+TG_LOOP_COUNT,  tg_exe_->loop_);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_WRITE_COUNT, tg_exe_->wcnt_);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_READ_COUNT,  tg_exe_->rcnt_);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_SEQ_START_ADDR_WR_L, tg_exe_->waddr_ & 0xFFFFFFFF);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_SEQ_START_ADDR_WR_H, tg_exe_->waddr_ >> 32);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_SEQ_START_ADDR_RD_L, tg_exe_->raddr_ & 0xFFFFFFFF);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_SEQ_START_ADDR_RD_H, tg_exe_->raddr_ >> 32);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_BURST_LENGTH, tg_exe_->bcnt_);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_SEQ_ADDR_INCR, tg_exe_->stride_);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_PPPG_SEL, tg_exe_->pattern_);

        // address increment mode
        tg_exe_->write32(tg_exe_->tg_offset_+TG_ADDR_MODE_WR, TG_ADDR_SEQ);
        tg_exe_->write32(tg_exe_->tg_offset_+TG_ADDR_MODE_RD, TG_ADDR_SEQ);
        return 0;
    }

    // The test state has been configured. Run one test instance.
    int run_mem_test(mem_tg *tg_exe_) const
    {
      int status = 0;
	
      // All threads do their set up and wait here for other threads so start write happens all at once
      std::unique_lock<std::mutex> lock(tg_start_write_mutex);
      tg_waiting_threads_counter++;
      if(tg_waiting_threads_counter == tg_num_threads) {
        tg_cv.notify_all(); 
      } else {
        tg_cv.wait(lock, [&](){ return tg_waiting_threads_counter == tg_num_threads; });
      }
      lock.unlock();

      // write32 is not thread safe so we wrap it in a lock
      lock.lock();
      tg_exe_->logger_->debug("Start Test");
      tg_exe_->write32(tg_exe_->tg_offset_ + TG_START, 0x1);
      lock.unlock();

      if (!tg_wait_test_completion(tg_exe_))
        status = -1;

      tg_perf(tg_exe_);

      return status;
    }

    int run_thread_single_channel(mem_tg *tg_exe_) {
      auto ret = config_input_options(tg_exe_);
      if (ret != 0) {
        std::cerr << "Failed to configure TG input options" << std::endl;
        return ret;
      }
      return run_mem_test(tg_exe_);
    }

    virtual int run(test_afu *afu, CLI::App *app) override
    {
      (void)app;
      auto d_afu = dynamic_cast<mem_tg *>(afu);
      tg_exe_ = dynamic_cast<mem_tg *>(afu);

      token_ = d_afu->get_token();

      // Read HW details
      const uint64_t tg_version = tg_exe_->read64(TG_VERSION);
      const uint32_t major_ver = (tg_version >> 12) & 0xf; // [51:48]
      const uint32_t minor_ver = (tg_version >> 48) & 0xf; // [15:12]
      tg_exe_->version_ = tg_exe_->version_code(major_ver, minor_ver);
      tg_exe_->logger_->debug("version: {}.{}", major_ver, minor_ver);

      if (0 == tg_exe_->mem_speed_) {
        tg_exe_->mem_speed_ = 300;
        std::cout << "Memory channel clock frequency unknown. Assuming "
          << tg_exe_->mem_speed_ << " MHz." << std::endl;
      } else {
        std::cout << "Memory clock from command line: "
          << tg_exe_->mem_speed_ << " MHz" << std::endl;
      }

      if (0 >= (tg_exe_->mem_ch_).size()) {
        std::cout << "Insufficient arguments provided" << std::endl;
        exit(1);
      }

      // Parse mem_ch_ into array of selected channels
      std::vector<uint32_t> channels;
      if ((tg_exe_->mem_ch_[0]).find("all") == 0) {	
        uint64_t mem_capability = tg_exe_->read64(MEM_TG_CTRL);
        for (uint32_t i = 0; i < 64; i++) { // number of iterations should be same as mem_capability
          if ((mem_capability & (1ULL << i)) != 0) {
            channels.emplace_back(i);
          }
        }
      } else {
        try {
          for (const std::string &mem_ch: tg_exe_->mem_ch_) {
            channels.emplace_back(std::stoi(mem_ch));
          }
        } catch (std::invalid_argument &e) {
          std::cerr << "Error: invalid argument to std::stoi";
          return 1;
        }
      }

      // Spawn threads for each channel
      std::vector<std::future<int>> futures;
      std::vector<std::thread> threads;
      tg_num_threads = channels.size();
      tg_waiting_threads_counter = 0;
      tg_channel_address_offset addr_offset(tg_exe_);
      for (auto c: channels) {
        std::promise<int> p;
        futures.emplace_back(p.get_future());
        threads.emplace_back([this, c, p = std::move(p), addr_offset]() mutable {
          mem_tg tg_exe;
          tg_exe_->duplicate(&tg_exe);
          tg_exe.mem_ch_.clear();
          tg_exe.mem_ch_.push_back(std::to_string(c));
          tg_exe.raddr_ = tg_exe.waddr_ = addr_offset.offset_for_channel(c);
          p.set_value(run_thread_single_channel(&tg_exe));
        });
      }

      // Wait for all threads to exit
      for (auto &thread : threads) {
        thread.join();
      }

      for (size_t i = 0; i < channels.size(); i++) {
        int ret = futures[i].get();
        std::cout << "Thread on channel " << channels[i] << " exited with status " << ret << std::endl;
      }

      return 0;
    }

protected:
    uint64_t tg_offset_;
    mem_tg *tg_exe_;
    token::ptr_t token_;
};

} // end of namespace mem_tg

