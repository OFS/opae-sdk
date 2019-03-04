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
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "types_int.h"
#include "sysfs_int.h"
#include "mock_opae.h"
extern "C" {
#include "token_list_int.h"
}
#include "xfpga.h"

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

class enum_c_p : public mock_opae_p<2, xfpga_> {
 protected:
  enum_c_p() : filter_(nullptr) {}

  virtual ~enum_c_p() {}

  virtual void test_setup() override {
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    num_matches_ = 0xc01a;
  }

  virtual void test_teardown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    token_cleanup();
    xfpga_plugin_finalize();
  }

  // Need a concrete way to determine the number of fpgas on the system
  // without relying on fpgaEnumerate() since that is the function that
  // is under test. 
  int GetNumFpgas() {
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

  int GetNumMatchedFpga () {
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

  void ExecuteCmd(std::string cmd, int &value) {
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

/**
 * @test       nullfilter
 *
 * @brief      When the filter is null and the number of filters
 *             is zero, the function returns all matches.
 */
TEST_P(enum_c_p, nullfilter) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
}

/**
 * @test       nullfilter_neg
 *
 * @brief      When the filter is null but the number of filters
 *             is greater than zero, the function returns
 *             FPGA_INVALID_PARAM.
 */
TEST_P(enum_c_p, nullfilter_neg) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}

/**
 * @test       nullmatches
 *
 * @brief      When the number of matches parameter is null,
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST_P(enum_c_p, nullmatches) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), NULL),
      FPGA_INVALID_PARAM);
}

/**
 * @test       nulltokens
 *
 * @brief      When the tokens parameter is null, the function
 *             returns FPGA_INVALID_PARAM.
 */
TEST_P(enum_c_p, nulltokens) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 0, NULL, tokens_.size(), &num_matches_),
      FPGA_INVALID_PARAM);
}

/**
 * @test       object_type_accel
 *
 * @brief      When the filter object type is set to
 *             FPGA_ACCELERATOR, the function returns the
 *             correct number of accelerator matches.
 */
TEST_P(enum_c_p, object_type_accel) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       object_type_dev
 *
 * @brief      When the filter object type is set to FPGA_DEVICE,
 *             the function returns the correct number of device
 *             matches.
 */
TEST_P(enum_c_p, object_type_dev) {
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       parent
 *
 * @brief      When the filter parent is set to a previously found
 *             FPGA_DEVICE, the function returns the child resource.
 */
TEST_P(enum_c_p, parent) {
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());

  ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);

  fpga_token token = nullptr;

  EXPECT_EQ(fpgaPropertiesSetParent(filter_, tokens_[0]), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, &token, 1, &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  ASSERT_NE(token, nullptr);
  EXPECT_EQ(xfpga_fpgaDestroyToken(&token), FPGA_OK);
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
  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), 1, &num_matches_),
            FPGA_OK);
  EXPECT_GT(num_matches_, 0);

  EXPECT_EQ(fpgaPropertiesSetParent(filter_, tokens_[0]), FPGA_OK);

  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, NULL, 0, &num_matches_), FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       segment
 *
 * @brief      When the filter segment is set and it is valid,
 *             the function returns the number of resources that
 *             match that segment.
 */
TEST_P(enum_c_p, segment) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, device.segment), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
}

/**
 * @test       segment_neg
 *
 * @brief      When the filter segment is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, segment_neg) {
  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, invalid_device_.segment), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       bus
 *
 * @brief      When the filter bus is set and it is valid, the
 *             function returns the number of resources that
 *             match that bus.
 */
TEST_P(enum_c_p, bus) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetBus(filter_, device.bus), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 2);
}

/**
 * @test       bus_neg
 *
 * @brief      When the filter bus is set and it is invalid, 
 *             the function returns zero matches
 */
TEST_P(enum_c_p, bus_neg) {
  ASSERT_EQ(fpgaPropertiesSetBus(filter_, invalid_device_.bus), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       device
 *
 * @brief      When the filter device is set and it is valid,
 *             the function returns the number of resources that
 *             match that device.
 */
TEST_P(enum_c_p, device) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, device.device), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
}

/**
 * @test       device_neg
 *
 * @brief      When the filter device is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, device_neg) {
  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, invalid_device_.device), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       function
 *
 * @brief      When the filter function is set and it is valid,
 *             the function returns the number of resources that
 *             match that function.
 */
TEST_P(enum_c_p, function) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, device.function), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2 - device.num_vfs);
  DestroyTokens();
  for (int i = 0; i < device.num_vfs; ++i) {
    num_matches_ = 0;
    ASSERT_EQ(fpgaPropertiesSetFunction(filter_, device.function+i), FPGA_OK);
    EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_),
              FPGA_OK);
    EXPECT_EQ(num_matches_, 1);
    DestroyTokens();
  }
}

/**
 * @test       function_neg
 *
 * @brief      When the filter function is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, function_neg) {
  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, invalid_device_.function),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}


/**
 * @test       socket_id_neg
 *
 * @brief      When the filter socket_id is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, socket_id_neg) {
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, invalid_device_.socket_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       vendor_id
 *
 * @brief      When the filter vendor_id is set and it is valid,
 *             the function returns the number of resources that
 *             match that vendor_id.
 */
TEST_P(enum_c_p, vendor_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, device.vendor_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
}

/**
 * @test       vendor_id_neg
 *
 * @brief      When the filter vendor_id is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, vendor_id_neg) {
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, invalid_device_.vendor_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       device_id
 *
 * @brief      When the filter device_id is set and it is valid,
 *             the function returns the number of resources that
 *             match that device_id.
 */
TEST_P(enum_c_p, device_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size() * 2 - device.num_vfs);
  DestroyTokens();

  for (int i = 0; i < device.num_vfs; ++i) {
    num_matches_ = 0;
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id+i), FPGA_OK);
    EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_),
              FPGA_OK);
    EXPECT_EQ(num_matches_, 1);
    DestroyTokens();
  }
}

/**
 * @test       device_id_neg
 *
 * @brief      When the filter device_id is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, device_id_neg) {
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, invalid_device_.device_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       object_id_fme
 *
 * @brief      When the filter object_id for fme is set and it is
 *             valid, the function returns the number of resources
 *             that match that object_id.
 */
TEST_P(enum_c_p, object_id_fme) {
  ASSERT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);

  ASSERT_GT(num_matches_, 0);

  fpga_properties prop;
  uint64_t object_id;

  EXPECT_EQ(xfpga_fpgaGetProperties(tokens_[0], &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
}

/**
 * @test       object_id_fme_neg
 *
 * @brief      When the filter object_id for fme is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, object_id_fme_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.fme_object_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       object_id_port
 *
 * @brief      When the filter port_id for port is set and it is
 *             valid, the function returns the number of resources
 *             that match that port_id.
 */
TEST_P(enum_c_p, object_id_port) {
  ASSERT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);

  ASSERT_GT(num_matches_, 0);

  fpga_properties prop;
  uint64_t object_id;

  EXPECT_EQ(xfpga_fpgaGetProperties(tokens_[0], &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  DestroyTokens();

  EXPECT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
}

/**
 * @test       object_id_port_neg
 *
 * @brief      When the filter object_id for port is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, object_id_port_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, invalid_device_.port_object_id),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}


/**
 * @test       num_errors_fme_neg
 *
 * @brief      When the filter num_errors for fme is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, num_errors_fme_neg) {
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, invalid_device_.fme_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}


/**
 * @test       num_errors_port_neg
 *
 * @brief      When the filter num_errors for port is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, num_errors_port_neg) {
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, invalid_device_.port_num_errors),
            FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       guid_fme
 *
 * @brief      When the filter guid for fme is set and it is
 *             valid, the function returns the number of resources
 *             that match that guid for fme.
 */
TEST_P(enum_c_p, guid_fme) {
  auto device = platform_.devices[0];

  fpga_guid fme_guid;
  ASSERT_EQ(uuid_parse(device.fme_guid, fme_guid), 0);

  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, fme_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
}

/**
 * @test       guid_fme_neg
 *
 * @brief      When the filter guid for fme is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, guid_fme_neg) {
  fpga_guid invalid_guid;
  ASSERT_EQ(uuid_parse(invalid_device_.fme_guid, invalid_guid), 0);

  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, invalid_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       guid_port
 *
 * @brief      When the filter guid for port is set and it is
 *             valid, the function returns the number of resources
 *             that match that guid for port.
 */
TEST_P(enum_c_p, guid_port) {
  auto device = platform_.devices[0];

  fpga_guid afu_guid;
  ASSERT_EQ(uuid_parse(device.afu_guid, afu_guid), 0);

  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, afu_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       guid_port_neg
 *
 * @brief      When the filter guid for port is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, guid_port_neg) {
  fpga_guid invalid_guid;
  ASSERT_EQ(uuid_parse(invalid_device_.afu_guid, invalid_guid), 0);

  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, invalid_guid), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       clone_token
 *
 * @brief      Given a valid source token and a valid destination,
 *             xfpga_fpgaCloneToken() returns FPGA_OK.
 */
TEST_P(enum_c_p, clone_token) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_GT(num_matches_, 0);
  fpga_token src = tokens_[0];
  fpga_token dst;
  EXPECT_EQ(xfpga_fpgaCloneToken(src, &dst), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyToken(&dst), FPGA_OK);
}

/**
 * @test       clone_token_neg
 *
 * @brief      Given an invalid source token or an invalid destination,
 *             xfpga_fpgaCloneToken() returns FPGA_INVALID_PARAM
 */
TEST_P(enum_c_p, clone_token_neg) {
  EXPECT_EQ(
      xfpga_fpgaEnumerate(nullptr, 0, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_GT(num_matches_, 0);
  fpga_token src = tokens_[0];
  fpga_token dst;
  EXPECT_EQ(xfpga_fpgaCloneToken(NULL, &dst), FPGA_INVALID_PARAM);
  EXPECT_EQ(xfpga_fpgaCloneToken(&src, NULL), FPGA_INVALID_PARAM);
}

/**
 * @test       destroy_token
 *
 * @brief      Given a valid token, xfpga_fpgaDestroyToken() returns
 *             FPGA_OK.
 */
TEST_P(enum_c_p, destroy_token) {
  fpga_token token;
  ASSERT_EQ(xfpga_fpgaEnumerate(nullptr, 0, &token, 1, &num_matches_),
            FPGA_OK);
  ASSERT_GT(num_matches_, 0);

  EXPECT_EQ(xfpga_fpgaDestroyToken(&token), FPGA_OK);
}

/**
 * @test       destroy_token_neg
 *
 * @brief      Given a null or invalid token, xfpga_fpgaDestroyToken()
 *             returns FPGA_INVALID_PARAM.
 */
TEST_P(enum_c_p, destroy_token_neg) {
  EXPECT_EQ(xfpga_fpgaDestroyToken(nullptr), FPGA_INVALID_PARAM);

  _fpga_token *dummy = new _fpga_token;
  memset(dummy, 0, sizeof(*dummy));
  EXPECT_EQ(xfpga_fpgaDestroyToken((fpga_token *)&dummy), FPGA_INVALID_PARAM);
  delete dummy;
}

/**
 * @test       num_slots
 *
 * @brief      When the filter num_slots is set and it is valid,
 *             the function returns the number of resources that
 *             match that number of slots.
 */
TEST_P(enum_c_p, num_slots) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, device.num_slots), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       num_slots_neg
 *
 * @brief      When the filter num_slots is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, num_slots_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, invalid_device_.num_slots), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       bbs_id
 *
 * @brief      When the filter bbs_id is set and it is valid,
 *             the function returns the number of resources that
 *             match that bbs_id.
 */
TEST_P(enum_c_p, bbs_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, device.bbs_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
}

/**
 * @test       bbs_id_neg
 *
 * @brief      When the filter bbs_id is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, bbs_id_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, invalid_device_.bbs_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       bbs_version
 *
 * @brief      When the filter bbs_version is set and it is valid,
 *             the function returns the number of resources that
 *             match that bbs_version.
 */
TEST_P(enum_c_p, bbs_version) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, device.bbs_version), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());
}

/**
 * @test       bbs_version_neg
 *
 * @brief      When the filter bbs_version is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, bbs_version_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, invalid_device_.bbs_version), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       accel_state
 *
 * @brief      When the filter accelerator state is set and it is
 *             valid, the function returns the number of resources
 *             that match that accelerator state.
 */
TEST_P(enum_c_p, accel_state) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, device.state), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       accel_state_neg
 *
 * @brief      When the filter accelerator state is set and it is
 *             invalid, the function returns zero matches.
 */
TEST_P(enum_c_p, state_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, invalid_device_.state), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       num_mmio
 *
 * @brief      When the filter num MMIO is set and it is valid,
 *             the function returns the number of resources that
 *             match that num MMIO.
 */
TEST_P(enum_c_p, num_mmio) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, device.num_mmio), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       num_mmio_neg
 *
 * @brief      When the filter num MMIO is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, num_mmio_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, invalid_device_.num_mmio), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
}

/**
 * @test       num_interrupts
 *
 * @brief      When the filter num interrupts is set and it is valid,
 *             the function returns the number of resources that
 *             match that num interrupts.
 */
TEST_P(enum_c_p, num_interrupts) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter_, device.num_interrupts), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

/**
 * @test       num_interrupts_neg
 *
 * @brief      When the filter num interrupts is set and it is invalid,
 *             the function returns zero matches.
 */
TEST_P(enum_c_p, num_interrupts_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
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
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);

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
  EXPECT_EQ(num_matches_, GetNumFpgas() * 2);
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
  EXPECT_GT(num_matches_, 0);
  EXPECT_EQ(fpgaDestroyProperties(&filterp), FPGA_OK);

  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[0], &prop), FPGA_OK);

  EXPECT_EQ(fpgaPropertiesGetGUID(prop, &guid), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}




INSTANTIATE_TEST_CASE_P(enum_c, enum_c_p, 
                        ::testing::ValuesIn(test_platform::platforms({})));

class enum_err_c_p : public enum_c_p {};
/**
 * @test       num_errors_fme
 *
 * @brief      When the filter num_errors for fme is set and it is
 *             valid, the function returns the number of resources
 *             that match that number of errors for fme.
 */
TEST_P(enum_err_c_p, num_errors_fme) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.fme_num_errors),
      FPGA_OK);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
      &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}


/**
 * @test       num_errors_port
 *
 * @brief      When the filter num_errors for port is set and it is
 *             valid, the function returns the number of resources
 *             that match that number of errors for port.
 */
TEST_P(enum_err_c_p, num_errors_port) {
  auto device = platform_.devices[0];

  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.port_num_errors),
      FPGA_OK);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
      &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumFpgas());
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_err_c_p,
                       ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc" })));

class enum_socket_c_p : public enum_c_p {};

/**
 * @test       socket_id
 *
 * @brief      When the filter socket_id is set and it is valid,
 *             the function returns the number of resources that
 *             match that socket_id.
 */
TEST_P(enum_socket_c_p, socket_id) {
  auto device = platform_.devices[0];
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, device.socket_id), FPGA_OK);
  EXPECT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, GetNumMatchedFpga() * 2);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_socket_c_p,
                          ::testing::ValuesIn(test_platform::platforms({ "skx-p"})));

class enum_mock_only : public enum_c_p {};

/**
 * @test       remove_port
 *
 * @brief      Given I have a system with at least one FPGA And I
 *             enumerate with a filter of objtype of FPGA_ACCELERATOR
 *             and I get one token for that accelerator
 *             When I remove the port device from the system
 *             And I enumerate again with the same filter
 *             Then I get zero tokens as the result.
 *
 */
TEST_P(enum_mock_only, remove_port) {
  fpga_properties filterp = NULL;

  ASSERT_EQ(xfpga_fpgaGetProperties(NULL, &filterp), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetObjectType(filterp, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filterp, 1, tokens_.data(), 1, &num_matches_),
            FPGA_OK);
  EXPECT_EQ(num_matches_, 1);
  const char *sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";

  EXPECT_EQ(system_->remove_sysfs_dir(sysfs_port), 0)
      << "error removing intel-fpga-port.0: " << strerror(errno);
  EXPECT_EQ(xfpga_fpgaEnumerate(&filterp, 1, tokens_.data(), 1, &num_matches_),
            FPGA_OK);
  EXPECT_EQ(num_matches_, 0);
  EXPECT_EQ(fpgaDestroyProperties(&filterp), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(enum_c, enum_mock_only,
                          ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p"})));
