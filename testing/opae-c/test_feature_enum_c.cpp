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

#include <json-c/json.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include "opae_int.h"
#include "safe_string/safe_string.h"


#ifdef __cplusplus
}
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class feature_enum_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  feature_enum_c_p() : filter_(nullptr), tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {

	ASSERT_TRUE(test_platform::exists(GetParam()));
	platform_ = test_platform::get(GetParam());
	system_ = test_system::instance();
	system_->initialize();
	system_->prepare_syfs(platform_);
	invalid_device_ = test_device::unknown();

	ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
	ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
	ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
	num_matches_ = 0;
	ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
		  &num_matches_),
		  FPGA_OK);
	EXPECT_EQ(num_matches_, platform_.devices.size());
	accel_ = nullptr;
	ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
	feature_filter_.type = DMA;     // TODO: 
	memset_s(feature_filter_.guid, sizeof(fpga_guid), 0);

  }

  void DestroyTokens() {
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

	for (auto &t : ftokens_) {
		if (t) {
			EXPECT_EQ(fpgaFeatureTokenDestroy(&t), FPGA_OK);
			t = nullptr;
		}
	}
    num_matches_ = 0;
  }

  virtual void TearDown() override {
    DestroyTokens();
    if (filter_ != nullptr) {
      EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    }
    system_->finalize();
  }

  std::array<fpga_feature_token, 2> ftokens_;
  fpga_handle accel_;
  fpga_feature_properties feature_filter_;

  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  uint32_t num_matches_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};


TEST_P(feature_enum_c_p, nullfilter) {
  EXPECT_EQ(
      fpgaFeatureEnumerate(accel_, nullptr, ftokens_.data(), ftokens_.size(), &num_matches_),
      FPGA_OK);
  //EXPECT_EQ(num_matches_, platform_.devices.size() * 2);  // TODO: 

  uint32_t matches = 0;
  EXPECT_EQ(fpgaFeatureEnumerate(accel_, nullptr, ftokens_.data(), ftokens_.size(), &matches),
            FPGA_INVALID_PARAM);
}

TEST_P(feature_enum_c_p, nullmatches) {
  EXPECT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(), ftokens_.size(), NULL),
            FPGA_INVALID_PARAM);
}

TEST_P(feature_enum_c_p, nulltokens) {
  EXPECT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, NULL, ftokens_.size(), &num_matches_),
            FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(feature_enum_c, feature_enum_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));
