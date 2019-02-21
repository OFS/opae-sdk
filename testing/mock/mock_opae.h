// Copyright(c) 2017-2019, Intel Corporation
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
/*
 * mock_opae.h
 */
#pragma once



#include <opae/fpga.h>
#include <dlfcn.h>
#include "gtest/gtest.h"
#include "test_system.h"

namespace opae {
namespace testing {

extern const char xfpga_[] = "xfpga_";
extern const char none_[] = "";

template<int _T = 2, const char *_P = none_>
class mock_opae_p : public ::testing::TestWithParam<std::string> {
 protected:
  mock_opae_p(): tokens_{ {} }, plugin_prefix_(_P) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();
    test_setup();
  }

  virtual void DestroyTokens() {
    std::string fn_name = plugin_prefix_ + "fpgaDestroyToken";
    auto fn = reinterpret_cast<fpga_result (*)(fpga_token *)>(
        dlsym(nullptr, fn_name.c_str()));
    ASSERT_NE(fn, nullptr);
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fn(&t), FPGA_OK);
        t = nullptr;
      }
    }
  }

  virtual void TearDown() override {
    DestroyTokens();
    test_teardown();
    system_->finalize();
  }

  virtual void test_setup() {
  }

  virtual void test_teardown() {
  }

  std::array<fpga_token, _T> tokens_;
  test_platform platform_;
  test_system *system_;
  test_device invalid_device_;
  std::string plugin_prefix_;
};

} // end of namespace testing
} // end of namespace opae
