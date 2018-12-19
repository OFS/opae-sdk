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
#include "gtest/gtest.h"
#include "test_system.h"
#include "test_utils.h"

extern "C" {
#include <bitstream_int.h>
#include <opae/access.h>
#include <opae/enum.h>
#include <opae/properties.h>
#include "intel-fpga.h"
#include "reconf_int.h"
#include "token_list_int.h"
#include "xfpga.h"
}

extern "C" {
fpga_result open_accel(fpga_handle handle, fpga_handle *accel);
fpga_result clear_port_errors(fpga_handle handle);
}

using namespace opae::testing;

class reconf_c : public ::testing::TestWithParam<std::string> {
 protected:
  reconf_c()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_), FPGA_OK);

    bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);
    // Valid bitstream - no clk
    std::string version = "630";

    auto fme_guid = platform_.devices[0].fme_guid;
    auto afu_guid = platform_.devices[0].afu_guid;

    // clang-format off
    auto bitstream_j = jobject
    ("version", version)
    ("afu-image", jobject
                  ("interface-uuid", fme_guid)
                  ("magic-no", int32_t(488605312))
                  ("accelerator-clusters", {
                                             jobject
                                             ("total-contexts", int32_t(1))
                                             ("name", "nlb")
                                             ("accelerator-type-uuid", afu_guid)
                                            }
                  )
    )
    ("platform-name", "");
    // clang-format on
    bitstream_valid_no_clk_ =
        system_->assemble_gbs_header(platform_.devices[0], bitstream_j.c_str());
    bitstream_j.put();
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) { 
        EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); 
        handle_ = nullptr;
    }

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
    token_cleanup();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::vector<uint8_t> bitstream_valid_;
  std::vector<uint8_t> bitstream_valid_no_clk_;
};

/**
* @test    set_afu_userclock
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid user clock or handle.
*/
TEST_P(reconf_c, set_afu_userclock) {
  fpga_result result;

  // Open port device
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Null handle
  result = set_afu_userclock(NULL, 0, 0);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid params
  result = set_afu_userclock(handle_, 0, 0);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

/**
* @test    set_fpga_pwr_threshold
* @brief   Tests: set_fpga_pwr_threshold
* @details set_fpga_pwr_threshold sets power threshold
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid power threshold or handle.
*/
TEST_P(reconf_c, set_fpga_pwr_threshold) {
  fpga_result result;
  bool have_powermgmt;
  struct stat _st;

  // Open port device
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Check if power attribute exists in sysfs tree
  struct _fpga_token *token = (struct _fpga_token *)tokens_[0];
  std::string sysfspath(token->sysfspath);
  auto power_mgmt = sysfspath + "/power_mgmt";
  have_powermgmt = stat(power_mgmt.c_str(), &_st) == 0;

  // NULL handle
  result = set_fpga_pwr_threshold(NULL, 0);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Zero GBS power
  result = set_fpga_pwr_threshold(handle_, 0);
  EXPECT_EQ(result, have_powermgmt ? FPGA_OK : FPGA_NOT_FOUND);

  // Exceed FPGA_GBS_MAX_POWER
  result = set_fpga_pwr_threshold(handle_, 65);
  EXPECT_EQ(result, FPGA_NOT_SUPPORTED);

  // Valid power threshold
  result = set_fpga_pwr_threshold(handle_, 60);
  EXPECT_EQ(result, have_powermgmt ? FPGA_OK : FPGA_NOT_FOUND);

  // Invalid token within handle
  struct _fpga_handle *handle = (struct _fpga_handle *)handle_;

  auto t = handle->token;
  handle->token = NULL;

  result = set_fpga_pwr_threshold(handle_, 60);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  handle->token = t;
}

/**
* @test    fpga_reconf_slot
* @brief   Tests: fpgaReconfigureSlot
* @details Returns FPGA_OK if bitstream is valid and is able
*          to reconfigure fpga. Returns error code if
*          bitstream, handle, or parameters are invalid.
*/
TEST_P(reconf_c, fpga_reconf_slot) {
  fpga_result result;
  uint8_t bitstream_empty[] = "";
  uint8_t bitstream_invalid_guid[] =
      "Xeon·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
  uint8_t bitstream_invalid_json[] =
      "XeonFPGA·GBSv001\53\02{\"version\": \"afu-image\"}";
  size_t bitstream_valid_len =
      get_bitstream_header_len(bitstream_valid_.data());
  uint32_t slot = 0;
  int flags = 0;

  // Open port device
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Invalid bitstream - null
  result = xfpga_fpgaReconfigureSlot(handle_, slot, NULL, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid bitstream - empty
  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_empty, 0, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid bitstream - invalid guid
  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_invalid_guid,
                                     bitstream_valid_len, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid bitstream - invalid json
  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_invalid_json,
                                     bitstream_valid_len, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Null handle
  result = xfpga_fpgaReconfigureSlot(NULL, slot, bitstream_valid_.data(),
                                     bitstream_valid_.size(), flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid handle file descriptor
  auto &no_clk_arr = bitstream_valid_no_clk_;
  struct _fpga_handle *handle = (struct _fpga_handle *)handle_;
  uint32_t fddev = handle->fddev;

  handle->fddev = -1;

  result =
      xfpga_fpgaReconfigureSlot(handle_, slot, no_clk_arr.data(), no_clk_arr.size(), flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  handle->fddev = fddev;
}

/**
* @test    open_accel
* @brief   Tests: open_accel_01
* @details Returns FPGA_INVALID_PARAM when calling open_accel with
*          an invalid handle.
*/
TEST_P(reconf_c, open_accel_01) {
  fpga_result result;
  fpga_handle accel = nullptr;

  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Null handle
  result = open_accel(NULL, &accel);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Valid handle
  result = open_accel(handle_, &accel);
  EXPECT_EQ(result, FPGA_OK);

  EXPECT_EQ(xfpga_fpgaClose(accel), FPGA_OK);

  // Invalid object type
  struct _fpga_handle *handle = (struct _fpga_handle *)handle_;
  struct _fpga_token *token = (struct _fpga_token *)&handle->token;

  handle->token = NULL;

  result = open_accel(handle_, &accel);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  handle->token = token;
}

/**
* @test    open_accel
* @brief   Tests: open_accel_02
* @details Returns FPGA_BUSY when calling open_accel with
*          an opened accel handle.
*/
TEST_P(reconf_c, open_accel_02) {
  fpga_properties filter_accel = nullptr;
  std::array<fpga_token, 2> tokens_accel = {{nullptr,nullptr}};
  fpga_handle handle_accel = nullptr;
  fpga_handle accel = nullptr;
  uint32_t num_matches_accel;

  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_accel), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetObjectType(filter_accel, FPGA_ACCELERATOR),
            FPGA_OK);
  ASSERT_EQ(xfpga_fpgaEnumerate(&filter_accel, 1, tokens_accel.data(),
                                tokens_accel.size(), &num_matches_accel),
            FPGA_OK);
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_accel[0], &handle_accel, 0));

  EXPECT_NE(handle_, nullptr);
  EXPECT_NE(handle_accel, nullptr);
  auto result = open_accel(handle_accel, &accel);
  EXPECT_EQ(result, FPGA_BUSY);

  EXPECT_EQ(accel, nullptr);
  EXPECT_EQ(fpgaDestroyProperties(&filter_accel), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaClose(handle_accel), FPGA_OK);
  for (auto &t : tokens_accel) {
    if (t != nullptr) {
      EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
      t = nullptr;
    }
  }
}

INSTANTIATE_TEST_CASE_P(reconf, reconf_c,
                        ::testing::ValuesIn(test_platform::platforms({})));

class reconf_c_mock_p : public ::testing::TestWithParam<std::string> {
 protected:
  reconf_c_mock_p()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

    // assemble valid bitstream header
    auto fme_guid = platform_.devices[0].fme_guid;
    auto afu_guid = platform_.devices[0].afu_guid;

    auto bitstream_j = jobject
    ("version", "640")
    ("afu-image", jobject
                  ("interface-uuid", fme_guid)
                  ("magic-no", int32_t(488605312))
                  ("accelerator-clusters", {
                                             jobject
                                             ("total-contexts", int32_t(1))
                                             ("name", "nlb")
                                             ("accelerator-type-uuid", afu_guid)
                                            }
                  )
    )
    ("platform-name", "");

    bitstream_valid_ =
          system_->assemble_gbs_header(platform_.devices[0], bitstream_j.c_str());
    bitstream_j.put();
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) { 
        EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); 
        handle_ = nullptr;
    }

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
    token_cleanup();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::vector<uint8_t> bitstream_valid_;
};

/**
 * @test    set_afu_userclock
 * @brief   Tests: set_afu_userclock
 * @details When given valid parameters, set_afu_userclock
 *          returns FPGA_NOT_SUPPORTED on mock platforms.
 */
TEST_P(reconf_c_mock_p, set_afu_userclock) {
  EXPECT_EQ(set_afu_userclock(handle_, 312, 156), FPGA_NOT_SUPPORTED);
}

/**
 * @test    fpga_reconf_slot
 * @brief   Tests: fpgaReconfigureSlot
 * @details Returns FPGA_OK if bitstream is valid and is able
 *          to reconfigure the fpga.
 */
TEST_P(reconf_c_mock_p, fpga_reconf_slot) {
  fpga_result result;
  uint32_t slot = 0;
  int flags = 0;

  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_valid_.data(),
                                     bitstream_valid_.size(), flags);
  EXPECT_EQ(result, FPGA_OK);
}

/**
 * @test    fpga_reconf_slot_einval
 * @brief   Tests: fpgaReconfigureSlot
 * @details Register an ioctl handler that returns -1 and sets
 *          errno to EINVAL. fpgaReconfigureSlot should return
 *          FPGA_INVALID_PARAM.
 */
TEST_P(reconf_c_mock_p, fpga_reconf_slot_einval) {
  fpga_result result;
  uint32_t slot = 0;
  int flags = 0;

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_FME_PORT_PR, dummy_ioctl<-1, EINVAL>);
  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_valid_.data(),
                                     bitstream_valid_.size(), flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

/**
 * @test    fpga_reconf_slot_enotsup
 * @brief   Tests: fpgaReconfigureSlot
 * @details Register an ioctl handler that returns -1 and sets
 *          errno to ENOTSUP. fpgaReconfigureSlot should return
 *          FPGA_EXCEPTION.
 */
TEST_P(reconf_c_mock_p, fpga_reconf_slot_enotsup) {
  fpga_result result;
  uint32_t slot = 0;
  int flags = 0;

  // register an ioctl handler that will return -1 and set errno to ENOTSUP
  system_->register_ioctl_handler(FPGA_FME_PORT_PR, dummy_ioctl<-1, ENOTSUP>);
  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_valid_.data(),
                                     bitstream_valid_.size(), flags);
  EXPECT_EQ(result, FPGA_EXCEPTION);
}

INSTANTIATE_TEST_CASE_P(reconf, reconf_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({})));

class reconf_c_hw_skx_p : public reconf_c {
  protected:
    reconf_c_hw_skx_p() {};
};

/**
 * @test    set_afu_userclock
 * @brief   Tests: set_afu_userclock
 * @details Given valid parameters set_afu_userlock returns
 *          FPGA_OK on mcp hw platforms.
 */
TEST_P(reconf_c_hw_skx_p, set_afu_userclock) {
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  EXPECT_EQ(set_afu_userclock(handle_, 312, 156), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(reconf, reconf_c_hw_skx_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"skx-p"})));

class reconf_c_hw_dcp_p : public reconf_c {
  protected:
    reconf_c_hw_dcp_p() {};
};

/**
 * @test    set_afu_userclock
 * @brief   Tests: set_afu_userclock
 * @details Given valid parameters set_afu_userlock returns
 *          FPGA_NOT_SUPPORTED on dcp hw platforms.
 */
TEST_P(reconf_c_hw_dcp_p, set_afu_userclock) {
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  EXPECT_EQ(set_afu_userclock(handle_, 312, 156), FPGA_NOT_SUPPORTED);
}

INSTANTIATE_TEST_CASE_P(reconf, reconf_c_hw_dcp_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"dcp-p"})));

/**
* @test    clear_port_errors
* @brief   Tests: clear_port_errors
* @details Returns FPGA_OK if handle is valid and
*          can clear port errors.
*/
TEST(reconf, clear_port_errors) {
  fpga_result result;

  // Null handle
  result = clear_port_errors(NULL);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

class reconf_c_hw_p : public reconf_c {
  protected:
    reconf_c_hw_p()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

    // assemble valid bitstream header
    auto fme_guid = platform_.devices[0].fme_guid;
    auto afu_guid = platform_.devices[0].afu_guid;

    auto bitstream_j = jobject
    ("version", "640")
    ("afu-image", jobject
                  ("interface-uuid", fme_guid)
                  ("magic-no", int32_t(488605312))
                  ("accelerator-clusters", {
                                             jobject
                                             ("total-contexts", int32_t(1))
                                             ("name", "nlb")
                                             ("accelerator-type-uuid", afu_guid)
                                            }
                  )
    )
    ("platform-name", "");

    bitstream_valid_ =
          system_->assemble_gbs_header(platform_.devices[0], bitstream_j.c_str());
    bitstream_j.put();
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) {
        EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK);
        handle_ = nullptr;
    }

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    system_->finalize();
    token_cleanup();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::vector<uint8_t> bitstream_valid_;
};

/*
 * @test    fpga_reconf_slot_inv_len
 *
 * @details When the bitstream length is invalid, the function
 *          returns FPGA_INVALID_PARAM.
 */
TEST_P(reconf_c_hw_p, fpga_reconf_slot_inv_len) {
  fpga_result result;
  uint32_t slot = 0;
  int flags = 0;

  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_valid_.data(),
                                     -123456789, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaReconfigureSlot(handle_, slot, bitstream_valid_.data(),
                                     123456789, flags);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(reconf, reconf_c_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms()));
