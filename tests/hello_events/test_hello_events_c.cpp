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
#include "mock/opae_fpgad_fixtures.h"

extern "C" {
void print_err(const char *s, fpga_result res);

fpga_result inject_ras_fatal_error(fpga_token fme_token, uint8_t err);

fpga_result parse_args(int argc, char *argv[]);

fpga_result find_fpga(fpga_properties device_filter,
		      fpga_token *fpga,
		      uint32_t *num_matches);

int hello_events_main(int argc, char *argv[]);
}

using namespace opae::testing;

class hello_events_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    optind = 0;
  }
};

/**
 * @test       print_err
 * @brief      Test: print_err
 * @details    print_err prints the given string and<br>
 *             the decoded representation of the fpga_result<br>
 *             to stderr.<br>
 */
TEST_P(hello_events_c_p, print_err) {
  print_err("msg", FPGA_OK);
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When passed an invalid command option,<br>
 *             parse_args prints a message and<br>
 *             returns a value other than FPGA_OK.<br>
 */
TEST_P(hello_events_c_p, parse_cmd0) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-Y");

  char *argv[] = { zero, one, NULL };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args1
 * @brief      Test: parse_args
 * @details    When given a command option that requires a param,<br>
 *             omitting the required param causes parse_args to<br>
 *             return a value other than FPGA_OK.<br>
 */
TEST_P(hello_events_c_p, parse_args1) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");

  char *argv[] = { zero, one, NULL };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(hello_events_c_p, parse_args2) {
  char zero[20];
  strcpy(zero, "hello_events");

  char *argv[] = { zero, NULL };

  EXPECT_EQ(parse_args(1, argv), FPGA_OK);
}

/**
 * @test       find_fpga0
 * @brief      Test: find_fpga
 * @details    When no device matching the filtering criteria is found,<br>
 *             find_fpga returns FPGA_OK,<br>
 *             and num_matches parameter is set to 0.<br>
 */
TEST_P(hello_events_c_p, find_fpga0) {
  fpga_properties filter = NULL;

  ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBus(filter, 99), FPGA_OK);

  fpga_token token = nullptr;
  uint32_t matches = 1;
  EXPECT_EQ(find_fpga(filter, &token, &matches), FPGA_OK);
  EXPECT_EQ(token, nullptr);
  EXPECT_EQ(matches, 0);

  EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       find_fpga1
 * @brief      Test: find_fpga
 * @details    When a device matching the filtering criteria is found,<br>
 *             find_fpga returns FPGA_OK,<br>
 *             and num_matches parameter is set to non-zero.<br>
 */
TEST_P(hello_events_c_p, find_fpga1) {
  fpga_properties filter = NULL;

  ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

  ASSERT_EQ(fpgaPropertiesSetSegment(filter, platform_.devices[0].segment), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetBus(filter, platform_.devices[0].bus), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetDevice(filter, platform_.devices[0].device), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetFunction(filter, platform_.devices[0].function), FPGA_OK);

  fpga_token token = nullptr;
  uint32_t matches = 0;
  EXPECT_EQ(find_fpga(filter, &token, &matches), FPGA_OK);
  EXPECT_NE(token, nullptr);
  EXPECT_GT(matches, 0);

  EXPECT_EQ(fpgaDestroyToken(&token), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       main0
 * @brief      Test: hello_events_main
 * @details    When given an invalid command option,<br>
 *             hello_events_main displays an error message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(hello_events_c_p, main0) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-Y");

  char *argv[] = { zero, one, NULL };

  EXPECT_NE(hello_events_main(2, argv), 0);
}

/**
 * @test       main1
 * @brief      Test: hello_events_main
 * @details    When no suitable accelerator device is found,<br>
 *             hello_events_main displays an error message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(hello_events_c_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");
  strcpy(two, "99");

  char *argv[] = { zero, one, two, NULL };

  EXPECT_NE(hello_events_main(3, argv), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hello_events_c_p);
INSTANTIATE_TEST_SUITE_P(hello_events_c, hello_events_c_p,
                         ::testing::ValuesIn(test_platform::platforms({})));

class mock_hello_events_c_fpgad_p : public opae_fpgad_p<> {
 protected:

  virtual void SetUp() override {
    opae_fpgad_p<>::SetUp();
    optind = 0;
    fpgad_watch();
  }

};

/**
 * @test       main2
 * @brief      Test: hello_events_main
 * @details    When a suitable accelerator device is found,<br>
 *             hello_events_main (mock) times out,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(mock_hello_events_c_fpgad_p, main2) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two, NULL };

  EXPECT_NE(hello_events_main(3, argv), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(mock_hello_events_c_fpgad_p);
INSTANTIATE_TEST_SUITE_P(mock_hello_events_c_fpgad, mock_hello_events_c_fpgad_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));

class hw_hello_events_c_fpgad_p : public mock_hello_events_c_fpgad_p {};

/**
 * @test       main2
 * @brief      Test: hello_events_main
 * @details    When a suitable accelerator device is found,<br>
 *             hello_events_main (hw) runs to completion,<br>
 *             and the fn returns zero.<br>
 */
TEST_P(hw_hello_events_c_fpgad_p, main2) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two, NULL };

  EXPECT_EQ(hello_events_main(3, argv), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hw_hello_events_c_fpgad_p);
INSTANTIATE_TEST_SUITE_P(hw_hello_events_c_fpgad, hw_hello_events_c_fpgad_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({}, fpga_driver::linux_intel)));
