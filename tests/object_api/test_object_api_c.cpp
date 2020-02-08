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
extern "C"{

struct config {
  int bus;
  float interval_sec;
};
extern struct config options;

void print_err(const char*, fpga_result);
fpga_result parse_args(int argc, char* argv[]);
int object_api_main(int argc, char* argv[]);
}

#include <config.h>
#include <intel-fpga.h>
#include <getopt.h>
#include <string.h>
#include <mock/test_system.h>
#include <gtest/gtest.h>
using namespace opae::testing;

class object_api_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  object_api_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    optind = 0;
    options_ = options;
  }

  virtual void TearDown() override {
    options = options_;
    fpgaFinalize();
    system_->finalize();
  }

  struct config options_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       test_print_error
 * @brief      Test: print_erro
 * @details    When params to print_err is valid, it displays the std error.<br>
 */
TEST(object_api_c, test_print_err){
  const std::string str = "Invalid string";
  print_err(str.c_str(), FPGA_INVALID_PARAM);
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When passed an invalid command option,<br>
 *             parse_args prints a message and<br>
 *             returns a value other than FPGA_OK.<br>
 */
TEST_P(object_api_c_p, parse_args0) {
  char zero[20];
  char one[20];
  strcpy(zero, "object_api");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args1
 * @brief      Test: parse_args
 * @details    When given a command option that requires a param,<br>
 *             omitting the required param causes parse_args to<br>
 *             return a value other than FPGA_OK.<br>
 */
TEST_P(object_api_c_p, parse_args1) {
  char zero[20];
  char one[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");

  char *argv[] = { zero, one };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given invalid command options,<br>
 *             parse_args returns FPGA_EXCEPTION.<br>
 */
TEST_P(object_api_c_p, parse_args2) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");
  strcpy(two, "3");
  strcpy(three, "-s");

  char *argv[] = { zero, one, two, three };

  EXPECT_EQ(parse_args(4, argv), FPGA_EXCEPTION);
}

/**
 * @test       parse_args3
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_api_c_p, parse_args3) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");
  strcpy(two, "3");

  char *argv[] = { zero, one, two };

  EXPECT_EQ(options.bus, -1);
  EXPECT_EQ(parse_args(3, argv), FPGA_OK);
  EXPECT_EQ(options.bus, 3);
}

/**
 * @test       main0
 * @brief      Test: hello_fpga_main
 * @details    When given an invalid command option,<br>
 *             object_api displays an error message,<br>
 *             and returns non-zero.<br>
 */
TEST_P(object_api_c_p, main0) {
  char zero[20];
  char one[20];
  strcpy(zero, "object_api");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_NE(object_api_main(2, argv), 0);
}

INSTANTIATE_TEST_CASE_P(object_api_c, object_api_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));


class object_api_c_mock_p : public object_api_c_p {
 protected:
  object_api_c_mock_p() {}
};

/**
 * @test       main1
 * @brief      Test: object_api_main
 * @details    When given a valid command line,<br>
 *             object_api_main checks *perf directories.<br>
 *             to add counter, print counters and<br>
 *             return FPGA_OK.<br>
 */
TEST_P(object_api_c_mock_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  EXPECT_EQ(object_api_main(3, argv), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(object_api_c, object_api_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));

class object_api_c_mcp_hw_p : public object_api_c_p {
};

/**
 * @test       main1
 * @brief      Test: object_api_main
 * @details    When given a valid command line,<br>
 *             object_api_main checks *perf directories.<br>
 *             to add counter, print counters and<br>
 *             return FPGA_OK.<br>
 */
TEST_P(object_api_c_mcp_hw_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  EXPECT_EQ(object_api_main(3, argv), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(object_api_c, object_api_c_mcp_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"skx-p"})));


class object_api_c_dcp_hw_p : public object_api_c_p {
};

/**
 * @test       main1
 * @brief      Test: object_api_main
 * @details    When given a valid command line,<br>
 *             object_api_main checks *perf directories.<br>
 *             to add counter, print counters. DCP machines<br>
 *             does not have dperf/cache. So it returns -1.<br>
 */
TEST_P(object_api_c_dcp_hw_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "object_api");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  EXPECT_NE(object_api_main(3, argv), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(object_api_c, object_api_c_dcp_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"dcp-rc"})));
