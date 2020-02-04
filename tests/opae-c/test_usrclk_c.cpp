// Copyright(c) 2018, Intel Corporation
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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "opae_int.h"

}

#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class usrclk_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  usrclk_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_,
                                        platform_.devices[0].device_id), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle accel_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       get
 * @brief      Test: fpgaGetUserClock
 * @details    When fpgaGetUserClock is called with valid parameters,<br>
 *             then it retrieves the user clock values,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(usrclk_c_p, get) {
  uint64_t low = 999;
  uint64_t high = 999;
  EXPECT_EQ(fpgaGetUserClock(accel_, &high, &low, 0), FPGA_OK);
  EXPECT_NE(low, 999);
  EXPECT_NE(high, 999);
}

// TODO: Fix user clock test for DCP
INSTANTIATE_TEST_CASE_P(usrclk_c, usrclk_c_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));

class usrclk_c_hw_p : public usrclk_c_p{
  protected:
    usrclk_c_hw_p() {};
};
/**
 * @test       set
 * @brief      Test: fpgaSetUserClock
 * @details    When fpgaSetUserClock is called with valid parameters,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(usrclk_c_hw_p, set) {
  uint64_t low = 25;
  uint64_t high = 600;
  EXPECT_EQ(fpgaSetUserClock(accel_, high, low, 0), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(usrclk_c, usrclk_c_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({ "skx-p","dcp-rc" }, fpga_driver::linux_intel)));

