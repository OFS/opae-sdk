// Copyright(c) 2021, Intel Corporation
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
#include "mock/test_system.h"

using namespace opae::testing;

class metrics_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_c_p()
  : tokens_accel_{{nullptr, nullptr}},
    tokens_dev_{{nullptr, nullptr}}
  {}

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
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_accel_.data(), tokens_accel_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_accel_[0], &accel_, 0), FPGA_OK);

    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_dev_.data(), tokens_dev_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    dev_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_dev_[0], &dev_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
    }
    if (dev_) {
        EXPECT_EQ(fpgaClose(dev_), FPGA_OK);
        dev_ = nullptr;
    }
    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    for (auto &t : tokens_dev_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
#ifdef LIBOPAE_DEBUG
    EXPECT_EQ(opae_wrapped_tokens_in_use(), 0);
#endif // LIBOPAE_DEBUG
  }

  std::array<fpga_token, 2> tokens_accel_;
  std::array<fpga_token, 2> tokens_dev_;
  fpga_properties filter_;
  fpga_handle accel_;
  fpga_handle dev_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       by_name0
 * @brief      Test: fpgaGetMetricsByName
 * @details    When fpgaGetMetricsByName is called with valid params,<br>
 *             then the fn fails because the AFU metrics BBB is not found.<br>
 */
TEST_P(metrics_c_p, by_name0) {
  uint64_t array_size = 2;
  const char *metric_strings[2] = { "power_mgmt:consumed",
                                    "performance:fabric:port0:mmio_read" };

  struct fpga_metric *metrics = (struct fpga_metric *)
	  calloc(sizeof(struct fpga_metric), array_size);
  ASSERT_NE(metrics, nullptr);

  EXPECT_NE(fpgaGetMetricsByName(accel_,
                                 (char **)metric_strings,
                                 array_size,
				 metrics),
            FPGA_OK);

  free(metrics);
}

/**
 * @test       threshold0
 * @brief      Test: fpgaGetMetricsThresholdInfo
 * @details    When fpgaGetMetricsThresholdInfo is called with valid params,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(metrics_c_p, threshold0) {
  uint32_t num_thresholds = 0;

  EXPECT_EQ(fpgaGetMetricsThresholdInfo(dev_,
                                        NULL,
                                        &num_thresholds),
            FPGA_OK);


}

INSTANTIATE_TEST_CASE_P(metrics_c, metrics_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));
