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
#include <linux/limits.h>
#include <libbitstream/bitstream.h>

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
        char     filename[PATH_MAX];
	opae_bitstream_info bitstr;
};
extern struct CoreIdleCommandLine coreidleCmdLine;

void CoreidleAppShowHelp();

void print_err(const char *s, fpga_result res);

int coreidle_main(int argc, char *argv[]);

int ParseCmds(struct CoreIdleCommandLine *coreidleCmdLine,
	      int argc,
	      char *argv[]);

}

#include <sys/types.h>
#include <sys/stat.h>
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
#include "mock/test_system.h"

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

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);

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

    EXPECT_EQ(fpgaFinalize(), FPGA_OK);
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

  struct CoreIdleCommandLine cmd =
  { -1, -1, -1, -1, -1, {0,}, OPAE_BITSTREAM_INFO_INITIALIZER };
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
  strcpy(zero, "  coreidle+_~_+=-");
  strcpy(one, "-Green_Bitstream");
  strcpy(two, tmp_gbs_);
  strcpy(three, "--seg ment|{};:'<>?");
  strcpy(four, "0x9999");
  strcpy(five, "-BBBBB");
  strcpy(six, "99");
  strcpy(seven, "-Devic\xF0\x90-\xBF ess\n\b\a\e\v");
  strcpy(eight, "99");
  strcpy(nine, "-F\x00\x08\x09\x0B\x0D");
  strcpy(ten, "7");
  strcpy(eleven, "\xF1-\xF3  \x80-\x8F");
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
 * @test       encoding_path*
 * @brief      Test: coreidle_main
 * @details    When command param is encoding path,<br>
 *             coreidle_main displays file error and returns zero.<br>
 */
TEST_P(coreidle_main_c_p, encoding_path1) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "copy_bitstream%2Egbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, encoding_path2) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "copy_bitstream..gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, encoding_path3) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "....copy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, encoding_path4) {
  copy_bitstream("copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "%252E%252E%252Fcopy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       relative_path1
 * @brief      Test: coreidle_main
 * @details    Tests for absolute path on gbs file. When file found<br>
 *             and successful, set_cpu_core_idle flags INVALID PARAM and
 *             forces coreidle to return 1.<br>
 */
TEST_P(coreidle_main_c_p, relative_path1) {
  copy_bitstream("../copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "../copy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       relative_path*
 * @brief      Test: coreidle_main
 * @details    When command param is encoding path,<br>
 *             coreidle_main displays file error and returns zero.<br>
 */
TEST_P(coreidle_main_c_p, relative_path2) {
  copy_bitstream("../copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "../..../copy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, relative_path3) {
  copy_bitstream("../copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "..%2fcopy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, relative_path4) {
  copy_bitstream("../copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "%2e%2e/copy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

TEST_P(coreidle_main_c_p, relative_path5) {
  copy_bitstream("../copy_bitstream.gbs");
  const char *argv[] = { "coreidle", "-G", "%2e%2e%2fcopy_bitstream.gbs"};

  EXPECT_EQ(coreidle_main(3, (char**)argv), 1);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       absolute_path 
 * @brief      Test: coreidle_main 
 * @details    Tests for absolute path on gbs file. When file found<br>
 *             and successful, set_cpu_core_idle flags INVALID PARAM and
 *             forces coreidle to return 1.<br>
 */
TEST_P(coreidle_main_c_p, absolute_path) {
  copy_bitstream("copy_bitstream.gbs");
  char zero[32];
  char one[32];
  char two[128];
  char *argv[] = { zero, one, two };

  strcpy(zero, "coreidle");
  strcpy(one, "-G");

  char* current_path = get_current_dir_name();
  std::string bitstream_path = (std::string)current_path + "/copy_bitstream.gbs";

  strcpy(two, bitstream_path.c_str());
  EXPECT_EQ(coreidle_main(3, argv), 1);

  free(current_path);
  unlink(copy_gbs_.c_str());
}

/**
 * @test       main_symlink_bs1
 * @brief      Test: coreidle_main
 * @details    Tests for symlink on gbs file. When successful,<br>
 *             set_cpu_core_idle flags INVALID PARAM and
 *             forces coreidle to return 1.<br>
 */
TEST_P(coreidle_main_c_p, main_symlink_bs1) {
  copy_bitstream("copy_bitstream.gbs"); 
  const std::string symlink_gbs = "bits_symlink";
  char zero[32];
  char one[32];
  char two[32];
  char *argv[] = { zero, one, two };
  strcpy(zero, "coreidle");
  strcpy(one, "-G");
 
  auto ret = symlink(copy_gbs_.c_str(), symlink_gbs.c_str());
  EXPECT_EQ(ret, 0);

  strcpy(two, symlink_gbs.c_str());
  EXPECT_EQ(coreidle_main(3, argv), 1);

  // remove bitstream file and symlink
  unlink(copy_gbs_.c_str());
  unlink(symlink_gbs.c_str());
}

/**
 * @test       main_symlink_bs2
 * @brief      Test: coreidle_main
 * @details    Tests for symlink on gbs file. When file pointed by <br>
 *             symlink doesn't exist. Coreidle fails to read file and <br>
 *             displays error and returns 0.<br>
 */
TEST_P(coreidle_main_c_p, main_symlink_bs2) {
  copy_bitstream("copy_bitstream.gbs"); 
  const std::string symlink_gbs = "bits_symlink";
  char zero[32];
  char one[32];
  char two[32];
  char *argv[] = { zero, one, two };
  strcpy(zero, "coreidle");
  strcpy(one, "-G");
 
  auto ret = symlink(copy_gbs_.c_str(), symlink_gbs.c_str());
  EXPECT_EQ(ret, 0);

  strcpy(two, symlink_gbs.c_str());

  // remove bitstream file
  unlink(copy_gbs_.c_str());

  // fails to read file
  EXPECT_EQ(coreidle_main(3, argv), 1);

  unlink(symlink_gbs.c_str());
}

/**
 * @test       circular_symlink
 * @brief      Test: read_bitstream
 * @details    Tests for circular symlink on gbs file. <br>
 *             read_bitstream displays error and returns -1.<br>
 */
TEST_P(coreidle_main_c_p, main_circular_symlink) {
  const std::string symlink_A = "./link1/bits_symlink_A";
  const std::string symlink_B = "./link2/bits_symlink_B";
  char zero[32];
  char one[32];
  char two[32];
  char *argv[] = { zero, one, two };
  strcpy(zero, "coreidle");
  strcpy(one, "-G");

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

  strcpy(two, symlink_A.c_str());
  EXPECT_EQ(coreidle_main(3, argv), 1);

  // Clean up
  unlink(symlink_A.c_str());
  unlink(symlink_B.c_str());
  remove("link1");
  remove("link2");
}

INSTANTIATE_TEST_CASE_P(coreidle_main_c, coreidle_main_c_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));
