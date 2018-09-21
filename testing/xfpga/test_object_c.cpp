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
#include "gtest/gtest.h"
#include "test_system.h"
#include "xfpga.h"

using namespace opae::testing;

class sysobject_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysobject_p() : tmpsysfs_("mocksys-XXXXXX") {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &dev_filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(dev_filter_, FPGA_DEVICE),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &acc_filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(acc_filter_, FPGA_ACCELERATOR),
              FPGA_OK);
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  std::string tmpsysfs_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
  std::array<fpga_token, 2> tokens_;
  fpga_properties dev_filter_;
  fpga_properties acc_filter_;
};

TEST_P(sysobject_p, xfpga_fpgaTokenGetObject) {
  uint32_t num_matches = 0;
  ASSERT_EQ(xfpga_fpgaEnumerate(&dev_filter_, 1, tokens_.data(), tokens_.size(),
                                &num_matches),
            FPGA_OK);
  const char *name = "bitstream_id";
  fpga_object object;
  int flags = 0;
  EXPECT_EQ(xfpga_fpgaTokenGetObject(tokens_[0], name, &object, flags),
            FPGA_OK);
  uint64_t bitstream_id = 0;
  EXPECT_EQ(xfpga_fpgaObjectRead64(object, &bitstream_id, FPGA_OBJECT_TEXT), FPGA_OK);
  EXPECT_EQ(bitstream_id, platform_.devices[0].bbs_id);
  EXPECT_EQ(xfpga_fpgaTokenGetObject(tokens_[0], "invalid_name", &object, 0), FPGA_NOT_FOUND);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

// TEST_P(sysobject_p, xfpga_fpgaHandleGetObject) {
//   fpga_token handle = 0;
//   const char *name = 0;
//   fpga_object *object = 0;
//   int flags = 0;
//   auto res = xfpga_fpgaHandleGetObject(handle, name, object, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST_P(sysobject_p, xfpga_fpgaObjectGetObject) {
//   fpga_object parent = 0;
//   fpga_handle handle = 0;
//   const char *name = 0;
//   fpga_object *object = 0;
//   int flags = 0;
//   auto res = xfpga_fpgaObjectGetObject(parent, handle, name, object, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST_P(sysobject_p, xfpga_fpgaDestroyObject) {
//   fpga_object *obj = 0;
//   auto res = xfpga_fpgaDestroyObject(obj);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST_P(sysobject_p, xfpga_fpgaObjectRead64) {
//   fpga_object obj = 0;
//   uint64_t *value = 0;
//   int flags = 0;
//   auto res = xfpga_fpgaObjectRead64(obj, value, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST_P(sysobject_p, xfpga_fpgaObjectRead) {
//   fpga_object obj = 0;
//   uint8_t *buffer = 0;
//   int offset = 0;
//   int len = 0;
//   int flags = 0;
//   auto res = xfpga_fpgaObjectRead(obj, buffer, offset, len, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST_P(sysobject_p, xfpga_fpgaObjectWrite64) {
//   fpga_object obj = 0;
//   uint64_t value = 0;
//   int flags = 0;
//   auto res = xfpga_fpgaObjectWrite64(obj, value, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }

INSTANTIATE_TEST_CASE_P(sysobject_c, sysobject_p,
                        ::testing::ValuesIn(test_platform::keys(true)));
