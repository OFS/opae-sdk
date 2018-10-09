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
    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_GT(tokens_.size(), 0);
    handle_ = handle::open(tokens_[0], 0);
    ASSERT_NE(handle_.get(), nullptr);
  }

  virtual void TearDown() override {
    system_->finalize();
    tokens_.clear();
    handle_->close();
    handle_.reset();
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

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
  obj = sysobject::get(handle_, "errors/errors");
  ASSERT_NE(obj.get(), nullptr);
  EXPECT_NO_THROW(obj->write64((0x1UL << 50)));
}

TEST_P(sysobject_cxx_p, object_object) {
  auto t_obj = sysobject::get(tokens_[0], "errors");
  ASSERT_NE(t_obj.get(), nullptr);
  auto t_value = t_obj->get("errors")->read64();
  auto h_obj = sysobject::get(handle_, "errors");
  ASSERT_NE(h_obj.get(), nullptr);
  auto h_value = h_obj->get("errors")->read64();
  EXPECT_EQ(t_value, h_value);
  EXPECT_THROW(t_obj->get("errors")->write64(0x100),
               opae::fpga::types::invalid_param);
  ASSERT_NO_THROW(h_obj->get("errors")->write64(0x100));
  h_value = h_obj->get("errors")->read64(FPGA_OBJECT_SYNC);
  EXPECT_NE(t_value, h_value);
}

TEST_P(sysobject_cxx_p, read_bytes) {
  auto obj = sysobject::get(tokens_[0], "afu_id");
  auto bytes = obj->bytes(3, 4);
  auto str1 = std::string(bytes.begin(), bytes.end());
  auto str2 = std::string(platform_.devices[0].afu_guid, 3, 4);
  ASSERT_TRUE(std::equal(str1.begin(), str1.end(), str2.begin(),
                         [](char lhs, char rhs) {
                           return std::tolower(lhs) == std::tolower(rhs);
                         }));
}

INSTANTIATE_TEST_CASE_P(sysobject_cxx, sysobject_cxx_p,
                        ::testing::ValuesIn(test_platform::keys(true)));
