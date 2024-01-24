// Copyright(c) 2023, Intel Corporation
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

#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <ofs/ofs_primitives.h>

#include "afu_test.h"
#include "cxl_mem_tg.h"
using namespace std;
using test_afu = opae::afu_test::afu;
using opae::fpga::types::token;

#define CXL_TG_BW_FACTOR   0.931323

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*
1) Write to TG_CLEAR with data=0xF to clear all the failure status registers.
2) Configure the registers with the value specified in table 1 below.
3) Write to TG_START to start the TG2 using the configuration in step 2.
This starts the traffic test in user mode.
4) Read from TG_TEST_COMPLETE until the read data =0x1,
indicating the traffic test has completed.
5) Read from TG_PASS, TG_FAIL, and TG_TIMEOUT to check the test result.
     a) TG_PASS. A value of 1 indicates that the traffic
     test passed at the end of all test stages.
     b) TG_FAIL. A value of 1 indicates that the configured
     traffic finished running but a failure (read miscompare) was observed. You
may read from other relevant registers to get more information about the
failure. Refer to the Configuration and Status Registers table for information
on the available registers. c) TG_TIMEOUT. A value of 1 indicates that a read
    response was not received from the interface for one or more read commands.
*/

template <typename X>
std::string int_to_hex(X x) {
  std::stringstream ss;
  ss << "0x" << std::setfill('0') << std::setw(2 * sizeof(X)) << std::hex << x;
  return ss.str();
}

namespace cxl_mem_tg {

class cxl_tg_test : public test_command {
 public:
  cxl_tg_test() : tg_offset_(0x0), timeout_usec_(MEM_TG_TEST_TIMEOUT), tg_exe_(NULL) {}

  virtual ~cxl_tg_test() {}

  virtual const char *name() const override { return "tg_test"; }

  virtual const char *description() const override {
    return "configure & run cxl mem traffic generator test";
  }

  virtual const char *afu_id() const override { return AFU_ID; }

  // Convert number of transactions to bandwidth (GB/s)
  double bw_calc(uint64_t xfer_bytes, uint64_t num_ticks) {
    return (double)(xfer_bytes)* CXL_TG_BW_FACTOR /
           ((1000.0 / (double)(tg_exe_->mem_speed_/1000) * (double)num_ticks));
  }

  int ofs_wait_for_eq32(uint32_t offset, uint32_t value,
      uint64_t timeout_usec, uint32_t sleep_usec) {
      OFS_TIMESPEC_USEC(ts, sleep_usec);
      struct timespec begin, now, save, rem;
      save = ts;
      uint32_t csr;
      csr = tg_exe_->read32(offset);
      clock_gettime(CLOCK_MONOTONIC, &begin);
      while (csr != value) {
          if (sleep_usec) {
              ts = save;
              while ((nanosleep(&ts, &rem) == -1) &&
                  (errno == EINTR))
                  ts = rem;
          }
          csr = tg_exe_->read32(offset);
          clock_gettime(CLOCK_MONOTONIC, &now);
          struct timespec delta;
          ofs_diff_timespec(&delta, &now, &begin);
          uint64_t delta_nsec = delta.tv_nsec + delta.tv_sec * SEC2NSEC;
          if (delta_nsec > timeout_usec * USEC2NSEC) {
              return 1;
          }
      }
      return 0;

  }

  void print_he_mem_tg() {
    tg_exe_->logger_->debug("DFH:0x{:x}", tg_exe_->read64(AFU_DFH));
    tg_exe_->logger_->debug("GUIDL:0x{:x}", tg_exe_->read64(AFU_ID_L));
    tg_exe_->logger_->debug("GUIDH:0x{:x}", tg_exe_->read64(AFU_ID_H));
    tg_exe_->logger_->debug("TG Contol:0x{:x}", tg_exe_->read64(MEM_TG_CTRL));
    tg_exe_->logger_->debug("TG Status:0x{:x}", tg_exe_->read64(MEM_TG_STAT));
    tg_exe_->logger_->debug("Memory Size:0x{:x}",
                            tg_exe_->read64(MEM_SIZE));
    tg_exe_->logger_->debug("TG Total clock count:0x{:x}",
                            tg_exe_->read64(MEM_TG_CLK_COUNT));
    tg_exe_->logger_->debug("TG Write Clock Count:0x{:x}",
                            tg_exe_->read64(MEM_TG_WR_COUNT));
    tg_exe_->logger_->debug("TG Frequency:{}", tg_exe_->read64(MEM_TG_CLK_FREQ));
    tg_exe_->logger_->debug("TG_LOOP_COUNT:0x{:x}",
                            tg_exe_->read32(TG_LOOP_COUNT));
    tg_exe_->logger_->debug("TG_WRITE_COUNT:0x{:x}",
                            tg_exe_->read32(TG_WRITE_COUNT));
    tg_exe_->logger_->debug("TG_READ_COUNT:0x{:x}",
                            tg_exe_->read32(TG_READ_COUNT));
    tg_exe_->logger_->debug("TG_BURST_LENGTH:0x{:x}",
                            tg_exe_->read32(TG_BURST_LENGTH));
    tg_exe_->logger_->debug("TG_PASS:0x{:x}", tg_exe_->read32(TG_PASS));
    tg_exe_->logger_->debug("TG_FAIL:0x{:x}", tg_exe_->read32(TG_FAIL));
  }

  // Write to TG_CLEAR with data=0xF to clear all the failure status registers
  int tg_clear() {
    tg_exe_->logger_->debug("clear tg all failure status registers");

    mem_tg_ctl tg_ctl;
    tg_ctl.value = 0;
    tg_ctl.counter_clear = 1;
    tg_exe_->write64(MEM_TG_CTRL, tg_ctl.value);

    tg_ctl.counter_clear = 0;
    tg_exe_->write64(MEM_TG_CTRL, tg_ctl.value);

    tg_exe_->write32(TG_CLEAR, 0xF);
    sleep(TG_SLEEP);
    return 0;
  }

  // print TG performance values
  void tg_perf() {
    tg_exe_->logger_->debug("TG performance ...");

    if (tg_exe_->status_ == TG_STATUS_TIMEOUT) {
      cerr << "TG timeout" << endl;
    } else if (tg_exe_->status_ == TG_STATUS_ERROR) {
      uint32_t tg_fail_exp;
      uint32_t tg_fail_act;
      uint64_t tg_fail_addr;
      tg_fail_addr = tg_exe_->read64(TG_FIRST_FAIL_ADDR_L);
      tg_fail_exp = tg_exe_->read64(TG_FAIL_EXPECTED_DATA);
      tg_fail_act = tg_exe_->read64(TG_FAIL_READ_DATA);
      cerr << "TG status error" << std::endl;
      cout << "Failed at address 0x" << std::hex << tg_fail_addr
                << " exp=0x" << tg_fail_exp << " act=0x" << tg_fail_act
                << endl;
    } else {
      tg_exe_->logger_->debug("TG pass");
    }

    uint64_t clk_count = tg_exe_->read64(MEM_TG_CLK_COUNT);
    cout << "TG Read and Write Clock Cycles: " << dec << clk_count
              << endl;
    uint64_t wr_clk_count = tg_exe_->read64(MEM_TG_WR_COUNT);
    cout << "TG Write Clock Cycles: " << dec << wr_clk_count << endl;

    uint64_t rd_clk_count = clk_count - wr_clk_count;
    cout << "TG Read Clock Cycles: " << dec << rd_clk_count
              << endl;

    uint64_t write_bytes =
        64 * (tg_exe_->loop_ * tg_exe_->wcnt_ * tg_exe_->bcnt_);
    uint64_t read_bytes =
        64 * (tg_exe_->loop_ * tg_exe_->rcnt_ * tg_exe_->bcnt_);

    cout << "Write bytes: " << std::dec << write_bytes
        << endl;
    cout << "Read bytes: " << std::dec << read_bytes
        << endl;

    if (wr_clk_count > 0)
        cout << "Write BW: " << bw_calc(write_bytes, wr_clk_count) << " GB/s"
            << endl;
    else 
        cout << "Write BW: N/A" << endl;

    if (rd_clk_count > 0)
        cout << "Read BW: " << bw_calc(read_bytes, rd_clk_count) << " GB/s"
              << endl;
    else
        cout << "Read BW: N/A" << endl;

    if (clk_count > 0)
        cout << "Total BW: " << bw_calc(write_bytes + read_bytes, clk_count) << " GB/s\n"
            << endl;
    else 
        cout << "Total BW: N/A" << endl;
  }

  void tg_print_fail_info() {
    tg_exe_->logger_->info("test fail status:{}", tg_exe_->read32(TG_FAIL));
    tg_exe_->logger_->info("Number of failed reads (lower 32 bits):{}",
                           tg_exe_->read32(TG_FAIL_COUNT_L));
    tg_exe_->logger_->info("Number of failed reads (upper 32 bits):{}",
                           tg_exe_->read32(TG_FAIL_COUNT_H));
    tg_exe_->logger_->info(
        "Address of the first failed read (lower 32 bits):{}",
        tg_exe_->read32(TG_FIRST_FAIL_ADDR_L));
    tg_exe_->logger_->info(
        "Address of the first failed read (upper 32 bits).:{}",
        tg_exe_->read32(TG_FIRST_FAIL_ADDR_H));
  }

  bool tg_wait_test_completion() {
    /* Wait for test completion */
    uint32_t timeout = MEM_TG_TEST_TIMEOUT;
    uint64_t tg_status = 0x0;
    uint32_t tg_fail = 0;
    tg_exe_->logger_->debug("tg wait for test completion...");

    int ret = ofs_wait_for_eq32(TG_TEST_COMPLETE, 0x1,
        timeout_usec_, TEST_SLEEP_INVL);
    if (ret != 0) {
        std::cerr << "test completion timeout " << std::endl;
        tg_exe_->status_ = -1;
        tg_print_fail_info();
        return false;
    }

    tg_exe_->logger_->debug("test complete status:{}", tg_exe_->read32(TG_TEST_COMPLETE));
    tg_exe_->logger_->debug("tg pass:{}", tg_exe_->read32(TG_PASS));

    tg_fail = tg_exe_->read32(TG_FAIL);
    if (tg_fail == 0x1) {
      std::cerr << "Tg test failed:" << tg_fail << std::endl;
      tg_exe_->status_ = -1;
      tg_print_fail_info();
      return false;
    }

    // poll while active bit is set (channel status =
    // {pass,fail,timeout,active})
    tg_status = 0x0;
    tg_status = 0xF & (tg_exe_->read64(MEM_TG_STAT));
    tg_exe_->logger_->debug("Memory Traffic Generator Status:{}",
                            tg_exe_->read64(MEM_TG_STAT));

    while (tg_status == TG_STATUS_ACTIVE) {
      tg_status = 0xF & (tg_exe_->read64(MEM_TG_STAT));
      std::this_thread::yield();
      if (--timeout == 0) {
        std::cerr << "Memory Traffic Generator status active" << std::endl;
        tg_exe_->status_ = -1;
        return false;
      }
    }
    if (tg_status == TG_STATUS_TIMEOUT) {
      std::cerr << "Memory Traffic Generator status timeout" << std::endl;
      tg_exe_->status_ = -1;
      return false;
    }

    if (tg_status == TG_STATUS_ERROR) {
      std::cerr << "Memory Traffic Generator status error" << std::endl;
      tg_exe_->status_ = -1;
      return false;
    }
    tg_exe_->status_ = tg_status;

    tg_exe_->logger_->debug("Memory Traffic Generator Status pass");
    return true;
  }

  // Configure the registers with the value
  int config_input_options() {

    mem_tg_ctl tg_ctl;
    tg_ctl.value = tg_exe_->read64(MEM_TG_CTRL);
    tg_exe_->logger_->debug("tg configure input options...");
    tg_exe_->logger_->debug("mem tg ctl:{0:x}", tg_ctl.value);

    tg_mem_size mem_size;
    mem_size.value = tg_exe_->read64(MEM_SIZE);

    uint64_t value = mem_size.total_mem_size * MB / CL;
    tg_exe_->logger_->debug("Total hardware memory size:{}", value);
    value = mem_size.hdm_mem_size * MB / CL;
    tg_exe_->logger_->debug("HDM memory size:{0:d}", value);

    if (mem_size.hdm_mem_size != 0)
        tg_exe_->hdm_size_ = (mem_size.total_mem_size - mem_size.hdm_mem_size) * MB / CL;

   cout << "HDM memory cache line size:" << dec << tg_exe_->hdm_size_ << endl;

    if (tg_ctl.tg_capability != 0x1) {
      cerr << "No traffic generator for memory" << endl;
      return -1;
    }

    if (tg_exe_->wcnt_ == 0 && tg_exe_->rcnt_ == 0) {
        cerr << "Invalid Read and Write input arguments" << endl;
        return -1;
    }

    if (tg_exe_->option_passed("--timeout")) {
        timeout_usec_ = tg_exe_->get_timeout() * 1000;
    }

    if (tg_exe_->option_passed("--writes") &&
        tg_exe_->option_passed("--reads") &&
        (tg_exe_->rcnt_ > tg_exe_->wcnt_)) {
        cerr << "Read count exceeds Write count" << endl;
        return -1;
    }

    if ( tg_exe_->wcnt_ == 0) {
        cerr << " Write count is zero" << endl;
        return -1;
    }

    if ( (MAX(tg_exe_->wcnt_, tg_exe_->rcnt_) * tg_exe_->loop_) >=
        tg_exe_->hdm_size_) {
        cerr << "Read,Write and loop count exceeds HDM memory size" << endl;
        return -1;
    }

    tg_exe_->mem_speed_ = tg_exe_->read64(MEM_TG_CLK_FREQ);
    std::cout  << "Memory clock frequency (kHz) : " << tg_exe_->mem_speed_ << std::endl;
    if (0 == tg_exe_->mem_speed_) {
      tg_exe_->mem_speed_ = TG_FREQ;
      std::cout << "Memory channel clock frequency unknown. Assuming "
                << tg_exe_->mem_speed_ / 1000 << " MHz." << std::endl;
    } else {
      std::cout << "Memory clock:"
                << tg_exe_->mem_speed_ / 1000 << " MHz" << std::endl;
    }

    tg_exe_->logger_->debug("loops:{}", tg_exe_->loop_);
    tg_exe_->logger_->debug("write count:{}", tg_exe_->wcnt_);
    tg_exe_->logger_->debug("read count:{}", tg_exe_->rcnt_);
    tg_exe_->logger_->debug("burst length:{}", tg_exe_->bcnt_);
    tg_exe_->logger_->debug("sride:{}", tg_exe_->stride_);
    tg_exe_->logger_->debug("data pattern:{}", tg_exe_->pattern_);

    tg_exe_->write32(TG_LOOP_COUNT, tg_exe_->loop_);
    tg_exe_->write32(TG_WRITE_COUNT, tg_exe_->wcnt_);
    tg_exe_->write32(TG_READ_COUNT, tg_exe_->rcnt_);
    tg_exe_->write32(TG_BURST_LENGTH, tg_exe_->bcnt_);
    tg_exe_->write32(TG_SEQ_ADDR_INCR, tg_exe_->stride_);
    tg_exe_->write32(TG_PPPG_SEL, tg_exe_->pattern_);

    tg_exe_->write32(TG_WRITE_REPEAT_COUNT, 0x1);
    tg_exe_->write32(TG_READ_REPEAT_COUNT, 0x1);

    tg_exe_->write32(TG_RW_GEN_IDLE_COUNT, 0x1);
    tg_exe_->write32(TG_RW_GEN_LOOP_IDLE_COUNT, 0x1);

    //Set Start address for writes to 0 (Fixed)
    tg_exe_->write32(TG_SEQ_START_ADDR_WR, 0x0000);
    tg_exe_->write32(TG_SEQ_START_ADDR_WR + 0x04, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_WR + 0x08, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_WR + 0x0C, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_WR + 0x10, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_WR + 0x14, 0x0);

    //Set address generator mode to 3 (Field unused)
    tg_exe_->write32(TG_ADDR_MODE_WR, 2);
    tg_exe_->write32(TG_ADDR_MODE_WR + 0x04 ,3 );
    tg_exe_->write32(TG_ADDR_MODE_WR + 0x08 ,3 );
    tg_exe_->write32(TG_ADDR_MODE_WR + 0x0C ,3 );
    tg_exe_->write32(TG_ADDR_MODE_WR + 0x10 ,3 );
    tg_exe_->write32(TG_ADDR_MODE_WR + 0x14 ,3 );

    tg_exe_->write32(TG_SEQ_ADDR_INCR, 0x1);

    //Set Start address for reads to 0 (Fixed)
    tg_exe_->write32(TG_SEQ_START_ADDR_RD, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_RD + 0x04, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_RD + 0x08, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_RD + 0x0C, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_RD + 0x10, 0x0);
    tg_exe_->write32(TG_SEQ_START_ADDR_RD + 0x14, 0x0);

    //Set address generator mode to 3 (Field unused)
    tg_exe_->write32(TG_ADDR_MODE_RD, 2);
    tg_exe_->write32(TG_ADDR_MODE_RD +0x04,3 );
    tg_exe_->write32(TG_ADDR_MODE_RD +0x08,3 );
    tg_exe_->write32(TG_ADDR_MODE_RD +0x0C,3 );
    tg_exe_->write32(TG_ADDR_MODE_RD +0x10,3 );
    tg_exe_->write32(TG_ADDR_MODE_RD +0x14,3 );

    tg_exe_->write32(TG_USER_WORM_EN, 0);
    tg_exe_->write32(TG_RETURN_TO_START_ADDR, 0);
    uint32_t data_seed = 0x55555555;
    tg_exe_->logger_->debug("configuring TG data seed");

    for (uint32_t i = 0; i < 8; i++) {

        tg_exe_->write32(TG_DATA_SEED + i * 4, data_seed);
        tg_exe_->write32(TG_BYTEEN_SEED + i * 4, 0xffffffff);
        tg_exe_->write32(TG_PPPG_SEL + i * 4, 0);
        tg_exe_->write32(TG_BYTEEN_SEL + i * 4, 0);
        data_seed = ~data_seed;
    }

    // set Address Generator MSB Indices
    for (uint32_t i = 0; i < 5; i++) {
        tg_exe_->write32(TG_ADDR_FIELD_MSB_INDEX + i * 4, 0x1a);
    }
    return 0;
  }

  // The test state has been configured. Run one test instance.
  int run_mem_test() {
    cout << "Start Test..." << endl;
    tg_exe_->write32(TG_START, 0x1);

    if (!tg_wait_test_completion()) {
      return -1;
    }
    cout << "Test completed successfully... " << endl;
    tg_perf();

    return 0;
  }

  virtual int run(test_afu *afu, CLI::App *app) override {
    (void)app;
    tg_exe_ = dynamic_cast<cxl_mem_tg *>(afu);

    if (!tg_exe_) 
        return -1;

    tg_clear();
    print_he_mem_tg();

    auto ret = config_input_options();
    if (ret != 0) {
      std::cerr << "Failed to configure TG input options" << std::endl;
      return ret;
    }

    ret = run_mem_test();
    if (ret != 0) {
      std::cerr << "Failed to run tg cxl memory test..." << std::endl;
      print_he_mem_tg();
      return ret;
    }

    print_he_mem_tg();
    return 0;
  }

 protected:
  uint64_t tg_offset_;
  uint64_t timeout_usec_;
  cxl_mem_tg *tg_exe_;
};

}  // namespace cxl_mem_tg
