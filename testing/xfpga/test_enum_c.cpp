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


#include <json-c/json.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"
#include "types_int.h"
#include "xfpga.h"

using namespace opae::testing;

class enum_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  enum_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    num_matches_ = 0xc01a;
    invalid_device_ = test_device::unknown();

  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    DestroyTokens();
    system_->finalize();
  }

  void DestroyTokens() {
    for (auto & t : tokens_) {
      if (t != nullptr) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

TEST_P(enum_c_p, nullfilter) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size()*2);

  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nullmatches) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 0, tokens_.data(), tokens_.size(), NULL),
      FPGA_INVALID_PARAM);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nulltokens) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 0, NULL, tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}


TEST_P(enum_c_p, object_type) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
}

TEST_P(enum_c_p, parent) {
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
  ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetParent(filter_, tokens_[0]), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
}

TEST_P(enum_c_p, segment) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, device.segment), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, invalid_device_.segment), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, bus) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetBus(filter_, device.bus), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetBus(filter_, invalid_device_.bus), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, device) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, device.device), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, invalid_device_.device), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, function) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, device.function), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, invalid_device_.function),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, socket_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, device.socket_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, invalid_device_.socket_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, vendor_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, device.vendor_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, invalid_device_.vendor_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, device_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);

  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, invalid_device_.device_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, object_id) {
  auto device = platform_.devices[0];
  // fme object id
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, device.fme_object_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.fme_object_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);

  // afu object id
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, device.port_object_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.port_object_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, num_errors) {
  auto device = platform_.devices[0];
  // fme num_errors
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.fme_num_errors), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  DestroyTokens();

  // afu num_errors
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.port_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  SCOPED_TRACE("num_errors: DestroyTokens()");
  DestroyTokens();

  // invalid
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, invalid_device_.port_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, guid) {
  auto device = platform_.devices[0];
  // fme guid
  fpga_guid fme_guid, afu_guid, random_guid;
  ASSERT_EQ(uuid_parse(device.fme_guid, fme_guid), 0);
  ASSERT_EQ(uuid_parse(device.afu_guid, afu_guid), 0);
  ASSERT_EQ(uuid_parse(invalid_device_.afu_guid, random_guid), 0);
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, fme_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  // afu guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, afu_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  // random guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, random_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, clone_token) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);
  fpga_token src = tokens_[0];
  fpga_token dst;
  EXPECT_EQ(xfpga_fpgaCloneToken(src, &dst), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyToken(&dst), FPGA_OK);
}

TEST_P(enum_c_p, clone_wo_src_dst) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);
  fpga_token src = tokens_[0];
  fpga_token dst;
  EXPECT_EQ(xfpga_fpgaCloneToken(NULL, &dst), FPGA_INVALID_PARAM);
  EXPECT_EQ(xfpga_fpgaCloneToken(&src, NULL), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, no_token_magic) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);
  fpga_token src,dst;
  EXPECT_NE(xfpga_fpgaCloneToken(&src, &dst), FPGA_OK);
}


TEST_P(enum_c_p, destroy_token) {
  // null filter_
  uint32_t num_matches_;
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);
  num_matches_ = 100;
  SCOPED_TRACE("destroy_token: DestroyTokens()");
  DestroyTokens();

  EXPECT_EQ(xfpga_fpgaDestroyToken(nullptr), FPGA_INVALID_PARAM);
  _fpga_token *dummy = new _fpga_token;
  EXPECT_EQ(xfpga_fpgaDestroyToken((fpga_token *)dummy), FPGA_INVALID_PARAM);
  delete dummy;
}

TEST_P(enum_c_p, num_slots) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, device.num_slots), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, invalid_device_.num_slots), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, bbs_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, device.bbs_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, invalid_device_.bbs_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, bbs_version) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, device.bbs_version), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, invalid_device_.bbs_version), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, state) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, device.state), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, invalid_device_.state), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, num_mmio) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, device.num_mmio), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, invalid_device_.num_mmio), FPGA_INVALID_PARAM);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, num_interrupts) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter_, device.num_interrupts), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter_, invalid_device_.num_interrupts), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       num_filter_neg
 *
 * @brief      When the num_filter parameter to fpgaEnumerate is zero,
 *             but the filter parameter is non-NULL, the function
 *             returns FPGA_INVALID_PARAM.
 */
TEST_P(enum_c_p, num_filter_neg) {
  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 0, tokens_.data(), 0, &num_matches_),
            FPGA_INVALID_PARAM);
}

/**
 * @test       max_tokens
 *
 * @brief      fpgaEnumerate honors the input max_tokens value by
 *             limiting the number of output entries written to the
 *             memory at match, even though more may exist.
 */
TEST_P(enum_c_p, max_tokens) {
  uint32_t max_tokens = 1;

  EXPECT_EQ(xfpga_fpgaEnumerate(NULL, 0, tokens_.data(), max_tokens, &num_matches_),
            FPGA_OK);
  EXPECT_EQ(2 * platform_.devices.size(), num_matches_);

  EXPECT_NE(tokens_[0], nullptr);
  EXPECT_EQ(tokens_[1], nullptr);
}

/**
 * @test       filter
 *
 * @brief      fpgaEnumerate honors a "don't care" properties filter by
 *             returning all available tokens.
 */
TEST_P(enum_c_p, filter) {
  EXPECT_EQ(FPGA_OK, xfpga_fpgaEnumerate(&filter_, 1, NULL, 0, &num_matches_));
  EXPECT_EQ(2 * platform_.devices.size(), num_matches_);
}

/**
 * @test       parent_neg
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             parent field set, but that parent is not found to be the
 *             parent of any device, fpgaEnumerate returns zero matches.
 */
TEST_P(enum_c_p, parent_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), 0, &num_matches_),
            FPGA_OK);
  EXPECT_EQ(num_matches_, 1);

  EXPECT_EQ(fpgaPropertiesSetParent(filter_, &tokens_[0]), FPGA_OK);

  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, NULL, 0, &num_matches_), FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       get_guid
 *
 * @brief      Given I have a system with at least one FPGA And I
 *             enumerate with a filter of objtype of FPGA_DEVICE When I
 *             get properties from the resulting token And I query the
 *             GUID from the properties Then the GUID is returned and
 *             the result is FPGA_OK.
 *
 */
TEST_P(enum_c_p, get_guid) {
  fpga_properties prop;
  fpga_guid guid;
  fpga_properties filterp = NULL;

  ASSERT_EQ(xfpga_fpgaGetProperties(NULL, &filterp), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetObjectType(filterp, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filterp, 1, tokens_.data(), 1, &num_matches_),
            FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  EXPECT_EQ(fpgaDestroyProperties(&filterp), FPGA_OK);

  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[0], &prop), FPGA_OK);

  EXPECT_EQ(fpgaPropertiesGetGUID(prop, &guid), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_c_p, ::testing::ValuesIn(test_platform::keys(true)));
