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

#include <linux/ioctl.h>

#include "fpga-dfl.h"
#include "mock/opae_fixtures.h"

using namespace opae::testing;

static int mmio_ioctl(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct dfl_fpga_port_region_info *rinfo = va_arg(argp, struct dfl_fpga_port_region_info *);
    if (!rinfo) {
      OPAE_MSG("rinfo is NULL");
      goto out_EINVAL;
    }
    if (rinfo->argsz != sizeof(*rinfo)) {
      OPAE_MSG("wrong structure size");
      goto out_EINVAL;
    }
    if (rinfo->index > 1 ) {
      OPAE_MSG("unsupported MMIO index");
      goto out_EINVAL;
    }
    if (rinfo->padding != 0) {
      OPAE_MSG("unsupported padding");
      goto out_EINVAL;
    }
    rinfo->flags = DFL_PORT_REGION_READ | DFL_PORT_REGION_WRITE | DFL_PORT_REGION_MMAP;
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

class mmio_c_p : public opae_p<> {
 protected:
  mmio_c_p() :
    which_mmio_(0)
  {}

  virtual void SetUp() override
  {
    opae_p<>::SetUp();
    system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    which_mmio_ = 0;
    uint64_t *mmio_ptr = nullptr;
    EXPECT_EQ(fpgaMapMMIO(accel_, which_mmio_, &mmio_ptr), FPGA_OK);
    EXPECT_NE(mmio_ptr, nullptr);
  }

  virtual void TearDown() override
  {
    EXPECT_EQ(fpgaUnmapMMIO(accel_, which_mmio_), FPGA_OK);
    opae_p<>::TearDown();
  }

  uint32_t which_mmio_;
  const uint64_t CSR_SCRATCHPAD0 = 0x100;
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
 * @test       mmio64_neg_test
 * @brief      Test: fpgaWriteMMIO64, fpgaReadMMIO64
 * @details    Write the scratchpad register with fpgaWriteMMIO64 using invalid handle,<br>
 *             read the register back with fpgaReadMMIO64 using invalid handle.<br>
 *             then, API should return FPGA_INVALID_PARAM.<br>
 */
TEST_P(mmio_c_p, mmio64_neg_test) {
  const uint64_t val_written = 0xdeadbeefdecafbad;
  EXPECT_EQ(fpgaWriteMMIO64(NULL, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_INVALID_PARAM);
  uint64_t val_read = 0;
  EXPECT_EQ(fpgaReadMMIO64(NULL, which_mmio_,
                           CSR_SCRATCHPAD0, &val_read), FPGA_INVALID_PARAM);
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
 * @test       mmio32_neg_test
 * @brief      Test: fpgaWriteMMIO32, fpgaReadMMIO32
 * @details    Write the scratchpad register with fpgaWriteMMIO32 using invalid handle<br>
 *             read the register back with fpgaReadMMIO32 using invalid handle<br>
 *             then, API should return FPGA_INVALID_PARAM.<br>
 */
TEST_P(mmio_c_p, mmio32_neg_test) {
  const uint32_t val_written = 0xc0cac01a;
  EXPECT_EQ(fpgaWriteMMIO32(NULL, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_INVALID_PARAM);
  uint32_t val_read = 0;
  EXPECT_EQ(fpgaReadMMIO32(NULL, which_mmio_,
                           CSR_SCRATCHPAD0, &val_read), FPGA_INVALID_PARAM);
}

TEST_P(mmio_c_p, fpgaMapMMIO_neg_test) {
    uint64_t *mmio_ptr = nullptr;
    EXPECT_EQ(fpgaMapMMIO(NULL, which_mmio_, &mmio_ptr), FPGA_INVALID_PARAM);
    EXPECT_EQ(fpgaUnmapMMIO(NULL, which_mmio_), FPGA_INVALID_PARAM);
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
TEST_P(mmio_c_p, mmio512_neg_test) {
  uint64_t val_written[8];
  int i;
  for (i = 0; i < 8; i++) {
    val_written[i] = 0xdeadbeefdecafbad << (i + 1);
  }
  EXPECT_EQ(fpgaWriteMMIO512(NULL, which_mmio_,
                            CSR_SCRATCHPAD0, val_written), FPGA_INVALID_PARAM);
  for (i = 0; i < 8; i++) {
    uint64_t val_read = 0;
    EXPECT_EQ(fpgaReadMMIO64(NULL, which_mmio_,
                             CSR_SCRATCHPAD0, &val_read), FPGA_INVALID_PARAM);
  }
}
#endif // TEST_SUPPORTS_AVX512

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(mmio_c_p);
INSTANTIATE_TEST_SUITE_P(mmio_c, mmio_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000"
                                                                      })));
