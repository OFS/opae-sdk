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
#include <opae/cxx/core/token.h>

#include <iostream>
#include <string>
#include <vector>

#include "afu_test.h"
using namespace std;
namespace cxl_mem_tg {
using opae::fpga::types::token;
const char *AFU_ID = "4DADEA34-2C78-48CB-A3DC-5B831F5CECBB";

static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t GB = MB * 1024;
static const uint64_t FPGA_32GB_CACHE_LINES = (32 * GB) / 64;
static const uint64_t MEM_TG_TEST_TIMEOUT = 10000000;
static const uint64_t TEST_SLEEP_INVL = 100;
static const uint64_t TG_CTRL_CLEAR = 0x8000000000000000;
static const uint64_t TG_SLEEP = 300 / 1000;
static const uint64_t TG_FREQ = 400 * 1000;

enum {
  TG_STATUS_ACTIVE = 0x1,
  TG_STATUS_TIMEOUT = 0x2,
  TG_STATUS_ERROR = 0x4,
  TG_STATUS_PASS = 0x8
};

enum {
  TG_ADDR_RAND = 0,
  TG_ADDR_SEQ = 1,
  TG_ADDR_RAND_SEQ = 2,
  TG_ADDR_ONE_HOT = 3
};

enum {
  TG_DATA_FIXED = 0,
  TG_DATA_PRBS7 = 1,
  TG_DATA_PRBS15 = 2,
  TG_DATA_PRBS31 = 3,
  TG_DATA_ROTATING = 4
};

const std::map<std::string, uint32_t> tg_pattern = {
    {"fixed", TG_DATA_FIXED},   {"prbs7", TG_DATA_PRBS7},
    {"prbs15", TG_DATA_PRBS15}, {"prbs31", TG_DATA_PRBS31},
    {"rot1", TG_DATA_PRBS31},
};

enum {
  AFU_DFH = 0x0000,
  AFU_ID_L = 0x0008,
  AFU_ID_H = 0x0010,
  NEXT_AFU = 0x0018,
  AFU_DFH_RSVD = 0x0020,
  SCRATCHPAD = 0x0028,
  MEM_TG_CTRL = 0x0030,
  MEM_TG_STAT = 0x0038,
  MEM_TG_CLK_COUNT = 0x0050,
  MEM_TG_WR_COUNT = 0x0058,
  MEM_TG_CLK_FREQ = 0x0060,
  MEM_SIZE = 0x0068,

};
const int MEM_TG_CFG_OFFSET = 0x1000;

// TG Address Map -- Byte addressed
enum {
  TG_VERSION = MEM_TG_CFG_OFFSET + 0x000,
  TG_START = MEM_TG_CFG_OFFSET + 0x004,
  TG_LOOP_COUNT = MEM_TG_CFG_OFFSET + 0x008,
  TG_WRITE_COUNT = MEM_TG_CFG_OFFSET + 0x00C,
  TG_READ_COUNT = MEM_TG_CFG_OFFSET + 0x010,
  TG_WRITE_REPEAT_COUNT = MEM_TG_CFG_OFFSET + 0x014,
  TG_READ_REPEAT_COUNT = MEM_TG_CFG_OFFSET + 0x018,
  TG_BURST_LENGTH = MEM_TG_CFG_OFFSET + 0x01C,
  TG_CLEAR = MEM_TG_CFG_OFFSET + 0x020,
  TG_RW_GEN_IDLE_COUNT = MEM_TG_CFG_OFFSET + 0x038,
  TG_RW_GEN_LOOP_IDLE_COUNT = MEM_TG_CFG_OFFSET + 0x03C,
  TG_SEQ_START_ADDR_WR = MEM_TG_CFG_OFFSET + 0x040,
  TG_ADDR_MODE_WR = MEM_TG_CFG_OFFSET + 0x080,
  TG_RETURN_TO_START_ADDR = MEM_TG_CFG_OFFSET + 0x0C0,
  TG_SEQ_ADDR_INCR = MEM_TG_CFG_OFFSET + 0x0100,
  TG_SEQ_START_ADDR_RD = MEM_TG_CFG_OFFSET + 0x140,
  TG_ADDR_MODE_RD = MEM_TG_CFG_OFFSET + 0x180,
  TG_PASS = MEM_TG_CFG_OFFSET + 0x1C0,
  TG_FAIL = MEM_TG_CFG_OFFSET + 0x1C4,
  TG_FAIL_COUNT_L = MEM_TG_CFG_OFFSET + 0x1C8,
  TG_FAIL_COUNT_H = MEM_TG_CFG_OFFSET + 0x01CC,
  TG_FIRST_FAIL_ADDR_L = MEM_TG_CFG_OFFSET + 0x01D0,
  TG_FIRST_FAIL_ADDR_H = MEM_TG_CFG_OFFSET + 0x01D4,
  TG_TOTAL_READ_COUNT_L = MEM_TG_CFG_OFFSET + 0x01D8,
  TG_TOTAL_READ_COUNT_H = MEM_TG_CFG_OFFSET + 0x01DC,
  TG_TEST_COMPLETE = MEM_TG_CFG_OFFSET + 0x01E0,
  TG_INVERT_BYTEEN = MEM_TG_CFG_OFFSET + 0x01E4,
  TG_RESTART_DEFAULT_TRAFFIC = MEM_TG_CFG_OFFSET + 0x01E8,
  TG_USER_WORM_EN = MEM_TG_CFG_OFFSET + 0x01EC,
  TG_TEST_BYTEEN = MEM_TG_CFG_OFFSET + 0x01F0,
  TG_TIMEOUT = MEM_TG_CFG_OFFSET + 0x01F4,
  TG_NUM_DATA_GEN = MEM_TG_CFG_OFFSET + 0x01F8,
  TG_NUM_BYTEEN_GEN = MEM_TG_CFG_OFFSET + 0x01FC,
  TG_RDATA_WIDTH = MEM_TG_CFG_OFFSET + 0x0200,
  TG_ERROR_REPORT = MEM_TG_CFG_OFFSET + 0x0208,
  TG_PNF = MEM_TG_CFG_OFFSET + 0x0240,
  TG_FAIL_EXPECTED_DATA = MEM_TG_CFG_OFFSET + 0x0340,
  TG_FAIL_READ_DATA = MEM_TG_CFG_OFFSET + 0x0440,
  TG_DATA_SEED = MEM_TG_CFG_OFFSET + 0x0540,
  TG_BYTEEN_SEED = MEM_TG_CFG_OFFSET + 0x0580,
  TG_PPPG_SEL = MEM_TG_CFG_OFFSET + 0x05C0,
  TG_BYTEEN_SEL = MEM_TG_CFG_OFFSET + 0x0600,
  TG_ADDR_FIELD_RELATIVE_FREQ = MEM_TG_CFG_OFFSET + 0x0640,
  TG_ADDR_FIELD_MSB_INDEX = MEM_TG_CFG_OFFSET + 0x0680,
  TG_BURSTLENGTH_OVERFLOW_OCCURRED = MEM_TG_CFG_OFFSET + 0x06C0,
  TG_BURSTLENGTH_FAIL_ADDR_L = MEM_TG_CFG_OFFSET + 0x0700,
  TG_BURSTLENGTH_FAIL_ADDR_H = MEM_TG_CFG_OFFSET + 0x0704,
  TG_WORM_MODE_TARGETTED_DATA = MEM_TG_CFG_OFFSET + 0x0740,
};

// Traffic generator capability
union mem_tg_ctl {
  enum { offset = MEM_TG_CTRL };
  uint64_t value;
  struct {
    uint64_t tg_capability : 4;
    uint64_t Rsvd_63_3 : 59;
    uint64_t counter_clear : 1;
  };
};

// Memory Traffic Generator Status
union mem_tg_status {
  enum { offset = MEM_TG_STAT };
  uint64_t value;
  struct {
    uint64_t tg_status0 : 4;
    uint64_t tg_status1 : 4;
    uint64_t tg_status2 : 4;
    uint64_t tg_status3 : 4;
    uint64_t Rsvd_63_16 : 48;
  };
};

// Memory Traffic Generator count
union mem_tg0_count {
  enum { offset = MEM_TG_CLK_COUNT };
  uint64_t value;
  struct {
    uint64_t count : 64;
  };
};

// Memory Traffic Generator count
union mem_tg1_count {
  enum { offset = MEM_TG_WR_COUNT };
  uint64_t value;
  struct {
    uint64_t count : 64;
  };
};

// Memory Size
union tg_mem_size {
    enum { offset = MEM_SIZE };
    uint64_t value;
    struct {
        uint64_t hdm_mem_size : 32;
        uint64_t total_mem_size : 32;
    };
};

using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

class cxl_mem_tg : public test_afu {
 public:
  cxl_mem_tg(const char *afu_id = AFU_ID)
      : test_afu("cxl_mem_tg_afu", afu_id),
        count_(1),
        mem_ch_(0),
        loop_(1),
        wcnt_(1),
        rcnt_(1),
        bcnt_(1),
        stride_(1),
        mem_speed_(TG_FREQ),
        hdm_size_(FPGA_32GB_CACHE_LINES)
  {
    // iterations
    app_.add_option("--count", count_, "Number of iterations to run")
        ->default_val("1");

    // Loops
    app_.add_option("--loops", loop_, "Number of read/write loops to be run")
        ->transform(CLI::Range(1, 268435456))->default_val("1");

    // Writes
    app_.add_option("-w,--writes", wcnt_,
                    "Number of unique write transactions per loop")
        ->transform(CLI::Range(0, 4095))->default_val("1");

    // Reads
    app_.add_option("-r,--reads", rcnt_,
                    "Number of unique read transactions per loop")
        ->transform(CLI::Range(0, 4095))->default_val("1");

    // Address Stride
    app_.add_option("--stride", stride_,
                    "Address stride for each sequential transaction")
        ->default_val("1");

    // Data pattern
    app_.add_option(
            "--data", pattern_,
            "Memory traffic data pattern: fixed, prbs7, prbs15, prbs31, rot1")
        ->transform(CLI::CheckedTransformer(tg_pattern))
        ->default_val("fixed");
  }

  virtual int run(CLI::App *app, test_command::ptr_t test) override {
    int res = exit_codes::not_run;
    logger_->info("starting test run, count of {0:d}", count_);
    uint32_t count = 0;
    try {
      while (count < count_) {
        logger_->debug("starting iteration: {0:d}", count + 1);
        res = test_afu::run(app, test);
        count++;
        logger_->debug("end iteration: {0:d}", count);
        if (res) break;
      }
    } catch (std::exception &ex) {
      logger_->error(ex.what());
      res = exit_codes::exception;
    }
    auto pass = res == exit_codes::success ? "PASS" : "FAIL";
    logger_->info("Test {}({}): {}", test->name(), count, pass);
    spdlog::drop_all();
    return res;
  }

 public:
  uint32_t count_;
  uint32_t mem_ch_;
  uint64_t loop_;
  uint64_t wcnt_;
  uint64_t rcnt_;
  uint64_t bcnt_;
  uint32_t stride_;
  uint32_t pattern_;
  uint64_t mem_speed_;
  uint32_t status_;
  uint64_t tg_offset_;
  uint64_t hdm_size_;

  std::map<uint32_t, uint32_t> limits_;

  uint32_t get_offset(uint32_t base, uint32_t i) const {
    auto limit = limits_.find(base);
    auto offset = base + sizeof(uint64_t) * i;
    if (limit != limits_.end() && offset > limit->second - sizeof(uint64_t)) {
      throw std::out_of_range("offset out range in csr space");
    }
    return offset;
  }

  token::ptr_t get_token() { return handle_->get_token(); }

  bool option_passed(string str) {
      CLI::Option* opt = app_.get_option_no_throw(str);
      if (opt && opt->count() == 1) {
          return true;
      }
      return false;
  }

  uint64_t get_timeout() {
      return timeout_msec_;
  }


};
}  // end of namespace cxl_mem_tg
