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
#include <opae/cxx/core/token.h>
#include <iostream>
#include <vector>
#include <string>

#include "afu_test.h"

namespace mem_tg {
using opae::fpga::types::token;
const char *AFU_ID  = "4DADEA34-2C78-48CB-A3DC-5B831F5CECBB";

static const uint64_t MEM_TG_TEST_TIMEOUT = 30000;
static const uint64_t TEST_SLEEP_INVL = 100;

enum {
  TG_STATUS_ACTIVE = 0x1,
  TG_STATUS_TIMEOUT = 0x2,
  TG_STATUS_ERROR = 0x4,
  TG_STATUS_PASS  = 0x8,
  TG_STATUS_RESPONSE_TIMEOUT = 0x10
};

enum {
  TG_ADDR_RAND     = 0,
  TG_ADDR_SEQ      = 1,
  TG_ADDR_RAND_SEQ = 2,
  TG_ADDR_ONE_HOT  = 3
};

enum {
  TG_DATA_FIXED    = 0,
  TG_DATA_PRBS7    = 1,
  TG_DATA_PRBS15   = 2,
  TG_DATA_PRBS31   = 3,
  TG_DATA_ROTATING = 4
};

const std::map<std::string, uint32_t> tg_pattern = {
  { "fixed",  TG_DATA_FIXED},
  { "prbs7",  TG_DATA_PRBS7},
  { "prbs15", TG_DATA_PRBS15},
  { "prbs31", TG_DATA_PRBS31},
  { "rot1",   TG_DATA_PRBS31},
};
  
enum {
  AFU_DFH         = 0x0000,
  AFU_ID_L        = 0x0008,
  AFU_ID_H        = 0x0010,
  NEXT_AFU        = 0x0018,
  AFU_DFH_RSVD    = 0x0020,
  SCRATCHPAD      = 0x0028,
  MEM_TG_CTRL     = 0x0030,
  MEM_TG_STAT     = 0x0038,
  MEM_TG_CLOCKS   = 0x0050  
};
const int MEM_TG_CFG_OFFSET = 0x1000;

// TG Address Map -- Byte addressed
enum { 
   TG_VERSION                 = 0x000,
   TG_START                   = 0x004,
   TG_LOOP_COUNT              = 0x008,
   TG_WRITE_COUNT             = 0x00C,
   TG_READ_COUNT              = 0x010,
   TG_WRITE_REPEAT_COUNT      = 0x014,
   TG_READ_REPEAT_COUNT       = 0x018,
   TG_BURST_LENGTH            = 0x01C,
   TG_CLEAR                   = 0x020,
   TG_RW_GEN_IDLE_COUNT       = 0x038,
   TG_RW_GEN_LOOP_IDLE_COUNT  = 0x03C,
   TG_SEQ_START_ADDR_WR_L     = 0x040,
   TG_SEQ_START_ADDR_WR_H     = 0x044,
   TG_ADDR_MODE_WR            = 0x048,
   TG_RAND_SEQ_ADDRS_WR       = 0x04C,
   TG_RETURN_TO_START_ADDR    = 0x050,
   TG_SEQ_ADDR_INCR           = 0x074,
   TG_SEQ_START_ADDR_RD_L     = 0x078,
   TG_SEQ_START_ADDR_RD_H     = 0x07C,
   TG_ADDR_MODE_RD            = 0x080,
   TG_RAND_SEQ_ADDRS_RD       = 0x084,
   TG_PASS                    = 0x088,
   TG_FAIL                    = 0x08C,
   TG_FAIL_COUNT_L            = 0x090,
   TG_FAIL_COUNT_H            = 0x094,
   TG_FIRST_FAIL_ADDR_L       = 0x098,
   TG_FIRST_FAIL_ADDR_H       = 0x09C,
   TG_TOTAL_READ_COUNT_L      = 0x0A0,
   TG_TOTAL_READ_COUNT_H      = 0x0A4,
   TG_TEST_COMPLETE           = 0x0A8,
   TG_INVERT_BYTEEN           = 0x0AC,
   TG_RESTART_DEFAULT_TRAFFIC = 0x0B0,
   TG_USER_WORM_EN            = 0x0B4,
   TG_TEST_BYTEEN             = 0x0B8,
   TG_TIMEOUT                 = 0x0BC,
   TG_NUM_DATA_GEN            = 0x0C4,
   TG_NUM_BYTEEN_GEN          = 0x0C8,
   TG_RDATA_WIDTH             = 0x0DC,
   TG_ERROR_REPORT            = 0x0EC,
   TG_DATA_RATE_WIDTH_RATIO   = 0x0F0,
   TG_PNF                     = 0x100,
   TG_FAIL_EXPECTED_DATA      = 0x200,
   TG_FAIL_READ_DATA          = 0x300,
   TG_DATA_SEED               = 0x400,
   TG_BYTEEN_SEED             = 0x200,
   TG_PPPG_SEL                = 0xC00,
   TG_BYTEEN_SEL              = 0xE80
};

using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

class mem_tg : public test_afu {
public:
  mem_tg(const char *afu_id = AFU_ID)
  : test_afu("mem_traffic_afu", afu_id)
  , count_(1)
  , mem_ch_(0)
  , loop_(1)
  , wcnt_(1)
  , rcnt_(1)
  , bcnt_(1)
  , stride_(1)
  , mem_speed_(0)
  {
    // Channel
    app_.add_option("-m,--mem-channel", mem_ch_, "Target memory banks for test to run on (0 indexed). Multiple banks seperated by ', '. 'all' will use every channel enumerated in MEM_TG_CTRL")
      ->default_val("0");

    // Loops
    app_.add_option("--loops", loop_, "Number of read/write loops to be run")
      ->default_val("1");

    // Writes
    app_.add_option("-w,--writes", wcnt_, "Number of unique write transactions per loop")
      ->default_val("1");

    // Reads
    app_.add_option("-r,--reads", rcnt_, "Number of unique read transactions per loop")
      ->default_val("1");

    // Burst length
    app_.add_option("-b,--bls", bcnt_, "Burst length of each request")
      ->default_val("1");

    // Address Stride
    app_.add_option("--stride", stride_, "Address stride for each sequential transaction")
      ->default_val("1");

    // Data pattern
    app_.add_option("--data", pattern_, "Memory traffic data pattern: fixed, prbs7, prbs15, prbs31, rot1")
      ->transform(CLI::CheckedTransformer(tg_pattern))->default_val("fixed");

    // AFU memory clock speed
    app_.add_option("-f,--mem-frequency", mem_speed_, "Memory traffic clock frequency in MHz")
      ->default_val("0");

    // Add Address mode?

  }

  virtual int run(CLI::App *app, test_command::ptr_t test) override
  {
    int res = exit_codes::not_run;
    logger_->info("starting test run, count of {0:d}", count_);
    uint32_t count = 0;
    try {
      while (count < count_) {
        logger_->debug("starting iteration: {0:d}", count+1);
        res = test_afu::run(app, test);
        count++;
        logger_->debug("end iteration: {0:d}", count);
        if (res)
          break;
      }
    } catch(std::exception &ex) {
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
  std::vector<std::string> mem_ch_;
  uint32_t loop_;
  uint32_t wcnt_;
  uint32_t rcnt_;
  uint32_t bcnt_;
  uint32_t stride_;
  uint32_t pattern_;
  uint32_t mem_speed_;
  uint32_t status_;
  uint64_t tg_offset_;

  std::map<uint32_t, uint32_t> limits_;

  uint32_t get_offset(uint32_t base, uint32_t i) const {
    auto limit = limits_.find(base);
    auto offset = base + sizeof(uint64_t)*i;
    if (limit != limits_.end() &&
        offset > limit->second - sizeof(uint64_t)) {
      throw std::out_of_range("offset out range in csr space");
    }
    return offset;
  }
  
  token::ptr_t get_token()
  {
    return handle_->get_token();
  }

  // Duplicate contents of this mem_tg to duplicate_obj.
  // commands_, current_command_, app_ are ommited since they are not
  // relevant to closed instances of mem_tg that don't interact with commands.
  void duplicate(mem_tg *duplicate_obj) const {
    duplicate_obj->count_        = this->count_;
    duplicate_obj->loop_         = this->loop_;
    duplicate_obj->wcnt_         = this->wcnt_;
    duplicate_obj->rcnt_         = this->rcnt_;
    duplicate_obj->bcnt_         = this->bcnt_;
    duplicate_obj->stride_       = this->stride_;
    duplicate_obj->pattern_      = this->pattern_;
    duplicate_obj->mem_speed_    = this->mem_speed_;
    duplicate_obj->name_         = this->name_;
    duplicate_obj->afu_id_       = this->afu_id_;
    duplicate_obj->pci_addr_     = this->pci_addr_;
    duplicate_obj->log_level_    = this->log_level_;
    duplicate_obj->shared_       = this->shared_;
    duplicate_obj->timeout_msec_ = this->timeout_msec_;
    duplicate_obj->handle_       = this->handle_;
    duplicate_obj->logger_       = this->logger_;
  }

};
} // end of namespace mem_tg
