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

#ifdef __cplusplus

extern "C" {
#endif
#include <opae/enum.h>
#include <opae/properties.h>
#undef  _GNU_SOURCE
#include "usrclk/user_clk_pgm_uclock.h"

#ifdef __cplusplus
}
#endif


#include "gtest/gtest.h"
#include "types_int.h"
#include "test_system.h"
#include "xfpga.h"

using namespace opae::testing;

class usrclk_c
    : public ::testing::TestWithParam<std::string> {
 protected:
  usrclk_c()
  : handle_dev_(nullptr),
    handle_accel_(nullptr),
    tokens_dev_{{nullptr, nullptr}},
    tokens_accel_{{nullptr, nullptr}},
    filter_dev_(nullptr),
    filter_accel_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_dev_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_dev_,
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_dev_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_dev_, 1, tokens_dev_.data(),
              tokens_dev_.size(), &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_accel_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_accel_,
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_accel_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_accel_, 1, tokens_accel_.data(),
              tokens_accel_.size(), &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_dev_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyProperties(&filter_accel_), FPGA_OK);

    for (auto &t : tokens_dev_) {
      if (t) {
        EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&t));
        t = nullptr;
      }
    }

    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&t));
        t = nullptr;
      }
    }

    if (handle_dev_ != nullptr) { EXPECT_EQ(xfpga_fpgaClose(handle_dev_), FPGA_OK); }
    if (handle_accel_ != nullptr) { EXPECT_EQ(xfpga_fpgaClose(handle_accel_), FPGA_OK); }
    system_->finalize();
  }

  fpga_handle handle_dev_;
  fpga_handle handle_accel_;
  std::array<fpga_token, 2> tokens_dev_;
  std::array<fpga_token, 2> tokens_accel_;
  fpga_properties filter_dev_;
  fpga_properties filter_accel_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
* @test    afu_usrclk_01
* @brief   Tests: fpac_GetErrMsg and fv_BugLog
* @details fpac_GetErrMsg returns error string
*          fv_BugLog sets bug log
*/
TEST(usrclk_c, afu_usrclk_01) {
  //Get error string
  const char * pmsg = fpac_GetErrMsg(1);
  EXPECT_EQ(NULL, !pmsg);

  //Get error string
  pmsg = fpac_GetErrMsg(5);
  EXPECT_EQ(NULL, !pmsg);

  //Get error string
  pmsg = fpac_GetErrMsg(16);
  EXPECT_EQ(NULL, !pmsg);

  //Get error string for invlaid index
  pmsg = NULL;
  pmsg = fpac_GetErrMsg(17);
  EXPECT_STREQ("ERROR: MSG INDEX OUT OF RANGE", pmsg);

  //Get error string for invlaid index
  pmsg = NULL;
  pmsg = fpac_GetErrMsg(-1);
  EXPECT_STREQ("ERROR: MSG INDEX OUT OF RANGE", pmsg);

  fv_BugLog(1);

  fv_BugLog(2);

}

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
* @test    fi_run_initz
* @brief   Tests: fi_RunInitz
* @details When the sysfs path is NULL, fi_RunInitz
*          returns -1.
*/
TEST(usrclk_c, fi_run_initz) {
  EXPECT_EQ(-1, fi_RunInitz(NULL));
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

  // Invalid object type
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_dev_[0], &handle_dev_, 0));

  // Invalid clk
  result = xfpga_fpgaSetUserClock(handle_dev_, 0, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid clk
  result = xfpga_fpgaSetUserClock(handle_dev_, 312, 156, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid object type
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, 0));

  // Invalid clk
  result = xfpga_fpgaSetUserClock(handle_accel_, 0, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  struct _fpga_handle  *_handle = (struct _fpga_handle *)handle_accel_;
  int fddev = _handle->fddev;

  // Token not found
  _handle->token = NULL;
  result = xfpga_fpgaSetUserClock(handle_accel_, 312, 156, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid file handle descriptor
  _handle->fddev = -1;
  result = xfpga_fpgaSetUserClock(handle_accel_, 312, 156, flags);
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

  // Invalid object type
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_dev_[0], &handle_dev_, 0));

  // Valid params, invalid object type
  result = xfpga_fpgaGetUserClock(handle_dev_, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid object type
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, 0));

  struct _fpga_handle  *_handle = (struct _fpga_handle *)handle_accel_;
  int fddev = _handle->fddev;

  // Token not found
  _handle->token = NULL;
  result = xfpga_fpgaGetUserClock(handle_accel_, &high, &low, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid file handle descriptor
  _handle->fddev = -1;
  result = xfpga_fpgaGetUserClock(handle_accel_, &high, &low, flags);
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
  ASSERT_EQ(xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, flags),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaGetUserClock(handle_accel_, &high, &low, flags),
            FPGA_OK);
  EXPECT_NE(high, 999);
  EXPECT_NE(low, 999);
}

INSTANTIATE_TEST_CASE_P(usrclk, usrclk_c,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p", "dcp-rc"})));

class usrclk_mock_c : public usrclk_c {};

/**
 * @test    set_user_clock
 * @brief   Tests: xfpga_fpgaSetUserClock()
 * @details When the parameters are valid, fpgaGetUserClock returns
 *          FPGA_NOT_SUPPORTED on mock platforms.
 */
TEST_P(usrclk_mock_c, set_user_clock) {
  uint64_t high = 312;
  uint64_t low = 156;
  int flags = 0;
  ASSERT_EQ(xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, flags),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaSetUserClock(handle_accel_, high, low, flags),
            FPGA_NOT_SUPPORTED);
}

INSTANTIATE_TEST_CASE_P(usrclk, usrclk_mock_c,
                        ::testing::ValuesIn(test_platform::mock_platforms()));

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
  ASSERT_EQ(xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, flags),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaSetUserClock(handle_accel_, high, low, flags),
            FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(usrclk, usrclk_hw_c,
                        ::testing::ValuesIn(test_platform::hw_platforms({"skx-p", "dcp-rc"})));
