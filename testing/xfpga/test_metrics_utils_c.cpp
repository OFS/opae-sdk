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
#include "types_int.h"
#include "metrics/vector.h"
#include "metrics/metrics_int.h"
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"
#include "xfpga.h"
#include "token_list_int.h"
#include <dlfcn.h>
#include "test_utils.h"
#include "safe_string/safe_string.h"
//#include "metrics/metrics_metadata.h"
using namespace opae::testing;

class metrics_utils_c_p : public ::testing::TestWithParam<std::string> {
protected:
  metrics_utils_c_p()
    : tokens_{ {nullptr, nullptr} },
      handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
      &num_matches_), FPGA_OK);

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

  char group_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(NULL));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir(group_sysfs));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/bitstream_id"));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_dir((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/bitstream_id1"));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));

  EXPECT_EQ(FPGA_OK, metric_sysfs_path_is_dir((const char*)sysclass_path.c_str()));
}

/**
 * @test       opaec
 * @brief      Tests: sysfs_path_is_file
 * @details    Validates input path as directory <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_101) {

  char metric_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(NULL));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file(metric_sysfs));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/"));

  EXPECT_NE(FPGA_OK, metric_sysfs_path_is_file((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/bitstream_id1"));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));

  snprintf_s_ss(metric_sysfs, sizeof(metric_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/bitstream_id");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", metric_sysfs);

  EXPECT_EQ(FPGA_OK, metric_sysfs_path_is_file((const char*)metric_sysfs));
}

/**
 * @test       opaec
 * @brief      Tests: add_metric_vector
 * @details    Validates add to metric vector <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_102) {

  char group_name[FPGA_METRIC_STR_SIZE] = { "power_mgmt" };
  char group_sysfs[FPGA_METRIC_STR_SIZE] = { "tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/" };

  char metrics_name[FPGA_METRIC_STR_SIZE] = { "consumed" };
  char metrics_sysfs[FPGA_METRIC_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/" };

  char qualifier_name[FPGA_METRIC_STR_SIZE] = { "power_mgmt" };
  char metric_units[FPGA_METRIC_STR_SIZE] = { "watts" };

  fpga_metric_vector metric_vector;

  EXPECT_NE(FPGA_OK, add_metric_vector(NULL, 0, qualifier_name, group_name, group_sysfs, metrics_name,
    metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, NULL, group_name, group_sysfs, metrics_name,
    metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, NULL, group_sysfs, metrics_name,
    metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, NULL, metrics_name,
    metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, NULL,
    metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name, NULL, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name, metrics_sysfs, NULL, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&metric_vector));

  EXPECT_EQ(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name, metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP,0));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&metric_vector));
}

/**
 * @test       opaec
 * @brief      Tests: enum_thermalmgmt_metrics
 * @details    Validates enumeration of thermal metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_103) {

  char group_sysfs[FPGA_METRIC_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0" };
  char group_sysfs_invalid[FPGA_METRIC_STR_SIZE] = { 0 };

  fpga_metric_vector vector;
  uint64_t metric_id = 0;

  EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, group_sysfs_invalid, FPGA_HW_MCP));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, group_sysfs, FPGA_HW_MCP));
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

/**
 * @test       opaec
 * @brief      Tests: enum_powermgmt_metrics
 * @details    Validates enumeration of power metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_104) {

  char group_sysfs[FPGA_METRIC_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0" };
  char group_sysfs_invalid[FPGA_METRIC_STR_SIZE] = { 0 };
  uint64_t metric_id = 0;
  fpga_metric_vector vector;

  EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, group_sysfs_invalid, FPGA_HW_MCP));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, group_sysfs, FPGA_HW_MCP));
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

/**
 * @test       opaec
 * @brief      Tests: enum_perf_counter_items
 * @details    Validates enumeration performance counters metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_105) {

  char qualifier_name[FPGA_METRIC_STR_SIZE] = { 0 };
  char group_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };
  char sysfs_name[FPGA_METRIC_STR_SIZE] = { 0 };
  uint64_t metric_id = 0;
  fpga_metric_vector vector;

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(NULL, &metric_id, qualifier_name, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, NULL, qualifier_name, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, NULL, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, NULL, sysfs_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, group_sysfs, NULL, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, group_sysfs, NULL, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)"/tmp/class/fpga/intel-fpga-dev.1", sysfs_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/iperf/");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)group_sysfs,
                     (char*)"fabric", FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/perf/");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)group_sysfs,
                     (char*)"port0", FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_HW_MCP));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

/**
 * @test       opaec
 * @brief      Tests: enum_perf_counter_metrics
 * @details    Validates enumeration performance counters metrics <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_106) {

  char group_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };
  fpga_metric_vector vector;
  uint64_t metric_id = 0;

  EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));
  EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));
  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, (char*)group_sysfs, FPGA_HW_MCP));

  EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, (char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.1", FPGA_HW_MCP));
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

/**
 * @test       opaec
 * @brief      Tests: add_metric_info
 * @details    Validates add metrics info <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_107) {

  struct _fpga_enum_metric _enum_metric = {
     "power_mgmt",
     "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/power_mgmt",
     "consumed",
     "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/power_mgmt/consumed",
    "power_mgmt:consumed",
     "watts",
    1,
    FPGA_METRIC_DATATYPE_INT,
     FPGA_METRIC_TYPE_POWER,
     FPGA_HW_MCP,
     0
  };

  struct fpga_metric_info  fpga_metric_info;

  EXPECT_NE(FPGA_OK, add_metric_info(NULL, &fpga_metric_info));
  EXPECT_NE(FPGA_OK, add_metric_info(&_enum_metric, NULL));
  EXPECT_EQ(FPGA_OK, add_metric_info(&_enum_metric, &fpga_metric_info));
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

  struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;
  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

  struct fpga_metric fpga_metric;

  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 1, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 5, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 10, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 15, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 20, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 25, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 30, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 34, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 31, &fpga_metric));
  EXPECT_EQ(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 32, &fpga_metric));
  EXPECT_NE(FPGA_OK, get_fme_metric_value(handle_, NULL, 1, &fpga_metric));
  EXPECT_NE(FPGA_OK, get_fme_metric_value(handle_, &(_handle->fpga_enum_metric_vector), 1, NULL));
}

/**
 * @test       opaec
 * @brief      Tests: parse_metric_num_name
 * @details    Validates parse metric string <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_11) {

  struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

  char serach_string[] = { "power_mgmt:consumed" };
  uint64_t metric_id;
  EXPECT_EQ(FPGA_OK, parse_metric_num_name((const char*)serach_string, &(_handle->fpga_enum_metric_vector), &metric_id));
  EXPECT_NE(FPGA_OK, parse_metric_num_name(NULL, &(_handle->fpga_enum_metric_vector), &metric_id));
  EXPECT_NE(FPGA_OK, parse_metric_num_name((const char*)serach_string, &(_handle->fpga_enum_metric_vector), NULL));
  EXPECT_NE(FPGA_OK, parse_metric_num_name((const char*)serach_string, NULL, &metric_id));
  EXPECT_NE(FPGA_OK, parse_metric_num_name((const char*) "power_mgmt consumed", &(_handle->fpga_enum_metric_vector), &metric_id));
}

/**
 * @test       opaec
 * @brief      Tests: enum_fpga_metrics
 * @details    Validates delete enum metric <br>
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_12) {

  EXPECT_NE(FPGA_OK, free_fpga_enum_metrics_vector(NULL));

  struct _fpga_handle _handle_invalid ;
  memset_s(&_handle_invalid, sizeof(struct _fpga_handle), 0);

  EXPECT_NE(FPGA_OK, free_fpga_enum_metrics_vector(&_handle_invalid));
}

/**
 * @test       opaec
 * @brief      Tests: enum_fpga_metrics
* @details    Validates delete enum metric <br>
 *
 *
 */
TEST_P(metrics_utils_c_p, test_metric_utils_13) {

  uint64_t  value;
  char group_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };

  EXPECT_NE(FPGA_OK, get_pwr_thermal_value(group_sysfs,NULL));
  EXPECT_NE(FPGA_OK, get_pwr_thermal_value(NULL, &value));
  EXPECT_NE(FPGA_OK, get_pwr_thermal_value((char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.1", &value));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/power_mgmt/fpga_limit");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, get_pwr_thermal_value(group_sysfs, &value));

  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/power_mgmt/xeon_limit");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  EXPECT_EQ(FPGA_OK, get_pwr_thermal_value(group_sysfs, &value));
}

TEST_P(metrics_utils_c_p, test_metric_utils_14) {

  uint64_t  value;
  char group_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };
  char metric_sysfs[FPGA_METRIC_STR_SIZE] = { 0 };

  EXPECT_NE(FPGA_OK, get_performance_counter_value(group_sysfs, metric_sysfs, NULL));
  EXPECT_NE(FPGA_OK, get_performance_counter_value(NULL, metric_sysfs, &value));
  EXPECT_NE(FPGA_OK, get_performance_counter_value(group_sysfs, NULL, &value));
  EXPECT_NE(FPGA_OK, get_performance_counter_value((char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.1", metric_sysfs, &value));

  std::string sysclass_path = system_->get_sysfs_path(std::string("/sys/class/fpga/intel-fpga-dev.0"));
  snprintf_s_ss(group_sysfs, sizeof(group_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/power_mgmt/iperf/cache");
  printf("sysclass_path %s \n", sysclass_path.c_str());
  printf("metric_sysfs %s \n", group_sysfs);

  snprintf_s_ss(metric_sysfs, sizeof(metric_sysfs), "%s/%s", sysclass_path.c_str(), "intel-fpga-fme.0/power_mgmt/iperf/cache/read_miss1");
  printf("metric_sysfs %s \n", metric_sysfs);

  EXPECT_NE(FPGA_OK, get_performance_counter_value(group_sysfs, metric_sysfs, &value));
}

TEST_P(metrics_utils_c_p, test_metric_utils_15) {

  fpga_objtype objtype;
  EXPECT_NE(FPGA_OK, get_fpga_object_type(NULL, &objtype));
  EXPECT_NE(FPGA_OK, get_fpga_object_type(handle_,NULL));
}

INSTANTIATE_TEST_CASE_P(metrics_utils_c, metrics_utils_c_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})) );


class metrics_utils_dcp_c_p : public ::testing::TestWithParam<std::string> {
protected:
  metrics_utils_dcp_c_p()
    : tokens_{ {nullptr, nullptr} },
      handle_(nullptr) {}

  virtual void SetUp() override {

    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
              &num_matches_), FPGA_OK);

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
  struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;
  fpga_metric_vector vector;

  _handle->bmc_handle = dlopen("liblibbmc.so", RTLD_LAZY | RTLD_LOCAL);

  if (!_handle->bmc_handle) {
    OPAE_ERR("--------------------------failed to load ");

  }

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));
  EXPECT_EQ(FPGA_OK, enum_bmc_metrics_info(_handle,  &vector, &metric_id, FPGA_HW_DCP_RC));
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

TEST_P(metrics_utils_dcp_c_p, test_metric_utils_13) {

  struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;
  _handle->bmc_handle = dlopen("liblibbmc.so", RTLD_LAZY | RTLD_LOCAL);

  if (!_handle->bmc_handle) {
    OPAE_ERR("--------------------------failed to load ");
  }

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));
}

TEST_P(metrics_utils_dcp_c_p, test_metric_utils_14) {

  struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;
  _handle->bmc_handle = dlopen("liblibbmc.so", RTLD_LAZY | RTLD_LOCAL);

  if (!_handle->bmc_handle) {
    OPAE_ERR("--------------------------failed to load ");
  }

  EXPECT_EQ(FPGA_OK, enum_fpga_metrics(handle_));

  struct _fpga_enum_metric _fpga_enum_metric;
  struct fpga_metric fpga_metric;

  get_bmc_metrics_values(handle_, &_fpga_enum_metric, &fpga_metric);

}

INSTANTIATE_TEST_CASE_P(metrics_utils_c, metrics_utils_dcp_c_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({ "dcp-rc" })));

