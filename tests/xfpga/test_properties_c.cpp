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
#include "props.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class properties_c_p : public opae_p<xfpga_> {
 protected:
  properties_c_p() :
    device_(nullptr),
    prop_(nullptr)
  {}

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual void SetUp() override {
    opae_p<xfpga_>::SetUp();

    EXPECT_EQ(xfpga_fpgaOpen(device_token_, &device_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaClose(device_), FPGA_OK);
    device_ = nullptr;

    if (prop_) {
      EXPECT_EQ(fpgaDestroyProperties(&prop_), FPGA_OK);
      prop_ = nullptr;
    }

    opae_p<xfpga_>::TearDown();
  }

  fpga_handle device_;
  fpga_properties prop_;
};

/**
 * @test    from_handle
 * @brief   Tests: xfpga_fpgaGetPropertiesFromHandle
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaGetPropertiesFromHandle with an valid handle,
 *          expected result is FPGA_OK.<br>
 */
TEST_P(properties_c_p, from_handle) {
  EXPECT_EQ(xfpga_fpgaGetPropertiesFromHandle(accel_, &prop_), FPGA_OK);
}

/**
 * @test       vendor_id_afu
 *
 * @brief      When querying the vendor ID of an AFU,
 * 	       0x8086 is returned.
 */
TEST_P(properties_c_p, vendor_id_afu) {
#ifndef BUILD_ASE
  test_device device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(accel_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetVendorID(prop_, &x), FPGA_OK);
  EXPECT_EQ(x, device.vendor_id);
#endif
}

/**
 * @test       vendor_id_fme
 *
 * @brief      When querying the vendor ID of an FME,
 * 	       0x8086 is returned.
 */
TEST_P(properties_c_p, vendor_id_fme) {
#ifndef BUILD_ASE
  test_device device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(device_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetVendorID(prop_, &x), FPGA_OK);
  EXPECT_EQ(x, device.vendor_id);
#endif
}

/**
 * @test       device_id_afu
 *
 * @brief      When querying the device ID of an AFU,
 * 	       0xbcc0 is returned.
 */
TEST_P(properties_c_p, device_id_afu) {
#ifndef BUILD_ASE
  test_device device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(accel_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetDeviceID(prop_, &x), FPGA_OK);
  uint16_t expected_id;
  if (!device.has_afu)
    expected_id = device.device_id; // Port on the PF.
  else
    expected_id = device.device_id + (device.num_vfs > 0 ? 1 : 0);
  EXPECT_EQ(static_cast<uint32_t>(x), expected_id);
#endif
}

/**
 * @test       device_id_fme
 *
 * @brief      When querying the device ID of an FME,
 * 	       0xbcc0 is returned.
 */
TEST_P(properties_c_p, device_id_fme) {
#ifndef BUILD_ASE
  test_device device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(device_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetDeviceID(prop_, &x), FPGA_OK);
  EXPECT_EQ(static_cast<uint32_t>(x), device.device_id);
#endif
}

/**
 * @test       valid_gets
 *
 * @brief      When fpgaGetPropertiesFromHandle is called
 *             with a valid handle.
 *             The function returns FPGA_OK with the
 *             returned valid properties for that handle.
 */
TEST_P(properties_c_p, valid_gets) {
  fpga_objtype objtype = FPGA_DEVICE;

  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(accel_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetObjectType(prop_, &objtype), FPGA_OK);
  EXPECT_EQ(objtype, FPGA_ACCELERATOR);
  EXPECT_EQ(fpgaDestroyProperties(&prop_), FPGA_OK);

  prop_ = NULL;
  objtype = FPGA_ACCELERATOR;

  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(device_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetObjectType(prop_, &objtype), FPGA_OK);
  EXPECT_EQ(objtype, FPGA_DEVICE);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(properties_c_p);
INSTANTIATE_TEST_SUITE_P(properties_c, properties_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));

/**
 * @test    fpga_get_properties
 * @brief   Tests: xfpga_fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaGetProperties with an invalid token,
 *          expected result is FPGA_INVALID_PARAM.<br>
 */
TEST(properties_c, fpga_get_properties) {
  char buf[sizeof(_fpga_token)];
  fpga_token token = buf;
  ((_fpga_token*)token)->hdr.magic = 0xbeef;
  fpga_properties prop;
  EXPECT_EQ(xfpga_fpgaGetProperties(token, &prop), FPGA_INVALID_PARAM);
}

/**
 * @test    invalid_handle
 * @brief   Tests: xfpga_fpgaGetPropertiesFromHandle
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaGetPropertiesFromHandle with an null handle,
 *          expected result is FPGA_INVALID_PARAM.<br>
 */
TEST(properties_c, invalid_handle) {
  fpga_properties prop = nullptr;
  EXPECT_EQ(xfpga_fpgaGetPropertiesFromHandle(nullptr, &prop), FPGA_INVALID_PARAM);
}
