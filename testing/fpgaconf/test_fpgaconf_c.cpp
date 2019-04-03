// Copyright(c) 2018-2019, Intel Corporation
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
#include <libbitstream/bitstream.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include "test_utils.h"
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

void help(void);

void print_err(const char *s, fpga_result res);

void print_msg(unsigned int verbosity, const char *s);

int parse_args(int argc, char *argv[]);

int print_interface_id(fpga_guid actual_interface_id);

int find_fpga(fpga_guid interface_id, fpga_token *fpga);

int program_bitstream(fpga_token token, uint32_t slot_num,
                      opae_bitstream_info *info, int flags);

int fpgaconf_main(int argc, char *argv[]);

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

    // assemble valid bitstream header
    auto fme_guid = platform_.devices[0].fme_guid;
    auto afu_guid = platform_.devices[0].afu_guid;

    auto bitstream_j = jobject
    ("version", int32_t(1))
    ("afu-image", jobject
                  ("clock-frequency-high", int32_t(312))
		  ("clock-frequency-low", int32_t(156))
		  ("power", int32_t(50))
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
    ("platform-name", "platformX");

    bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0], bitstream_j.c_str());
    bitstream_j.put();

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

  void copy_bitstream(std::string path) {
    copy_gbs_ = path;
    std::ifstream src(tmp_gbs_, std::ios::binary);
    std::ofstream dst(copy_gbs_, std::ios::binary);
  
    dst << src.rdbuf(); 
  }

  char tmp_gbs_[20];
  std::string copy_gbs_;
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
  char tmpfilename[] = "tmp-empty-XXXXXX.gbs";
  close(mkstemps(tmpfilename, 4));
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
  strcpy(sixteen, tmpfilename);

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
  EXPECT_STREQ(basename(config.filename), tmpfilename);
  free(config.filename);
  unlink(tmpfilename);
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
 * @test       invalid_parse_args1
 * @brief      Test: parse_args
 * @details    When given an invalid command options,<br>
 *             parse_args populates the global config struct<br>
 *             and the fn returns -1.<br>
 */
TEST_P(fpgaconf_c_p, invalid_parse_args1) {
  char zero[32];
  char one[32];
  char two[32];
  char three[36];
  char four[32];
  char five[32];
  char six[32];
  char seven[32];
  char eight[32];
  char nine[32];
  char ten[48];
  char eleven[32];
  char twelve[32];
  char thirteen[32];
  strcpy(zero, " fpgaconf Q&%^#;'kk/");
  strcpy(one, "-verbosesss \n\t\b\e\a\?");
  strcpy(two, "--n");
  strcpy(three, "--f123 23ksfa;.'/'l|hrce");
  strcpy(four, "--se!gmentt  lsdfhskfa");
  strcpy(five, "0x1234");
  strcpy(six, "-bussssss");
  strcpy(seven, "0x5e");
  strcpy(eight, "-Devic\xF0\x90sss \t\n\b\a\e\v");
  strcpy(nine, "0xab");
  strcpy(ten, " =====%34 -Function \x09\x0B\x0D");
  strcpy(eleven, "3");
  strcpy(twelve, "-Socket__ \xF1-\xF3 \x8F");
  strcpy(thirteen, "2");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen };

  EXPECT_LT(parse_args(14, argv), 0);
}

/**
 * @test       invalid_parse_args2
 * @brief      Test: parse_args
 * @details    When given an invalid command options,<br>
 *             parse_args populates the global config struct<br>
 *             and the fn returns -1.<br>
 */
TEST_P(fpgaconf_c_p, invalid_parse_args2) {
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
  strcpy(zero, "        ");
  strcpy(one, "-v");
  strcpy(two, "-n");
  strcpy(three, "--force");
  strcpy(four, "--segment");
  strcpy(five, "0xffff1234");
  strcpy(six, "-B");
  strcpy(seven, "0x5eeeeeeeee");
  strcpy(eight, "-D");
  strcpy(nine, "0xab124 \xF1 0");
  strcpy(ten, "-F");
  strcpy(eleven, "-33492\t000");
  strcpy(twelve, "-S");
  strcpy(thirteen, "\000 00000000");
  strcpy(fourteen, "-A");

  char *argv[] = { zero, one, two, three,
                   ten, eleven, twelve, thirteen, fourteen,
                   four, five, six, seven, eight, nine};

  EXPECT_LT(parse_args(15, argv), 0);
} 

/*
 * @test       parse_args3
 * @brief      Test: parse_args
 * @details    When given a gbs file that does not exist<br>
 *             parse_args fails at parsing the command line<br>
 *             and the fn returns non-zero value
 */
TEST_P(fpgaconf_c_p, parse_args3) {
  const char *argv[] = { "fpgaconf", "no-file.gbs" };
  EXPECT_NE(parse_args(2, (char**)argv), 0);
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
 * @test       embed_nullchar
 * @brief      Test: fpgaconf_main
 * @details    When the command params contains nullbyte,<br>
 *             fpgaconf_main displays an error and returns non-zero.<br>
 */
TEST_P(fpgaconf_c_p, embed_nullchar1) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "fpgaconf", "-B", "0x5e", "copy_bitstream\0.gbs"};

  EXPECT_NE(fpgaconf_main(4, (char**)argv), 0);
  unlink(copy_gbs_.c_str());
}

TEST_P(fpgaconf_c_p, embed_nullchar2) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "fpgaconf", "-B", "0x5e", "\0 copy_bitstream.gbs"};

  EXPECT_NE(fpgaconf_main(4, (char**)argv), 0);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       encoding_path
 * @brief      Test: fpgaconf_main
 * @details    When command param is encoding path,<br>
 *             fpgaconf_main displays file error and returns non-zero.<br>
 */
TEST_P(fpgaconf_c_p, encoding_path) {
  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char three[34];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-B");
  strcpy(two, "0x5e");

  char *argv[] = { zero, one, two, three};

  // File not found
  strcpy(three, "copy_bitstream%2egbs");
  EXPECT_NE(fpgaconf_main(4, argv), 0);

  // File not found
  memset(three, 0, sizeof(three));
  strcpy(three, "copy_bitstream..gbs");
  EXPECT_NE(fpgaconf_main(4, argv), 0);
  
  // File not found
  memset(three, 0, sizeof(three));
  strcpy(three, "....copy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(4, argv), 0);

  // File not found
  memset(three, 0, sizeof(three));
  strcpy(three, "%252E%252E%252Fcopy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(4, argv), 0);

  unlink(copy_gbs_.c_str());
}

/**
 * @test       relative_path
 * @brief      Test: fpgaconf_main
 * @details    When gbs file locates in parent directory and command params<br>
 *             contains path traversal. On success, fpgaconf_main loads bitstream<br>
 *             and returns zero. Otherwise, it displays file error and returns non-zero.<br>
 */
TEST_P(fpgaconf_c_p, relative_path) {
  copy_bitstream("../copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[32];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-n");
  strcpy(two, "-B");
  strcpy(three, "0x5e");

  char *argv[] = { zero, one, two, three, four};

  strcpy(four, "../copy_bitstream.gbs");
  EXPECT_EQ(fpgaconf_main(5, argv), 0);

  // Fail not found
  memset(four, 0, 32);
  strcpy(four, "../..../copy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(5, argv), 0);

  // Fail not found
  memset(four, 0, 32);
  strcpy(four, "..%2fcopy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(5, argv), 0);

  // Fail not found
  memset(four, 0, 32);
  strcpy(four, "%2e%2e/copy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(5, argv), 0);

  // Fail not found
  memset(four, 0, 32);
  strcpy(four, "%2e%2e%2fcopy_bitstream.gbs");
  EXPECT_NE(fpgaconf_main(5, argv), 0);

  unlink(copy_gbs_.c_str());
}

/**
 * @test       absolute_path_pos 
 * @brief      Test: fpgaconf_main
 * @details    When the command params are valid with absolute gbs path,<br>
 *             fpgaconf_main loads in bitstream and returns 0.<br>
 */
TEST_P(fpgaconf_c_p, absolute_path_pos) {
  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[128];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-n");
  strcpy(two, "-B");
  strcpy(three, "0x5e");

  char *argv[] = { zero, one, two, three, four};
  char *current_path = get_current_dir_name();
  std::string bitstream_path = (std::string)current_path + "/copy_bitstream.gbs";

  strcpy(four, bitstream_path.c_str());
  EXPECT_EQ(fpgaconf_main(5, argv), 0);

  free(current_path);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       absolute_path_neg 
 * @brief      Test: fpgaconf_main
 * @details    When the command params are valid but bitstream data is,<br>
 *             invalid, fpgaconf_main loads in bitstream and fails to <br>
 *             set userclock, it returns none-zero.<br>
 */
TEST_P(fpgaconf_c_p, absolute_path_neg) {
  bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);
  std::ofstream gbs;
  gbs.open(tmp_gbs_, std::ios::out|std::ios::binary);
  gbs.write((const char *) bitstream_valid_.data(), bitstream_valid_.size());
  gbs.close();

  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char three[128];
  strcpy(zero, "fpgaconf");
  strcpy(one, "-B");
  strcpy(two, "0x5e");

  char *argv[] = { zero, one, two, three};
  char *current_path = get_current_dir_name();
  std::string bitstream_path = (std::string)current_path + "/copy_bitstream.gbs";

  strcpy(three, bitstream_path.c_str());
  EXPECT_NE(fpgaconf_main(4, argv), 0);

  free(current_path);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       read_symlink_bs
 * @brief      Test: 
 * @details    Tests for symlink on gbs file. When successful,<br>
 *             fpgaconf_main loads the bitstream into the gbs_data field<br>
 *             and the fn returns 0. When file doesn't exist, fpgaconf_main<br>
 *             displays error and returns -1.<br>
 */
TEST_P(fpgaconf_c_p, main_symlink_bs) {
  copy_bitstream("copy_bitstream.gbs"); 
  const std::string symlink_gbs = "bits_symlink";
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
  strcpy(four, "0x5e");

  char *argv[] = { zero, one, two, three, four,
                   five };

  auto ret = symlink(copy_gbs_.c_str(), symlink_gbs.c_str());
  EXPECT_EQ(ret, 0);
  // Success case
  strcpy(five, symlink_gbs.c_str());
  EXPECT_EQ(fpgaconf_main(6, argv), 0);

  // remove bitstream file
  unlink(copy_gbs_.c_str());

  // Fail case
  EXPECT_NE(fpgaconf_main(6, argv), 0);
  unlink(symlink_gbs.c_str());
}

/**
 * @test       circular_symlink
 * @brief      Test: fpgaconf_c_p
 * @details    Tests for circular symlink on gbs file. <br>
 *             fpgaconf_c_p displays error and returns -1.<br>
 */
TEST_P(fpgaconf_c_p, circular_symlink) {
  const std::string symlink_A = "./link1/bits_symlink_A";
  const std::string symlink_B = "./link2/bits_symlink_B";
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
  strcpy(four, "0x5e");

  char *argv[] = { zero, one, two, three, four,
                   five };

  // Create link directories
  auto ret = mkdir("./link1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  EXPECT_EQ(ret, 0);
  ret = mkdir("./link2", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  EXPECT_EQ(ret, 0);

  // Create circular symlinks
  ret = symlink("link1", symlink_B.c_str());
  EXPECT_EQ(ret, 0);
  ret = symlink("link2", symlink_A.c_str());
  EXPECT_EQ(ret, 0);

  strcpy(five, symlink_A.c_str());
  EXPECT_NE(fpgaconf_main(6, argv), 0);

  memset(five, 0, 20);
  strcpy(five, symlink_B.c_str());
  EXPECT_NE(fpgaconf_main(6, argv), 0);
  
  // Clean up
  unlink(symlink_A.c_str());
  unlink(symlink_B.c_str());
  remove("link1");
  remove("link2");
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

  opae_bitstream_info info;
  ASSERT_EQ(opae_load_bitstream(tmp_gbs_, &info), FPGA_OK);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);

  EXPECT_EQ(program_bitstream(tok, 0, &info, 0), 1);

  EXPECT_EQ(opae_unload_bitstream(&info), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

/**
 * @test       prog_bs1
 * @brief      Test: program_bitstream
 * @details    When config.dry_run is set to false,<br>
 *             program_bitstream attempts the PR,<br>
 *             but fails to set user clocks,<br>
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

  opae_bitstream_info info;
  ASSERT_EQ(opae_load_bitstream(tmp_gbs_, &info), FPGA_OK);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);

  EXPECT_EQ(program_bitstream(tok, 0, &info, 0), -1);

  EXPECT_EQ(opae_unload_bitstream(&info), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

/**
 * @test       prog_bs2
 * @brief      Test: program_bitstream
 * @details    When config.dry_run is set to false,<br>
 *             program_bitstream attempts the PR,<br>
 *             which fails when given an invalid bitstream,<br>
 *             causing the function to return -1.<br>
 */
TEST_P(fpgaconf_c_mock_p, prog_bs2) {
  bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);
  std::ofstream gbs;
  gbs.open(tmp_gbs_, std::ios::out|std::ios::binary);
  gbs.write((const char *) bitstream_valid_.data(), bitstream_valid_.size());
  gbs.close();

  config.target.segment = platform_.devices[0].segment;
  config.target.bus = platform_.devices[0].bus;
  config.target.device = platform_.devices[0].device;
  config.target.function = platform_.devices[0].function;
  config.target.socket = platform_.devices[0].socket_id;

  ASSERT_EQ(config.dry_run, false);

  fpga_guid pr_ifc_id;
  ASSERT_EQ(uuid_parse(platform_.devices[0].fme_guid, pr_ifc_id), 0);

  opae_bitstream_info info;
  ASSERT_EQ(opae_load_bitstream(tmp_gbs_, &info), FPGA_OK);

  fpga_token tok = nullptr;
  EXPECT_EQ(find_fpga(pr_ifc_id, &tok), 1);
  ASSERT_NE(tok, nullptr);

  EXPECT_EQ(program_bitstream(tok, 0, &info, 0), -1);

  EXPECT_EQ(opae_unload_bitstream(&info), FPGA_OK);
  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(fpgaconf_c, fpgaconf_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));

