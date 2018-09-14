// Copyright(c) 2017-2018, Intel Corporation
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

#include "gtest/gtest.h"
#include "test_system.h"
#include "xfpga.h"
#include "types_int.h"

using namespace opae::testing;

class openclose_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  openclose_c_p() : tmpsysfs_("mocksys-XXXXXX"), handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  fpga_handle handle_;
  std::array<fpga_token, 2> tokens_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};



/**
 * @test       open_01
 *
 * @brief      When the fpga_handle * parameter to xfpga_fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_P(openclose_c_p, open_01) {
  fpga_result res;
  res = xfpga_fpgaOpen(NULL, NULL, 0);
  ASSERT_EQ(FPGA_INVALID_PARAM, res);
}

/**
 * @test       open_02
 *
 * @brief      When the fpga_token parameter to xfpga_fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */

TEST_P(openclose_c_p, open_02) {
  fpga_handle handle_;
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaOpen(NULL, &handle_, 0));
}


/**
 * @test       open_03
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST_P(openclose_c_p, open_03) {
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaOpen(tokens_[0], &handle_, 42));
}

/**
 * @test       open_04
 *
 * @brief      When the flags parameter to xfpga_fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM and FPGA_NO_DRIVER.
 *
 */
//TEST_P(openclose_c_p, open_04) {
//  fpga_result res;
//  struct _fpga_token* _token = (struct _fpga_token*)tokens_[0];
//
//
//  res = xfpga_fpgaOpen(tokens_[0], &handle_, 42);
//  ASSERT_EQ(FPGA_INVALID_PARAM, res);
//
//  //_token->magic = FPGA_TOKEN_MAGIC;
//  *tokens_[0].magic = FPGA_TOKEN_MAGIC;
//  res = xfpga_fpgaOpen(tokens_[0], &handle_, FPGA_OPEN_SHARED);
//  ASSERT_EQ(FPGA_OK, res);
//  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_));
//  
//
//  strcpy(_token->devpath,"/dev/intel-fpga-fme.01");
//  res = xfpga_fpgaOpen(tokens_[0], &handle_, FPGA_OPEN_SHARED);
//  ASSERT_EQ(FPGA_NO_DRIVER, res);
//
//}


INSTANTIATE_TEST_CASE_P(openclose_c, openclose_c_p, ::testing::ValuesIn(test_platform::keys(true)));
