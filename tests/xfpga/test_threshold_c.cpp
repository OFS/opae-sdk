// Copyright(c) 2019-2022, Intel Corporation
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
#include "metrics/metrics_int.h"
#include "metrics/metrics_max10.h"
#include "metrics/threshold.h"
#include "metrics/vector.h"
#include "opae_int.h"
#include "types_int.h"
#include "xfpga.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class metrics_threshold_c_p : public opae_device_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

};

/**
* @test       metrics_threshold_1
* @brief      Tests: xfpga_fpgaGetMetricsThresholdInfo
* @details    When the parameters are valid xfpga_fpgaGetMetricsThresholdInfo
*             reads metrics threshold info
*             When the parameters are invalid xfpga_fpgaGetMetricsThresholdInfo
*             retuns error.
*
*/
TEST_P(metrics_threshold_c_p, metrics_threshold_1) {
  uint32_t num_thresholds;

  EXPECT_EQ(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(NULL, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, NULL), FPGA_OK);
}

/**
* @test       metrics_threshold_2
* @brief      Tests: get_max10_threshold_info
* @details    When the parameters are valid get_max10_threshold_info
*             reads max10 threshold info
*             When the parameters are invalid get_max10_threshold_info
*             retuns error.
*
*/
TEST_P(metrics_threshold_c_p, metrics_threshold_2) {
  uint32_t num_thresholds;
  metric_threshold *pmetric_thresholds;

  EXPECT_NE(get_max10_threshold_info(device_, NULL, NULL), FPGA_OK);
  EXPECT_NE(get_max10_threshold_info(NULL, NULL, &num_thresholds), FPGA_OK);

  EXPECT_EQ(get_max10_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);

  pmetric_thresholds = (struct metric_threshold *)opae_calloc(
      sizeof(struct metric_threshold), num_thresholds);
  ASSERT_NE(pmetric_thresholds, (void *)nullptr);

  EXPECT_EQ(
      get_max10_threshold_info(device_, pmetric_thresholds, &num_thresholds),
      FPGA_OK);

  if (pmetric_thresholds) opae_free(pmetric_thresholds);

  EXPECT_NE(get_bmc_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(metrics_threshold_c_p);
INSTANTIATE_TEST_SUITE_P(metrics_threshold_c_c, metrics_threshold_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "dcp-vc" })));

class metrics_bmc_threshold_c_p : public metrics_threshold_c_p {};

/**
* @test       metrics_threshold_3
* @brief      Tests: xfpga_fpgaGetMetricsThresholdInfo
* @details    When the parameters are valid and invalid platform
*             xfpga_fpgaGetMetricsThresholdInfo returns error
*
*/
TEST_P(metrics_bmc_threshold_c_p, metrics_threshold_3) {
  uint32_t num_thresholds;

  EXPECT_EQ(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(NULL, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, NULL), FPGA_OK);
}

/**
* @test       metrics_threshold_4
* @brief      Tests: get_bmc_threshold_info
* @details    When the parameters are valid get_bmc_threshold_info
*             reads bmc threshold info
*             When the parameters are invalid get_bmc_threshold_info
*             retuns error.
*
*/
TEST_P(metrics_bmc_threshold_c_p, metrics_threshold_4) {
  uint32_t num_thresholds;
  metric_threshold *pmetric_thresholds;
  struct _fpga_handle *_handle = (struct _fpga_handle *)device_;

  EXPECT_NE(get_bmc_threshold_info(device_, NULL, NULL), FPGA_OK);
  EXPECT_NE(get_bmc_threshold_info(NULL, NULL, &num_thresholds), FPGA_OK);

  _handle->bmc_handle = metrics_load_bmc_lib();
  ASSERT_NE(_handle->bmc_handle, (void *)nullptr);

  EXPECT_EQ(get_bmc_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);

  pmetric_thresholds = (struct metric_threshold *)opae_calloc(
      sizeof(struct metric_threshold), num_thresholds);
  ASSERT_NE(pmetric_thresholds, (void *)nullptr);

  EXPECT_EQ(
      get_bmc_threshold_info(device_, pmetric_thresholds, &num_thresholds),
      FPGA_OK);

  if (pmetric_thresholds) opae_free(pmetric_thresholds);

  EXPECT_NE(get_max10_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(metrics_bmc_threshold_c_p);
INSTANTIATE_TEST_SUITE_P(metrics_threshold_c_c, metrics_bmc_threshold_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));

class metrics_mcp_threshold_c_p : public metrics_threshold_c_p {};

/**
* @test       metrics_threshold_5
* @brief      Tests: xfpga_fpgaGetMetricsThresholdInfo
*              get_max10_threshold_info
* @details    When the parameters are valid and invalid platform
*             retuns error
*
*/
TEST_P(metrics_mcp_threshold_c_p, metrics_threshold_5) {
  uint32_t num_thresholds;

  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, &num_thresholds),
            FPGA_OK);

  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(NULL, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(device_, NULL, NULL), FPGA_OK);

  EXPECT_NE(get_max10_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);

  EXPECT_NE(get_bmc_threshold_info(device_, NULL, &num_thresholds), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(metrics_mcp_threshold_c_p);
INSTANTIATE_TEST_SUITE_P(metrics_threshold_c_c, metrics_mcp_threshold_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));

class metrics_afu_threshold_c_p : public opae_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

};

/**
* @test       metrics_threshold_6
* @brief      Tests: xfpga_fpgaGetMetricsThresholdInfo
* @details    When the parameters are valid and invalid object type
*             retuns error
*
*/
TEST_P(metrics_afu_threshold_c_p, metrics_threshold_6) {
  uint32_t num_thresholds;

  EXPECT_EQ(xfpga_fpgaGetMetricsThresholdInfo(accel_, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(NULL, NULL, &num_thresholds),
            FPGA_OK);
  EXPECT_NE(xfpga_fpgaGetMetricsThresholdInfo(accel_, NULL, NULL), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(metrics_afu_threshold_c_p);
INSTANTIATE_TEST_SUITE_P(metrics_threshold_c_c, metrics_afu_threshold_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"dcp-vc"})));
