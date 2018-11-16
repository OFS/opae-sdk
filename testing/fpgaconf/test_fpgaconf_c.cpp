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

#include <opae/fpga.h>

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

struct config {
  unsigned int verbosity;
  bool dry_run;
  enum { INTERACTIVE, /* ask if ambiguous */
         NORMAL,      /* stop if ambiguous */
         AUTOMATIC    /* choose if ambiguous */
       } mode;
  int flags;
  struct target {
    int segment;
    int bus;
    int device;
    int function;
    int socket;
  } target;
  char *filename;
};
extern struct config config;

struct bitstream_info {
  char *filename;
  uint8_t *data;
  size_t data_len;
  uint8_t *rbf_data;
  size_t rbf_len;
  fpga_guid interface_id;
};

fpga_result get_bitstream_ifc_id(const uint8_t *bitstream, fpga_guid *guid);

int parse_metadata(struct bitstream_info *info);

int read_bitstream(char *filename, struct bitstream_info *info);

void help(void);

void print_err(const char *s, fpga_result res);

void print_msg(unsigned int verbosity, const char *s);

int parse_args(int argc, char *argv[]);

int print_interface_id(fpga_guid actual_interface_id);

int find_fpga(fpga_guid interface_id, fpga_token *fpga);

int program_bitstream(fpga_token token, uint32_t slot_num,
                      struct bitstream_info *info, int flags);

int fpgaconf_main(int argc, char *argv[]);

uint64_t read_int_from_bitstream(const uint8_t *bitstream, uint8_t size);

fpga_result string_to_guid(const char *guid, fpga_guid *result);

fpga_result check_bitstream_guid(const uint8_t *bitstream);

#define GUID_LEN      36
#define AFU_NAME_LEN 512

struct gbs_metadata {
  double version;                             // version

  struct afu_image_content {

    uint64_t magic_num;                 // Magic number
    char interface_uuid[GUID_LEN + 1];  // Interface id
    int clock_frequency_high;           // user clock frequency hi
    int clock_frequency_low;            // user clock frequency low
    int power;                          // power

    struct afu_clusters_content {
      char name[AFU_NAME_LEN];     // AFU Name
      int  total_contexts;         // total contexts
      char afu_uuid[GUID_LEN + 1]; // afu guid
    } afu_clusters;

  } afu_image;

};

fpga_result read_gbs_metadata(const uint8_t *bitstream,
                              struct gbs_metadata *gbs_metadata);

}

fpga_guid test_guid = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };

#include <config.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgaconf_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgaconf_c_p() {}

  virtual void SetUp() override {
    strcpy(tmp_gbs_, "tmp-XXXXXX.gbs");
    close(mkstemps(tmp_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmp_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) bitstream_valid_.data(), bitstream_valid_.size());
    gbs.close();

    optind = 0;
    config_ = config;
  }

  virtual void TearDown() override {
    config = config_;

    system_->finalize();
    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmp_gbs_);
    }
  }

  char tmp_gbs_[20];
  std::vector<uint8_t> bitstream_valid_;
  struct config config_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: help
 * @details    help displays the application help message.<br>
 */
TEST_P(fpgaconf_c_p, help) {
  help();
}


/**
 * @test       parse_err0
 * @brief      Test: parse_metadata
 * @details    When parse_metadata is given a null pointer,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, parse_err0) {
  EXPECT_LT(parse_metadata(nullptr), 0);
}

/**
 * @test       parse_err1
 * @brief      Test: parse_metadata
 * @details    When the parameter to parse_metadata has a data_len field,<br>
 *             that is less than the size of a gbs header,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, parse_err1) {
  struct bitstream_info info;
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
TEST_P(fpgaconf_c_p, parse_err2) {
  struct bitstream_info info;
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
TEST_P(fpgaconf_c_p, parse) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmp_gbs_, &info), 0);
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
TEST_P(fpgaconf_c_p, get_bits_err0) {
  EXPECT_EQ(get_bitstream_ifc_id(nullptr, nullptr), FPGA_EXCEPTION);
}

/**
 * @test       get_bits_err1
 * @brief      Test: get_bitstream_ifc_id
 * @details    When malloc fails,<br>
 *             get_bitstream_ifc_id returns FPGA_NO_MEMORY.<br>
 */
TEST_P(fpgaconf_c_p, get_bits_err1) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmp_gbs_, &info), 0);

  fpga_guid guid;
  system_->invalidate_malloc(0, "get_bitstream_ifc_id");
  EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_NO_MEMORY);
  free(info.data);
}

/**
 * @test       get_bits_err2
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of zero,
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(fpgaconf_c_p, get_bits_err2) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmp_gbs_, &info), 0);

  fpga_guid guid;
  *(uint32_t *) (info.data + 16) = 0;
  EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_OK);
  free(info.data);
}

/**
 * @test       get_bits_err3
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of that is invalid,
 *             then the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(fpgaconf_c_p, get_bits_err3) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmp_gbs_, &info), 0);

  fpga_guid guid;
  *(uint32_t *) (info.data + 16) = 65535;
  EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_EXCEPTION);
  free(info.data);
}

/**
 * @test       print_err
 * @brief      Test: print_err
 * @details    print_err sends the given string and a decoding of the fpga_result<br>
 *             to stderr.<br>
 */
TEST_P(fpgaconf_c_p, print_err) {
  print_err("msg", FPGA_OK);
}

/**
 * @test       print_msg
 * @brief      Test: print_msg
 * @details    print_msg sends the given string to stdout<br>
 *             if the given verbosity is less than or equal config.verbosity.<br>
 */
TEST_P(fpgaconf_c_p, print_msg) {
  print_msg(0, "msg");
}

/**
 * @test       read_bits_err0
 * @brief      Test: read_bitstream
 * @details    When given a NULL filename,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, read_bits_err0) {
  struct bitstream_info info;
  EXPECT_LT(read_bitstream(nullptr, &info), 0);
}

/**
 * @test       read_bits_err1
 * @brief      Test: read_bitstream
 * @details    When the filename is not found,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, read_bits_err1) {
  struct bitstream_info info;
  EXPECT_LT(read_bitstream((char *) "/doesnt/exist", &info), 0);
}

/**
 * @test       read_bits_err2
 * @brief      Test: read_bitstream
 * @details    When malloc fails,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, read_bits_err2) {
  struct bitstream_info info;
  system_->invalidate_malloc(0, "read_bitstream");
  EXPECT_LT(read_bitstream(tmp_gbs_, &info), 0);
}

/**
 * @test       read_bits
 * @brief      Test: read_bitstream
 * @details    When the parameters to read_bitstream are valid,<br>
 *             the function loads the given gbs file into the bitstream_info.<br>
 */
TEST_P(fpgaconf_c_p, read_bits) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmp_gbs_, &info), 0);
  free(info.data);
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When given an invalid command option,<br>
 *             parse_args returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, parse_args0) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_LT(parse_args(2, argv), 0);
}

/**
 * @test       parse_args1
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct<br>
 *             and the fn returns 0.<br>
 */
TEST_P(fpgaconf_c_p, parse_args1) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  char six[20];
  char seven[20];
  char eight[20];
  char nine[20];
  char ten[20];
  char eleven[20];
  char twelve[20];
  char thirteen[20];
  char fourteen[20];
  char fifteen[20];
  char sixteen[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-v");
  strcpy(two, "-n");
  strcpy(three, "--force");
  strcpy(four, "--segment");
  strcpy(five, "0x1234");
  strcpy(six, "-B");
  strcpy(seven, "0x5e");
  strcpy(eight, "-D");
  strcpy(nine, "0xab");
  strcpy(ten, "-F");
  strcpy(eleven, "3");
  strcpy(twelve, "-S");
  strcpy(thirteen, "2");
  strcpy(fourteen, "-A");
  strcpy(fifteen, "-I");
  strcpy(sixteen, "file.gbs");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen,
                   fifteen, sixteen };

  EXPECT_EQ(parse_args(17, argv), 0);
  EXPECT_EQ(config.verbosity, 1);
  EXPECT_NE(config.dry_run, 0);
  EXPECT_NE(config.flags & FPGA_RECONF_FORCE, 0);
  EXPECT_EQ(config.target.segment, 0x1234);
  EXPECT_EQ(config.target.bus, 0x5e);
  EXPECT_EQ(config.target.device, 0xab);
  EXPECT_EQ(config.target.function, 3);
  EXPECT_EQ(config.target.socket, 2);
  EXPECT_EQ(config.mode, 0);
  ASSERT_NE(config.filename, nullptr);
  EXPECT_STREQ(config.filename, "file.gbs");
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given "-h",<br>
 *             parse_args prints the help message and returns a negative value.<br>
 */
TEST_P(fpgaconf_c_p, parse_args2) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-h");

  char *argv[] = { zero, one };

  EXPECT_LT(parse_args(2, argv), 0);
}

/**
 * @test       ifc_id1
 * @brief      Test: print_interface_id
 * @details    When the given config.target settings match no device,<br>
 *             print_interface_id returns 0.<br>
 */
TEST_P(fpgaconf_c_p, ifc_id1) {
  config.target.bus = 0xff;
  EXPECT_EQ(print_interface_id(test_guid), 0);
}

/**
 * @test       find_fpga0
 * @brief      Test: find_fpga
 * @details    When the given config.target settings match no device,<br>
 *             find_fpga returns 0.<br>
 */
TEST_P(fpgaconf_c_p, find_fpga0) {
  config.target.bus = 0xff;
  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(test_guid, &tok), 0);
  EXPECT_EQ(tok, nullptr);
}

/**
 * @test       main0
 * @brief      Test: fpgaconf_main
 * @details    When the command params are invalid,<br>
 *             fpgaconf_main returns 1.<br>
 */
TEST_P(fpgaconf_c_p, main0) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_EQ(fpgaconf_main(2, argv), 1);
}

/**
 * @test       main1
 * @brief      Test: fpgaconf_main
 * @details    When the command params are valid,<br>
 *             and they identify a valid accelerator device,<br>
 *             fpgaconf_main loads the bitstream, finds the device,<br>
 *             and PR's it, returning 0.<br>
 */
TEST_P(fpgaconf_c_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  char six[20];
  char seven[20];
  char eight[20];
  char nine[20];
  char ten[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-n");
  strcpy(two, "--segment");
  sprintf(three, "%d", platform_.devices[0].segment);
  strcpy(four, "-B");
  sprintf(five, "%d", platform_.devices[0].bus);
  strcpy(six, "-D");
  sprintf(seven, "%d", platform_.devices[0].device);
  strcpy(eight, "-F");
  sprintf(nine, "%d", platform_.devices[0].function);
  strcpy(ten, tmp_gbs_);

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
		   ten };

  EXPECT_EQ(fpgaconf_main(11, argv), 0);
}

/**
 * @test       main2
 * @brief      Test: fpgaconf_main
 * @details    When the command params are valid,<br>
 *             but no valid accelerator device can be found,<br>
 *             fpgaconf_main displays an error and returns non-zero.<br>
 */
TEST_P(fpgaconf_c_p, main2) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-v");
  strcpy(two, "-n");
  strcpy(three, "-B");
  strcpy(four, "0xff");
  strcpy(five, tmp_gbs_);

  char *argv[] = { zero, one, two, three, four,
                   five };

  EXPECT_NE(fpgaconf_main(6, argv), 0);
}

/**
 * @test       main_symlink
 * @brief      Test: fpgaconf_main
 * @details    When the command params are valid,<br>
 *             but the input bitstream file is a symlink<br>
 *             to a bitstream, fpgaconf returns non-zero.<br>
 */
TEST_P(fpgaconf_c_p, main_symlink) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-v");
  strcpy(two, "-n");
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "test_symlink");

  char *argv[] = { zero, one, two, three, four, five };

  ASSERT_EQ(symlink(tmp_gbs_, "test_symlink"), 0);

  EXPECT_NE(fpgaconf_main(6, argv), 0);
  EXPECT_EQ(unlink("test_symlink"), 0);
}

/**
 * @test       str_to_guid
 * @brief      Test: string_to_guid
 * @details    When the given string does not represent a valid GUID,<br>
 *             string_to_guid returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(fpgaconf_c_p, str_to_guid) {
  fpga_guid guid;
  EXPECT_EQ(string_to_guid("abc", &guid), FPGA_INVALID_PARAM);
}

/**
 * @test       read_int
 * @brief      Test: read_int_from_bitstream
 * @details    Verifies read_int_from_bitstream for all valid size.<br>
 *             Also attempts an invalid size.<br>
 */
TEST_P(fpgaconf_c_p, read_int) {
  uint64_t data = 0xdeadbeefdecafbad;
  EXPECT_EQ(read_int_from_bitstream((const uint8_t *) &data, 1), 0xad);
  EXPECT_EQ(read_int_from_bitstream((const uint8_t *) &data, 2), 0xfbad);
  EXPECT_EQ(read_int_from_bitstream((const uint8_t *) &data, 4), 0xdecafbad);
  EXPECT_EQ(read_int_from_bitstream((const uint8_t *) &data, 8), 0xdeadbeefdecafbad);
  EXPECT_EQ(read_int_from_bitstream((const uint8_t *) &data, 7), 0);
}

/**
 * @test       check_guid
 * @brief      Test: check_bitstream_guid
 * @details    When the given bitstream pointer has a GUID<br>
 *             that does not match the GBS Metadata GUID,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(fpgaconf_c_p, check_guid) {
  fpga_guid zero = { 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0 };
  EXPECT_EQ(check_bitstream_guid((const uint8_t *) zero), FPGA_INVALID_PARAM);
}

uint8_t bitstream_null[10] = "abcd";
uint8_t bitstream_invalid_guid[] = "Xeon·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_metadata_size[] = "XeonFPGA·GBSv001S";
uint8_t bitstream_empty[] = "XeonFPGA·GBSv001";
uint8_t bitstream_metadata_length[] = "XeonFPGA·GBSv001S {\"version/\": 640, \"afu-image\":  \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\"\
      : [{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\": \
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_gbs_version[] = "XeonFPGA·GBSv001\53\02\00\00{\"version99\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1, \
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_afu_image[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640 }, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_interface_id[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
      \"power\": 50,  \"magic-no\": 488605312, \"accelerator-clusters\": \
      [{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_interface_id[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_mismatch_interface_id[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"00000000-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_error_interface_id[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"00000000-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_accelerator_id[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\":\
      [{\"total-contexts\": 1, \"name\": \"nlb_400\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_length[] = "XeonFPGA·GBSv001\00\00\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_accelerator[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_no_magic_no[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no99\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_magic_no[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 000000000, \"accelerator-clusters\": [{\"total-contexts\": 1,\
      \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
      \"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
uint8_t bitstream_invalid_json[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": \"afu-image\":\
      {\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
      \"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
      \"magic-no\": 488605312}, \"platform-name\": \"MCP\"}";

/**
* @test    read_gbs_metadata
* @brief   Tests: read_gbs_metadata
* @details read_gbs_metadata returns BS metadata
*          Then the return value is FPGA_OK
*/
TEST_P(fpgaconf_c_p, read_gbs_metadata) {
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

  system_->invalidate_malloc();

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

INSTANTIATE_TEST_CASE_P(fpgaconf_c, fpgaconf_c_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));


class fpgaconf_c_mock_p : public fpgaconf_c_p{
  protected:
    fpgaconf_c_mock_p(){}
};

/**
 * @test       ifc_id0
 * @brief      Test: print_interface_id
 * @details    When the config.target struct is populated with<br>
 *             bus, device, function, and socket,<br>
 *             print_interface_id uses those settings for enumeration,<br>
 *             returning the number of matches found.<br>
 */
TEST_P(fpgaconf_c_mock_p, ifc_id0) {
  config.target.segment = platform_.devices[0].segment;
  config.target.bus = platform_.devices[0].bus;
  config.target.device = platform_.devices[0].device;
  config.target.function = platform_.devices[0].function;
  config.target.socket = platform_.devices[0].socket_id;
  EXPECT_EQ(print_interface_id(test_guid), 1);
}

/**
 * @test       find_fpga1
 * @brief      Test: find_fpga
 * @details    When the config.target struct is populated with<br>
 *             bus, device, function, and socket,<br>
 *             find_fpga uses those settings in conjunction with the
 *             given PR interface ID for enumeration,<br>
 *             returning the number of matches found.<br>
 */
TEST_P(fpgaconf_c_mock_p, find_fpga1) {
  config.target.segment = platform_.devices[0].segment;
  config.target.bus = platform_.devices[0].bus;
  config.target.device = platform_.devices[0].device;
  config.target.function = platform_.devices[0].function;
  config.target.socket = platform_.devices[0].socket_id;

  fpga_guid pr_ifc_id;
  ASSERT_EQ(uuid_parse(platform_.devices[0].fme_guid, pr_ifc_id), 0);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

/**
 * @test       prog_bs0
 * @brief      Test: program_bitstream
 * @details    When config.dry_run is set to true,<br>
 *             program_bitstream skips the PR step,<br>
 *             and the fn returns 1.<br>
 */
TEST_P(fpgaconf_c_mock_p, prog_bs0) {
  config.target.segment = platform_.devices[0].segment;
  config.target.bus = platform_.devices[0].bus;
  config.target.device = platform_.devices[0].device;
  config.target.function = platform_.devices[0].function;
  config.target.socket = platform_.devices[0].socket_id;

  config.dry_run = true;

  fpga_guid pr_ifc_id;
  ASSERT_EQ(uuid_parse(platform_.devices[0].fme_guid, pr_ifc_id), 0);

  struct bitstream_info info;
  ASSERT_EQ(read_bitstream(tmp_gbs_, &info), 0);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);

  EXPECT_EQ(program_bitstream(tok, 0, &info, 0), 1);

  free(info.data);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

/**
 * @test       prog_bs1
 * @brief      Test: program_bitstream
 * @details    When config.dry_run is set to false,<br>
 *             program_bitstream attempts the PR,<br>
 *             which fails when given an invalid bitstream,<br>
 *             causing the function to return -1.<br>
 */
TEST_P(fpgaconf_c_mock_p, prog_bs1) {
  config.target.segment = platform_.devices[0].segment;
  config.target.bus = platform_.devices[0].bus;
  config.target.device = platform_.devices[0].device;
  config.target.function = platform_.devices[0].function;
  config.target.socket = platform_.devices[0].socket_id;

  ASSERT_EQ(config.dry_run, false);

  fpga_guid pr_ifc_id;
  ASSERT_EQ(uuid_parse(platform_.devices[0].fme_guid, pr_ifc_id), 0);

  struct bitstream_info info;
  ASSERT_EQ(read_bitstream(tmp_gbs_, &info), 0);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);

  EXPECT_EQ(program_bitstream(tok, 0, &info, 0), -1);

  free(info.data);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(fpgaconf_c, fpgaconf_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));


