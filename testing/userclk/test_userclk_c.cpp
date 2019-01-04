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

struct UserClkCommandLine
{
	int      segment;
	int      bus;
	int      device;
	int      function;
	int      socket;
	int      freq_high;
	int      freq_low;
};
extern struct UserClkCommandLine userclkCmdLine;

void UserClkAppShowHelp(void);

void print_err(const char *s, fpga_result res);

int ParseCmds(struct UserClkCommandLine *userclkCmdLine, int argc, char *argv[]);

int userclk_main(int argc, char *argv[]);

}

#include <config.h>

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class userclk_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  userclk_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    optind = 0;
    cmd_line_ = userclkCmdLine;
  }

  virtual void TearDown() override {
    userclkCmdLine = cmd_line_;
    fpgaFinalize();
    system_->finalize();
  }

  struct UserClkCommandLine cmd_line_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: UserClkAppShowHelp
 * @details    UserClkAppShowHelp displays the application help message.<br>
 */
TEST_P(userclk_c_p, help) {
  UserClkAppShowHelp();
}

/**
 * @test       print_err
 * @brief      Test: print_err
 * @details    print_err prints the given string and<br>
 *             the decoded representation of the fpga_result<br>
 *             to stderr.<br>
 */
TEST_P(userclk_c_p, print_err) {
  print_err("msg", FPGA_OK);
}

/**
 * @test       parse_cmd0
 * @brief      Test: ParseCmds
 * @details    When passed an invalid command option,<br>
 *             ParseCmds prints a message and<br>
 *             returns a negative value.<br>
 */
TEST_P(userclk_c_p, parse_cmd0) {
  struct UserClkCommandLine cmd;

  char zero[20];
  char one[20];
  strcpy(zero, "userclk");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);
}

/**
 * @test       parse_cmd1
 * @brief      Test: ParseCmds
 * @details    When called with "-h",<br>
 *             ParseCmds prints the app help and<br>
 *             returns a negative value.<br>
 */
TEST_P(userclk_c_p, parse_cmd1) {
  struct UserClkCommandLine cmd;

  char zero[20];
  char one[20];
  strcpy(zero, "userclk");
  strcpy(one, "-h");

  char *argv[] = { zero, one };

  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);
}

/**
 * @test       parse_cmd2
 * @brief      Test: ParseCmds
 * @details    When given a command option that requires a param,<br>
 *             omitting the required param causes ParseCmds to<br>
 *             return a negative value.<br>
 */
TEST_P(userclk_c_p, parse_cmd2) {
  struct UserClkCommandLine cmd;

  char zero[20];
  char one[20];
  strcpy(zero, "userclk");

  char *argv[] = { zero, one };

  strcpy(one, "--segment");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-B");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-D");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-F");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-S");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-P");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-H");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);

  optind = 0;
  strcpy(one, "-L");
  EXPECT_LT(ParseCmds(&cmd, 2, argv), 0);
}

/**
 * @test       parse_cmd3
 * @brief      Test: ParseCmds
 * @details    When given valid command options,<br>
 *             ParseCmds populates the given UserClkCommandLine,<br>
 *             returning zero.<br>
 */
TEST_P(userclk_c_p, parse_cmd3) {
  struct UserClkCommandLine cmd = cmd_line_;

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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  strcpy(two, "0x1234");
  strcpy(three, "-B");
  strcpy(four, "3");
  strcpy(five, "-D");
  strcpy(six, "4");
  strcpy(seven, "-F");
  strcpy(eight, "5");
  strcpy(nine, "-S");
  strcpy(ten, "6");
  strcpy(eleven, "-H");
  strcpy(twelve, "8");
  strcpy(thirteen, "-L");
  strcpy(fourteen, "9");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(ParseCmds(&cmd, 15, argv), 0);
  EXPECT_EQ(cmd.segment, 0x1234);
  EXPECT_EQ(cmd.bus, 3);
  EXPECT_EQ(cmd.device, 4);
  EXPECT_EQ(cmd.function, 5);
  EXPECT_EQ(cmd.socket, 6);
  EXPECT_EQ(cmd.freq_high, 8);
  EXPECT_EQ(cmd.freq_low, 9);
}

/**
 * @test       invalid_cmd_characters_02
 * @brief      Test: ParseCmds, userclk_main 
 * @details    When given invalid command options,<br>
 *             ParseCmds populates the given UserClkCommandLine,<br>
 *             returning FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_p, invalid_cmd_characters_01) {
  struct UserClkCommandLine cmd = cmd_line_;

  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[32];
  char five[48];
  char six[32];
  char seven[32];
  char eight[32];
  char nine[32];
  char ten[32];
  char eleven[32];
  char twelve[32];
  char thirteen[32];
  char fourteen[32];
  strcpy(zero, "userclk_+*(^> ");
  strcpy(one, "--segm ent");
  strcpy(two, "0x1234");
  strcpy(three, "-Bus");
  strcpy(four, "3");
  strcpy(five, "-Devi +_( \v           -0923198 (*)& ces");
  strcpy(six, "4");
  strcpy(seven, "-Fun\t\nction");
  strcpy(eight, "5");
  strcpy(nine, "-Sockett\e \'\?//tttttt \b");
  strcpy(ten, "6");
  strcpy(eleven, "-   High");
  strcpy(twelve, "-100");
  strcpy(thirteen, " ---Low");
  strcpy(fourteen, "-200");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_NE(ParseCmds(&cmd, 15, argv), 0);
  EXPECT_NE(userclk_main(15, argv), 0);
}

/**
 * @test       invalid_cmd_characters_02
 * @brief      Test: ParseCmds, userclk_main 
 * @details    When given invalid command options,<br>
 *             ParseCmds populates the given UserClkCommandLine,<br>
 *             returning zero. Userclk_main returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_p, invalid_cmd_characters_02) {
  struct UserClkCommandLine cmd;

  char zero[32];
  char one[32];
  char two[48];
  char three[32];
  char four[32];
  char five[32];
  char six[32];
  char seven[32];
  char eight[48];
  char nine[32];
  char ten[32];
  char eleven[32];
  char twelve[32];
  char thirteen[32];
  char fourteen[32];
  strcpy(zero, "");
  strcpy(one, "--segment");
  strcpy(two, "0x0123897349  *(^%$%^$%@^?><? 6121234");
  strcpy(three, " --B");
  strcpy(four, "334987238689 \x01\x05\x0a\x15");
  strcpy(five, "-D");
  strcpy(six, "41278991 02a8974913");
  strcpy(seven, "-F");
  strcpy(eight, "529378190 \t 3haskfhahhi\\ | // o=1-21");
  strcpy(nine, "-S");
  strcpy(ten, "        6`1238 \n -349287419=-0;");
  strcpy(eleven, "-H");
  strcpy(twelve, "400000000000000000000000000");
  strcpy(thirteen, "-L");
  strcpy(fourteen, "9.999");

  char *argv[] = { zero, one, two, three, four,
                   eleven, twelve, thirteen, fourteen,
                   five, six, seven, eight, nine, ten};

  EXPECT_EQ(ParseCmds(&cmd, 15, argv), 0);
  EXPECT_NE(userclk_main(15, argv), 0);
}


/**
 * @test       main0
 * @brief      Test: userclk_main
 * @details    When given no command options,<br>
 *             userclk_main displays the app help message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(userclk_c_p, main0) {
  char zero[20];
  strcpy(zero, "userclk");

  char *argv[] = { zero };

  EXPECT_NE(userclk_main(1, argv), 0);
}

/**
 * @test       main1
 * @brief      Test: userclk_main
 * @details    When given "-h",<br>
 *             userclk_main displays the app help message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(userclk_c_p, main1) {
  char zero[20];
  char one[20];
  strcpy(zero, "userclk");
  strcpy(one, "-h");

  char *argv[] = { zero, one };

  EXPECT_NE(userclk_main(2, argv), 0);
}

/**
 * @test       main2
 * @brief      Test: userclk_main
 * @details    When given valid command parameters,<br>
 *             but no device is identified by those parameters,<br>
 *             userclk_main displays an error message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(userclk_c_p, main2) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "userclk");
  strcpy(one, "-B");
  strcpy(two, "99");

  char *argv[] = { zero, one, two };

  EXPECT_NE(userclk_main(3, argv), 0);
}

INSTANTIATE_TEST_CASE_P(userclk_c, userclk_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));

class userclk_c_hw_p : public userclk_c_p{
  protected:
    userclk_c_hw_p() {};
};

/**
 * @test       main3
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if --freq-low is not given,<br>
 *             then low is calculated from high,<br>
 *             and the fn returns zero.<br>
 */
TEST_P(userclk_c_hw_p, main3) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-H");
  strcpy(twelve, "400");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  EXPECT_EQ(userclk_main(13, argv), FPGA_OK);
}

/**
 * @test       main4
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if --freq-high is not given,<br>
 *             then high is calculated from low,<br>
 *             and the fn returns zero.<br>
 */
TEST_P(userclk_c_hw_p, main4) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-L");
  strcpy(twelve, "200");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  EXPECT_EQ(userclk_main(13, argv), FPGA_OK);
}

/**
 * @test       main5
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if both --freq-high (-H) and --freq-low (-L) are given,<br>
 *             then high must equal 2 * low,<br>
 *             or else the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_hw_p, main5) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-H");
  strcpy(twelve, "300");
  strcpy(thirteen, "-L");
  strcpy(fourteen, "100");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(userclk_main(15, argv), FPGA_INVALID_PARAM);
}

/**
 * @test       main6
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if neither --freq-high (-H) nor --freq-low (-L) is given,<br>
 *             then the function prints an error<br>
 *             and returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_hw_p, main6) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten };

  EXPECT_EQ(userclk_main(11, argv), FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(userclk_c, userclk_c_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"skx-p","dcp-rc"})));


class userclk_c_mock_p : public userclk_c_p{
  protected:
    userclk_c_mock_p() {};
};

/**
 * @test       main3
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if --freq-low is not given,<br>
 *             then low is calculated from high,<br>
 *             and the fn returns zero.<br>
 */
TEST_P(userclk_c_mock_p, main3) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-H");
  strcpy(twelve, "400");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  /*
  ** FIXME: main should return zero in this case, but
  ** the user clocks API polls on a sysfs file with a
  ** timeout. Because the sysfs file never updates in
  ** a mock environment, the API will time out and return
  ** FPGA_NOT_SUPPORTED.
  EXPECT_EQ(userclk_main(13, argv), 0);
  */
  EXPECT_EQ(userclk_main(13, argv), FPGA_NOT_SUPPORTED);
}

/**
 * @test       main4
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if --freq-high is not given,<br>
 *             then high is calculated from low,<br>
 *             and the fn returns zero.<br>
 */
TEST_P(userclk_c_mock_p, main4) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-L");
  strcpy(twelve, "200");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve };

  /*
  ** FIXME: main should return zero in this case, but
  ** the user clocks API polls on a sysfs file with a
  ** timeout. Because the sysfs file never updates in
  ** a mock environment, the API will time out and return
  ** FPGA_NOT_SUPPORTED.
  EXPECT_EQ(userclk_main(13, argv), 0);
  */
  EXPECT_EQ(userclk_main(13, argv), FPGA_NOT_SUPPORTED);
}

/**
 * @test       main5
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if both --freq-high (-H) and --freq-low (-L) are given,<br>
 *             then high must equal 2 * low,<br>
 *             or else the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_mock_p, main5) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);
  strcpy(eleven, "-H");
  strcpy(twelve, "300");
  strcpy(thirteen, "-L");
  strcpy(fourteen, "100");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(userclk_main(15, argv), FPGA_INVALID_PARAM);
}

/**
 * @test       main6
 * @brief      Test: userclk_main
 * @details    When given valid command parameters that identify an accelerator,<br>
 *             if neither --freq-high (-H) nor --freq-low (-L) is given,<br>
 *             then the function prints an error<br>
 *             and returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(userclk_c_mock_p, main6) {
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
  strcpy(zero, "userclk");
  strcpy(one, "--segment");
  sprintf(two, "%d", platform_.devices[0].segment);
  strcpy(three, "-B");
  sprintf(four, "%d", platform_.devices[0].bus);
  strcpy(five, "-D");
  sprintf(six, "%d", platform_.devices[0].device);
  strcpy(seven, "-F");
  sprintf(eight, "%d", platform_.devices[0].function);
  strcpy(nine, "-S");
  sprintf(ten, "%d", platform_.devices[0].socket_id);

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten };

  EXPECT_EQ(userclk_main(11, argv), FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(userclk_c, userclk_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p", "dcp-rc"})));


