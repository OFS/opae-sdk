// Copyright(c) 2018-2020, Intel Corporation
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
#include "types_int.h"
#include "metrics/metrics_int.h"
#include "metrics/vector.h"
#include "opae_int.h"
}

#include <config.h>
#include <opae/fpga.h>

#include <dlfcn.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "mock/test_utils.h"
#include "token_list_int.h"
#include "xfpga.h"
#include "sysfs_int.h"

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class metrics_utils_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_utils_c_p() 
    : tokens_{{nullptr, nullptr}}, 
      handle_(nullptr) {}

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
                                  &num_matches_), FPGA_OK);
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
 * @test       opaec
 * @brief      Tests: sysfs_path_is_dir
 * @details    Validates input path as directory <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_100) {
  char group_sysfs[FPGA_METRIC_STR_SIZE] = {0};

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(NULL));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(group_sysfs));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(
                     (const char *)"/tmp/class/fpga/intel-fpga-dev.0/"
                                       "intel-fpga-fme.0/bitstream_id"));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(
                     (const char *)"/tmp/class/fpga/intel-fpga-dev.0/"
                                       "intel-fpga-fme.0/bitstream_id1"));

  std::string sysclass_path =
              system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));

  EXPECT_EQ(FPGA_OK,
            metric_sysfs_path_is_dir((const char *)sysclass_path.c_str()));
}

/**
 * @test       opaec
 * @brief      Tests: sysfs_path_is_file
 * @details    Validates input path as directory <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_101) {
  char metric_sysfs[FPGA_METRIC_STR_SIZE] = {0};
  size_t len;

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(NULL));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(metric_sysfs));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(
                     (const char *)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/"));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(
                     (const char *)"/tmp/class/fpga/intel-fpga-dev.0/"
                                       "intel-fpga-fme.0/bitstream_id1"));

  std::string sysclass_path =
              system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));

  strncpy(metric_sysfs, sysclass_path.c_str(), sysclass_path.size() + 1);
  strncat(metric_sysfs, "/", 2);
  len = strnlen("intel-fpga-fme.0/bitstream_id", sizeof(metric_sysfs) - (sysclass_path.size() + 1));
  strncat(metric_sysfs, "intel-fpga-fme.0/bitstream_id", len + 1);

  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", metric_sysfs);

  EXPECT_EQ(FPGA_OK, metric_sysfs_path_is_file((const char *)metric_sysfs));
}

/**
 * @test       opaec
 * @brief      Tests: add_metric_vector
 * @details    Validates add to metric vector <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_102) {
  char group_name[FPGA_METRIC_STR_SIZE] = {"power_mgmt"};
  char group_sysfs[FPGA_METRIC_STR_SIZE] = {
       "tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/"};

  char metrics_name[FPGA_METRIC_STR_SIZE] = {"consumed"};
  char metrics_sysfs[FPGA_METRIC_STR_SIZE] = {
       "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/"};

  char qualifier_name[FPGA_METRIC_STR_SIZE] = {"power_mgmt"};
  char metric_units[FPGA_METRIC_STR_SIZE] = {"watts"};

  fpga_metric_vector metric_vector;

  EXPECT_NE(FPGA_OK, add_metric_vector(NULL, 0, qualifier_name, group_name,
                                       group_sysfs, metrics_name, metrics_sysfs,
                                       metric_units, FPGA_METRIC_DATATYPE_INT,
                                       FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, NULL, group_name,
                                       group_sysfs, metrics_name, metrics_sysfs,
                                       metric_units, FPGA_METRIC_DATATYPE_INT,
                                       FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, NULL,
                                       group_sysfs, metrics_name, metrics_sysfs,
                                       metric_units, FPGA_METRIC_DATATYPE_INT,
                                       FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK,
            add_metric_vector(&metric_vector, 0, qualifier_name, group_name,
                              NULL, metrics_name, metrics_sysfs, metric_units,
                              FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER,
                              FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK,
            add_metric_vector(&metric_vector, 0, qualifier_name, group_name,
                              group_sysfs, NULL, metrics_sysfs, metric_units,
                              FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER,
                              FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK,
            add_metric_vector(&metric_vector, 0, qualifier_name, group_name,
                              group_sysfs, metrics_name, NULL, metric_units,
                              FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER,
                              FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK,
            add_metric_vector(&metric_vector, 0, qualifier_name, group_name,
                              group_sysfs, metrics_name, metrics_sysfs, NULL,
                              FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER,
                              FPGA_HW_MCP, 0));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&metric_vector));

  EXPECT_EQ(FPGA_OK,
            add_metric_vector(&metric_vector, 0, qualifier_name, group_name,
                              group_sysfs, metrics_name, metrics_sysfs,
                              metric_units, FPGA_METRIC_DATATYPE_INT,
                              FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&metric_vector));
}




/**
 * @test       opaec
 * @brief      Tests: enum_fpga_metrics
 * @details    Validates enumeration fpga metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_109) {
  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

  EXPECT_NE(FPGA_OK, enum_fpga_metrics(NULL));
}

/**
 * @test       opaec
 * @brief      Tests: enum_fpga_metrics
 * @details    Validates enumeration fpga metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_10) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

  struct fpga_metric fpga_metric;

  EXPECT_EQ(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 1, &fpga_metric));

  EXPECT_EQ(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 5, &fpga_metric));

  EXPECT_EQ(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 10, &fpga_metric));

  EXPECT_EQ(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 15, &fpga_metric));

  EXPECT_EQ(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 20, &fpga_metric));

  EXPECT_NE(FPGA_OK, get_fme_metric_value(handle_, NULL, 1, &fpga_metric));

  EXPECT_NE(FPGA_OK,
            get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector),
                                 1, NULL));
}

/**
 * @test       opaec
 * @brief      Tests: parse_metric_num_name
 * @details    Validates parse metric string <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_11) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

  char serach_string[] = {"power_mgmt:consumed"};
  uint64_t metric_id;

  EXPECT_NE(FPGA_OK,
            parse_metric_num_name(NULL, &(_handle->fpga_enum_metric_vector),
                                  &metric_id));

  EXPECT_NE(FPGA_OK,
            parse_metric_num_name((const char *)serach_string,
                                  &(_handle->fpga_enum_metric_vector), NULL));

  EXPECT_NE(FPGA_OK, parse_metric_num_name((const char *)serach_string, NULL,
                                           &metric_id));

  EXPECT_NE(FPGA_OK, parse_metric_num_name((const char *)"power_mgmt consumed",
                                           &(_handle->fpga_enum_metric_vector),
                                           &metric_id));
}

/**
 * @test       opaec
 * @brief      Tests: enum_fpga_metrics
 * @details    Validates delete enum metric <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_12) {
  EXPECT_NE(FPGA_OK, free_fpga_enum_metrics_vector(NULL));

  struct _fpga_handle _handle_invalid;
  memset(&_handle_invalid, 0, sizeof(struct _fpga_handle));

  EXPECT_NE(FPGA_OK, free_fpga_enum_metrics_vector(&_handle_invalid));
}



TEST_P(metrics_utils_c_p, test_metric_utils_15) {
  fpga_objtype objtype;
  EXPECT_NE(FPGA_OK, get_fpga_object_type(NULL, &objtype));

  EXPECT_NE(FPGA_OK, get_fpga_object_type(handle_, NULL));
}

INSTANTIATE_TEST_CASE_P(metrics_utils_c, metrics_utils_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));

class metrics_utils_dcp_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_utils_dcp_c_p() 
    : tokens_{{nullptr, nullptr}},
      handle_(nullptr) {}

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
                                  &num_matches_), FPGA_OK);
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

TEST_P(metrics_utils_dcp_c_p, test_metric_utils_12) {
  uint64_t metric_id;

  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;
  fpga_metric_vector vector;

  _handle->bmc_handle = metrics_load_bmc_lib();
  ASSERT_NE(_handle->bmc_handle, (void *)nullptr);

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  EXPECT_EQ(FPGA_OK, enum_bmc_metrics_info(_handle, &vector, &metric_id,
                                           FPGA_HW_DCP_RC));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

TEST_P(metrics_utils_dcp_c_p, test_metric_utils_13) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;

  _handle->bmc_handle = metrics_load_bmc_lib();
  ASSERT_NE(_handle->bmc_handle, (void *)nullptr);

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));
}

TEST_P(metrics_utils_dcp_c_p, test_metric_utils_14) {
  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;

  _handle->bmc_handle = metrics_load_bmc_lib();
  ASSERT_NE(_handle->bmc_handle, (void *)nullptr);

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

  struct _fpga_enum_metric _fpga_enum_metric = {
      "", "", "", "", "", "", 0,
      FPGA_METRIC_DATATYPE_INT,
      FPGA_METRIC_TYPE_POWER,
      FPGA_HW_UNKNOWN,
      0};

  struct fpga_metric fpga_metric;

  EXPECT_EQ(FPGA_OK, get_bmc_metrics_values(handle_, &_fpga_enum_metric, &fpga_metric));
}

INSTANTIATE_TEST_CASE_P(metrics_utils_c, metrics_utils_dcp_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));
