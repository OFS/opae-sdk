// Copyright(c) 2019-2020, Intel Corporation
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
#include "metrics/metrics_int.h"
#include "metrics/metrics_max10.h"
#include "metrics/vector.h"
#include "opae_int.h"
#include "types_int.h"
}

#include <config.h>
#include <opae/fpga.h>

#include <vector>
#include "gtest/gtest.h"
#include "sysfs_int.h"
#include "mock/test_system.h"
#include "mock/test_utils.h"
#include "token_list_int.h"
#include "xfpga.h"

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class metrics_max10_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_max10_c_p() : tokens_{{nullptr, nullptr}}, handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_),
              FPGA_OK);
    ASSERT_GT(num_matches_, 0);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    if (handle_ != nullptr) {
      EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK);
      handle_ = nullptr;
    }
    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
* @test       test_metric_max10_1
* @brief      Tests: read_sensor_sysfs_file
* @details    When the parameters are valid read_sensor_sysfs_file
*             reads system attributes
*             When the parameters are invalid read_sensor_sysfs_file
*             retuns error.
*
*/
TEST_P(metrics_max10_c_p, test_metric_max10_1) {
  uint32_t tot_bytes_ret;
  void *buf = NULL;
  char file[] = "curr1_input";
  char sysfs[] =
		"/sys/class/fpga_region/region*/dfl-fme.*/dfl-fme.*/spi-altera.*.auto/"
		"spi_master/spi4/spi*/*-hwmon.*.auto/hwmon/hwmon*";

  EXPECT_NE(read_sensor_sysfs_file(NULL, file, &buf, &tot_bytes_ret), FPGA_OK);

  EXPECT_NE(read_sensor_sysfs_file(sysfs, NULL, &buf, &tot_bytes_ret), FPGA_OK);

  EXPECT_NE(read_sensor_sysfs_file(sysfs, file, &buf, NULL), FPGA_OK);

  EXPECT_EQ(read_sensor_sysfs_file(sysfs, file, &buf, &tot_bytes_ret), FPGA_OK);

  EXPECT_NE(read_sensor_sysfs_file(sysfs, "test", &buf, &tot_bytes_ret),
            FPGA_OK);

  if (buf) free(buf);
}

/**
* @test       test_metric_max10_2
* @brief      Tests: enum_max10_metrics_info
* @details    When the parameters are valid enum_max10_metrics_info
*             eunum max10 metrics and add to vector
*             When the parameters are invalid enum_max10_metrics_info
*             retuns error.
*
*/
TEST_P(metrics_max10_c_p, test_metric_max10_2) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;
  fpga_metric_vector vector;
  uint64_t metric_num = 0;

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  EXPECT_EQ(FPGA_OK, dfl_enum_max10_metrics_info(_handle, &vector, &metric_num,
                                             FPGA_HW_DCP_N3000));

  EXPECT_NE(FPGA_OK,
	  dfl_enum_max10_metrics_info(_handle, &vector, NULL, FPGA_HW_DCP_N3000));

  EXPECT_NE(FPGA_OK, dfl_enum_max10_metrics_info(NULL, &vector, &metric_num,
                                             FPGA_HW_DCP_N3000));

  EXPECT_NE(FPGA_OK, dfl_enum_max10_metrics_info(_handle, NULL, &metric_num,
                                             FPGA_HW_DCP_N3000));

  EXPECT_NE(FPGA_OK,
	  dfl_enum_max10_metrics_info(_handle, &vector, NULL, FPGA_HW_DCP_N3000));

  EXPECT_EQ(FPGA_OK, dfl_enum_max10_metrics_info(_handle, &vector, &metric_num,
                                             FPGA_HW_UNKNOWN));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}
INSTANTIATE_TEST_CASE_P(metrics_max10_c, metrics_max10_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"dcp-d5005-dfl"})));

class metrics_invalid_max10_c_p : public metrics_max10_c_p {};

/**
* @test       test_metric_max10_3
* @brief      Tests: enum_max10_metrics_info
* @details    When the parameters are valid  and run on not supportd platform
*             enum_max10_metrics_info retuns error.
*
*/
TEST_P(metrics_invalid_max10_c_p, test_metric_max10_3) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;
  fpga_metric_vector vector;
  uint64_t metric_num = 0;

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  EXPECT_NE(FPGA_OK, dfl_enum_max10_metrics_info(_handle, &vector, &metric_num,
                                             FPGA_HW_DCP_N3000));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}
INSTANTIATE_TEST_CASE_P(metrics_max10_c, metrics_invalid_max10_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));


class metrics_max10_vc_c_p : public metrics_max10_c_p {
protected:
	metrics_max10_vc_c_p() {}
};

/**
* @test       test_metric_max10_4
* @brief      Tests: read_max10_value
* @details    When passed with valid argument fn reads metric values <br>
*             When passed with invalid argument return
*             FPGA_INVALID_PARAM <br>
*/
TEST_P(metrics_max10_vc_c_p, test_metric_max10_4) {

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;
	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

	struct fpga_metric fpga_metric;

	EXPECT_EQ(FPGA_OK,
		get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
			1, &fpga_metric));

	double dvalue = 0;
	EXPECT_EQ(FPGA_INVALID_PARAM, read_max10_value(NULL, &dvalue));
}
INSTANTIATE_TEST_CASE_P(metrics_max10_c, metrics_max10_vc_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "dcp-d5005-dfl" })));
