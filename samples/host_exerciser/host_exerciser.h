// Copyright(c) 2020, Intel Corporation
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

#include "afu_test.h"

const char *AFU_ID = "91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73";

namespace host_exerciser {
using opae::fpga::types::event;
using opae::fpga::types::shared_buffer;

static const uint64_t HELPBK_TEST_TIMEOUT = 30000;
static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t LOG2_CL = 6;
static const size_t LPBK1_DSM_SIZE = 2 * MB;
static const size_t LPBK1_BUFFER_SIZE = 1 * MB;
static const size_t LPBK1_BUFFER_ALLOCATION_SIZE = 2 * MB;
static const uint32_t DSM_STATUS_TEST_COMPLETE = 0x40;

// Host execiser CSR Offset
enum {
  DFH_HEADER	= 0x0000,
  DFH_CAPABILITY_HEADER = 0x0008,
  DFH_CAPABILITY_GUID = 0x0010,
  DFH_CAPABILITY_INT = 0x0018,
  DSM_STATUS = 0x0040,
  CSR_SCRATCHPAD0 = 0x0100,
  CSR_SCRATCHPAD1 = 0x0108,
  CSR_AFU_DSM_BASEL = 0x0110,
  CSR_AFU_DSM_BASEH = 0x0114,
  CSR_SRC_ADDR = 0x0120,
  CSR_DST_ADDR = 0x0128,
  CSR_NUM_LINES = 0x0130,
  CSR_CTL = 0x0138,
  CSR_CFG = 0x0138,
  CSR_INACT_THRESH = 0x0148,
  CSR_INTERRUPT0 = 0x0150,
  CSR_SWTEST_MSG = 0x0158,
  CSR_STATUS0 = 0x0160,
  CSR_STATUS1 = 0x0168,
  CSR_ERROR = 0x0170,
  CSR_ERROR1 = 0x0178
};

//configures test mode
typedef enum {
	HOST_EXEMODE_LPBK1= 0x0,
	HOST_EXEMODE_READ = 0x1,
	HOST_EXEMODE_WRITE= 0x2,
	HOST_EXEMODE_TRUPT = 0x3,
} host_exe_mode;

//read request type
typedef enum {
	HOSTEXE_RDLINE_S = 0x0,
	HOSTEXE_RDLINE_L = 0x1,
	HOSTEXE_RDLINE_MIX = 0x2,
} hostexe_rd;

//read request type
typedef enum {
	HOSTEXE_WRLINE_M = 0x0,
	HOSTEXE_WRLINE_L = 0x1,
} hostexe_wr;

// DFH Header
union dfh_header  {
  enum {
    offset = DFH_HEADER
  };

  dfh_header(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint16_t FeatureID : 12;
    uint8_t FeatureRev : 4;
    uint32_t NextDfhByteOffset : 24;
	uint8_t EOL : 1;
	uint8_t Reserved41 : 7;
	uint8_t AFUminor : 4;
	uint8_t DFHversion : 8;
	uint8_t FeatureType : 4;
  };
};

// DFH CAPABILITY Header
union dfh_cap_hdr {
	enum {
		offset = DFH_CAPABILITY_HEADER
	};
	dfh_cap_hdr(uint32_t v) : value(v) {}
	uint32_t value;
	struct {
		uint16_t CapID : 16;
		uint8_t CapIDVersion : 4;
		uint16_t NextCapOffset : 12;
	};
};

// TBD
union dfh_cap_guid {
	enum {
		offset = DFH_CAPABILITY_GUID
	};

	dfh_cap_guid(uint8_t* v)
	{
		for (int i = 0; i < 24; i++) {
			memcpy(&value[i], v, 1);
			v++;
		}
	}
	uint8_t  value[24];
	struct {
		uint64_t  GuidHi;
		uint64_t GuidLo;
		uint32_t Reserved;
		uint32_t dfhCapHeader;
	};
};


union dfh_cap_init {
	enum {
		offset = DFH_CAPABILITY_INT
	};

	dfh_cap_init(uint8_t* v)
	{
		for (int i = 0; i < 20; i++) {
			memcpy(&value[i], v, 1);
			v++;
		}
	}
	uint8_t  value[20];
	struct {
		uint32_t IntVectorLength ;
		uint32_t IntVectorStart;
		uint32_t IntControl ;
		uint32_t IntFlags ;
		uint32_t DfhCapHeader;
	};
};



union csr_afu_dsm_basel {
	enum {
		offset = CSR_AFU_DSM_BASEL
	};
	csr_afu_dsm_basel(uint32_t v) : value(v) {}
	uint32_t value;
	struct {
		uint32_t DsmBaseL : 32;
	};
};

union csr_afu_dsm_baseh {
	enum {
		offset = CSR_AFU_DSM_BASEH
	};
	csr_afu_dsm_baseh(uint32_t v) : value(v) {}
	uint32_t value;
	struct {
		uint32_t DsmBaseH : 32;
	};
};

union csr_ctl{
	enum {
		offset = CSR_CTL
	};

	csr_ctl(uint32_t v) : value(v) {}
	uint32_t value;
	struct {
		uint32_t Reserved4 : 29;
		uint8_t ForcedTestCmpl : 1;
		uint8_t Start : 1;
		uint8_t ResetL : 1;
	};
};

union csr_cfg {
	enum {
		offset = CSR_CFG
	};

	csr_cfg(uint32_t v) : value(v) {}
	uint32_t value;
	struct {

		uint32_t RsvdZ1 : 2;
		uint32_t cr_interrupt_testmode : 1;
		uint32_t cr_interrupt_on_error : 1;
		uint32_t cr_test_cfg : 8;
		uint32_t RsvdZ2 : 1;
		uint32_t cr_chsel : 2;
		uint32_t cr_rdsel : 2;
		uint32_t cr_delay_en : 1;

		uint32_t cr_multiCL_len : 2;
		uint32_t cr_mode : 3;
		uint32_t c_cont : 1;
		uint32_t cr_wrthru_en : 1;
	};
};


union csr_inact_thresh {
	enum {
		offset = CSR_INACT_THRESH
	};

	csr_inact_thresh(uint32_t v) : value(v) {}
	uint32_t value;
	struct {

	uint32_t csr_inact_thresh_value :32;
	};
};

union csr_interrupt0 {
	enum {
		offset = CSR_INTERRUPT0
	};

	csr_interrupt0(uint32_t v) : value(v) {}
	uint32_t value;
	struct {

	uint32_t VectorNUm : 16;
	uint32_t apci_id : 16;
	};
};


union csr_swtest_msg {
	enum {
		offset = CSR_SWTEST_MSG
	};
	csr_swtest_msg(uint64_t v) : value(v) {}
	uint64_t value;
	struct {
		uint64_t swtest_msg : 64;
	};
};


union csr_status0 {
	enum {
		offset = CSR_STATUS0
	};
	csr_status0(uint64_t v) : value(v) {}
	uint64_t value;
	struct {
		uint32_t numReads : 32;
		uint32_t numWrites : 32;
	};
};


union csr_status1 {
	enum {
		offset = CSR_STATUS1
	};
	csr_status1(uint64_t v) : value(v) {}
	uint64_t value;
	struct {
		uint32_t numPendReads : 32;
		uint32_t numPendWrites : 32;
	};
};


union csr_error {
	enum {
		offset = CSR_ERROR
	};
	csr_error(uint64_t v) : value(v) {}
	uint64_t value;
	struct {
		uint32_t RsvdZ1 : 32;
		uint32_t error : 32;
	};
};


union csr_error1 {
	enum {
		offset = CSR_ERROR1
	};
	csr_error1(uint64_t v) : value(v) {}
	uint64_t value;
	struct {
		uint64_t error : 32;
	};
};

using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

class host_exerciser : public test_afu {
public:
	host_exerciser(const char *afu_id = AFU_ID)
  : test_afu("host_exerciser", afu_id)
  , count_(1)
  {
    app_.add_option("-m,--mode", mode_str_, "mode {lpbk,read, write, trput}")->default_val("lpbk");
    app_.add_option("-r,--read", read_str_, "read request type{rdline_s, rdline_l,mixed}")->default_val("rdline_s");
    app_.add_option("-w,--write", write_str_, "write request type {wrline_m, wrline_l}")->default_val("wrline_m");
    app_.add_option("-d,--delay", delay_, "Enables random delay insertion between requests")->default_val(false);
    app_.add_option("--multi-cl", multi_cl_, "multi Cache lineone of {0, 1, 3}")->default_val(0);
    app_.add_option("--ccont", c_cont_, "Configures test rollover or test termination")->default_val(false);
  }


  virtual int run(CLI::App *app, test_command::ptr_t test) override
  {
    int res = exit_codes::not_run;
    logger_->set_level(spdlog::level::trace);
    logger_->info("starting test run, count of {0:d}", count_);
    uint32_t count = 0;
    try {
      while (count < count_) {
        logger_->debug("starting iteration: {0:d}", count+1);
        handle_->reset();
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
    handle_->reset();
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
    std::uniform_int_distribution<uint32_t> dist(1, 4096);
    auto sz = sizeof(uint32_t);
    for (uint32_t i = 0; i < buffer->size()/sz; i+=sz){
      buffer->write<uint32_t>(dist(mt), i);
    }

  }

  void fill(shared_buffer::ptr_t buffer, uint32_t value)
  {
    buffer->fill(value);
  }

  event::ptr_t register_interrupt()
  {
    auto event = event::register_event(handle_, FPGA_EVENT_INTERRUPT);
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
  uint32_t cacheline_;
  std::string mode_str_;
  std::string read_str_;
  std::string write_str_;
  uint32_t mode_;
  uint32_t read_;
  uint32_t write_;
  bool delay_;
  uint32_t multi_cl_;
  bool c_cont_;
  uint32_t thresh_limit_;

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

};
} // end of namespace host_exerciser

