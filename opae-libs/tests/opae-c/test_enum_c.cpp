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
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "mock/mock_opae.h"

using namespace opae::testing;

class enum_c_p : public mock_opae_p<2, none_> {
 protected:
  enum_c_p() {}

  virtual void test_setup() override {

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    filter_ = nullptr;
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    num_matches_ = 0;
  }


  virtual void test_teardown() override {
    num_matches_ = 0;
    if (filter_ != nullptr) {
      EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    }
    fpgaFinalize();
  }

  // Need a concrete way to determine the number of fpgas on the system
  // without relying on fpgaEnumerate() since that is the function that
  // is under test.
  virtual int GetNumFpgas() {
    if (platform_.mock_sysfs != nullptr) {
      return platform_.devices.size();
    }

    int value;
    std::string cmd =
        "(ls -l /sys/class/fpga*/region*/*fme*/dev || "
        "ls -l /sys/class/fpga*/*intel*) |  (wc -l)";

    ExecuteCmd(cmd, value);
    return value;
  }

  virtual int GetNumMatchedFpga () {
    if (platform_.mock_sysfs != nullptr) {
      return 1;
    }

    int matches = 0;
    int socket_id;
    int i;
    for (i = 0; i < GetNumFpgas(); i++) {
      std::string cmd = "cat /sys/class/fpga*/*" + std::to_string(i) +
                        "/*fme." + std::to_string(i) + "/socket_id";

      ExecuteCmd(cmd, socket_id);
      if (socket_id == (int)platform_.devices[0].socket_id) {
          matches++;
      }
    }

    return matches;
  }

  int GetMatchedGuidFpgas() {
    if (platform_.mock_sysfs != nullptr) {
      return platform_.devices.size();
    }

    int matches = 0;
    std::string afu_id;
    std::string afu_id_expected = platform_.devices[0].afu_guid;

    afu_id_expected.erase(std::remove(afu_id_expected.begin(),
                                      afu_id_expected.end(), '-'),
                          afu_id_expected.end());
    transform(afu_id_expected.begin(), afu_id_expected.end(),
              afu_id_expected.begin(), ::tolower);

    int i;
    for (i = 0; i < GetNumFpgas(); i++) {
      std::string cmd = "cat /sys/class/fpga*/*" + std::to_string(i) +
                        "/*port." + std::to_string(i) + "/afu_id > output.txt";
      EXPECT_EQ(std::system(cmd.c_str()), 0);
      std::ifstream file("output.txt");
      EXPECT_TRUE(file.is_open());
      EXPECT_TRUE(std::getline(file, afu_id));
      file.close();
      EXPECT_EQ(unlink("output.txt"), 0);

      if (afu_id == afu_id_expected) {
          matches++;
      }
    }

    return matches;
  }

  virtual int GetNumDeviceID() {
    if (platform_.mock_sysfs != nullptr) {
      return 1;
    }

    std::stringstream stream;
    stream << std::hex << platform_.devices[0].device_id;
    std::string device_id(stream.str());

    int value;
    std::string cmd = "lspci | "
                      "grep \'Processing accelerators: "
                            "Intel Corporation\' | "
                      "grep -oE \'[^ ]+$\' | "
                      "grep " + device_id + " | "
                      "wc -l";

    ExecuteCmd(cmd, value);
    return value;
  }

  virtual void ExecuteCmd(std::string cmd, int &value) {
    std::string line;
    std::string command = cmd + " > output.txt";

    EXPECT_EQ(std::system(command.c_str()), 0);

    std::ifstream file("output.txt");

    ASSERT_TRUE(file.is_open());
    EXPECT_TRUE(std::getline(file, line));
    file.close();

    EXPECT_EQ(std::system("rm output.txt"), 0);

    value = std::stoi(line);
  }

  fpga_properties filter_;
  uint32_t num_matches_;
};

TEST_P(enum_c_p, nullfilter) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);

  uint32_t matches = 0;
  EXPECT_EQ(fpgaEnumerate(nullptr, 1, tokens_.data(), tokens_.size(), &matches),
            FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nullmatches) {
  EXPECT_EQ(fpgaEnumerate(&filter_, 0, tokens_.data(), tokens_.size(), NULL),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nulltokens) {
  EXPECT_EQ(fpgaEnumerate(&filter_, 0, NULL, tokens_.size(), &num_matches_),
            FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, object_type) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

TEST_P(enum_c_p, parent) {
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  fpga_token tok = nullptr;
  ASSERT_EQ(fpgaCloneToken(tokens_[0], &tok), FPGA_OK);

  DestroyTokens();

  ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetParent(filter_, tok), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

TEST_P(enum_c_p, segment) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, device.segment), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  // multiply by two to account for port/fme devices
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, invalid_device_.segment),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}


TEST_P(enum_c_p, bus) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetBus(filter_, device.bus), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  // multiply by two to account for port/fme devices
  EXPECT_EQ(num_matches_, 2);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBus(filter_, invalid_device_.bus), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, device) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, device.device), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, invalid_device_.device), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, function) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, device.function), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * (device.num_vfs == 0 ? 2 : 1));
  num_matches_ = 0;
  DestroyTokens();
  for (int i = 1; i < device.num_vfs+1; ++i) {
    ASSERT_EQ(fpgaPropertiesSetFunction(filter_, i), FPGA_OK);
    EXPECT_EQ(
        fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
        FPGA_OK);
    EXPECT_EQ(num_matches_, 1);
    DestroyTokens();
  }
}


TEST_P(enum_c_p, invalid_function) {

  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, invalid_device_.function),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}


TEST_P(enum_c_p, vendor_id) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, device.vendor_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
}


TEST_P(enum_c_p, invalid_vendor_id) {


  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, invalid_device_.vendor_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, device_id) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumDeviceID() * (device.num_vfs == 0 ? 2 : 1));
  DestroyTokens();
  num_matches_ = 0;
  for (int i = 1; i < device.num_vfs+1; ++i) {
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id+i), FPGA_OK);
    EXPECT_EQ(
        fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
        FPGA_OK);
    EXPECT_EQ(num_matches_, 1);
    DestroyTokens();
  }
}

TEST_P(enum_c_p, invalid_device_id) {

  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, invalid_device_.device_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, object_id_fme) {
  fpga_properties prop = nullptr;
  uint64_t object_id;

  ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                          &num_matches_),
            FPGA_OK);
  ASSERT_GT(num_matches_, 0);

  EXPECT_EQ(fpgaGetProperties(tokens_[0], &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
}

TEST_P(enum_c_p, object_id_fme_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.fme_object_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, object_id_port) {
  fpga_properties prop = nullptr;
  uint64_t object_id;

  ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                          &num_matches_),
            FPGA_OK);
  ASSERT_GT(num_matches_, 0);

  EXPECT_EQ(fpgaGetProperties(tokens_[0], &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
}

TEST_P(enum_c_p, object_id_port_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.port_object_id),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
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
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  DestroyTokens();

  // afu guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, afu_guid), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetMatchedGuidFpgas());

  DestroyTokens();

  // random guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, random_guid), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, clone_token01) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  ASSERT_EQ(num_matches_, GetNumFpgas() * 2);
  fpga_token src = tokens_[0];
  fpga_token dst = nullptr;
  EXPECT_EQ(fpgaCloneToken(src, &dst), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&dst), FPGA_OK);
}

TEST_P(enum_c_p, clone_wo_src_dst) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
  fpga_token src = tokens_[0];
  fpga_token dst;
  EXPECT_EQ(fpgaCloneToken(NULL, &dst), FPGA_INVALID_PARAM);
  EXPECT_EQ(fpgaCloneToken(&src, NULL), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, no_token_magic) {
  fpga_token src = nullptr, dst = nullptr;
  EXPECT_NE(fpgaCloneToken(&src, &dst), FPGA_OK);
}

TEST_P(enum_c_p, destroy_token) {
  opae_wrapped_token *dummy = new opae_wrapped_token;
  memset(dummy, 0, sizeof(opae_wrapped_token));
  EXPECT_EQ(fpgaDestroyToken((fpga_token *)&dummy), FPGA_INVALID_PARAM);
  delete dummy;
  EXPECT_EQ(fpgaDestroyToken(nullptr), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, num_slots) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, device.num_slots), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, invalid_device_.num_slots),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, bbs_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, device.bbs_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, invalid_device_.bbs_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, bbs_version) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, device.bbs_version), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, invalid_device_.bbs_version),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, state) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, device.state), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, invalid_device_.state),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, num_mmio) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, device.num_mmio), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, invalid_device_.num_mmio),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST_P(enum_c_p, num_interrupts) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter_, device.num_interrupts),
            FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  ASSERT_EQ(
      fpgaPropertiesSetNumInterrupts(filter_, invalid_device_.num_interrupts),
      FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

TEST(wrapper, validate) {
  EXPECT_EQ(NULL, opae_validate_wrapped_token(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_handle(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_event_handle(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_object(NULL));
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));

class enum_c_mock_p : public enum_c_p {};

TEST_P(enum_c_mock_p, clone_token02) {
  EXPECT_EQ(
      fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  ASSERT_EQ(num_matches_, GetNumFpgas() * 2);
  fpga_token src = tokens_[0];
  fpga_token dst = nullptr;
  // Invalidate the allocation of the wrapped token.
  system_->invalidate_malloc(0, "opae_allocate_wrapped_token");
  EXPECT_EQ(fpgaCloneToken(src, &dst), FPGA_NO_MEMORY);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));

class enum_c_err_p : public enum_c_p {};

TEST_P(enum_c_err_p, num_errors) {
  auto device = platform_.devices[0];

  // fme num_errors
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.fme_num_errors), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  // afu num_errors
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.port_num_errors),
     FPGA_OK);
  EXPECT_EQ(
     fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
     FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  DestroyTokens();

  // invalid
  ASSERT_EQ(
        fpgaPropertiesSetNumErrors(filter_, invalid_device_.port_num_errors),
        FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_c_err_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc" })));

class enum_c_socket_p : public enum_c_p {};

TEST_P(enum_c_socket_p, socket_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, device.socket_id), FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumMatchedFpga() * 2);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, invalid_device_.socket_id),
      FPGA_OK);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_c_socket_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc","dcp-vc" })));
