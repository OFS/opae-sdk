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
#include "fpga-dfl.h"
#include "intel-fpga.h"
#include <linux/ioctl.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

static int mmio_ioctl(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
    if (!rinfo) {
      FPGA_MSG("rinfo is NULL");
      goto out_EINVAL;
    }
    if (rinfo->argsz != sizeof(*rinfo)) {
      FPGA_MSG("wrong structure size");
      goto out_EINVAL;
    }
    if (rinfo->index > 1 ) {
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

class mmio_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  mmio_c_p() : tokens_{{nullptr, nullptr}} {}

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
                            &num_matches_), FPGA_OK);

    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);

    which_mmio_ = 0;
    uint64_t *mmio_ptr = nullptr;
    EXPECT_EQ(fpgaMapMMIO(accel_, which_mmio_, &mmio_ptr), FPGA_OK);
    EXPECT_NE(mmio_ptr, nullptr);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaUnmapMMIO(accel_, which_mmio_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
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

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle accel_;
  uint32_t which_mmio_;
  const uint64_t CSR_SCRATCHPAD0 = 0x100;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       mmio64
 * @brief      Test: fpgaWriteMMIO64, fpgaReadMMIO64
 * @details    Write the scratchpad register with fpgaWriteMMIO64,<br>
 *             read the register back with fpgaReadMMIO64.<br>
 *             Value written should equal value read.<br>
 */
TEST_P(mmio_c_p, mmio64) {
  const uint64_t val_written = 0xdeadbeefdecafbad;
  EXPECT_EQ(fpgaWriteMMIO64(accel_, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_OK);
  uint64_t val_read = 0;
  EXPECT_EQ(fpgaReadMMIO64(accel_, which_mmio_,
                           CSR_SCRATCHPAD0, &val_read), FPGA_OK);
  EXPECT_EQ(val_written, val_read);
}

/**
 * @test       mmio32
 * @brief      Test: fpgaWriteMMIO32, fpgaReadMMIO32
 * @details    Write the scratchpad register with fpgaWriteMMIO32,<br>
 *             read the register back with fpgaReadMMIO32.<br>
 *             Value written should equal value read.<br>
 */
TEST_P(mmio_c_p, mmio32) {
  const uint32_t val_written = 0xc0cac01a;
  EXPECT_EQ(fpgaWriteMMIO32(accel_, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_OK);
  uint32_t val_read = 0;
  EXPECT_EQ(fpgaReadMMIO32(accel_, which_mmio_,
                           CSR_SCRATCHPAD0, &val_read), FPGA_OK);
  EXPECT_EQ(val_written, val_read);
}

/**
 * @test       mmio512
 * @brief      Test: fpgaWriteMMIO512
 * @details    Write the scratchpad register with fpgaWriteMMIO512,<br>
 *             read the register back with fpgaReadMMIO64.<br>
 *             Value written should equal value read.<br>
 */
#ifdef TEST_SUPPORTS_AVX512
TEST_P(mmio_c_p, mmio512) {
  uint64_t val_written[8];
  int i;
  for (i = 0; i < 8; i++) {
    val_written[i] = 0xdeadbeefdecafbad << (i + 1);
  }
  EXPECT_EQ(fpgaWriteMMIO512(accel_, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_OK);
  for (i = 0; i < 8; i++) {
    uint64_t val_read = 0;
    EXPECT_EQ(fpgaReadMMIO64(accel_, which_mmio_,
                             CSR_SCRATCHPAD0, &val_read), FPGA_OK);
    EXPECT_EQ(val_written[i], val_read);
  }
}
#endif // TEST_SUPPORTS_AVX512

INSTANTIATE_TEST_CASE_P(mmio_c, mmio_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));
