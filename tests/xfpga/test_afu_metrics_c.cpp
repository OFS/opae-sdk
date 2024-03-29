// Copyright(c) 2018-2022, Intel Corporation
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

extern "C" {
#include "types_int.h"
#include "sysfs_int.h"
#include "metrics/metrics_int.h"
#include "metrics/vector.h"
#include "opae_int.h"
#include "xfpga.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

#include <linux/ioctl.h>
#include "intel-fpga.h"

#define NO_OPAE_C
#include "mock/opae_fixtures.h"
KEEP_XFPGA_SYMBOLS

using namespace opae::testing;

int mmio_ioctl(mock_object *m, int request, va_list argp) {
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct fpga_port_region_info *rinfo =
      va_arg(argp, struct fpga_port_region_info *);
  if (!rinfo) {
    OPAE_MSG("rinfo is NULL");
    goto out_EINVAL;
  }
  if (rinfo->argsz != sizeof(*rinfo)) {
    OPAE_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (rinfo->index > 1) {
    OPAE_MSG("unsupported MMIO index");
    goto out_EINVAL;
  }
  if (rinfo->padding != 0) {
    OPAE_MSG("unsupported padding");
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

class afu_metrics_c_p : public opae_p<xfpga_> {
 protected:
  afu_metrics_c_p() :
    which_mmio_(0)
  {}

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual void SetUp() override {
    opae_p<xfpga_>::SetUp();

    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    which_mmio_ = 0;
    uint64_t *mmio_ptr = nullptr;
    EXPECT_EQ(xfpga_fpgaMapMMIO(accel_, which_mmio_, &mmio_ptr), FPGA_OK);
    EXPECT_NE(mmio_ptr, nullptr);
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaUnmapMMIO(accel_, which_mmio_), FPGA_OK);

    opae_p<xfpga_>::TearDown();
  }

  void create_metric_bbb_dfh();
  void create_metric_bbb_csr();

  uint32_t which_mmio_;
};

void afu_metrics_c_p::create_metric_bbb_dfh() {
  struct DFH dfh;
  dfh.id = 0x1;
  dfh.revision = 0;
  dfh.next_header_offset = 0x100;
  dfh.eol = 1;
  dfh.reserved = 0;
  dfh.type = 0x1;

  printf("------dfh.csr = %lx \n", dfh.csr);
  // AFU DFH
  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, 0, 0x0, dfh.csr));
  // AFU GUID
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x8, 0xf89e433683f9040b));
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x10, 0xd8424dc4a4a3c413));

  struct DFH dfh_bbb = {0};

  dfh_bbb.type = 0x2;
  dfh_bbb.id = 0x1;
  dfh_bbb.revision = 0;
  dfh_bbb.next_header_offset = 0x000;
  dfh_bbb.eol = 1;
  dfh_bbb.reserved = 0;
  printf("------dfh_bbb.csr = %lx \n", dfh_bbb.csr);

  // Metrics DFH
  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, 0, 0x100, dfh_bbb.csr));
  // Metrics GUID
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x108, 0x9D73E8F258E9E3D7));
  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x110, 0x87816958C1484CD0));
}

void afu_metrics_c_p::create_metric_bbb_csr() {
  struct metric_bbb_group group_csr = {0};
  struct metric_bbb_value value_csr = {0};

  group_csr.eol = 0;
  group_csr.group_id = 0x2;
  group_csr.units = 0x2;
  group_csr.next_group_offset = 0x30;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, 0, 0x120, group_csr.csr));
  printf("------group_csr.csr = %lx \n", group_csr.csr);

  value_csr.eol = 0x0;
  value_csr.counter_id = 0xa;
  value_csr.value = 0x99;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, 0, 0x128, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  value_csr.eol = 0x1;
  value_csr.counter_id = 0xb;
  value_csr.value = 0x89;

  EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, 0, 0x130, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  // second group
  group_csr.eol = 1;
  group_csr.group_id = 0x3;
  group_csr.units = 0x3;
  group_csr.next_group_offset = 0x0;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x120 + 0x30, group_csr.csr));
  printf("------group_csr.csr = %lx \n", group_csr.csr);
  // second value
  value_csr.eol = 0x0;
  value_csr.counter_id = 0xc;
  value_csr.value = 0x79;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x120 + 0x38, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);

  value_csr.eol = 0x1;
  value_csr.counter_id = 0xd;
  value_csr.value = 0x69;

  EXPECT_EQ(FPGA_OK,
            xfpga_fpgaWriteMMIO64(accel_, 0, 0x120 + 0x40, value_csr.csr));
  printf("------value_csr.csr = %lx \n", value_csr.csr);
}

TEST_P(afu_metrics_c_p, test_afu_metrics_01) {
  create_metric_bbb_dfh();
  uint64_t offset;
  // Valid discover
  EXPECT_EQ(FPGA_OK, discover_afu_metrics_feature(accel_, &offset));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, discover_afu_metrics_feature(accel_, NULL));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, discover_afu_metrics_feature(NULL, &offset));
}

TEST_P(afu_metrics_c_p, test_afu_metrics_02) {
  uint64_t metric_id = 0;
  fpga_metric_vector vector;

  uint64_t offset;
  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  EXPECT_EQ(FPGA_OK, discover_afu_metrics_feature(accel_, &offset));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));
  EXPECT_EQ(FPGA_OK, enum_afu_metrics(accel_, &vector, &metric_id, offset));

  // NULL input
  EXPECT_NE(FPGA_OK, enum_afu_metrics(NULL, &vector, &metric_id, offset));

  EXPECT_NE(FPGA_OK, enum_afu_metrics(accel_, NULL, &metric_id, offset));

  EXPECT_NE(FPGA_OK, enum_afu_metrics(accel_, &vector, NULL, offset));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

TEST_P(afu_metrics_c_p, test_afu_metrics_03) {
  uint64_t metric_id = 0;
  fpga_metric_vector vector;

  uint64_t offset;
  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  EXPECT_EQ(FPGA_OK, discover_afu_metrics_feature(accel_, &offset));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));
  EXPECT_EQ(FPGA_OK,
            add_afu_metrics_vector(&vector, &metric_id, 0x1234, 0x5678, 0x100));

  // NULL input
  EXPECT_NE(FPGA_OK,
            add_afu_metrics_vector(NULL, &metric_id, 0x1234, 0x5678, 0x100));

  EXPECT_NE(FPGA_OK,
            add_afu_metrics_vector(&vector, NULL, 0x1234, 0x5678, 0x100));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

TEST_P(afu_metrics_c_p, test_afu_metrics_04) {
  fpga_metric_vector vector;
  uint64_t offset;

  create_metric_bbb_dfh();
  create_metric_bbb_csr();

  struct fpga_metric fpga_metric;

  EXPECT_EQ(FPGA_OK, discover_afu_metrics_feature(accel_, &offset));

  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));
  EXPECT_NE(FPGA_OK, get_afu_metric_value(accel_, &vector, 0x1, &fpga_metric));

  // NULL input
  EXPECT_NE(FPGA_OK, get_afu_metric_value(NULL, &vector, 0x1, &fpga_metric));

  EXPECT_NE(FPGA_OK, get_afu_metric_value(accel_, NULL, 0x1, &fpga_metric));

  EXPECT_NE(FPGA_OK, get_afu_metric_value(accel_, &vector, 0x1, NULL));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(afu_metrics_c_p);
INSTANTIATE_TEST_SUITE_P(afu_metrics_c, afu_metrics_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "skx-p",
                                                                             "dcp-rc",
                                                                             "dcp-vc"
                                                                           })));
