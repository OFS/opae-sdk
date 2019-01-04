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
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class error_c_skx_dcp_p : public ::testing::TestWithParam<std::string> {
 protected:
  error_c_skx_dcp_p() : tokens_{{nullptr, nullptr}} {}

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
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    EXPECT_GT(num_matches_, 0);
  }

  virtual void TearDown() override {
    if (filter_) {
      EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
  }

  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       read
 * @brief      Test: fpgaReadError
 * @details    When fpgaReadError is called with valid params,<br>
 *             it retrieves the value of the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_skx_dcp_p, read) {
  uint64_t val = 0xdeadbeefdecafbad;
  EXPECT_EQ(fpgaReadError(tokens_[0], 0, &val), FPGA_OK);
  EXPECT_EQ(val, 0);
}

/**
 * @test       get_info
 * @brief      Test: fpgaGetErrorInfo
 * @details    When fpgaGetErrorInfo is called with valid params,<br>
 *             it retrieves the info of the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_skx_dcp_p, get_info) {
  fpga_properties props = nullptr;
  uint32_t num_errors = 0;
  ASSERT_EQ(fpgaGetProperties(tokens_[0], &props), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetNumErrors(props, &num_errors), FPGA_OK);
  // this is a port, which only has three error registers
  ASSERT_EQ(num_errors, platform_.devices[0].port_num_errors);
  std::map<std::string, bool> knows_errors = {
      {"errors", true}, {"first_error", false}, {"first_malformed_req", false}};
  std::vector<fpga_error_info> info_list(num_errors);
  for (int i = 0; i < num_errors; ++i) {
    fpga_error_info &info = info_list[i];
    EXPECT_EQ(fpgaGetErrorInfo(tokens_[0], i, &info), FPGA_OK);
    EXPECT_EQ(info.can_clear, knows_errors[info.name]);
  }

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

/**
 * @test       clear
 * @brief      Test: fpgaClearError
 * @details    When fpgaClearError is called with valid params,<br>
 *             it clears the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_skx_dcp_p, clear) {
  fpga_error_info info;
  uint32_t e = 0;
  bool cleared = false;
  while (fpgaGetErrorInfo(tokens_[0], e, &info) == FPGA_OK) {
    if (info.can_clear) {
      EXPECT_EQ(fpgaClearError(tokens_[0], e), FPGA_OK);
      cleared = true;
      break;
    }
    ++e;
  }
  EXPECT_EQ(cleared, true);
}

INSTANTIATE_TEST_CASE_P(error_c, error_c_skx_dcp_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p", "dcp-rc"})));

class error_c_p_all : public error_c_skx_dcp_p {
  protected:
    error_c_p_all() {}
};
/**
 * @test       clear
 * @brief      Test: fpgaClearAllErrors
 * @details    When fpgaClearAllErrors is called with valid params,<br>
 *             it clears the requested errors,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_p_all, clear_all) {
  EXPECT_EQ(fpgaClearAllErrors(tokens_[0]), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(error_c, error_c_p_all,
                        ::testing::ValuesIn(test_platform::platforms({})));
