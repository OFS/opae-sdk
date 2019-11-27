// Copyright(c) 2017-2018, Intel Corporation
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
#include <opae/fpga.h>
#include <algorithm>
#include "gtest/gtest.h"
#include "test_system.h"
#include "types_int.h"
#include "xfpga.h"
#include "props.h"
#include "sysfs_int.h"

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);

}
using namespace opae::testing;

class properties_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  properties_c_p()
      : tokens_accel_{{nullptr, nullptr}},
        tokens_dev_{{nullptr, nullptr}},
        prop_(nullptr),
        filter_accel_(nullptr),
        filter_dev_(nullptr),
        handle_accel_(nullptr),
        handle_dev_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_accel_), FPGA_OK);
    auto accel_dev_id = platform_.devices[0].device_id + platform_.devices[0].num_vfs;
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_accel_, accel_dev_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_accel_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_accel_, 1, tokens_accel_.data(), tokens_accel_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    ASSERT_EQ(xfpga_fpgaOpen(tokens_accel_[0], &handle_accel_, 0), FPGA_OK);
    ASSERT_EQ(fpgaClearProperties(filter_accel_), FPGA_OK);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_dev_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_dev_,
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_dev_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_dev_, 1, tokens_dev_.data(), tokens_dev_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    ASSERT_EQ(xfpga_fpgaOpen(tokens_dev_[0], &handle_dev_, 0), FPGA_OK);
    ASSERT_EQ(fpgaClearProperties(filter_dev_), FPGA_OK);

    num_matches_ = 0xc01a;
    invalid_device_ = test_device::unknown();
    prop_ = nullptr;
  }

  virtual void TearDown() override {
    if (prop_) { EXPECT_EQ(fpgaDestroyProperties(&prop_), FPGA_OK); };

    if (filter_accel_) { EXPECT_EQ(fpgaDestroyProperties(&filter_accel_), FPGA_OK); }
    if (filter_dev_) { EXPECT_EQ(fpgaDestroyProperties(&filter_dev_), FPGA_OK); }
    if (handle_accel_) { EXPECT_EQ(xfpga_fpgaClose(handle_accel_), FPGA_OK); }
    if (handle_dev_) { EXPECT_EQ(xfpga_fpgaClose(handle_dev_), FPGA_OK); }

    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

    for (auto &t : tokens_dev_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_accel_;
  std::array<fpga_token, 2> tokens_dev_;
  fpga_properties prop_;
  fpga_properties filter_accel_;
  fpga_properties filter_dev_;
  fpga_handle handle_accel_;
  fpga_handle handle_dev_;
  uint32_t num_matches_;
  test_platform platform_;
  test_device invalid_device_;
  test_system* system_;
};

/**
 * @test    from_handle
 * @brief   Tests: xfpga_fpgaGetPropertiesFromHandle
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaGetPropertiesFromHandle with an valid handle,
 *          expected result is FPGA_OK.<br>
 */
TEST_P(properties_c_p, from_handle) {
  EXPECT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_accel_, &prop_), FPGA_OK);
}

/**
 * @test       vendor_id_afu
 *
 * @brief      When querying the vendor ID of an AFU,
 * 	       0x8086 is returned.
 */
TEST_P(properties_c_p, vendor_id_afu) {
#ifndef BUILD_ASE
  auto device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_accel_, &prop_), FPGA_OK);
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
  auto device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_dev_, &prop_), FPGA_OK);
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
  auto device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_accel_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetDeviceID(prop_, &x), FPGA_OK);
  auto expected_id = device.device_id + (device.num_vfs > 0 ? 1 : 0);
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
  auto device = platform_.devices[0];
  uint16_t x = 0;
  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_dev_, &prop_), FPGA_OK);
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

  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_accel_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetObjectType(prop_, &objtype), FPGA_OK);
  EXPECT_EQ(objtype, FPGA_ACCELERATOR);
  EXPECT_EQ(fpgaDestroyProperties(&prop_), FPGA_OK);

  prop_ = NULL;
  objtype = FPGA_ACCELERATOR;

  ASSERT_EQ(xfpga_fpgaGetPropertiesFromHandle(handle_dev_, &prop_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetObjectType(prop_, &objtype), FPGA_OK);
  EXPECT_EQ(objtype, FPGA_DEVICE);
}

INSTANTIATE_TEST_CASE_P(properties_c, properties_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));

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
  ((_fpga_token*)token)->magic = 0xbeef;
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
  fpga_properties prop = NULL;
  EXPECT_EQ(xfpga_fpgaGetPropertiesFromHandle(nullptr, &prop), FPGA_INVALID_PARAM);
}
