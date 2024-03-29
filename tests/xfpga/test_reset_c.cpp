// Copyright(c) 2017-2022, Intel Corporation
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
#include "types_int.h"
#include "xfpga.h"
#include "fpga-dfl.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

#include <linux/ioctl.h>

using namespace opae::testing;

class reset_c_p : public opae_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

};

/**
 * @test       reset_c
 * @brief      test_port_drv_reset
 * @details    When the parameters are invalid and the drivers are loaded,
 *             xfpga_fpgaReset returns FPGA_INVALID_PARAM.
 *
 */
TEST_P(reset_c_p, test_port_drv_reset) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReset(NULL));
}

/**
 * @test       reset_c
 * @brief      test_port_drv_reset_02
 * @details    When the parameters are invalid and the drivers are
 *             loaded, xfpga_fpgaReset return error.
 *
 */
TEST_P(reset_c_p, test_port_drv_reset_02) {

  // Reset slot
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReset(NULL));

  struct _fpga_handle* _handle = (struct _fpga_handle*)accel_;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, xfpga_fpgaReset(accel_));

  _handle->magic = FPGA_HANDLE_MAGIC;
}

/**
 * @test       reset_c
 * @brief      test_port_drv_reset_03
 * @details    When the fddev is invalid and the drivers are
 *             loaded, xfpga_fpgaReset return error.
 *
 */
TEST_P(reset_c_p, test_port_drv_reset_03) {
  int fddev = -1;
  struct _fpga_handle* _handle = (struct _fpga_handle*)accel_;

#ifndef BUILD_ASE
  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, xfpga_fpgaReset(accel_));
#else
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReset(accel_));
#endif
  _handle->fddev = fddev;
}

/**
 * @test       reset_c
 * @brief      valid_port_reset
 * @details    When the handle is valid and the drivers are
 *             loaded, xfpga_fpgaReset return FPGA_OK.
 *
 */
TEST_P(reset_c_p, valid_port_reset) {
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReset(accel_));
} 

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(reset_c_p);
INSTANTIATE_TEST_SUITE_P(reset_c, reset_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                         "dfl-c6100"
                                                                      })));

class reset_c_mock_p : public reset_c_p {};

/**
 * @test       reset_c
 * @brief      test_port_drv_reset_01
 * @details    When the parameters are valid and the drivers are loaded,
 *             xfpga_fpgaReset returns FPGA_EXCEPTION.
 *
 */
TEST_P(reset_c_mock_p, test_port_drv_reset_01) {
  system_->register_ioctl_handler(DFL_FPGA_PORT_RESET, dummy_ioctl<-1, EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReset(accel_));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(reset_c_mock_p);
INSTANTIATE_TEST_SUITE_P(reset_c, reset_c_mock_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));
