// Copyright(c) 2019, Intel Corporation
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

#include "bitstream.h"

extern "C" {

fpga_result opae_bitstream_read_file(const char *file,
				     uint8_t **buf,
				     size_t *len);

void opae_resolve_legacy_bitstream(opae_bitstream_info *info);

void *opae_bitstream_parse_metadata(const char *metadata,
				    fpga_guid pr_interface_id,
				    int *version);

fpga_result opae_resolve_bitstream(opae_bitstream_info *info);

extern fpga_guid valid_GBS_guid;

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <vector>

#include "gtest/gtest.h"
#include "test_system.h"
#include "safe_string/safe_string.h"

const fpga_guid guid = {
  0x02, 0x7f, 0x3a, 0x1a,
  0xcb, 0x3b,
  0x0c, 0x87,
  0x53, 0x4b,
  0x56, 0x7d, 0x4a, 0xf6, 0x93, 0xe9
};
const fpga_guid guid_reversed = {
  0xe9, 0x93, 0xf6, 0x4a,
  0x7d, 0x56,
  0x4b, 0x53,
  0x87, 0x0c,
  0x3b, 0xcb, 0x1a, 0x3a, 0x7f, 0x02
};

using namespace opae::testing;

class bitstream_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpnull_gbs_, 4));

    null_gbs_ = system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) null_gbs_.data(), null_gbs_.size());
    gbs.close();
  }

  virtual void TearDown() override {
    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
        unlink(tmpnull_gbs_);
    }
  }

  char tmpnull_gbs_[20];
  std::vector<uint8_t> null_gbs_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       read_err0
 * @brief      Test: opae_bitstream_read_file
 * @details    If the given file doesn't exist,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, read_err0) {
  uint8_t *buf = nullptr;
  size_t len = 0;
  EXPECT_EQ(opae_bitstream_read_file("doesntexist", &buf, &len),
	    FPGA_EXCEPTION);
}

/**
 * @test       is_legacy
 * @brief      Test: opae_is_legacy_bitstream
 * @details    If the given info pointer is that for a<br>
 *             legacy format bitstream,<br>
 *             the fn returns true.<br>
 */
TEST_P(bitstream_c_p, is_legacy) {
  opae_legacy_bitstream_header hdr;
  hdr.legacy_magic = OPAE_LEGACY_BITSTREAM_MAGIC;

  opae_bitstream_info info;
  info.data = (uint8_t *)&hdr;
  info.data_len = sizeof(hdr);

  EXPECT_TRUE(opae_is_legacy_bitstream(&info));
}

/**
 * @test       resolve_legacy
 * @brief      Test: opae_resolve_legacy_bitstream
 * @details    Given an opae_bitstream_info with data and<br>
 *             data_len fields populated,<br>
 *             opae_resolve_legacy_bitstream fills the<br>
 *             pr_interface_id, rbf_data, and rbf_len<br>
 *             fields appropriately.<br>
 */
TEST_P(bitstream_c_p, resolve_legacy) {
  char buf[sizeof(opae_legacy_bitstream_header) + sizeof(uint32_t)];

  opae_legacy_bitstream_header *hdr = 
    (opae_legacy_bitstream_header *)buf;
  hdr->legacy_magic = OPAE_LEGACY_BITSTREAM_MAGIC;
  memcpy(hdr->legacy_pr_ifc_id, guid, sizeof(guid));

  opae_bitstream_info info;
  info.data = (uint8_t *)hdr;
  info.data_len = sizeof(buf);
  info.metadata_version = 0;
  info.parsed_metadata = nullptr;

  opae_resolve_legacy_bitstream(&info);

  EXPECT_EQ(info.rbf_data, (uint8_t *)&buf[sizeof(opae_legacy_bitstream_header)]);
  EXPECT_EQ(info.rbf_len, sizeof(uint32_t));
  EXPECT_EQ(memcmp(info.pr_interface_id, guid_reversed, sizeof(guid_reversed)), 0);
  EXPECT_EQ(info.metadata_version, 0);
  EXPECT_EQ(info.parsed_metadata, nullptr);
}

/**
 * @test       parse_err0
 * @brief      Test: opae_bitstream_parse_metadata
 * @details    If the given metadata string is not valid JSON,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(bitstream_c_p, parse_err0) {
    const char *mdata =
    R"mdata({
)mdata";
  fpga_guid guid;
  int ver = 0;

  EXPECT_EQ(opae_bitstream_parse_metadata(mdata, guid, &ver), nullptr);
}

/**
 * @test       parse_err1
 * @brief      Test: opae_bitstream_parse_metadata
 * @details    If the given metadata string contains no version key,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(bitstream_c_p, parse_err1) {
    const char *mdata =
    R"mdata({
})mdata";
  fpga_guid guid;
  int ver = 0;

  EXPECT_EQ(opae_bitstream_parse_metadata(mdata, guid, &ver), nullptr);
}

/**
 * @test       parse_err2
 * @brief      Test: opae_bitstream_parse_metadata
 * @details    If the given metadata string contains a version key,<br>
 *             the type of which is not integer,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(bitstream_c_p, parse_err2) {
    const char *mdata =
    R"mdata({
  "version": "ver"
})mdata";
  fpga_guid guid;
  int ver = 0;

  EXPECT_EQ(opae_bitstream_parse_metadata(mdata, guid, &ver), nullptr);
}

/**
 * @test       parse_err3
 * @brief      Test: opae_bitstream_parse_metadata
 * @details    If the given metadata string contains a version key<br>
 *             which matches no valid metadata version,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(bitstream_c_p, parse_err3) {
    const char *mdata =
    R"mdata({
  "version": 99
})mdata";
  fpga_guid guid;
  int ver = 0;

  EXPECT_EQ(opae_bitstream_parse_metadata(mdata, guid, &ver), nullptr);
}

/**
 * @test       resolve_err0
 * @brief      Test: opae_resolve_bitstream
 * @details    Given an opae_bitstream_info that has a guid<br>
 *             that does not match the valid GBS guid,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, resolve_err0) {
  opae_bitstream_header hdr;
  memcpy(hdr.valid_gbs_guid, valid_GBS_guid, sizeof(fpga_guid));
  hdr.valid_gbs_guid[0] = ~hdr.valid_gbs_guid[0];
  hdr.metadata_length = 1;

  opae_bitstream_info info;
  info.data = (uint8_t *)&hdr;
  info.data_len = sizeof(hdr);
  info.filename = (char*)"dummy_file.txt";

  EXPECT_EQ(opae_resolve_bitstream(&info), FPGA_INVALID_PARAM);
}

/**
 * @test       resolve_err1
 * @brief      Test: opae_resolve_bitstream
 * @details    Given an opae_bitstream_info that has a metadata_length<br>
 *             field that causes the header size to exceed the file size,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, resolve_err1) {
  opae_bitstream_header hdr;
  memcpy(hdr.valid_gbs_guid, valid_GBS_guid, sizeof(fpga_guid));
  hdr.metadata_length = 2;

  opae_bitstream_info info;
  info.filename = tmpnull_gbs_;
  info.data = (uint8_t *)&hdr;
  info.data_len = sizeof(hdr);

  EXPECT_EQ(opae_resolve_bitstream(&info), FPGA_INVALID_PARAM);
}

/**
 * @test       load_err0
 * @brief      Test: opae_load_bitstream
 * @details    If either of the parameters is NULL,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, load_err0) {
  EXPECT_EQ(opae_load_bitstream(tmpnull_gbs_, nullptr), FPGA_INVALID_PARAM);
}

/**
 * @test       load_err1
 * @brief      Test: opae_load_bitstream
 * @details    If the given file name doesn't exist,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, load_err1) {
  opae_bitstream_info info;
  EXPECT_EQ(opae_load_bitstream("doesntexist", &info), FPGA_INVALID_PARAM);
}

/**
 * @test       load_ok0
 * @brief      Test: opae_load_bitstream
 * @details    If the given opae_bitstream_info represents a<br>
 *             legacy formatted bitstream,<br>
 *             the fn resolve it and returns FPGA_OK.<br>
 */
TEST_P(bitstream_c_p, load_ok0) {
  opae_legacy_bitstream_header hdr;
  hdr.legacy_magic = OPAE_LEGACY_BITSTREAM_MAGIC;
  memcpy(hdr.legacy_pr_ifc_id, guid, sizeof(fpga_guid));

  std::ofstream gbs;
  gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
  gbs.write((const char *)&hdr, sizeof(hdr));
  gbs.close();

  opae_bitstream_info info;
  EXPECT_EQ(opae_load_bitstream(tmpnull_gbs_, &info), FPGA_OK);
  EXPECT_STREQ(tmpnull_gbs_, info.filename);
  ASSERT_NE(info.data, nullptr);
  EXPECT_EQ(info.data_len, sizeof(hdr));
  EXPECT_EQ(info.rbf_data, info.data + sizeof(hdr));
  EXPECT_EQ(info.rbf_len, 0);
  EXPECT_EQ(memcmp(info.pr_interface_id, guid_reversed, sizeof(fpga_guid)), 0);
  EXPECT_EQ(info.metadata_version, 0);
  EXPECT_EQ(info.parsed_metadata, nullptr);
  EXPECT_EQ(opae_unload_bitstream(&info), FPGA_OK);
}

/**
 * @test       unload_err0
 * @brief      Test: opae_unload_bitstream
 * @details    When passed NULL,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, unload_err0) {
  EXPECT_EQ(opae_unload_bitstream(nullptr), FPGA_INVALID_PARAM);
}

/**
 * @test       unload_err1
 * @brief      Test: opae_unload_bitstream
 * @details    When passed an opae_bitstream_info that<br>
 *             has an unsupported metadata version,
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, unload_err1) {
  opae_bitstream_info info;
  info.data = nullptr;
  info.parsed_metadata = malloc(4);
  uint8_t *save = (uint8_t *)info.parsed_metadata;
  info.metadata_version = 99;

  EXPECT_EQ(opae_unload_bitstream(&info), FPGA_EXCEPTION);

  free(save);
}

INSTANTIATE_TEST_CASE_P(bitstream_c, bitstream_c_p,
    ::testing::ValuesIn(test_platform::platforms({})));


class mock_bitstream_c_p : public bitstream_c_p {};

/**
 * @test       read_err1
 * @brief      Test: opae_bitstream_read_file
 * @details    If malloc fails,<br>
 *             the fn returns FPGA_NO_MEMORY.<br>
 */
TEST_P(mock_bitstream_c_p, read_err1) {
  uint8_t *buf = nullptr;
  size_t len = 0;
  system_->invalidate_malloc(0, "opae_bitstream_read_file");
  EXPECT_EQ(opae_bitstream_read_file(tmpnull_gbs_, &buf, &len), FPGA_NO_MEMORY);
}

/**
 * @test       resolve_err2
 * @brief      Test: opae_resolve_bitstream
 * @details    When malloc fails<br>
 *             the fn returns FPGA_NO_MEMORY.<br>
 */
TEST_P(mock_bitstream_c_p, resolve_err2) {
  opae_bitstream_header hdr;
  memcpy(hdr.valid_gbs_guid, valid_GBS_guid, sizeof(fpga_guid));
  hdr.metadata_length = 1;

  opae_bitstream_info info;
  info.filename = tmpnull_gbs_;
  info.data = (uint8_t *)&hdr;
  info.data_len = sizeof(hdr);

  system_->invalidate_malloc(0, "opae_resolve_bitstream");
  EXPECT_EQ(opae_resolve_bitstream(&info), FPGA_NO_MEMORY);
}

INSTANTIATE_TEST_CASE_P(bitstream_c, mock_bitstream_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({})));
