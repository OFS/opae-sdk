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
#include <chrono>
#include <deque>
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;

template<class T>
int check_stat(std::shared_ptr<spdlog::logger> logger, const char *name, T stat)
{
  bool pass = stat->TrafficGenTestPass,
       fail = stat->TrafficGenTestFail,
       timeout = stat->TrafficGenTestTimeout;
  logger->debug("[{0}]: PASS={1}, FAIL={2}, TIMEOUT={3}", name, pass, fail, timeout);
  return stat->value & 0b110;
}

class ddr_test : public test_command
{
public:
  ddr_test(){}
  virtual ~ddr_test(){}
  virtual const char *name() const
  {
    return "ddr";
  }

  virtual const char *description() const
  {
    return "run ddr test";
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)app;
    using namespace std::chrono;
    auto bank0 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank0_stat::offset);
    auto bank1 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank1_stat::offset);
    auto bank2 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank2_stat::offset);
    auto bank3 = afu->register_ptr<uint64_t>(dummy_afu::ddr_test_bank3_stat::offset);
    std::deque<volatile uint64_t*> ptrs;
    ptrs.push_back(bank0);
    ptrs.push_back(bank1);
    ptrs.push_back(bank2);
    ptrs.push_back(bank3);
    for (auto p : ptrs)
      if (*p & 0b111)
        throw std::runtime_error("banks not zero");

    // start the test
    afu->write64(dummy_afu::DDR_TEST_CTRL, 0x00000000);
    afu->write64(dummy_afu::DDR_TEST_CTRL, 0x0000000f);

    auto begin = high_resolution_clock::now();
    while (!ptrs.empty()) {
      if (*ptrs.front() & 0b111) {
        ptrs.pop_front();
      }
      auto now = high_resolution_clock::now();
      if (duration_cast<duration<double>>(now - begin).count() > 1.0) {
        throw std::runtime_error("timeout error");
      }
    }
    int res = 0;
    auto logger = spdlog::get(this->name());
    res += check_stat(logger, "Bank0",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank0_stat*>(bank0));
    res += check_stat(logger, "Bank1",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank1_stat*>(bank1));
    res += check_stat(logger, "Bank2",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank2_stat*>(bank2));
    res += check_stat(logger, "Bank3",
      reinterpret_cast<volatile dummy_afu::ddr_test_bank3_stat*>(bank3));
    return res;
  }
};

