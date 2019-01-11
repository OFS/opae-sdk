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

extern "C" {
#include "token_list_int.h"
fpga_result get_interface_id(fpga_handle, uint64_t*, uint64_t*);
}

#include <uuid/uuid.h>
#include <bitstream_int.h>
#include <types_int.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include "xfpga.h"

using namespace opae::testing;

class metadata_c
    : public ::testing::TestWithParam<std::string> {
 protected:
  metadata_c()
  : handle_(nullptr),
    tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    fpga_guid fme_guid;

    ASSERT_EQ(uuid_parse(platform_.devices[0].fme_guid, fme_guid), 0);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetGUID(filter_, fme_guid), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
						&num_matches_),  FPGA_OK);
    bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);
    mdata_ = platform_.devices[0].mdata;
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) { EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    system_->finalize();
  }

  fpga_handle handle_;
  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::string mdata_;
  std::vector<uint8_t> bitstream_valid_;
};

uint8_t bitstream_null[10] = "abcd";
uint8_t bitstream_invalid_guid[] = "Xeon�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_metadata_size[] = "XeonFPGA�GBSv001S";
uint8_t bitstream_empty[] = "XeonFPGA�GBSv001";
uint8_t bitstream_metadata_length[] = "XeonFPGA�GBSv001S {\"version/\": 640, \"afu-image\":  \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\"\
      : [{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\": \
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_gbs_version[] = "XeonFPGA�GBSv001\53\02\00\00{\"version99\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1, \
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_afu_image[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640 }, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_interface_id[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
      \"power\": 50,  \"magic-no\": 488605312, \"accelerator-clusters\": \
      [{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_interface_id[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_mismatch_interface_id[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"00000000-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_error_interface_id[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"00000000-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_accelerator_id[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\":\
      [{\"total-contexts\": 1, \"name\": \"nlb_400\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_length[] = "XeonFPGA�GBSv001\00\00\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_accelerator[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_magic_no[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no99\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_magic_no[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 000000000, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_json[] = "XeonFPGA�GBSv001\53\02\00\00{\"version\": \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312}, \"platform-name\": \"MCP\"}";

/**
* @test    read_gbs_metadata
* @brief   Tests: read_gbs_metadata
* @details read_gbs_metadata returns BS metadata
*          Then the return value is FPGA_OK
*/
TEST_P(metadata_c, read_gbs_metadata) {
  struct gbs_metadata gbs_metadata;

  // Invalid input parameters - null bitstream and metadata
  fpga_result result = read_gbs_metadata(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameter - null bitstream
  result = read_gbs_metadata(NULL, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameter - null metadata
  result = read_gbs_metadata(bitstream_null, NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input bitstream
  result = read_gbs_metadata(bitstream_null, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid bitstream metadata size
  result = read_gbs_metadata(bitstream_metadata_size, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Zero metadata length with no data
  result = read_gbs_metadata(bitstream_empty, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata length
  result = read_gbs_metadata(bitstream_metadata_length, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input bitstream - no GBS version
  result = read_gbs_metadata(bitstream_no_gbs_version, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Valid metadata
  result = read_gbs_metadata(bitstream_valid_.data(), &gbs_metadata);
  EXPECT_EQ(result, FPGA_OK);

  test_system::instance()->invalidate_malloc();

  // Valid metadata - malloc fail
  result = read_gbs_metadata(bitstream_valid_.data(), &gbs_metadata);
  EXPECT_EQ(result, FPGA_NO_MEMORY);

  // Invalid metadata afu-image node
  result = read_gbs_metadata(bitstream_no_afu_image, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata interface-uuid
  result = read_gbs_metadata(bitstream_no_interface_id, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata afu-uuid
  result = read_gbs_metadata(bitstream_no_accelerator_id, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input bitstream
  result = read_gbs_metadata(bitstream_invalid_length, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input bitstream - no accelerator clusters
  result = read_gbs_metadata(bitstream_no_accelerator, &gbs_metadata);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input bitstream - invalid json
  result = read_gbs_metadata(bitstream_invalid_json, &gbs_metadata);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);
}

/**
* @test    validate_metadata
* @brief   Tests: validate_bitstream_metadata
* @details validate_bitstream_metadata validates BS metadata
*          Returns FPGA_OK if metadata is valid
*/
TEST_P(metadata_c, validate_bitstream_metadata) {
  fpga_result result;

  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Valid metadata
  result = validate_bitstream_metadata(handle_, bitstream_valid_.data());
  EXPECT_EQ(result, FPGA_OK);

  test_system::instance()->invalidate_malloc();

  // Valid metadata - malloc fail
  result = validate_bitstream_metadata(handle_, bitstream_valid_.data());
  EXPECT_EQ(result, FPGA_NO_MEMORY);

  // Invalid input bitstream
  result = validate_bitstream_metadata(handle_, bitstream_invalid_guid);
  EXPECT_NE(result, FPGA_OK);

  // Empty metadata
  result = validate_bitstream_metadata(handle_, bitstream_empty);
  EXPECT_EQ(result, FPGA_OK);

  // Invalid metadata size
  result = validate_bitstream_metadata(handle_, bitstream_metadata_size);
  EXPECT_EQ(result, FPGA_EXCEPTION);

  // Invalid metadata - no magic-no
  result = validate_bitstream_metadata(handle_, bitstream_no_magic_no);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid metadata - invalid interface id
  result = validate_bitstream_metadata(handle_, bitstream_invalid_interface_id);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata - interface ID check failed
  result = validate_bitstream_metadata(handle_, bitstream_mismatch_interface_id);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata - interface ID check failed
  result = validate_bitstream_metadata(handle_, bitstream_error_interface_id);
  EXPECT_NE(result, FPGA_OK);

  // Invalid metadata - no afu-image
  result = validate_bitstream_metadata(handle_, bitstream_no_afu_image);
  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  // Invalid metadata - invalid magic-no
  result = validate_bitstream_metadata(handle_, bitstream_invalid_magic_no);
  EXPECT_EQ(result, FPGA_NOT_FOUND);
}

/**
* @test    get_bitstream_header_len
* @brief   Tests: get_bitstream_header_len
* @details get_bitstream_header_len returns bitstream header length.
*/
TEST_P(metadata_c, get_bitstream_header_len) {
  int len;

  // Valid metadata
  len = get_bitstream_header_len(bitstream_valid_.data());
  EXPECT_EQ(len, mdata_.size() + sizeof(fpga_guid) + sizeof(uint32_t));

  // Invalid guid
  len = get_bitstream_header_len(bitstream_invalid_guid);
  EXPECT_EQ(len, -1);

  // Null bitstream
  len = get_bitstream_header_len(NULL);
  EXPECT_EQ(len, -1);
}

/**
* @test    get_bitstream_json_len
* @brief   Tests: get_bitstream_json_len
* @details get_bitstream_json_len returns json length.
*/
TEST_P(metadata_c, get_bitstream_json_len) {
  int len;

  // Valid metadata
  len = get_bitstream_json_len(bitstream_valid_.data());
  EXPECT_EQ(len, mdata_.size());

  // Invalid metadata
  len = get_bitstream_json_len(bitstream_invalid_guid);
  EXPECT_EQ(len, -1);

  // Null bitstream
  len = get_bitstream_json_len(NULL);
  EXPECT_EQ(len, -1);
}

/**
* @test    get_interface_id_01
* @brief   Tests: get_interface_id
* @details Given invalid params, the function returns FPGA_INVALID_PARAM
*/
TEST_P(metadata_c, get_interface_id_01) {
  uint64_t id_l;
  uint64_t id_h;
  auto _token = (struct _fpga_token *)tokens_[0];
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // Invalid object type
  _token->magic = 0x123;
  auto res = get_interface_id(handle_, &id_l, &id_h);
  EXPECT_EQ(res, FPGA_INVALID_PARAM);

  _token->magic = FPGA_TOKEN_MAGIC;

  res = get_interface_id(handle_, nullptr, nullptr);
  EXPECT_EQ(res, FPGA_INVALID_PARAM);
}

/**
* @test    get_interface_id_02
* @brief   Tests: get_interface_id
* @details Given invalid params, the function returns FPGA_INVALID_PARAM
*/
TEST_P(metadata_c, get_interface_id_02) {
  uint64_t id_l;
  uint64_t id_h;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  struct _fpga_handle  *handle = (struct _fpga_handle *)handle_;

  handle->token = NULL;
  auto res = get_interface_id(handle_, &id_l, &id_h);
  EXPECT_EQ(res, FPGA_INVALID_PARAM);
}

/**
* @test    get_interface_id_03
* @brief   Tests: get_interface_id
* @details Given invalid params, the function returns FPGA_EXCEPTION
*/
TEST_P(metadata_c, get_interface_id_03) {
  std::string sysfs_fme = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.01";
  uint64_t id_l;
  uint64_t id_h;
  auto _token = (struct _fpga_token *)tokens_[0];
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  // invalid file
  strncpy(_token->sysfspath, sysfs_fme.c_str(), SYSFS_PATH_MAX);
  auto res = get_interface_id(handle_, &id_l, &id_h);
  EXPECT_EQ(res, FPGA_EXCEPTION);
}


INSTANTIATE_TEST_CASE_P(metadata, metadata_c, ::testing::ValuesIn(test_platform::keys(true)));
