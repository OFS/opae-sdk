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

#include "he_cache_test.h"

#define MEM_TG_FEATURE_ID 0x25
#define MEM_TG_FEATURE_GUIDL 0x81599b5c2ebd4b23
#define MEM_TG_FEATURE_GUIDH 0x0118e06b1fa349b9
const char *HE_CACHE_AFU_ID = "0118E06B-1FA3-49B9-8159-9b5C2EBD4b23";

#define RUNNIG_PTR_DATA_PATTERN 0x123456

namespace host_exerciser {

static const uint64_t HE_CACHE_TEST_TIMEOUT = 30000;
static const uint64_t HE_CACHE_TEST_SLEEP_INVL = 100;
static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t BUFFER_SIZE_2MB = 2 * MB;
static const uint64_t BUFFER_SIZE_32KB = 32* KB;
static const uint64_t FPGA_32KB_CACHE_LINES = (32 * KB) / 64;
static const uint64_t FPGA_2MB_CACHE_LINES = (2 * MB) / 64;
static const uint64_t FPGA_512CACHE_LINES = 512;
static const double LATENCY_FACTOR = 2.5;

// Host execiser CSR Offset
enum {
  HE_DFH = 0x0000,
  HE_ID_L = 0x0008,
  HE_ID_H = 0x0010,
  HE_DFH_RSVD0 = 0x0018,
  HE_DFH_RSVD1 = 0x0020,
  HE_SCRATCHPAD0 = 0x028,
  HE_DSM_BASE = 0x030,
  HE_CTL = 0x038,
  HE_INFO = 0x040,
  HE_WR_NUM_LINES = 0x048,
  HE_WR_BYTE_ENABLE = 0x050,
  HE_WR_CONFIG = 0x058,
  HE_WR_ADDR_TABLE_CTRL = 0x060,
  HE_WR_ADDR_TABLE_DATA = 0x068,
  HE_RD_NUM_LINES = 0x070,
  HE_RD_CONFIG = 0x078,
  HE_RD_ADDR_TABLE_CTRL = 0x080,
  HE_RD_ADDR_TABLE_DATA = 0x088,
  HE_ERROR_STATUS = 0x090,
  HE_ERROR_EXP_DATA = 0x098,
  HE_ERROR_ACT_DATA0 = 0x0A0,
  HE_ERROR_ACT_DATA1 = 0x0A8,
  HE_ERROR_ACT_DATA2 = 0x0B0,
  HE_ERROR_ACT_DATA3 = 0x0B8,
  HE_ERROR_ACT_DATA4 = 0x0C0,
  HE_ERROR_ACT_DATA5 = 0x0C8,
  HE_ERROR_ACT_DATA6 = 0x0D0,
  HE_ERROR_ACT_DATA7 = 0x0D8,
};

// Read Traffic Opcode
typedef enum {
  RD_LINE_I = 0x0,
  RD_LINE_S = 0x1,
  RD_LINE_EM = 0x2,
} he_rd_opcode;

// Write Traffic Opcode
typedef enum {
  WR_LINE_I = 0x0,
  WR_LINE_M = 0x1,
  WR_PUSH_I = 0x2,
  WR_BARRIER_FRNCE = 0x3,
  WR_FLUSH_CL = 0x4,
  WR_FLUSH_CL_HCOH = 0x5,
  WR_FLUSH_CL_DCOH = 0x6,
} he_wr_opcode;


// Write Traffic Opcode
typedef enum {
    RD_WR_TEST = 0x0,
    RUNNING_POINTER = 0x10,
    PING_PONG = 0x20,
 } he_test_type;

// DFH Header
union he_dfh {
  enum { offset = HE_DFH };
  uint64_t value;
  struct {
    uint64_t CcipVersionNumber : 12;
    uint64_t AfuMajVersion : 4;
    uint64_t NextDfhOffset : 24;
    uint64_t EOL : 1;
    uint64_t Reserved : 19;
    uint64_t FeatureType : 4;
  };
};

// DSM BASE
union he_dsm_base {
  enum { offset = HE_DSM_BASE };
  uint64_t value;
  struct {
    uint64_t DsmBase : 64;
  };
};

// CSR CTL
union he_ctl {
  enum { offset = HE_CTL };
  uint64_t value;
  struct {
    uint64_t ResetL : 1;
    uint64_t Start : 1;
    uint64_t ForcedTestCmpl : 1;
    uint64_t bias_support : 2;
    uint64_t Reserved : 3;
    uint64_t test_type : 8;
    uint64_t Reserved1 :48;
  };
};

// CSR INFO
union he_info {
  enum { offset = HE_INFO };
  uint64_t value;
  struct {
    uint64_t write_addr_table_size : 4;
    uint64_t read_addr_table_size : 4;
    uint64_t Reserved : 56;
  };
};

// HE_WR_NUM_LINES
union he_wr_num_lines {
  enum { offset = HE_WR_NUM_LINES };
  uint64_t value;
  struct {
    uint64_t write_num_lines : 16;
    uint64_t reserved : 48;
  };
};

// HE_WR_BYTE_ENABLE
union he_wr_byte_enable {
  enum { offset = HE_WR_BYTE_ENABLE };
  uint64_t value;
  struct {
    uint64_t write_byte_enable : 64;
  };
};

// HE_WR_CONFIG
union he_wr_config {
  enum { offset = HE_WR_CONFIG };
  uint64_t value;
  struct {
    uint64_t write_traffic_enable : 1;
    uint64_t continuous_mode_enable : 1;
    uint64_t barrier : 1;
    uint64_t preread_sync_enable : 1;
    uint64_t postread_sync_enable : 1;
    uint64_t data_pattern : 2;
    uint64_t cl_evict_enable : 1;
    uint64_t opcode : 4;
    uint64_t line_repeat_count : 8;
    uint64_t rsvd_31_20 : 12;
    uint64_t repeat_write_fsm : 16;
    uint64_t disable_waitfor_completion : 1;
    uint64_t rsvd_63_48 : 15;
  };
};

// HE_WR_ADDR_TABLE_CTRL
union he_wr_addr_table_ctrl {
  enum { offset = HE_WR_ADDR_TABLE_CTRL };
  uint64_t value;
  struct {
    uint64_t enable_address_table : 1;
    uint64_t enable_address_stride : 1;
    uint64_t stride : 2;
    uint64_t reserved : 60;
  };
};

// HE_WR_ADDR_TABLE_DATA
union he_wr_addr_table_data {
  enum { offset = HE_WR_ADDR_TABLE_DATA };
  uint64_t value;
  struct {
    uint64_t address_table_value : 64;
  };
};

// HE_RD_NUM_LINES
union he_rd_num_lines {
  enum { offset = HE_RD_NUM_LINES };
  uint64_t value;
  struct {
    uint64_t read_num_lines : 16;
    uint64_t reserved : 16;
    uint64_t max_count : 32;
  };
};

// HE_RD_CONFIG
union he_rd_config {
  enum { offset = HE_RD_CONFIG };
  uint64_t value;
  struct {
    uint64_t read_traffic_enable : 1;
    uint64_t continuous_mode_enable : 1;
    uint64_t waitfor_completion : 1;
    uint64_t prewrite_sync_enable : 1;
    uint64_t postwrite_sync_enable : 1;
    uint64_t data_pattern : 2;
    uint64_t data_check_enable : 1;
    uint64_t opcode : 4;
    uint64_t line_repeat_count : 8;
    uint64_t rsvd_31_20 : 12;
    uint64_t repeat_read_fsm : 16;
    uint64_t rsvd_63_40 : 16;
  };
};

// HE_RD_ADDR_TABLE_CTRL
union he_rd_addr_table_ctrl {
  enum { offset = HE_RD_ADDR_TABLE_CTRL };
  uint64_t value;
  struct {
    uint64_t enable_address_table : 1;
    uint64_t enable_address_stride : 1;
    uint64_t stride : 2;
    uint64_t reserved : 60;
  };
};

// HE_RD_ADDR_TABLE_DATA
union he_rd_addr_table_data {
  enum { offset = HE_RD_ADDR_TABLE_DATA };
  uint64_t value;
  struct {
    uint64_t address_table_value : 64;
  };
};

// ERROR_STATUS
union he_err_status {
  enum { offset = HE_ERROR_STATUS };
  uint64_t value;
  struct {
    uint64_t data_error : 1;
    uint64_t rsvd1 : 15;
    uint64_t err_index : 16;
    uint64_t rsvd2 : 32;
  };
};

// HE DSM status
struct he_cache_dsm_status {
  uint32_t test_completed : 1;
  uint32_t dsm_number : 15;
  uint32_t res1 : 16;
  uint32_t err_vector : 32;
  uint64_t num_ticks : 64;
  uint32_t num_reads : 32;
  uint32_t num_writes : 32;
  uint32_t penalty_start : 32;
  uint32_t penalty_end : 32;
  uint32_t actual_data : 32;
  uint32_t expected_data : 32;
  uint32_t res5[2];
};

// configures test mode
typedef enum {
  HE_FPGA_RD_CACHE_HIT = 0x0,
  HE_FPGA_WR_CACHE_HIT = 0x1,

  HE_FPGA_RD_CACHE_MISS = 0x2,
  HE_FPGA_WR_CACHE_MISS = 0x3,

  HE_HOST_RD_CACHE_HIT = 0x4,
  HE_HOST_WR_CACHE_HIT = 0x5,

  HE_HOST_RD_CACHE_MISS = 0x6,
  HE_HOST_WR_CACHE_MISS = 0x7,

  HE_CACHE_PING_PONG = 0x8,
  HE_CACHE_RUNNING_POINTER= 0x9,


} he_test_mode;

// configures traget
typedef enum {
  HE_TARGET_HOST = 0x0,
  HE_TARGET_FPGA = 0x1,
  HE_TARGET_BOTH = 0x2,
} he_target;


// he cxl cache latency
typedef enum {
    HE_CXL_LATENCY_NONE = 0x0,
    HE_CXL_RD_LATENCY = 0x1,
    HE_CXL_WR_LATENCY = 0x2,
    HE_CXL_RD_WR_LATENCY = 0x3,
} he_cxl_latency;

const std::map<std::string, uint32_t> he_test_modes = {
    {"fpgardcachehit", HE_FPGA_RD_CACHE_HIT},
    {"fpgawrcachehit", HE_FPGA_WR_CACHE_HIT},
    {"fpgardcachemiss", HE_FPGA_RD_CACHE_MISS},
    {"fpgawrcachemiss", HE_FPGA_WR_CACHE_MISS},
    {"hostrdcachehit", HE_HOST_RD_CACHE_HIT},
    {"hostwrcachehit", HE_HOST_WR_CACHE_HIT},
    {"pingpong", HE_CACHE_PING_PONG},
    {"runningpointer", HE_CACHE_RUNNING_POINTER},
};

// Bias Support
typedef enum {
    HOSTMEM_BIAS = 0x0,
    HOST_BIAS_NA = 0x1,
    FPGAMEM_HOST_BIAS = 0x2,
    FPGAMEM_DEVICE_BIAS = 0x3,
} he_bias_support;

const std::map<std::string, uint32_t> he_targets = {
    {"host", HE_TARGET_HOST},
    {"fpga", HE_TARGET_FPGA},
    {"both", HE_TARGET_BOTH},
};

// Bias support
const std::map<std::string, uint32_t> he_bias = {
    {"host", HOSTMEM_BIAS},
    {"device", FPGAMEM_DEVICE_BIAS},
};

// he cxl cache device instance
typedef enum {
    HE_CXL_DEVICE0 = 0x0,
    HE_CXL_DEVICE1 = 0x1,
} he_cxl_dev;

const std::map<std::string, uint32_t> he_cxl_device = {
    {"/dev/dfl-cxl-cache.0", HE_CXL_DEVICE0},
    {"/dev/dfl-cxl-cache.1", HE_CXL_DEVICE1},
};

// configures test mode
typedef enum {
  HE_ADDRTABLE_SIZE4096 = 0xC,
  HE_ADDRTABLE_SIZE2048 = 0xB,
  HE_ADDRTABLE_SIZE1024 = 0xA,
  HE_ADDRTABLE_SIZE512 = 0x9,
  HE_ADDRTABLE_SIZE256 = 0x8,
  HE_ADDRTABLE_SIZE128 = 0x7,
  HE_ADDRTABLE_SIZE64 = 0x6,
  HE_ADDRTABLE_SIZE32 = 0x5,
  HE_ADDRTABLE_SIZE16 = 0x4,
  HE_ADDRTABLE_SIZE8 = 0x3,
  HE_ADDRTABLE_SIZE4 = 0x2,
  HE_ADDRTABLE_SIZE2 = 0x1,
} he_addrtable_size;

// he test type
typedef enum {
  HE_ENABLE_TRAFFIC_STAGE = 0x0,
  HE_SIP_SEQ_STAGE = 0x1,
} he_traffic_enable;

const std::map<std::string, uint32_t> traffic_enable = {
    {"enable", HE_ENABLE_TRAFFIC_STAGE},
    {"skip", HE_SIP_SEQ_STAGE},
};

std::map<uint32_t, uint32_t> addrtable_size = {
    {HE_ADDRTABLE_SIZE4096, 4096}, {HE_ADDRTABLE_SIZE2048, 2048},
    {HE_ADDRTABLE_SIZE1024, 1024}, {HE_ADDRTABLE_SIZE512, 512},
    {HE_ADDRTABLE_SIZE256, 256},   {HE_ADDRTABLE_SIZE128, 128},
    {HE_ADDRTABLE_SIZE64, 64},     {HE_ADDRTABLE_SIZE32, 32},
    {HE_ADDRTABLE_SIZE16, 16},     {HE_ADDRTABLE_SIZE8, 8},
    {HE_ADDRTABLE_SIZE4, 4},       {HE_ADDRTABLE_SIZE2, 2},

};

// HE Cache Running pointer
struct he_cache_running_ptr {
    uint64_t phy_next_ptr;
    uint64_t data;
    he_cache_running_ptr *virt_next_ptr;
    uint64_t rsvd[4];
    union {
        uint64_t mode;
        struct {
            uint64_t rsvd_0_62 : 62;
            uint64_t biasmode :2;
        };
    };
};


using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

class host_exerciser : public test_afu {
public:
  host_exerciser()
      : test_afu("host_exerciser", nullptr, "info"), count_(1) {}

  virtual int run(CLI::App *app, test_command::ptr_t test) override {
    int res = exit_codes::not_run;

    logger_->set_pattern("    %v");
    // Info prints details of an individual run. Turn it on if doing only one
    // test and the user hasn't changed level from the default.
    if ((log_level_.compare("warning") == 0))
       logger_->set_level(spdlog::level::info);


    logger_->info("starting test run, count of {0:d}", count_);
    uint32_t count = 0;
    try {
      while (count < count_) {
        logger_->debug("starting iteration: {0:d}", count + 1);

        res = test_afu::run(app, test);
        count++;
        logger_->debug("end iteration: {0:d}", count);
        if (res)
          break;
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

  bool option_passed(std::string option_str) {
    if (app_.count(option_str) == 0)
      return false;
    return true;
  }
};
} // namespace host_exerciser
