// Copyright(c) 2021-2022, Intel Corporation
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

#include "mock/opae_fixtures.h"

using namespace opae::testing;

class metrics_c_p : public opae_device_p<> {
 protected:
  metrics_c_p() :
    accel_(nullptr)
  {}

  virtual void SetUp() override
  {
    opae_device_p<>::SetUp();
    ASSERT_EQ(opae_device_p<>::Open(accel_token_, &accel_,
                                    opae_device_p<>::open_flags()), FPGA_OK);
  }

  virtual void TearDown() override
  {
    ASSERT_EQ(opae_device_p<>::Close(accel_), FPGA_OK);
    accel_ = nullptr;
    opae_device_p<>::TearDown();
  }

  fpga_handle accel_;
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
                                 metrics), FPGA_OK);

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

  EXPECT_EQ(fpgaGetMetricsThresholdInfo(device_,
                                        NULL,
                                        &num_thresholds), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(metrics_c_p);
INSTANTIATE_TEST_SUITE_P(metrics_c, metrics_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));
