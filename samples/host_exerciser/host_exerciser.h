// Copyright(c) 2020-2021, Intel Corporation
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
#include <opae/cxx/core/events.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>

#include "afu_test.h"

namespace host_exerciser {
using opae::fpga::types::event;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::token;

static const uint64_t HELPBK_TEST_TIMEOUT = 30000;
static const uint64_t HELPBK_TEST_SLEEP_INVL = 100;
static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t LOG2_CL = 6;
static const size_t LPBK1_DSM_SIZE = 2 * KB;
static const size_t LPBK1_BUFFER_SIZE = 64 * KB;
static const size_t LPBK1_BUFFER_ALLOCATION_SIZE = 64 * KB;
static const size_t MAX_NUM_MEM_CHANNELS = 8; // The maximum number of assumed memory channels created by the dfl-emif driver. 

// Host execiser CSR Offset
enum {
  HE_DFH = 0x0000,
  HE_ID_L = 0x0008,
  HE_ID_H = 0x0010,
  HE_DFH_RSVD0 = 0x0018,
  HE_DFH_RSVD1 = 0x0020,
  HE_SCRATCHPAD0 = 0x0100,
  HE_SCRATCHPAD1 = 0x0104,
  HE_SCRATCHPAD2 = 0x0108,
  HE_DSM_BASEL = 0x0110,
  HE_DSM_BASEH = 0x0114,
  HE_SRC_ADDR = 0x0120,
  HE_DST_ADDR = 0x0128,
  HE_NUM_LINES = 0x0130,
  HE_CTL = 0x0138,
  HE_CFG = 0x0140,
  HE_INACT_THRESH = 0x0148,
  HE_INTERRUPT0 = 0x0150,
  HE_SWTEST_MSG = 0x0158,
  HE_STATUS0 = 0x0160,
  HE_STATUS1 = 0x0168,
  HE_ERROR = 0x0170,
  HE_STRIDE = 0x0178,
  HE_INFO0 = 0x0180,
};

//configures test mode
typedef enum {
  HOST_EXEMODE_LPBK1 = 0x0,
  HOST_EXEMODE_READ = 0x1,
  HOST_EXEMODE_WRITE = 0x2,
  HOST_EXEMODE_TRPUT = 0x3,
} host_exe_mode;

//request cache line
typedef enum {
  HOSTEXE_CLS_1 = 0x0,
  HOSTEXE_CLS_2 = 0x1,
  HOSTEXE_CLS_4 = 0x2,
  HOSTEXE_CLS_8 = 0x3,
  HOSTEXE_CLS_16 = 0x4,
} hostexe_req_len;

//configures atomic transactions
typedef enum {
  // Bit 0 enables atomic function mode
  // Bit 1 selects 4 byte or 8 byte requests
  // Bits [3:2] select the function
  HOSTEXE_ATOMIC_OFF = 0,
  HOSTEXE_ATOMIC_FADD_4 = 0x1,
  HOSTEXE_ATOMIC_FADD_8 = 0x3,
  HOSTEXE_ATOMIC_SWAP_4 = 0x5,
  HOSTEXE_ATOMIC_SWAP_8 = 0x7,
  HOSTEXE_ATOMIC_CAS_4 = 0x9,
  HOSTEXE_ATOMIC_CAS_8 = 0xb,
} hostexe_atomic_func;

//configures data mover vs. power user encoding of requests
typedef enum {
  HOSTEXE_ENCODING_DEFAULT = 0x0,  // Use the RTL PU_MEM_REQ parameter
  HOSTEXE_ENCODING_DM = 0x1,       // Only data mover
  HOSTEXE_ENCODING_PU = 0x2,       // Only power user
  HOSTEXE_ENCODING_RANDOM = 0x3,   // Both DM and PU in the same stream
} hostexe_encoding;

//he test type
typedef enum {
  HOSTEXE_TEST_ROLLOVER = 0x0,
  HOSTEXE_TEST_TERMINATION = 0x1,
} hostexe_test_mode;


// DFH Header
union he_dfh  {
  enum {
    offset = HE_DFH
  };
  uint64_t value;
  struct {
    uint16_t CcipVersionNumber : 12;
    uint8_t  AfuMajVersion : 4;
    uint32_t NextDfhOffset : 24;
    uint8_t  EOL : 1;
    uint32_t Reserved : 19;
    uint8_t  FeatureType : 4;
  };
};


// DSM BASEL
union he_dsm_basel {
  enum {
    offset = HE_DSM_BASEL
  };
  uint32_t value;
  struct {
    uint32_t DsmBaseL : 32;
  };
};

// DSM BASEH
union he_dsm_baseh {
  enum {
    offset = HE_DSM_BASEH
  };
  uint32_t value;
  struct {
    uint32_t DsmBaseH : 32;
  };
};

// NUM_LINES
union he_num_lines {
  enum {
    offset = HE_NUM_LINES
  };
  uint32_t value;
  struct {
    uint32_t NumCacheLines : 32;
    uint32_t Reserved : 32;
  };
};


// CSR CTL
union he_ctl{
  enum {
    offset = HE_CTL
  };
  uint32_t value;
  struct {
    uint32_t ResetL : 1;
    uint32_t Start : 1;
    uint32_t ForcedTestCmpl : 1;
    uint32_t Reserved : 29;
  };
};


// CSR CFG
union he_cfg {
  enum {
    offset = HE_CFG
  };
  uint64_t value;
  struct {
    uint64_t DelayEn : 1;
    uint64_t Continuous : 1;
    uint64_t TestMode : 3;
    uint64_t ReqLen : 2;
    uint64_t AtomicFunc : 5;
    uint64_t Encoding : 2;
    uint64_t Rsvd_19_14 : 6;
    uint64_t TputInterleave : 3;
    uint64_t TestCfg : 5;
    uint64_t IntrOnErr : 1;
    uint64_t IntrTestMode : 1;
    uint64_t ReqLen_High : 2;
    uint64_t Rsvd_63_32 : 32;
  };
};

// CSR INACT THRESH
union he_inact_thresh {
  enum {
    offset = HE_INACT_THRESH
  };
  uint32_t value;
  struct {
    uint32_t InactivtyThreshold : 32;
  };
};

// INTERRUPT0
union he_interrupt0 {
  enum {
    offset = HE_INTERRUPT0
  };
  uint32_t value;
  struct {
    uint32_t apci_id : 16;
    uint32_t VectorNum : 16;
  };
};

// SWTEST MSG
union he_swtest_msg {
  enum {
    offset = HE_SWTEST_MSG
  };
  uint64_t value;
  struct {
    uint64_t swtest_msg : 64;
  };
};

// STATUS0
union he_status0 {
  enum {
    offset = HE_STATUS0
  };
  uint64_t value;
  struct {
    uint64_t numWrites : 32;
    uint64_t numReads : 32;
  };
};

// STATUS1
union he_status1 {
  enum {
    offset = HE_STATUS1
  };
  uint64_t value;
  struct {
    uint64_t numPendWrites : 16;
    uint64_t numPendReads : 16;
    uint64_t numPendEmifWrites : 16;
    uint64_t numPendEmifReads : 16;
  };
};


// ERROR
union he_error {
  enum {
    offset = HE_ERROR
  };
  uint64_t value;
  struct {
    uint64_t error : 32;
    uint64_t Rsvd : 32;
  };
};

// STRIDE
union he_stride {
  enum {
    offset = HE_STRIDE
  };
  uint32_t value;
  struct {
    uint32_t Stride : 32;
  };
};

// HE DSM status
struct he_dsm_status {
	uint64_t test_completed : 1;
	uint64_t dsm_number : 15;
	uint64_t res1 : 16;
	uint64_t err_vector : 32;
	uint64_t num_ticks : 40;
	uint64_t res2 : 8;
	uint64_t num_reads_h : 8;
	uint64_t num_writes_h : 8;
	uint64_t num_reads_l : 32;
	uint64_t num_writes_l : 32;
	uint64_t penalty_start : 16;
	uint64_t res3 : 16;
	uint64_t penalty_end : 8;
	uint64_t res4 : 24;
	uint64_t ab_error_info : 32;
	uint32_t res5[7];
};

const std::map<std::string, uint32_t> he_modes = {
  { "lpbk", HOST_EXEMODE_LPBK1},
  { "read", HOST_EXEMODE_READ},
  { "write", HOST_EXEMODE_WRITE},
  { "trput", HOST_EXEMODE_TRPUT},
};

struct MapKeyComparator
{
  bool operator()( const std::string& a, const std::string& b ) const
  {
    if (a.length() != b.length())
      return (a.length() < b.length());
    else
      return (a < b);
  }
};

const std::map<std::string, uint32_t, MapKeyComparator> he_req_cls_len = {
  { "cl_1", HOSTEXE_CLS_1},
  { "cl_2", HOSTEXE_CLS_2},
  { "cl_4", HOSTEXE_CLS_4},
  { "cl_8", HOSTEXE_CLS_8},
  { "cl_16", HOSTEXE_CLS_16},
};

const std::map<std::string, uint32_t> he_req_atomic_func = {
  { "off", HOSTEXE_ATOMIC_OFF},
  { "fadd_4", HOSTEXE_ATOMIC_FADD_4},
  { "fadd_8", HOSTEXE_ATOMIC_FADD_8},
  { "swap_4", HOSTEXE_ATOMIC_SWAP_4},
  { "swap_8", HOSTEXE_ATOMIC_SWAP_8},
  { "cas_4", HOSTEXE_ATOMIC_CAS_4},
  { "cas_8", HOSTEXE_ATOMIC_CAS_8},
};

const std::map<std::string, uint32_t> he_req_encoding = {
  { "default", HOSTEXE_ENCODING_DEFAULT},
  { "dm", HOSTEXE_ENCODING_DM},
  { "pu", HOSTEXE_ENCODING_PU},
  { "random", HOSTEXE_ENCODING_RANDOM},
};

const std::map<std::string, uint32_t> he_test_mode = {
  { "test_rollover", HOSTEXE_TEST_ROLLOVER},
  { "test_termination", HOSTEXE_TEST_TERMINATION}
};


using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

// Inerleave help
const char *interleave_help = R"desc(Interleave requests pattern to use in throughput mode {0, 1, 2}
indicating one of the following series of read/write requests:
0: rd-wr-rd-wr
1: rd-rd-wr-wr
2: rd-rd-rd-rd-wr-wr-wr-wr)desc";


class host_exerciser : public test_afu {
public:
    host_exerciser()
  : test_afu("host_exerciser", nullptr, "warning")
  , count_(1)
  , he_interleave_(0)
  , he_interrupt_(0xffff)
  {
    // Mode
    app_.add_option("-m,--mode", he_modes_, "host exerciser mode {lpbk,read, write, trput}")
      ->transform(CLI::CheckedTransformer(he_modes))->default_val("lpbk");

    // Cache line
    app_.add_option("--cls", he_req_cls_len_, "number of CLs per request{cl_1, cl_2, cl_4, cl_8, cl_16}")
       ->transform(CLI::CheckedTransformer(he_req_cls_len))->default_val("cl_1");

    // Configures test rollover or test termination
    app_.add_option("--continuousmode", he_continuousmode_, "test rollover or test termination")->default_val("false");

    // Atomic function
    app_.add_option("--atomic", he_req_atomic_func_, "atomic requests (only permitted in combination with lpbk/cl_1)")
      ->transform(CLI::CheckedTransformer(he_req_atomic_func))->default_val("off");

    // Encoding
    app_.add_option("--encoding", he_req_encoding_, "data mover or power user encoding -- random interleaves both in the same stream")
      ->transform(CLI::CheckedTransformer(he_req_encoding))->default_val("default");

    // Delay
    app_.add_option("-d,--delay", he_delay_, "Enables random delay insertion between requests")->default_val("false");

    // Configure interleave requests in Throughput mode
    app_.add_option("--interleave", he_interleave_, interleave_help)->transform(CLI::Range(0, 2));

    // The Interrupt Vector Number for the device
    app_.add_option("--interrupt", he_interrupt_,
        "The Interrupt Vector Number for the device")
        ->transform(CLI::Range(0, 4095));

     // Continuous mode time
    app_.add_option("--contmodetime", he_contmodetime_,
        "Continuous mode time in seconds")->default_val("1");

    // Test all
    app_.add_option("--testall", he_test_all_, "Run all tests")->default_val("false");

    app_.add_option("--clock-mhz", he_clock_mhz_,
        "Clock frequency (MHz) -- when zero, read the frequency from the AFU")->default_val("0");
   }

  virtual int run(CLI::App *app, test_command::ptr_t test) override
  {
    int res = exit_codes::not_run;

    logger_->set_pattern("    %v");
    // Info prints details of an individual run. Turn it on if doing only one test
    // and the user hasn't changed level from the default.
    if ((log_level_.compare("warning") == 0) && !he_test_all_)
        logger_->set_level(spdlog::level::info);

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

  template<typename T>
  inline T read(uint32_t offset) const {
    return *reinterpret_cast<T*>(handle_->mmio_ptr(offset));
  }

  template<typename T>
  inline void write(uint32_t offset, T value) const {
    *reinterpret_cast<T*>(handle_->mmio_ptr(offset)) = value;
  }

  template<typename T>
  T read(uint32_t offset, uint32_t i) const {
    return read<T>(get_offset(offset, i));
  }

  template<typename T>
  void write(uint32_t offset, uint32_t i, T value) const {
    write<T>(get_offset(offset, i), value);
  }

  shared_buffer::ptr_t allocate(size_t size)
  {
    return shared_buffer::allocate(handle_, size);
  }

  void fill(shared_buffer::ptr_t buffer)
  {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(1, -1);
    auto sz = sizeof(uint32_t);
    for (uint32_t i = 0; i < buffer->size(); i+=sz){
      buffer->write<uint32_t>(dist(mt), i);
    }

  }

  void fill(shared_buffer::ptr_t buffer, uint32_t value)
  {
    buffer->fill(value);
  }

  event::ptr_t register_interrupt(uint32_t vector)
  {
    auto event = event::register_event(handle_, FPGA_EVENT_INTERRUPT, vector);
    return event;
  }

  void interrupt_wait(event::ptr_t event, int timeout=-1)
  {
    struct pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = event->os_object();
    auto ret = poll(&pfd, 1, timeout);

    if (ret < 0)
      throw std::runtime_error(strerror(errno));
    if (ret == 0)
      throw std::runtime_error("timeout error");
  }

  void compare(shared_buffer::ptr_t b1,
               shared_buffer::ptr_t b2, uint32_t count = 0)
  {
      if (b1->compare(b2, count ? count : b1->size())) {
        throw std::runtime_error("buffers mismatch");
      }
  }

  template<class T>
  T read_register()
  {
    return *reinterpret_cast<T*>(handle_->mmio_ptr(T::offset));
  }

  template<class T>
  volatile T* register_ptr(uint32_t offset)
  {
    return reinterpret_cast<volatile T*>(handle_->mmio_ptr(offset));
  }

  template<class T>
  void write_register(uint32_t offset, T* reg)
  {
    *reinterpret_cast<T*>(handle_->mmio_ptr(offset)) = *reg;
  }

public:
  uint32_t count_;
  uint32_t he_modes_;
  uint32_t he_req_cls_len_;
  uint32_t he_req_atomic_func_;
  uint32_t he_req_encoding_;
  bool he_delay_;
  bool he_continuousmode_;
  bool he_test_all_;
  uint32_t he_interleave_;
  uint32_t he_interrupt_;
  uint32_t he_contmodetime_;
  uint32_t he_clock_mhz_;

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

  token::ptr_t get_token_device()
  {
    if (handle_device_)
        return handle_device_->get_token();
    return nullptr;
  }

  bool option_passed(std::string option_str)
  {
      if (app_.count(option_str) == 0)
            return false;
      return true;
  }
};
} // end of namespace host_exerciser

