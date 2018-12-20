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

struct  CoreIdleCommandLine
{
        int      segment;
        int      bus;
        int      device;
        int      function;
        int      socket;
        char     filename[512];
        uint8_t *gbs_data;
        size_t   gbs_len;

};
extern struct CoreIdleCommandLine coreidleCmdLine;

void CoreidleAppShowHelp();

void print_err(const char *s, fpga_result res);

int coreidle_main(int argc, char *argv[]);

int ParseCmds(struct CoreIdleCommandLine *coreidleCmdLine,
	      int argc,
	      char *argv[]);

int read_bitstream(struct CoreIdleCommandLine *cmdline);

}

#include <config.h>
#include <opae/fpga.h>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class coreidle_main_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  coreidle_main_c_p() {}

  virtual void SetUp() override {
    strcpy(tmp_gbs_, "tmp-XXXXXX.gbs");
    close(mkstemps(tmp_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    std::vector<uint8_t> gbs_file = system_->assemble_gbs_header(platform_.devices[0]);
    std::ofstream gbs;
    gbs.open(tmp_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) gbs_file.data(), gbs_file.size());
    gbs.close();

    optind = 0;
    cmd_line_ = coreidleCmdLine;
  }

  virtual void TearDown() override {
    coreidleCmdLine = cmd_line_;

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

  struct CoreIdleCommandLine cmd_line_;
  char tmp_gbs_[20];
  std::string copy_gbs_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: CoreidleAppShowHelp
 * @details    CoreidleAppShowHelp displays the application help message.<br>
 */
TEST_P(coreidle_main_c_p, help) {
  CoreidleAppShowHelp();
}

/**
 * @test       print_err
 * @brief      Test: print_err
 * @details    print_err prints the given string and a decoding of<br>
 *             the given fpga_result to stderr.<br>
 */
TEST_P(coreidle_main_c_p, print_err) {
  print_err("msg", FPGA_OK);
}

/**
 * @test       main_help0
 * @brief      Test: coreidle_main
 * @details    When called with no parameters,<br>
 *             coreidle_main displays the application help and returns 1.<br>
 */
TEST_P(coreidle_main_c_p, main_help0) {
  char zero[20];
  strcpy(zero, "coreidle");
  char *argv[] = { zero };

  EXPECT_EQ(coreidle_main(1, argv), 1);
}

/**
 * @test       main_err0
 * @brief      Test: coreidle_main
 * @details    When called with an invalid command parameter,<br>
 *             coreidle_main displays an error message and returns 2.<br>
 */
TEST_P(coreidle_main_c_p, main_err0) {
  char zero[20];
  char one[20];
  strcpy(zero, "coreidle");
  strcpy(one, "-Y");
  char *argv[] = { zero, one };

  EXPECT_EQ(coreidle_main(2, argv), 2);
}

/**
 * @test       main_err1
 * @brief      Test: coreidle_main
 * @details    When called with command parameters that specify<br>
 *             a device that doesn't exist,<br>
 *             coreidle_main returns non-zero.
 */
TEST_P(coreidle_main_c_p, main_err1) {
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
  strcpy(zero, "coreidle");
  strcpy(one, "-G");
  strcpy(two, tmp_gbs_);
  strcpy(three, "--segment");
  strcpy(four, "0x9999");
  strcpy(five, "-B");
  strcpy(six, "99");
  strcpy(seven, "-D");
  strcpy(eight, "99");
  strcpy(nine, "-F");
  strcpy(ten, "7");
  strcpy(eleven, "-S");
  strcpy(twelve, "99");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  EXPECT_NE(coreidle_main(13, argv), 0);
}


/**
 * @test       main0
 * @brief      Test: coreidle_main
 * @details    When called with parameters that specify<br>
 *             a device that is found,<br>
 *             but that device is a TDP+ SKU,<br>
 *             coreidle_main runs to completion, returns 1.<br>
 */
TEST_P(coreidle_main_c_p, main0) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "coreidle");
  strcpy(one, "-G");
  strcpy(two, tmp_gbs_);

  char *argv[] = { zero, one, two };

  EXPECT_EQ(coreidle_main(3, argv), 1);
}

/**
 * @test       parse0
 * @brief      Test: ParseCmds
 * @details    When given "-h",<br>
 *             ParseCmds displays the app help and returns -2.<br>
 */
TEST_P(coreidle_main_c_p, parse0) {
  char zero[20];
  char one[20];
  strcpy(zero, "coreidle");
  strcpy(one, "-h");
  char *argv[] = { zero, one };

  struct CoreIdleCommandLine cmd;
  EXPECT_EQ(ParseCmds(&cmd, 2, argv), -2);
}

/**
 * @test       parse1
 * @brief      Test: ParseCmds
 * @details    When given a valid command line,<br>
 *             ParseCmds populates the given CoreIdleCommandLine struct<br>
 *             and returns 0.<br>
 */
TEST_P(coreidle_main_c_p, parse1) {
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
  strcpy(zero, "coreidle");
  strcpy(one, "--segment");
  strcpy(two, "0x1234");
  strcpy(three, "-B");
  strcpy(four, "0x5e");
  strcpy(five, "-D");
  strcpy(six, "0xab");
  strcpy(seven, "-F");
  strcpy(eight, "0xba");
  strcpy(nine, "-S");
  strcpy(ten, "3");
  strcpy(eleven, "-G");
  strcpy(twelve, "file.gbs");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  struct CoreIdleCommandLine cmd = { -1, -1, -1, -1, -1, {0,}, NULL, 0 };
  EXPECT_EQ(ParseCmds(&cmd, 13, argv), 0);

  EXPECT_EQ(cmd.segment, 0x1234);
  EXPECT_EQ(cmd.bus, 0x5e);
  EXPECT_EQ(cmd.device, 0xab);
  EXPECT_EQ(cmd.function, 0xba);
  EXPECT_EQ(cmd.socket, 3);
  EXPECT_STREQ(cmd.filename, "file.gbs");
}

/**
 * @test       parse_err0
 * @brief      Test: ParseCmds
 * @details    When given an invalid commnad option,<br>
 *             ParseCmds displays an error message and returns -1.<br>
 */
TEST_P(coreidle_main_c_p, parse_err0) {
  char zero[20];
  char one[20];
  strcpy(zero, "coreidle");
  strcpy(one, "-Y");
  char *argv[] = { zero, one };

  struct CoreIdleCommandLine cmd;
  EXPECT_EQ(ParseCmds(&cmd, 2, argv), -1);
}

/**
 * @test       parse_err1
 * @brief      Test: ParseCmds
 * @details    When given an invalid commnad option,<br>
 *             ParseCmds displays an error message and returns -1.<br>
 */
TEST_P(coreidle_main_c_p, parse_err1) {
  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[32];
  char five[32];
  char six[32];
  char seven[32];
  char eight[32];
  char nine[32];
  char ten[32];
  char eleven[32];
  char twelve[32];
  strcpy(zero, "    coreidle+_=`~!@#$%^&*_+=-");
  strcpy(one, "-Green_Bitstream");
  strcpy(two, tmp_gbs_);
  strcpy(three, "--seg ment|{};:'<>?");
  strcpy(four, "0x9999");
  strcpy(five, "-BBBBB");
  strcpy(six, "99");
  strcpy(seven, "-Devic\xF0\x90-\xBF essssss \t\n\b\a\e\v");
  strcpy(eight, "99");
  strcpy(nine, "-F\x00\x08\x09\x0B\x0D");
  strcpy(ten, "7");
  strcpy(eleven, "\xF1-\xF3    \x80-\x8F");
  strcpy(twelve, "99");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  struct CoreIdleCommandLine cmd;
  EXPECT_LT(ParseCmds(&cmd, 13, argv), 0);
}

/**
 * @test       parse_err2
 * @brief      Test: ParseCmds
 * @details    When given an invalid commnad option,<br>
 *             ParseCmds and coreidle_main return 0.<br>
 */
TEST_P(coreidle_main_c_p, parse_err2) {
  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[32];
  char five[32];
  char six[32];
  char seven[32];
  char eight[32];
  char nine[32];
  char ten[32];
  char eleven[32];
  char twelve[32];
  strcpy(zero, "        ");
  strcpy(one, "-G");
  strcpy(two, tmp_gbs_);
  strcpy(three, "--segment");
  strcpy(four, "---6121\xE1-\xEC\xFF\xEF 234");
  strcpy(five, " --B");
  strcpy(six, "\000 \x01\x05\x0a\x15");
  strcpy(seven, "-D");
  strcpy(eight, "\xC2-\xDF");
  strcpy(nine, "-F");
  strcpy(ten, "4\x09\x0A\x0D\x20-\x7E");
  strcpy(eleven, "-S");
  strcpy(twelve, "   \xED\x80-\x9F 2374891shf./m'");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  struct CoreIdleCommandLine cmd;
  EXPECT_EQ(ParseCmds(&cmd, 13, argv), 0);
  EXPECT_NE(coreidle_main(13, argv), 0);
}


/**
 * @test       lead_null_char
 * @brief      Test: coreidle_main
 * @details    When called with parameters that contains nullbytes<br>
 *             it fails to find file and coreidle_main flags bitstream
 *             invalid error. It returns 0 on cleanup success.<br>
 */
TEST_P(coreidle_main_c_p, lead_null_char) {
  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char *argv[] = { zero, one, two };

  strcpy(zero, "coreidle");
  strcpy(one, "-G");
  strcpy(two, "\0 copy_bitstream.gbs");
  EXPECT_NE(coreidle_main(3, argv), 0);

  unlink(copy_gbs_.c_str());
}

/**
 * @test       mid_null_char
 * @brief      Test: coreidle_main
 * @details    When called with parameters that contains nullbytes<br>
 *             it fails to find file and coreidle_main flags bitstream
 *             invalid error. It returns 0 on cleanup success.<br>
 */
TEST_P(coreidle_main_c_p, mid_null_char) {
  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[32];
  char *argv[] = { zero, one, two };

  strcpy(zero, "coreidle");
  strcpy(one, "-G");
  strcpy(two, "copy_bit\0stream.gbs");
  EXPECT_NE(coreidle_main(3, argv), 0);

  unlink(copy_gbs_.c_str());
}

/**
 * @test       read_bits0
 * @brief      Test: read_bitstream
 * @details    When read_bitstream is given a NULL pointer,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(coreidle_main_c_p, read_bits0) {
  EXPECT_LT(read_bitstream(nullptr), 0);
}

/**
 * @test       read_bits1
 * @brief      Test: read_bitstream
 * @details    When read_bitstream is given a command line struct<br>
 *             with a filename field that names a non-existent file,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(coreidle_main_c_p, read_bits1) {
  struct CoreIdleCommandLine cmd;
  strcpy(cmd.filename, "/doesnt/exist");
  EXPECT_LT(read_bitstream(&cmd), 0);
}

/**
 * @test       read_bits2
 * @brief      Test: read_bitstream
 * @details    When malloc fails,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(coreidle_main_c_p, read_bits2) {
  struct CoreIdleCommandLine cmd;
  strcpy(cmd.filename, tmp_gbs_);
  system_->invalidate_malloc(0, "read_bitstream");
  EXPECT_LT(read_bitstream(&cmd), 0);
}

/**
 * @test       read_bits3
 * @brief      Test: read_bitstream
 * @details    When successful,<br>
 *             read_bitstream loads the bitstream into the gbs_data field<br>
 *             and the fn returns 0.<br>
 */
TEST_P(coreidle_main_c_p, read_bits3) {
  struct CoreIdleCommandLine cmd;
  strcpy(cmd.filename, tmp_gbs_);
  EXPECT_EQ(read_bitstream(&cmd), 0);
  free(cmd.gbs_data);
}

INSTANTIATE_TEST_CASE_P(coreidle_main_c, coreidle_main_c_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));
