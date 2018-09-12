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

#ifdef __cplusplus

extern "C" {
#endif

#include <json-c/json.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include "opae_int.h"

#ifdef __cplusplus
}
#endif

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class enum_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  enum_c_p() : tmpsysfs("mocksys-XXXXXX") {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter), FPGA_OK);
    num_matches = 0xc01a;
    invalid_device_ = test_device::unknown();

  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    if (!tmpsysfs.empty() && tmpsysfs.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs;
  fpga_properties filter;
  std::array<fpga_token, 2> tokens;
  uint32_t num_matches;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

// TEST_P(enum_c_p, matches_1filter) {
//  // null filter
//  EXPECT_EQ(fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(),
//                          &num_matches),
//            FPGA_OK);
//
//  EXPECT_FALSE(matches_filter(devices[0], filters["invalid"]));
//
//  //
//}
//
// TEST_P(enum_c_p, matches_filters) {
//  const struct dev_list *attr = 0;
//  const fpga_properties *filter = 0;
//  uint32_t num_filter = 0;
//  auto res = matches_filters(attr, filter, num_filter);
//  EXPECT_EQ(res, 0);
//}
//
// TEST_P(enum_c_p, add_dev) {
//  const char *sysfspath = 0;
//  const char *devpath = 0;
//  struct dev_list *parent = 0;
//  auto res = add_dev(sysfspath, devpath, parent);
//  (void)res;
//}
//
// TEST_P(enum_c_p, enum_fme) {
//  const char *sysfspath = 0;
//  const char *name = 0;
//  struct dev_list *parent = 0;
//  auto res = enum_fme(sysfspath, name, parent);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
// TEST_P(enum_c_p, enum_afu) {
//  const char *sysfspath = 0;
//  const char *name = 0;
//  struct dev_list *parent = 0;
//  auto res = enum_afu(sysfspath, name, parent);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
// TEST_P(enum_c_p, enum_top_dev) {
//  const char *sysfspath = 0;
//  struct dev_list *list = 0;
//  int include_port = 0;
//  auto res = enum_top_dev(sysfspath, list, include_port);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
// TEST_P(enum_c_p, include_afu) {
//  const fpga_properties *filters = 0;
//  uint32_t num_filters = 0;
//  auto res = include_afu(filters, num_filters);
//  EXPECT_EQ(res, 0);
//}
//
TEST_P(enum_c_p, nullfilter) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, platform_.devices.size()*2);

  EXPECT_EQ(
      fpgaEnumerate(nullptr, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nullmatches) {
  EXPECT_EQ(
      fpgaEnumerate(&filter, 0, tokens.data(), tokens.size(), NULL),
      FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nulltokens) {
  EXPECT_EQ(
      fpgaEnumerate(&filter, 0, NULL, tokens.size(), &num_matches),
      FPGA_INVALID_PARAM);
}


TEST_P(enum_c_p, object_type) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, platform_.devices.size());

  EXPECT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, platform_.devices.size());
}

TEST_P(enum_c_p, parent) {
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, platform_.devices.size());
  ASSERT_EQ(fpgaClearProperties(filter), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetParent(filter, tokens[0]), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, platform_.devices.size());
}

TEST_P(enum_c_p, segment) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSegment(filter, device.segment), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetSegment(filter, invalid_device_.segment), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, bus) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetBus(filter, device.bus), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetBus(filter, invalid_device_.bus), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, device) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDevice(filter, device.device), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetDevice(filter, invalid_device_.device), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, function) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetFunction(filter, device.function), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetFunction(filter, invalid_device_.function),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, socket_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter, device.socket_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetSocketID(filter, invalid_device_.socket_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, vendor_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter, device.vendor_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetVendorID(filter, invalid_device_.vendor_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, device_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter, device.device_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);

  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter, invalid_device_.device_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, object_id) {
  auto device = platform_.devices[0];
  // fme object id
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter, device.fme_object_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter, invalid_device_.fme_object_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
  // afu object id
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter, device.port_object_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter, invalid_device_.port_object_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, num_errors) {
  auto device = platform_.devices[0];
  // fme num_errors
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter, device.fme_num_errors), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  // afu num_errors
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter, device.port_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  // invalid
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter, invalid_device_.port_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, guid) {
  auto device = platform_.devices[0];
  // fme guid
  fpga_guid fme_guid, afu_guid, random_guid;
  ASSERT_EQ(uuid_parse(device.fme_guid, fme_guid), 0);
  ASSERT_EQ(uuid_parse(device.afu_guid, afu_guid), 0);
  ASSERT_EQ(uuid_parse(invalid_device_.afu_guid, random_guid), 0);
  ASSERT_EQ(fpgaPropertiesSetGUID(filter, fme_guid), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);
  // afu guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter, afu_guid), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);
  // random guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter, random_guid), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, clone_token) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  ASSERT_EQ(num_matches, 2);
  fpga_token src = tokens[0];
  fpga_token dst;
  EXPECT_EQ(fpgaCloneToken(src, &dst), FPGA_OK);
}

TEST_P(enum_c_p, clone_wo_src_dst) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);
  fpga_token src = tokens[0];
  fpga_token dst;
  EXPECT_EQ(fpgaCloneToken(NULL, &dst), FPGA_INVALID_PARAM);
  EXPECT_EQ(fpgaCloneToken(&src, NULL), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, no_token_magic) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 2);
  fpga_token src,dst;
  EXPECT_NE(fpgaCloneToken(&src, &dst), FPGA_OK);
}


TEST_P(enum_c_p, destroy_token) {
  opae_wrapped_token *dummy = new opae_wrapped_token;
  EXPECT_EQ(fpgaDestroyToken((fpga_token *)dummy), FPGA_INVALID_PARAM);
  delete dummy;
  // null filter
  uint32_t num_matches;
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  ASSERT_EQ(num_matches, 2);
  num_matches = 100;
  for (auto t : tokens) {
    if (t != nullptr) {
      EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
    }
  }

  EXPECT_EQ(fpgaDestroyToken(nullptr), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, num_slots) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter, device.num_slots), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter, invalid_device_.num_slots), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, bbs_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter, device.bbs_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetBBSID(filter, invalid_device_.bbs_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, bbs_version) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter, device.bbs_version), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter, invalid_device_.bbs_version), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, state) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter, device.state), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter, invalid_device_.state), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, num_mmio) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter, device.num_mmio), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter, invalid_device_.num_mmio), FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}

TEST_P(enum_c_p, num_interrupts) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter, device.num_interrupts), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 1);

  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter, invalid_device_.num_interrupts), FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &num_matches),
      FPGA_OK);
  EXPECT_EQ(num_matches, 0);
}


INSTANTIATE_TEST_CASE_P(enum_c, enum_c_p, ::testing::ValuesIn(test_platform::keys(true)));
