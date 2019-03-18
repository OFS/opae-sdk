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

int parse_metadata(opae_bitstream_info *info);

fpga_result string_to_guid(const char *guid, fpga_guid *result);

fpga_result get_bitstream_ifc_id(const uint8_t *bitstream,
				 size_t bs_len,
				 fpga_guid *guid);

fpga_result read_bitstream(const char *filename, opae_bitstream_info *info);

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <vector>

#include "gtest/gtest.h"
#include "test_system.h"
#include "safe_string/safe_string.h"

using namespace opae::testing;

class bitstream_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpnull_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    null_gbs_ = system_->assemble_gbs_header(platform_.devices[0]);

    uint8_t *zeros = new uint8_t[4096];
    ASSERT_NE(zeros, nullptr);
    memset_s(zeros, 4096, 0);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) null_gbs_.data(), null_gbs_.size());
    gbs.write((const char *) zeros, 4096);
    gbs.close();

    delete[] zeros;
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
 * @test       parse_err0
 * @brief      Test: parse_metadata
 * @details    When parse_metadata is given a null pointer,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(bitstream_c_p, parse_err0) {
  EXPECT_LT(parse_metadata(nullptr), 0);
}

/**
 * @test       parse_err1
 * @brief      Test: parse_metadata
 * @details    When the parameter to parse_metadata has a data_len field,<br>
 *             that is less than the size of a gbs header,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(bitstream_c_p, parse_err1) {
  opae_bitstream_info info;
  info.data_len = 3;
  EXPECT_LT(parse_metadata(&info), 0);
}

/**
 * @test       parse_err2
 * @brief      Test: parse_metadata
 * @details    When the parameter to parse_metadata has a data field<br>
 *             with an invalid gbs magic value in the first 32 bits,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(bitstream_c_p, parse_err2) {
  opae_bitstream_info info;
  uint32_t blah = 0;
  info.data_len = 20;
  info.data = (uint8_t *) &blah;
  EXPECT_LT(parse_metadata(&info), 0);
}

/**
 * @test       parse
 * @brief      Test: parse_metadata
 * @details    When parse_metadata is called with valid input<br>
 *             the fn returns zero.<br>
 */
TEST_P(bitstream_c_p, parse) {
  opae_bitstream_info info;
  ASSERT_EQ(read_bitstream(tmpnull_gbs_, &info), FPGA_OK);
  *(uint32_t *) info.data = 0x1d1f8680;
  EXPECT_EQ(parse_metadata(&info), 0);
  free(info.data);
}

/**
 * @test       get_bits_err0
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a NULL bitstream buffer,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, get_bits_err0) {
  EXPECT_EQ(get_bitstream_ifc_id(nullptr, 0, nullptr), FPGA_EXCEPTION);
}

/**
 * @test       get_bits_err1
 * @brief      Test: get_bitstream_ifc_id
 * @details    When malloc fails,<br>
 *             get_bitstream_ifc_id returns FPGA_NO_MEMORY.<br>
 */
TEST_P(bitstream_c_p, get_bits_err1) {
  opae_bitstream_info info;
  ASSERT_EQ(read_bitstream(tmpnull_gbs_, &info), FPGA_OK);

  fpga_guid guid;
  system_->invalidate_malloc(0, "get_bitstream_ifc_id");
  EXPECT_EQ(get_bitstream_ifc_id(info.data, info.data_len, &guid), FPGA_NO_MEMORY);
  free(info.data);
}

/**
 * @test       get_bits_err2
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of zero,
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(bitstream_c_p, get_bits_err2) {
  opae_bitstream_info info;
  ASSERT_EQ(read_bitstream(tmpnull_gbs_, &info), FPGA_OK);

  fpga_guid guid;
  *(uint32_t *) (info.data + 16) = 0;
  EXPECT_EQ(get_bitstream_ifc_id(info.data, info.data_len, &guid), FPGA_OK);
  free(info.data);
}

/**
 * @test       get_bits_err3
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of that is invalid,
 *             then the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, get_bits_err3) {
  opae_bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);

  fpga_guid guid;
  *(uint32_t *) (info.data + 16) = 65535;
  EXPECT_EQ(get_bitstream_ifc_id(info.data, info.data_len, &guid), FPGA_EXCEPTION);
  free(info.data);
}

/**
 * @test       read_bits_err0
 * @brief      Test: read_bitstream
 * @details    When given a NULL filename,<br>
 *             read_bitstream returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, read_bits_err0) {
  opae_bitstream_info info;
  EXPECT_EQ(read_bitstream(nullptr, &info), FPGA_INVALID_PARAM);
}

/**
 * @test       read_bits_err1
 * @brief      Test: read_bitstream
 * @details    When the filename is not found,<br>
 *             read_bitstream returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, read_bits_err1) {
  opae_bitstream_info info;
  EXPECT_EQ(read_bitstream("/doesnt/exist", &info), FPGA_EXCEPTION);
}

/**
 * @test       read_bits_err2
 * @brief      Test: read_bitstream
 * @details    When malloc fails,<br>
 *             read_bitstream returns FPGA_EXCEPTION.<br>
 */
TEST_P(bitstream_c_p, read_bits_err2) {
  opae_bitstream_info info;
  system_->invalidate_malloc(0, "read_bitstream");
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), FPGA_EXCEPTION);
}

/**
 * @test       read_bits
 * @brief      Test: read_bitstream
 * @details    When the parameters to read_bitstream are valid,<br>
 *             the function loads the given gbs file into the bitstream_info.<br>
 */
TEST_P(bitstream_c_p, read_bits) {
  opae_bitstream_info info;
  ASSERT_EQ(read_bitstream(tmpnull_gbs_, &info), FPGA_OK);
  free(info.data);
}

/**
 * @test       load_unload
 * @brief      Test: opae_load_bitstream, opae_unload_bitstream
 * @details    When the parameters to opae_load_bitstream are valid,<br>
 *             the function loads the given gbs file.<br>
 *             opae_unload_bitstream releases the gbs file.<br>
 */
TEST_P(bitstream_c_p, load_unload) {
  opae_bitstream_info info;
  ASSERT_EQ(opae_load_bitstream(tmpnull_gbs_, &info), FPGA_OK);
  opae_unload_bitstream(&info);
}

/**
 * @test       string_err
 * @brief      Test: string_to_guid
 * @details    When given an invalid guid string,<br>
 *             the function returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(bitstream_c_p, string_err) {
  fpga_guid guid;
  EXPECT_EQ(string_to_guid("blah", &guid), FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(bitstream_c, bitstream_c_p,
    ::testing::ValuesIn(test_platform::platforms({})));

class skx_p_bitstream_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  std::vector<uint8_t> bitstream_no_uuid()
  {
#if 0
    const char *skx_mdata =
    R"mdata({"version": 640,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "interface-uuid": "1a422218-6dba-448e-b302-425cbcde1406",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";
#endif

    const char *skx_mdata_no_uuid =
    R"mdata({"version": 640,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";

    return system_->assemble_gbs_header(platform_.devices[0],
					skx_mdata_no_uuid);
  }

  std::vector<uint8_t> bitstream_bad_uuid()
  {
    const char *skx_mdata_bad_uuid =
    R"mdata({"version": 640,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "interface-uuid": "wrong",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";

    return system_->assemble_gbs_header(platform_.devices[0],
					skx_mdata_bad_uuid);
  }

  std::vector<uint8_t> bitstream_no_afu_image()
  {
    const char *skx_mdata_smafu =
    R"mdata({"version": 640,
   "smafu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "interface-uuid": "1a422218-6dba-448e-b302-425cbcde1406",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";

    return system_->assemble_gbs_header(platform_.devices[0],
					skx_mdata_smafu);
  }

  test_platform platform_;
  test_system *system_;
};

/**
 * @test       missing_interface_uuid
 * @brief      Test: get_bitstream_ifc_id
 * @details    When given a bitstream that lacks an,<br>
 *             interface uuid, the function returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(skx_p_bitstream_c_p, missing_interface_uuid) {
  std::vector<uint8_t> bits = bitstream_no_uuid();
  fpga_guid guid;
  EXPECT_EQ(get_bitstream_ifc_id(bits.data(), bits.size(), &guid),
            FPGA_INVALID_PARAM);
}

/**
 * @test       bad_interface_uuid
 * @brief      Test:get_bitstream_ifc_id
 * @details    When given a bitstream that has an invalid<br>
 *             interface uuid, the function returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(skx_p_bitstream_c_p, bad_interface_uuid) {
  std::vector<uint8_t> bits = bitstream_bad_uuid();
  fpga_guid guid;
  EXPECT_EQ(get_bitstream_ifc_id(bits.data(), bits.size(), &guid),
            FPGA_INVALID_PARAM);
}

/**
 * @test       no_afu_image
 * @brief      Test:get_bitstream_ifc_id
 * @details    When given a bitstream that has no<br>
 *             afu-image section, the function returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(skx_p_bitstream_c_p, no_afu_image) {
  std::vector<uint8_t> bits = bitstream_no_afu_image();
  fpga_guid guid;
  EXPECT_EQ(get_bitstream_ifc_id(bits.data(), bits.size(), &guid),
            FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(bitstream_c, skx_p_bitstream_c_p,
    ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
