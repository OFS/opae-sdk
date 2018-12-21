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
#include "intel-fpga.h"
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"
#include "token_list_int.h"
#include "xfpga.h"
#include "test_utils.h"
#include <linux/ioctl.h>
#include <sys/mman.h>

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

using namespace opae::testing;

int mmio_ioctl(mock_object *m, int request, va_list argp) {
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct fpga_port_region_info *rinfo =
      va_arg(argp, struct fpga_port_region_info *);
  if (!rinfo) {
    FPGA_MSG("rinfo is NULL");
    goto out_EINVAL;
  }
  if (rinfo->argsz != sizeof(*rinfo)) {
    FPGA_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (rinfo->index > 1) {
    FPGA_MSG("unsupported MMIO index");
    goto out_EINVAL;
  }
  if (rinfo->padding != 0) {
    FPGA_MSG("unsupported padding");
    goto out_EINVAL;
  }
  rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
  rinfo->size = 0x40000;
  rinfo->offset = 0;
  retval = 0;
  errno = 0;

out:
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}


class metrics_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_c_p() : tokens_{{nullptr, nullptr}}, handle_(nullptr) {}

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
    if (handle_) {
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
* @test    test_metric_01
* @brief   Tests: xfpga_fpgaGetNumMetrics
* @details Validates get number metrics
*
*/
TEST_P(metrics_c_p, test_metric_01) {
  // get number of metrics
  uint64_t num_metrics;
  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, &num_metrics));

  // NULL input parameters
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, NULL));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumMetrics(NULL, &num_metrics));

  struct _fpga_handle *_handle = (struct _fpga_handle *)handle_;

  int fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, &num_metrics));
  _handle->fddev = fddev;
}

/**
* @test    test_metric_02
* @brief   Tests: xfpga_fpgaGetMetricsInfo
* @details Validates get metrics info
*
*/
TEST_P(metrics_c_p, test_metric_02) {
  struct _fpga_handle *_handle = NULL;
  uint64_t num_metrics;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, &num_metrics));

  struct fpga_metric_info *fpga_metric_info = (struct fpga_metric_info *)calloc(
         sizeof(struct fpga_metric_info), num_metrics);

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaGetMetricsInfo(handle_, fpga_metric_info, &num_metrics));

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsInfo(NULL, fpga_metric_info, &num_metrics));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsInfo(handle_, NULL, &num_metrics));

  _handle = (struct _fpga_handle *)handle_;

  int fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsInfo(handle_, fpga_metric_info, &num_metrics));

  _handle->fddev = fddev;

  free(fpga_metric_info);
}

/**
* @test    test_metric_03
* @brief   Tests: xfpga_fpgaGetMetricsByIndex
* @details Validates get metric value by index
*
*/
TEST_P(metrics_c_p, test_metric_03) {
  struct _fpga_handle *_handle = NULL;

  uint64_t id_array[] = {1, 5, 30, 35, 10};

  struct fpga_metric *metric_array =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), 5);

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, id_array, 5, metric_array));

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(NULL, id_array, 5, metric_array));

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, NULL, 5, metric_array));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsByIndex(handle_, id_array, 5, NULL));

  _handle = (struct _fpga_handle *)handle_;
  int fddev = _handle->fddev;

  _handle->fddev = -1;
  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, id_array, 5, metric_array));

  _handle->fddev = fddev;

  free(metric_array);
}

/**
* @test    test_metric_03
* @brief   Tests: xfpga_fpgaGetMetricsByName
* @details Validates get metric value by name
*
*/
TEST_P(metrics_c_p, test_metric_04) {
  struct _fpga_handle *_handle = NULL;

  const char *metric_string[2] = {"power_mgmt:consumed",
                                  "performance:fabric:port0:mmio_read"};
  uint64_t array_size = 2;

  struct fpga_metric *metric_array_search =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), array_size);

  xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string, array_size,
                             metric_array_search);

  const char *metric_string_invalid[2] = {
      "power_mgmtconsumed1", "performance1:fabric:port0:mmio_read1"};
  struct fpga_metric *metric_array_search_invalid =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), array_size);

  xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string_invalid,
                             array_size, metric_array_search_invalid);

  free(metric_array_search_invalid);

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByName(NULL, (char **)metric_string, array_size,
                                       metric_array_search));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsByName(handle_, NULL, array_size,
                                                metric_array_search));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string,
                                                array_size, NULL));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string,
                                                0, metric_array_search));

  _handle = (struct _fpga_handle *)handle_;

  int fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string,
                                       array_size, metric_array_search));

  _handle->fddev = fddev;
  free(metric_array_search);
}

INSTANTIATE_TEST_CASE_P(metrics_c, metrics_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));

/**
* @brief metrics afu gtest fixture
*
*/
class metrics_afu_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  metrics_afu_c_p() 
    : tokens_{{nullptr, nullptr}}, 
      handle_(nullptr) {}

  void create_metric_bbb_dfh();
  void create_metric_bbb_csr();
  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);

    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    which_mmio_ = 0;
    uint64_t *mmio_ptr = nullptr;
    EXPECT_EQ(xfpga_fpgaMapMMIO(handle_, which_mmio_, &mmio_ptr), FPGA_OK);
    EXPECT_NE(mmio_ptr, nullptr);
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaUnmapMMIO(handle_, which_mmio_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    if (handle_) {
      EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK);
      handle_ = nullptr;
    }

    system_->finalize();
  }

  uint32_t which_mmio_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

void metrics_afu_c_p::create_metric_bbb_dfh() {
  struct DFH dfh;
  dfh.id = 0x1;
  dfh.revision = 0;
  dfh.next_header_offset = 0x100;
  dfh.eol = 1;
  dfh.reserved = 0;
  dfh.type = 0x1;

  printf("------dfh.csr = %lx \n", dfh.csr);
  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, 0x0, dfh.csr));

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x8, 0xf89e433683f9040b));
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x10, 0xd8424dc4a4a3c413));

  struct DFH dfh_bbb = {0};

  dfh_bbb.type = 0x2;
  dfh_bbb.id = 0x1;
  dfh_bbb.revision = 0;
  dfh_bbb.next_header_offset = 0x000;
  dfh_bbb.eol = 1;
  dfh_bbb.reserved = 0;
  printf("------dfh_bbb.csr = %lx \n", dfh_bbb.csr);

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, 0x100, dfh_bbb.csr));

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x108, 0x9D73E8F258E9E3D7));
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x110, 0x87816958C1484CD0));
}

void metrics_afu_c_p::create_metric_bbb_csr() {
  struct metric_bbb_group group_csr = {0};
  struct metric_bbb_value value_csr = {0};

  group_csr.eol = 0;
  group_csr.group_id = 0x2;
  group_csr.units = 0x2;
  group_csr.next_group_offset = 0x30;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, 0x120, group_csr.csr));
  printf("------group_csr.csr = %lx \n", group_csr.csr);

  value_csr.eol = 0x0;
  value_csr.counter_id = 0xa;
  value_csr.value = 0x99;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, 0x128, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  value_csr.eol = 0x1;
  value_csr.counter_id = 0xb;
  value_csr.value = 0x89;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, 0x130, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  // second group
  group_csr.eol = 1;
  group_csr.group_id = 0x3;
  group_csr.units = 0x3;
  group_csr.next_group_offset = 0x0;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x120 + 0x30, group_csr.csr));
  printf("------group_csr.csr = %lx \n", group_csr.csr);
  // second value
  value_csr.eol = 0x0;
  value_csr.counter_id = 0xc;
  value_csr.value = 0x79;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x120 + 0x38, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  value_csr.eol = 0x1;
  value_csr.counter_id = 0xd;
  value_csr.value = 0x69;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(handle_, 0, 0x120 + 0x40, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);
}

/**
* @test    test_afc_metric_01
* @brief   Tests: xfpga_fpgaGetNumMetrics
*                 xfpga_fpgaGetNumMetrics functions
* @details Validates AFU get number metrics &
*          get metrics info
*
*/
TEST_P(metrics_afu_c_p, test_afc_metric_01) {
  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  uint64_t num_metrics = 0;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, &num_metrics));
  printf("num_metrics =%ld \n", num_metrics);

  struct fpga_metric_info *fpga_metric_info = (struct fpga_metric_info *)calloc(
         sizeof(struct fpga_metric_info), num_metrics);

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaGetMetricsInfo(handle_, fpga_metric_info, &num_metrics));

  free(fpga_metric_info);
}

/**
* @test    test_afc_metric_02
* @brief   Tests: xfpga_fpgaGetNumMetrics
*                 xfpga_fpgaGetNumMetrics
*                 xfpga_fpgaGetMetricsByIndex functions
* @details Validates no AFU metrics
*
*/
TEST_P(metrics_afu_c_p, test_afc_metric_02) {
  uint64_t num_metrics = 0;
  auto handle = (struct _fpga_handle*)handle_;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumMetrics(handle_, &num_metrics));
  printf("num_metrics =%ld \n", num_metrics);
  EXPECT_EQ(FPGA_OK, free_fpga_enum_metrics_vector(handle));

  // No AFU metrics
  struct fpga_metric_info *fpga_metric_info = (struct fpga_metric_info *)calloc(
         sizeof(struct fpga_metric_info), num_metrics);

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsInfo(handle_, fpga_metric_info, &num_metrics));
  free(fpga_metric_info);

  EXPECT_EQ(FPGA_OK, free_fpga_enum_metrics_vector(handle));

  // No AFU metrics
  uint64_t id_array[] = {1, 2, 3};
  struct fpga_metric *metric_array =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), 3);

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, id_array, 3, metric_array));

  free(metric_array);
}

/**
* @test    test_afc_metric_03
* @brief   Tests: xfpga_fpgaGetMetricsByIndex functions
* @details Validates AFU get metric value by index
*
*/
TEST_P(metrics_afu_c_p, test_afc_metric_03) {
  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  // valid index
  uint64_t id_array[] = {1, 2, 3};
  struct fpga_metric *metric_array =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), 3);

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, id_array, 3, metric_array));

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByIndex(handle_, id_array, 0, metric_array));

  free(metric_array);

  // invalid index
  uint64_t id_array_invalid[] = {10, 20, 30};
  struct fpga_metric *metric_array_invalid =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), 3);

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetMetricsByIndex(handle_, id_array_invalid, 3,
                                                 metric_array_invalid));

  free(metric_array_invalid);
}

/**
* @test    test_afc_metric_03
* @brief   Tests: xfpga_fpgaGetMetricsByName functions
* @details Validates AFU get metric value by name
*
*/
TEST_P(metrics_afu_c_p, test_afc_metric_04) {
  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  // valid afu Metrics name
  const char *metric_string[2] = {"AFU:2:a", "AFU:3:b"};
  uint64_t array_size = 2;

  struct fpga_metric *metric_array_search =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), array_size);

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string,
                                       array_size, metric_array_search));

  free(metric_array_search);

  // invalid afu Metrics name
  const char *metric_string_invalid[2] = {"power_mgmt:consumed",
                                          "performance:fabric:port0:mmio_read"};

  metric_array_search =
         (struct fpga_metric *)calloc(sizeof(struct fpga_metric), array_size);

  EXPECT_NE(FPGA_OK,
            xfpga_fpgaGetMetricsByName(handle_, (char **)metric_string_invalid,
                                       array_size, metric_array_search));

  free(metric_array_search);
}
INSTANTIATE_TEST_CASE_P(metrics_c, metrics_afu_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));
