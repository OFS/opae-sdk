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

namespace dummy_afu {
using opae::fpga::types::event;
using opae::fpga::types::shared_buffer;

enum {
  AFU_DFH = 0x0000,
  AFU_ID_L = 0x0008,
  AFU_ID_H = 0x0010,
  NEXT_AFU = 0x0018,
  AFU_DFH_RSVD = 0x0020,
  SCRATCHPAD = 0x0028,
  MMIO_TEST_SCRATCHPAD = 0x1000,
  MEM_TEST_CTRL = 0x2040,
  MEM_TEST_STAT = 0x2048,
  MEM_TEST_SRC_ADDR = 0x2050,
  MEM_TEST_DST_ADDR = 0x2058,
  DDR_TEST_CTRL = 0x3000,
  DDR_TEST_STAT = 0x3008,
  DDR_TEST_BANK0_STAT = 0x3010,
  DDR_TEST_BANK1_STAT = 0x3018,
  DDR_TEST_BANK2_STAT = 0x3020,
  DDR_TEST_BANK3_STAT = 0x3028,
};


union afu_dfh  {
  enum {
    offset = AFU_DFH
  };

  afu_dfh(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t FeatureID : 12;
    uint64_t FeatureRev : 4;
    uint64_t NextDfhByteOffset : 24;
    uint64_t EOL : 1;
    uint64_t Reserved41 : 19;
    uint64_t FeatureType : 4;
  };
};

union mem_test_ctrl  {
  enum {
    offset = MEM_TEST_CTRL
  };

  mem_test_ctrl(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t StartTest : 1;
    uint64_t Reserved1 : 63;
  };
};

union ddr_test_ctrl  {
  enum {
    offset = DDR_TEST_CTRL
  };

  ddr_test_ctrl(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t DDRBank0StartTest : 1;
    uint64_t DDRBank1StartTest : 1;
    uint64_t DDRBank2StartTest : 1;
    uint64_t DDRBank3StartTest : 1;
    uint64_t Reserved4 : 60;
  };
};

union ddr_test_stat  {
  enum {
    offset = DDR_TEST_STAT
  };

  ddr_test_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t NumDDRBank : 8;
    uint64_t Reserved8 : 56;
  };
};

union ddr_test_bank0_stat  {
  enum {
    offset = DDR_TEST_BANK0_STAT
  };

  ddr_test_bank0_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank1_stat  {
  enum {
    offset = DDR_TEST_BANK1_STAT
  };

  ddr_test_bank1_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank2_stat  {
  enum {
    offset = DDR_TEST_BANK2_STAT
  };

  ddr_test_bank2_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

union ddr_test_bank3_stat  {
  enum {
    offset = DDR_TEST_BANK3_STAT
  };

  ddr_test_bank3_stat(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t TrafficGenTestPass : 1;
    uint64_t TrafficGenTestFail : 1;
    uint64_t TrafficGenTestTimeout : 1;
    uint64_t TrafficGenFSMState : 4;
    uint64_t Reserved4 : 57;
  };
};

using test_afu = opae::afu_test::afu;
using test_command = opae::afu_test::command;

class dummy_afu : public test_afu {
public:
  dummy_afu(const char *afu_id = "91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73")
  : test_afu("dummy_afu", afu_id)
  , count_(1)
  {
    app_.add_option("-c,--count", count_, "Number of times to run test")->default_val(count_);
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

private:
  uint32_t count_;
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
} // end of namespace dummy_afu

