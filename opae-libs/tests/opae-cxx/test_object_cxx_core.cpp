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
#include <opae/cxx/core/sysobject.h>

#include <array>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;
using namespace opae::fpga::types;

class sysobject_cxx_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysobject_cxx_p() {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);

    properties::ptr_t props = properties::get(FPGA_ACCELERATOR);
    props->device_id = platform_.devices[0].device_id;

    tokens_ = token::enumerate({props});
    ASSERT_GT(tokens_.size(), 0);
    handle_ = handle::open(tokens_[0], 0);
    ASSERT_NE(handle_.get(), nullptr);

    properties::ptr_t props_dev = properties::get(FPGA_DEVICE);
    props_dev->device_id = platform_.devices[0].device_id;

    tokens_dev_ = token::enumerate({props_dev});
    ASSERT_GT(tokens_dev_.size(), 0);
    handle_dev_ = handle::open(tokens_dev_[0], 0);
    ASSERT_NE(handle_dev_.get(), nullptr);
  }

  virtual void TearDown() override {
    tokens_.clear();
    tokens_dev_.clear();
    handle_->close();
    handle_.reset();
    handle_dev_.reset();
    fpgaFinalize();
    system_->finalize();
  }

  std::vector<token::ptr_t> tokens_;
  std::vector<token::ptr_t> tokens_dev_;
  handle::ptr_t handle_;
  handle::ptr_t handle_dev_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

/**
 * @btest token_object
 * Given an enumerated token object
 * When I get the afui_id as an object from
 * the token
 * And I  get the object's buffer using the bytes function
 * Then the normalized GUID is the same as the normalized GUID of the test
 * platform.
 */
TEST_P(sysobject_cxx_p, token_object) {
  auto obj = sysobject::get(tokens_[0], "afu_id");
  ASSERT_NE(obj.get(), nullptr);
  auto bytes = obj->bytes();
  ASSERT_NE(bytes.size(), 0);
  auto guid_read = std::string(bytes.begin(), bytes.end());
  auto afu_guid = std::string(platform_.devices[0].afu_guid);
  system_->normalize_guid(guid_read);
  system_->normalize_guid(afu_guid);
  ASSERT_STREQ(afu_guid.c_str(), guid_read.c_str());
}

/**
 * @btest handle_object
 * Given an open handle object
 * When I get the afui_id as an object from
 * the handle
 * And I  get the object's buffer using the bytes function
 * Then the normalized GUID is the same as the normalized GUID of the test
 * platform.
 */
TEST_P(sysobject_cxx_p, handle_object) {
  auto obj = sysobject::get(handle_, "afu_id");
  ASSERT_NE(obj.get(), nullptr);
  auto bytes = obj->bytes();
  ASSERT_NE(bytes.size(), 0);
  auto guid_read = std::string(bytes.begin(), bytes.end());
  auto afu_guid = std::string(platform_.devices[0].afu_guid);
  system_->normalize_guid(guid_read);
  system_->normalize_guid(afu_guid);
  ASSERT_STREQ(afu_guid.c_str(), guid_read.c_str());
}

/**
 * @btest handle_object_write
 * Given an open handle object
 * When I get a suboject from the handle
 * And I  write a 64-bit value to it using its write64 function
 * Then no exceptions are thrown
 */
TEST_P(sysobject_cxx_p, handle_object_write) {
  std::string path = "iperf/fabric/freeze";;

  if (platform_.devices[0].device_id == 0x09c4 || 
      platform_.devices[0].device_id == 0x09c5 ||
      platform_.devices[0].device_id == 0x0b30) {
    path = "dperf/fabric/freeze";
  }

  auto obj = sysobject::get(handle_dev_, path);
  ASSERT_NE(obj.get(), nullptr);
  EXPECT_NO_THROW(obj->write64(0x1));
  EXPECT_NO_THROW(obj->write64(0x0));
}

/**
 * @btest object_object
 * Given an object from an enumerated token
 * And an object from an open handle
 * When I use read64 from both objects
 * Then the values are the same
 */
TEST_P(sysobject_cxx_p, object_object) {
  auto t_obj = sysobject::get(tokens_[0], "errors");
  ASSERT_NE(t_obj.get(), nullptr);
  auto t_value = t_obj->get("errors")->read64();
  auto h_obj = sysobject::get(handle_, "errors");
  ASSERT_NE(h_obj.get(), nullptr);
  auto h_value = h_obj->get("errors")->read64();
  EXPECT_EQ(t_value, h_value);
  EXPECT_EQ(t_obj->get("abc").get(), nullptr);
  EXPECT_EQ(h_obj->get("abc").get(), nullptr);
}

/**
 * @btest token_subobject_write
 * Given an object from an enumerated token
 * And a suboject from that object
 * When I use write64 from the token subobject
 * Then an invalid_param exception is thrown
 */
TEST_P(sysobject_cxx_p, token_subobject_write) {
  auto t_obj = sysobject::get(tokens_[0], "errors");
  ASSERT_NE(t_obj.get(), nullptr);
  EXPECT_THROW(t_obj->get("errors")->write64(0x100),
               opae::fpga::types::invalid_param);
}

/**
 * @btest handle_subobject_write
 * Given an object from an open handle
 * And a suboject from that object
 * When I use write64 from the token subobject
 * Then no exceptions are thrown
 * And the value has changed from its original value
 */
TEST_P(sysobject_cxx_p, handle_subobject_write) {
  std::string path = "iperf/fabric";;

  if (platform_.devices[0].device_id == 0x09c4 ||
      platform_.devices[0].device_id == 0x09c5 ||
      platform_.devices[0].device_id == 0x0b30) {
    path = "dperf/fabric";
  }

  auto h_obj = sysobject::get(handle_dev_, path);
  ASSERT_NE(h_obj.get(), nullptr);
  ASSERT_NO_THROW(h_obj->get("freeze")->read64(FPGA_OBJECT_SYNC));
  ASSERT_NO_THROW(h_obj->get("freeze")->write64(0x1));
  ASSERT_NO_THROW(h_obj->get("freeze")->read64(FPGA_OBJECT_SYNC));
  ASSERT_NO_THROW(h_obj->get("freeze")->write64(0x0));
  EXPECT_EQ(h_obj->get("freeze")->read64(FPGA_OBJECT_SYNC), 0x0);
}

/**
 * @btest read_bytes
 * Given an enumerated token object
 * And its afu_id as an object from the token
 * And I  read an arbitrary number of bytes from an arbitrary offset
 * Then the string made from those bytes are equal to the string made
 * from the test_platform afu_id using the same size and offset
 */
TEST_P(sysobject_cxx_p, read_bytes) {
  // get the test platform GUID and normalize it to exclude hyphens
  std::string test_guid(platform_.devices[0].afu_guid);
  system_->normalize_guid(test_guid, false);

  for (int i = 0; i < test_guid.size(); ++i) {
    for (int  j = 1; j < test_guid.size() - i; j++) {
      int offset = i;
      int size = j;
      ASSERT_LE(offset + size, test_guid.size());

      auto obj = sysobject::get(tokens_[0], "afu_id");
      // get size bytes starting form the offset
      auto bytes = obj->bytes(offset, size);
      // make this a string
      auto str1 = std::string(bytes.begin(), bytes.end());
      // make substring from the offset and size used before
      auto str2 = std::string(test_guid, offset, size);
      ASSERT_TRUE(std::equal(str1.begin(), str1.end(), str2.begin(),
                             [](char lhs, char rhs) {
                               return std::tolower(lhs) == std::tolower(rhs);
                             }));

    }

  }
}

INSTANTIATE_TEST_CASE_P(sysobject_cxx, sysobject_cxx_p,
         ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc","dcp-vc" })));

