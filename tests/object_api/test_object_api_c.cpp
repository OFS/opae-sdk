// Copyright(c) 2018-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

extern "C"{
struct config {
  float interval_sec;
};
extern struct config options;

void print_err(const char*, fpga_result);
fpga_result parse_args(int argc, char* argv[]);
int object_api_main(int argc, char* argv[]);

extern int cleanup_size;
}

using namespace opae::testing;

class object_api_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    optind = 0;
    options_ = options;

    cleanup_size = 0;
  }

  virtual void TearDown() override {
    options = options_;

    opae_base_p<>::TearDown();
  }

  struct config options_;
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

  char *argv[] = { zero, one, NULL };

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
  strcpy(zero, "object_api");
  strcpy(one, "-s");

  char *argv[] = { zero, one, NULL };

  EXPECT_EQ(parse_args(2, argv), FPGA_EXCEPTION);
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

  char *argv[] = { zero, one, NULL };

  EXPECT_NE(object_api_main(2, argv), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(object_api_c_p);
INSTANTIATE_TEST_SUITE_P(object_api_c, object_api_c_p,
                         ::testing::ValuesIn(test_platform::platforms({})));

class object_api_c_mock_p : public object_api_c_p {};

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

  char *argv[] = { zero, one, two, NULL };

  EXPECT_EQ(object_api_main(3, argv), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(object_api_c_mock_p);
INSTANTIATE_TEST_SUITE_P(object_api_c, object_api_c_mock_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));

class object_api_c_mcp_hw_p : public object_api_c_p {};

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

  char *argv[] = { zero, one, two, NULL };

  EXPECT_EQ(object_api_main(3, argv), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(object_api_c_mcp_hw_p);
INSTANTIATE_TEST_SUITE_P(object_api_c, object_api_c_mcp_hw_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({"skx-p"})));

class object_api_c_dcp_hw_p : public object_api_c_p {};

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

  char *argv[] = { zero, one, two, NULL };

  EXPECT_NE(object_api_main(3, argv), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(object_api_c_dcp_hw_p);
INSTANTIATE_TEST_SUITE_P(object_api_c, object_api_c_dcp_hw_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({"dcp-rc"})));
