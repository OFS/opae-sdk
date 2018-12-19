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

#include "test_system.h"
#include "gtest/gtest.h"
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>

using namespace opae::testing;
using namespace opae::fpga::types;

class properties_cxx_core : public ::testing::TestWithParam<std::string> {
protected:
  properties_cxx_core() : handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);

    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_TRUE(tokens_.size() > 0);
  }

  virtual void TearDown() override {
	  tokens_.clear();
	  handle_.reset();
	  fpgaFinalize();
	  system_->finalize();
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  test_platform platform_;
  test_system *system_;
};

fpga_guid guid_invalid = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                          0xf8, 0x9e, 0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b};

const char *TEST_GUID_STR = "ae2878a7-926f-4332-aba1-2b952ad6df8e";

/**
 * @test properties::get_no_filter
 * Calling properties::get with no filter returns a properties object
 * that will return all tokens when enumerated.
 */
TEST_P(properties_cxx_core, get_no_filter) {
  std::vector<token::ptr_t> tokens;

  tokens = token::enumerate({properties::get()});
  EXPECT_GT(tokens.size(), 0);
}

/**
 * @test properties::get_guid_valid
 * Calling properties::get with a valid guid returns a properties
 * object that will return a token with the same guid when enumerated.
 */
TEST_P(properties_cxx_core, get_guid_valid) {
  std::vector<token::ptr_t> tokens;
  const char *guid = nullptr;
  fpga_guid valid_guid;

  // Retrieve first platform device afu guid.
  guid = platform_.devices[0].afu_guid;
  ASSERT_EQ(0, uuid_parse(guid, valid_guid));

  tokens = token::enumerate({properties::get(valid_guid)});
  EXPECT_GT(tokens.size(), 0);
}

/**
 * @test properties::get_guid_invalid
 * Calling properties::get with an invalid guid returns a properties
 * object that will return no tokens when enumerated.
 */
TEST_P(properties_cxx_core, get_guid_invalid) {
  std::vector<token::ptr_t> tokens;

  tokens = token::enumerate({properties::get(guid_invalid)});
  EXPECT_EQ(tokens.size(), 0);
}

/**
 * @test properties::get_token
 * Calling properties::get with a token returns a properties object
 * that will return the a token with the same attributes.
 */
TEST_P(properties_cxx_core, get_token) {
  std::vector<token::ptr_t> tokens;

  tokens = token::enumerate({properties::get(tokens_[0])});
  EXPECT_GT(tokens.size(), 0);
}

/**
 * @test properties::get_handle
 * Calling properties::get with a handle returns a properties object
 * that will return a token with the same attributes.
 */
TEST_P(properties_cxx_core, get_handle) {
  std::vector<token::ptr_t> tokens;

  handle_ = handle::open(tokens_[0], FPGA_OPEN_SHARED);
  ASSERT_NE(nullptr, handle_.get());
  tokens = token::enumerate({properties::get(handle_)});
  EXPECT_GT(tokens.size(), 0);
}

/**
 * @test set_guid
 * Given a new properties object and a valid fpga_guid object
 * When I set the guid property to the fpga_guid object
 * And I retrieve the same property using fpgaGetPropertiesGUID
 * Then the known guid matches the one retrieved
 */
TEST_P(properties_cxx_core, set_guid) {
  fpga_guid guid_in, guid_out;
  auto p = properties::get();
  // set the guid to an fpga_guid
  ASSERT_EQ(0, uuid_parse(TEST_GUID_STR, guid_in));
  p->guid = guid_in;

  // now check we set the guid using C APIs
  ASSERT_EQ(fpgaPropertiesGetGUID(p->c_type(), &guid_out), FPGA_OK);
  EXPECT_EQ(memcmp(guid_in, guid_out, sizeof(fpga_guid)), 0);
}

/**
 * @test parse_guid
 * Given a new properties object
 * When I set the guid property using its `parse` method
 * And I retrieve the same property using fpgaGetPropertiesGUID
 * Then the known guid string parsed matches the one retrieved
 */
TEST_P(properties_cxx_core, parse_guid) {
  fpga_guid guid_out;
  auto p = properties::get();
  // set the guid to an fpga_guid
  p->guid.parse(TEST_GUID_STR);

  // now check we set the guid using C APIs
  ASSERT_EQ(fpgaPropertiesGetGUID(p->c_type(), &guid_out), FPGA_OK);
  char guid_str[84];
  uuid_unparse(guid_out, guid_str);
  EXPECT_STREQ(TEST_GUID_STR, guid_str);
}

/**
 * @test get_guid
 * Given a new properties object and a valid fpga_guid object
 * When I set the guid property using fpgaPropertiesSetGUID
 * And I get a pointer to the guid member variable of the property object
 * Then the known guid matches the one retrieved
 */
TEST_P(properties_cxx_core, get_guid) {
  fpga_guid guid_in;
  auto p = properties::get();
  // set the guid using fpgaPropertiesSetGUID
  uuid_parse(TEST_GUID_STR, guid_in);
  fpgaPropertiesSetGUID(p->c_type(), guid_in);

  uint8_t *guid_ptr = p->guid;
  ASSERT_NE(nullptr, guid_ptr);
  EXPECT_EQ(memcmp(guid_in, guid_ptr, sizeof(fpga_guid)), 0);
}

/**
 * @test compare_guid
 * Given a new properties object with a known guid
 * When I set compare its guid with the known guid
 * Then the result is true
 */
TEST_P(properties_cxx_core, compare_guid) {
  fpga_guid guid_in;
  auto p = properties::get();
  ASSERT_EQ(0, uuid_parse(TEST_GUID_STR, guid_in));
  EXPECT_FALSE(p->guid == guid_in);
  p->guid = guid_in;
  ASSERT_EQ(memcmp(p->guid.c_type(), guid_in, sizeof(fpga_guid)), 0);
  EXPECT_TRUE(p->guid == guid_in);
}

/**
 * @test props_ctor_01
 * Given a new properties object with a known guid
 * passed in the constructor
 * When I set compare its guid with the known guid
 * Then the result is true
 */
TEST_P(properties_cxx_core, props_ctor_01) {
  fpga_guid guid_in;
  ASSERT_EQ(0, uuid_parse(TEST_GUID_STR, guid_in));
  auto p = properties::get(guid_in);
  ASSERT_EQ(memcmp(p->guid.c_type(), guid_in, sizeof(fpga_guid)), 0);
  EXPECT_TRUE(p->guid == guid_in);
}

/**
 * @test set_objtype
 * Given a new properties object
 * When I set the object type to a known value
 * Then the property is set
 */
TEST_P(properties_cxx_core, set_objtype) {
  auto p = properties::get();
  p->type = FPGA_ACCELERATOR;
  fpga_objtype t = p->type;
  fpga_objtype other_t =
      (t == FPGA_ACCELERATOR) ? FPGA_DEVICE : FPGA_ACCELERATOR;
  p->type = other_t;
  EXPECT_TRUE(p->type == other_t);
}

/**
 * @test get_model
 * Given a properties object
 * When I get the model property
 * Then I get an empty string
 */
TEST_P(properties_cxx_core, get_model) {
  auto p = properties::get();
  std::string model = "";
  // Model is currently not supported in libopae-c
  EXPECT_THROW(model = p->model, not_supported);
}

/**
 * @test get_num_errors
 * Given a properties properties object with the num_errors property set to a
 * known value
 * When I get the num_errors property
 * Then the number is the expected value
 */
TEST_P(properties_cxx_core, get_num_errors) {
  auto p = properties::get();
  p->num_errors = 9;
  EXPECT_EQ(static_cast<uint32_t>(p->num_errors), 9);
}

/**
 * @test get_segment
 * Given a properties properties object with the segment property set to a
 * known value
 * When I get the segment property
 * Then the number is the expected value
 */
TEST_P(properties_cxx_core, get_segment) {
  auto p = properties::get();
  p->segment = 9090;
  EXPECT_EQ(static_cast<uint16_t>(p->segment), 9090);
}

INSTANTIATE_TEST_CASE_P(properties, properties_cxx_core,
                        ::testing::ValuesIn(test_platform::keys(true)));
