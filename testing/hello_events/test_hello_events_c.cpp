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

void print_err(const char *s, fpga_result res);

struct events_config {
  struct target {
    int bus;
  } target;
};
extern struct events_config events_config;

fpga_result inject_ras_fatal_error(fpga_token fme_token, uint8_t err);

fpga_result parse_args(int argc, char *argv[]);

fpga_result find_fpga(fpga_token *fpga, uint32_t *num_matches);

fpga_result get_bus(fpga_token tok, uint8_t *bus);

int hello_events_main(int argc, char *argv[]);

}

#include <config.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include "fpgad_control.h"

using namespace opae::testing;

class hello_events_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  hello_events_c_p() : token_(nullptr) {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    optind = 0;
    events_config_ = events_config;
  }

  virtual void TearDown() override {
    events_config = events_config_;
    if (token_) {
      EXPECT_EQ(fpgaDestroyToken(&token_), FPGA_OK);
      token_ = nullptr;
    }
    fpgaFinalize();
    system_->finalize();
  }

  fpga_token token_;
  struct events_config events_config_;
  std::thread fpgad_;
  test_platform platform_;
  test_system *system_;
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
TEST_P(hello_events_c_p, parse_args1) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");

  char *argv[] = { zero, one };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global struct events_config,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(hello_events_c_p, parse_args2) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");
  strcpy(two, "3");

  char *argv[] = { zero, one, two };

  EXPECT_EQ(parse_args(3, argv), FPGA_OK);
  EXPECT_EQ(events_config.target.bus, 3);
}

/**
 * @test       find_fpga0
 * @brief      Test: find_fpga
 * @details    When no device matching the filtering criteria is found,<br>
 *             find_fpga returns FPGA_OK,<br>
 *             and num_matches parameter is set to 0.<br>
 */
TEST_P(hello_events_c_p, find_fpga0) {
  events_config.target.bus = 99;

  uint32_t matches = 1;
  EXPECT_EQ(find_fpga(&token_, &matches), FPGA_OK);
  EXPECT_EQ(token_, nullptr);
  EXPECT_EQ(matches, 0);
}

/**
 * @test       find_fpga1
 * @brief      Test: find_fpga
 * @details    When a device matching the filtering criteria is found,<br>
 *             find_fpga returns FPGA_OK,<br>
 *             and num_matches parameter is set to non-zero.<br>
 */
TEST_P(hello_events_c_p, find_fpga1) {
  events_config.target.bus = platform_.devices[0].bus;

  uint32_t matches = 0;
  EXPECT_EQ(find_fpga(&token_, &matches), FPGA_OK);
  EXPECT_NE(token_, nullptr);
  EXPECT_GT(matches, 0);
}

/**
 * @test       get_bus
 * @brief      Test: get_bus
 * @details    When a valid fpga_token is passed to get_bus,<br>
 *             the function retrieves the bus associated with the token,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(hello_events_c_p, get_bus) {
  uint32_t matches = 0;
  EXPECT_EQ(find_fpga(&token_, &matches), FPGA_OK);
  ASSERT_NE(token_, nullptr);
  EXPECT_GT(matches, 0);

  fpga_properties prop = nullptr;
  ASSERT_EQ(fpgaGetProperties(token_, &prop), FPGA_OK);
  ASSERT_NE(prop, nullptr);
  uint8_t expected_bus = 99;

  ASSERT_EQ(fpgaPropertiesGetBus(prop, &expected_bus), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  uint8_t bus = 99;
  EXPECT_EQ(get_bus(token_, &bus), FPGA_OK);
  EXPECT_EQ(expected_bus, bus);
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

  char *argv[] = { zero, one };

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

  char *argv[] = { zero, one, two };

  EXPECT_NE(hello_events_main(3, argv), 0);
}

INSTANTIATE_TEST_CASE_P(hello_events_c, hello_events_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));


class mock_hello_events_c_fpgad_p : public ::testing::TestWithParam<std::string>,
                                    public fpgad_control {
 protected:
  mock_hello_events_c_fpgad_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    optind = 0;
    events_config_ = events_config;

    fpgad_start();
  }

  virtual void TearDown() override {
    fpgad_stop();
    events_config = events_config_;

    fpgaFinalize();
    system_->finalize();
  }

  struct events_config events_config_;
  test_platform platform_;
  test_system *system_;
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

  char *argv[] = { zero, one, two };

  EXPECT_NE(hello_events_main(3, argv), 0);
}

INSTANTIATE_TEST_CASE_P(mock_hello_events_c_fpgad, mock_hello_events_c_fpgad_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));


class hw_hello_events_c_fpgad_p : public mock_hello_events_c_fpgad_p {
 protected:
  hw_hello_events_c_fpgad_p() {}
};

/**
 * @test       main2
 * @brief      Test: hello_events_main
 * @details    When a suitable accelerator device is found,<br>
 *             hello_events_main (hw) runs to completion,<br>
 *             and the fn returns zero.<br>
 */
/* Disabling this test, because there is no way for fpgad
   to monitor hello_events' fpga_device_token. That token
   is known only to hello_events_main.
*/
TEST_P(hw_hello_events_c_fpgad_p, DISABLED_main2) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_events");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  EXPECT_EQ(hello_events_main(3, argv), 0);
}

INSTANTIATE_TEST_CASE_P(hw_hello_events_c_fpgad, hw_hello_events_c_fpgad_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({}, fpga_driver::linux_intel)));
