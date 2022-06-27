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

#include <linux/ioctl.h>

extern "C" {
#include "intel-fpga.h"
#include "fpga-dfl.h"
}

#include "mock/opae_fixtures.h"

static bool gEnableIRQ = true;

using namespace opae::testing;

int port_info(mock_object * m, int request, va_list argp){
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct fpga_port_info *pinfo = va_arg(argp, struct fpga_port_info *);
  if (!pinfo) {
  	OPAE_MSG("pinfo is NULL");
  	goto out_EINVAL;
  }
  if (pinfo->argsz != sizeof(*pinfo)) {
  	OPAE_MSG("wrong structure size");
  	goto out_EINVAL;
  }
  pinfo->flags = 0;
  pinfo->num_regions = 2;
  pinfo->num_umsgs = 8;
  if (gEnableIRQ) {
  	pinfo->capability = FPGA_PORT_CAP_ERR_IRQ | FPGA_PORT_CAP_UAFU_IRQ;
  	pinfo->num_uafu_irqs = 2;
  } else {
  	pinfo->capability = 0;
  	pinfo->num_uafu_irqs = 0;
  }
  retval = 0;
  errno = 0;
out:
  va_end(argp);
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}

int dfl_port_info(mock_object * m, int request, va_list argp){
  int retval = -1;
  errno = EINVAL;
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  struct dfl_fpga_port_info *pinfo = va_arg(argp, struct dfl_fpga_port_info *);
  if (!pinfo) {
  	OPAE_MSG("pinfo is NULL");
  	goto out_EINVAL;
  }
  if (pinfo->argsz != sizeof(*pinfo)) {
  	OPAE_MSG("wrong structure size");
  	goto out_EINVAL;
  }
  pinfo->flags = 0;
  pinfo->num_regions = 2;
  pinfo->num_umsgs = 0;
/*
  if (gEnableIRQ) {
  	pinfo->capability = FPGA_PORT_CAP_ERR_IRQ | FPGA_PORT_CAP_UAFU_IRQ;
  	pinfo->num_uafu_irqs = 2;
  } else {
  	pinfo->capability = 0;
  	pinfo->num_uafu_irqs = 0;
  }
*/
  retval = 0;
  errno = 0;
out:
  va_end(argp);
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}

class enum_c_p : public opae_base_p<> {
 protected:

  enum_c_p() :
    filter_(nullptr),
    matches_(0)
  {}

  virtual void SetUp() override {
    opae_base_p<>::SetUp();
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);

    matches_ = 0;
    ASSERT_EQ(num_tokens_for(nullptr, matches_), FPGA_OK);
    tokens_.resize(matches_, nullptr);
  }

  void DestroyTokens() {
    for(auto &t: tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
  }

  virtual void TearDown() override {
    DestroyTokens();
    tokens_.clear();

    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    opae_base_p<>::TearDown();
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

  int GetNumMatchedFpga() {
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

  int GetNumDeviceID() {
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
  uint32_t matches_;
  std::vector<fpga_token> tokens_;
};

TEST_P(enum_c_p, nullfilter) {
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas() * 2);

  EXPECT_EQ(fpgaEnumerate(nullptr, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nullmatches) {
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          nullptr), FPGA_INVALID_PARAM);

  EXPECT_EQ(fpgaEnumerate(&filter_, 0,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, nulltokens) {
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          nullptr, 2,
                          &matches_), FPGA_INVALID_PARAM);
}

TEST_P(enum_c_p, object_type) {
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  matches_ = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas());

  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  matches_ = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          nullptr, 0,
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumFpgas());
}

TEST_P(enum_c_p, parent) {
  fpga_token device = get_device_token(0);
  ASSERT_NE(device, nullptr);

  EXPECT_EQ(device_tokens_.size(), GetNumFpgas());

  fpga_token device_clone = nullptr;
  ASSERT_EQ(fpgaCloneToken(device, &device_clone), FPGA_OK);

  ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetParent(filter_, device_clone), FPGA_OK);

  matches_ = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(get_accelerator_tokens(device).size(), matches_);
  EXPECT_EQ(fpgaDestroyToken(&device_clone), FPGA_OK);
}

TEST_P(enum_c_p, segment) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, device.segment), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas() * 2);
  DestroyTokens();

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetSegment(filter_, invalid_device_.segment), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, bus) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetBus(filter_, device.bus), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, 2);
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBus(filter_, invalid_device_.bus), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, device) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, device.device), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas() * 2);

  ASSERT_EQ(fpgaPropertiesSetDevice(filter_, invalid_device_.device), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, function) {
  test_device device = platform_.devices[0];
  if (!device.has_afu) {
    // This test is only valid for the original definition of Port.
    // On a platform that has HEMs, the following rules do not hold.
    GTEST_SKIP();
  }

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, device.function), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumFpgas() * (device.num_vfs == 0 ? 2 : 1));
  DestroyTokens();

  for (int i = 1 ; i < device.num_vfs + 1 ; ++i) {
    matches_ = 0;
    ASSERT_EQ(fpgaPropertiesSetFunction(filter_, i), FPGA_OK);
    EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                            tokens_.data(), tokens_.size(),
                            &matches_), FPGA_OK);
    EXPECT_EQ(matches_, 1);
    DestroyTokens();
  }
}

TEST_P(enum_c_p, invalid_function) {
  test_device device = platform_.devices[0];
  if (!device.has_afu) {
    // A device that has HEM's may have up to 7 functions.
    GTEST_SKIP();
  }

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetFunction(filter_, invalid_device_.function), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, vendor_id) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, device.vendor_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas() * 2);
}

TEST_P(enum_c_p, invalid_vendor_id) {
  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetVendorID(filter_, invalid_device_.vendor_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, device_id) {
  test_device device = platform_.devices[0];
  if (!device.has_afu) {
    // This test is only valid for the original definition of Port.
    // On a platform that has HEMs, the following rules do not hold.
    GTEST_SKIP();
  }

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumDeviceID() * (device.num_vfs == 0 ? 2 : 1));
  DestroyTokens();

  for (int i = 1 ; i < device.num_vfs + 1 ; ++i) {
    matches_ = 0;
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, device.device_id+i), FPGA_OK);
    EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                            tokens_.data(), tokens_.size(),
                            &matches_), FPGA_OK);
    EXPECT_EQ(matches_, 1);
    DestroyTokens();
  }
}

TEST_P(enum_c_p, invalid_device_id) {
  ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, invalid_device_.device_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, object_id_fme) {
  fpga_token device = get_device_token(0);
  ASSERT_NE(device, nullptr);

  fpga_properties prop = nullptr;
  uint64_t object_id = 0;
  EXPECT_EQ(fpgaGetProperties(device, &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  matches_ = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 1);
}

TEST_P(enum_c_p, object_id_fme_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_,
                                      invalid_device_.fme_object_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, object_id_port) {
  fpga_token device = get_device_token(0);
  ASSERT_NE(device, nullptr);
  fpga_token accel = get_accelerator_token(device, 0);
  ASSERT_NE(accel, nullptr);

  uint64_t object_id = 0;
  fpga_properties prop = nullptr;

  EXPECT_EQ(fpgaGetProperties(accel, &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesGetObjectID(prop, &object_id), FPGA_OK);

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_, object_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 1);
 
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

TEST_P(enum_c_p, object_id_port_neg) {
  ASSERT_EQ(fpgaPropertiesSetObjectID(filter_,
                                      invalid_device_.port_object_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, guid) {
  test_device device = platform_.devices[0];
  if (!device.has_afu) {
    // Can't compare afu_id's if there aren't any.
    GTEST_SKIP();
  }

  fpga_guid fme_guid, afu_guid, random_guid;

  ASSERT_EQ(uuid_parse(device.fme_guid, fme_guid), 0);
  ASSERT_EQ(uuid_parse(device.afu_guid, afu_guid), 0);
  ASSERT_EQ(uuid_parse(invalid_device_.afu_guid, random_guid), 0);

  matches_ = 0;
  // fme guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, fme_guid), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, platform_.devices.size());
  DestroyTokens();

  if (device.has_afu) {
    matches_ = 0;
    // afu guid
    ASSERT_EQ(fpgaPropertiesSetGUID(filter_, afu_guid), FPGA_OK);
    EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                            tokens_.data(), tokens_.size(),
                            &matches_), FPGA_OK);
    EXPECT_EQ(matches_, GetMatchedGuidFpgas());
    DestroyTokens();
  }

  // random guid
  ASSERT_EQ(fpgaPropertiesSetGUID(filter_, random_guid), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, clone_token01) {
  matches_ = 0;
  fpga_token token = nullptr;
  EXPECT_EQ(fpgaEnumerate(nullptr, 0,
                          &token, 1,
                          &matches_), FPGA_OK);
  ASSERT_GE(matches_, GetNumFpgas() * 2);
 
  fpga_token clone = nullptr;
  EXPECT_EQ(fpgaCloneToken(token, &clone), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&clone), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&token), FPGA_OK);
}

TEST_P(enum_c_p, clone_wo_src_dst) {
  fpga_token device = get_device_token(0);
  fpga_token clone = nullptr;

  EXPECT_EQ(fpgaCloneToken(NULL, &clone), FPGA_INVALID_PARAM);
  EXPECT_EQ(clone, nullptr);
  EXPECT_EQ(fpgaCloneToken(device, NULL), FPGA_INVALID_PARAM);
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
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, device.num_slots), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumFpgas());
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetNumSlots(filter_, invalid_device_.num_slots), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, bbs_id) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, device.bbs_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, platform_.devices.size());
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBBSID(filter_, invalid_device_.bbs_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, bbs_version) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, device.bbs_version), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, platform_.devices.size());
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetBBSVersion(filter_, invalid_device_.bbs_version), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, state) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, device.state), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas());
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetAcceleratorState(filter_, invalid_device_.state), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, num_mmio) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  system_->register_ioctl_handler(FPGA_PORT_GET_INFO, port_info);
  system_->register_ioctl_handler(DFL_FPGA_PORT_GET_INFO, dfl_port_info);

  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, device.num_mmio), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas());
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumMMIO(filter_, invalid_device_.num_mmio),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST_P(enum_c_p, num_interrupts) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumInterrupts(filter_, device.num_interrupts),
            FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_GE(matches_, GetNumFpgas());
  DestroyTokens();

  ASSERT_EQ(
      fpgaPropertiesSetNumInterrupts(filter_, invalid_device_.num_interrupts),
      FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

TEST(wrapper, validate) {
  EXPECT_EQ(NULL, opae_validate_wrapped_token(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_handle(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_event_handle(NULL));
  EXPECT_EQ(NULL, opae_validate_wrapped_object(NULL));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(enum_c_p);
INSTANTIATE_TEST_SUITE_P(enum_c, enum_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-d5005",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));

class enum_c_mock_p : public enum_c_p {};

TEST_P(enum_c_mock_p, clone_token02) {
  matches_ = 0;
  fpga_token token = nullptr;

  EXPECT_EQ(fpgaEnumerate(nullptr, 0, &token, 1, &matches_), FPGA_OK);
  ASSERT_GT(matches_, 0); 

  fpga_token clone = nullptr;
  // Invalidate the allocation of the wrapped token.
  system_->invalidate_malloc(0, "opae_allocate_wrapped_token");
  EXPECT_EQ(fpgaCloneToken(token, &clone), FPGA_NO_MEMORY);

  EXPECT_EQ(fpgaDestroyToken(&token), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(enum_c_mock_p);
INSTANTIATE_TEST_SUITE_P(enum_c, enum_c_mock_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-n3000",
                                                                             "dfl-d5005",
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));

class enum_c_err_p : public enum_c_p {};

TEST_P(enum_c_err_p, num_errors) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  // fme num_errors
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.fme_num_errors), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumFpgas());
  DestroyTokens();

  if (device.has_afu) {
    // afu num_errors
    matches_ = 0;
    EXPECT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetNumErrors(filter_, device.port_num_errors), FPGA_OK);
    EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                            tokens_.data(), tokens_.size(),
                            &matches_), FPGA_OK);
    EXPECT_GE(matches_, GetNumFpgas());
    DestroyTokens();
  }

  // invalid
  ASSERT_EQ(
        fpgaPropertiesSetNumErrors(filter_, invalid_device_.port_num_errors),
        FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(enum_c_err_p);
INSTANTIATE_TEST_SUITE_P(enum_c, enum_c_err_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-d5005",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));

class enum_c_socket_p : public enum_c_p {};

TEST_P(enum_c_socket_p, socket_id) {
  test_device device = platform_.devices[0];

  matches_ = 0;
  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, device.socket_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, GetNumMatchedFpga() * 2);
  DestroyTokens();

  ASSERT_EQ(fpgaPropertiesSetSocketID(filter_, invalid_device_.socket_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter_, 1,
                          tokens_.data(), tokens_.size(),
                          &matches_), FPGA_OK);
  EXPECT_EQ(matches_, 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(enum_c_socket_p);
INSTANTIATE_TEST_SUITE_P(enum_c, enum_c_socket_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-d5005",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));
