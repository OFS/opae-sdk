// Copyright(c) 2018-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#define NO_OPAE_C
#include "mock/opae_fixtures.h"
KEEP_XFPGA_SYMBOLS

extern "C" {
#undef  _GNU_SOURCE
#include "usrclk/fpga_user_clk.c"
#include "types_int.h"
#include "xfpga.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class usrclk_c : public opae_p<xfpga_> {
 protected:
  usrclk_c() :
    device_(nullptr)
  {}

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual void SetUp() override {
    opae_p<xfpga_>::SetUp();

    ASSERT_EQ(xfpga_fpgaOpen(device_token_, &device_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaClose(device_), FPGA_OK);

    opae_p<xfpga_>::TearDown();
  }

  fpga_handle device_;
};

/**
* @test    set_user_clock
* @brief   Tests: set_userclock
* @details When the sysfs path is NULL, set_userclock
*          returns FPGA_INVALID_PARAM.
*/
TEST(usrclk_c, set_userclock_null) {
  fpga_result result;

  // Null handle
  result = set_userclock(NULL, 0, 0);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

/**
* @test    get_user_clock
* @brief   Tests: get_userclock
* @details When the sysfs path is NULL, get_userclock
*          returns FPGA_INVALID_PARAM.
*/
TEST(usrclk_c, get_userclock_null) {
  fpga_result result;

  // Null handle
  result = get_userclock(NULL, 0, 0);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

/**
* @test    fpga_set_user_clock
* @brief   Tests: fpgaSetUserClock
* @details fpgaSetUserClock
*/
TEST_P(usrclk_c, set_user_clock_neg) {
  fpga_result result;
  int flags = 0;

  // Null handle
  result = xfpga_fpgaSetUserClock(NULL, 0, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid clk
  result = xfpga_fpgaSetUserClock(device_, 0, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid clk
  result = xfpga_fpgaSetUserClock(device_, 312, 156, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid clk
  result = xfpga_fpgaSetUserClock(accel_, 0, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  struct _fpga_handle  *_handle = (struct _fpga_handle *)accel_;
  int fddev = _handle->fddev;

  // Token not found
  _handle->token = NULL;
  result = xfpga_fpgaSetUserClock(accel_, 312, 156, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid file handle descriptor
  _handle->fddev = -1;
  result = xfpga_fpgaSetUserClock(accel_, 312, 156, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  _handle->fddev = fddev;
}

/**
* @test    fpga_get_user_clock
* @brief   Tests: fpgaGetUserClock
* @details fpgaGetUserClock
*/
TEST_P(usrclk_c, get_user_clock_neg) {
  fpga_result result;
  uint64_t high;
  uint64_t low;
  int flags = 0;

  // Null handle
  result = xfpga_fpgaGetUserClock(NULL, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid params, invalid object type
  result = xfpga_fpgaGetUserClock(device_, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid object type
  struct _fpga_handle  *_handle = (struct _fpga_handle *)accel_;
  int fddev = _handle->fddev;

  // Token not found
  _handle->token = NULL;
  result = xfpga_fpgaGetUserClock(accel_, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid file handle descriptor
  _handle->fddev = -1;
  result = xfpga_fpgaGetUserClock(accel_, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  _handle->fddev = fddev;
}

/**
 * @test    get_user_clock
 * @brief   Tests: xfpga_fpgaGetUserClock()
 * @details When the parameters are valid, fpgaGetUserClock returns
 *          FPGA_OK.
 */
TEST_P(usrclk_c, get_user_clock) {
  uint64_t high = 999;
  uint64_t low = 999;
  int flags = 0;
  EXPECT_NE(xfpga_fpgaGetUserClock(accel_, &high, &low, flags),
            FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(usrclk_c);
INSTANTIATE_TEST_SUITE_P(usrclk, usrclk_c,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000"
                                                                      })));

class usrclk_mock_c : public usrclk_c {};

/**
 * @test    set_user_clock
 * @brief   Tests: xfpga_fpgaSetUserClock()
 * @details When the parameters are valid, fpgaGetUserClock returns
 *          FPGA_NOT_FOUND on mock platforms.
 */
TEST_P(usrclk_mock_c, set_user_clock) {
  uint64_t high = 312;
  uint64_t low = 156;
  int flags = 0;
  EXPECT_EQ(xfpga_fpgaSetUserClock(accel_, high, low, flags),
            FPGA_NOT_FOUND);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(usrclk_mock_c);
INSTANTIATE_TEST_SUITE_P(usrclk, usrclk_mock_c,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000"
                                                                           })));

class usrclk_hw_c : public usrclk_c {};

/**
 * @test    set_user_clock
 * @brief   Tests: xfpga_fpgaSetUserClock()
 * @details When the parameters are valid, fpgaGetUserClock returns
 *          FPGA_OK.
 */
TEST_P(usrclk_hw_c, set_user_clock) {
uint64_t high = 312;
  uint64_t low = 156;
  int flags = 0;
  EXPECT_EQ(xfpga_fpgaSetUserClock(accel_, high, low, flags),
            FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(usrclk_hw_c);
INSTANTIATE_TEST_SUITE_P(usrclk, usrclk_hw_c,
                         ::testing::ValuesIn(test_platform::hw_platforms({
                                                                           "skx-p",
                                                                           "dcp-rc"
                                                                         })));
