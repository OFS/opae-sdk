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

#ifdef __cplusplus

extern "C" {
#endif

#include <opae/enum.h>
#include <opae/properties.h>
#include <opae/access.h>
extern fpga_result set_afu_userclock(fpga_handle handle,
				uint64_t usrlclock_high,
				uint64_t usrlclock_low);
//
//extern fpga_result set_fpga_pwr_threshold(fpga_handle handle,
//			uint64_t gbs_power);
#ifdef __cplusplus
}
#endif
#include "reconf_int.h"
#include "test_system.h" 
#include "gtest/gtest.h"
#include "types_int.h"

using namespace opae::testing;

class reconf_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  reconf_c_p() : tmpsysfs_("mocksys-XXXXXX"), handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    //ASSERT_EQ(fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};



/**
* @test    gbs_reconf_01
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Then the return value  FPGA_OK if set or
*..........Returns error code
*/
TEST_P(reconf_c_p, gbs_reconf_01) {
  uint64_t usrlclock_high = 0;
  uint64_t usrlclock_low = 0;
  
  //EXPECT_EQ(FPGA_INVALID_PARAM, set_afu_userclock(h, usrlclock_high, usrlclock_low));
  
  // Open port device
  ASSERT_EQ(FPGA_OK, fpgaOpen(tokens_[0], &handle_, 0));
  
  
  	
  EXPECT_EQ(FPGA_INVALID_PARAM, set_afu_userclock(handle_, usrlclock_high, usrlclock_low));
  
  usrlclock_high = 300;
  
  EXPECT_NE(FPGA_OK, set_afu_userclock(handle_, usrlclock_high, usrlclock_low));

}

/**
* @test    gbs_reconf_02
* @brief   Tests: set_fpga_pwr_threshold
* @details set_fpga_pwr_threshold sets power threshold
*          Then the return value  FPGA_OK if set or
*..........Returns error code
*/
//TEST_P(reconf_c_p, gbs_reconf_02) {
//
//	fpga_handle h;
//	struct _fpga_token _tok;
//	fpga_token tok = &_tok;
//
//	uint64_t gbs_power = 40;
//
//	// NULL handle
//	EXPECT_EQ(FPGA_INVALID_PARAM, set_fpga_pwr_threshold(NULL, gbs_power));
//
//	// Open  port device
//	token_for_fme0(&_tok);
//	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
//
//	// Zero GBS power
//	gbs_power = 0;
//	EXPECT_EQ(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));
//
//	// Max GBS power
//	gbs_power = 200;
//	EXPECT_NE(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));
//
//	gbs_power = 65;
//	EXPECT_NE(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));
//
//	gbs_power = 60;
//	EXPECT_EQ(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));
//
//	ASSERT_EQ(FPGA_OK, fpgaClose(h));
//}



INSTANTIATE_TEST_CASE_P( reconf_c, reconf_c_p, ::testing::ValuesIn(test_platform::keys(true)));
