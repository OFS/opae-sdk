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

struct config {
  struct target {
    int bus;
  } target;
  int open_flags;
};
extern struct config config;

fpga_result parse_args(int argc, char *argv[]);

fpga_result find_fpga(fpga_guid afu_guid,
                      fpga_token *accelerator_token,
                      uint32_t *num_matches_accelerators);

fpga_result get_bus(fpga_token tok, uint8_t *bus);

int hello_fpga_main(int argc, char *argv[]);

}

#define INVALID_AFU_ID "00000000-0000-0000-0000-000000000000"

#include <config.h>

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include <linux/ioctl.h>
#include "intel-fpga.h"

using namespace opae::testing;

int mmio_ioctl(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
    if (!rinfo) {
      FPGA_MSG("rinfo is NULL");
      goto out_EINVAL;
    }
    if (rinfo->argsz != sizeof(*rinfo)) {
      FPGA_MSG("wrong structure size");
      goto out_EINVAL;
    }
    if (rinfo->index > 1 ) {
      FPGA_MSG("unsupported MMIO index");
      goto out_EINVAL;
    }
    if (rinfo->padding != 0) {
      FPGA_MSG("unsupported padding");
      goto out_EINVAL;
    }
    rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
    rinfo->size = 0x40000;
    rinfo->offset = 0;
    retval = 0;
    errno = 0;
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;
}

class hello_fpga_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  hello_fpga_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);

    optind = 0;
    config_ = config;
  }

  virtual void TearDown() override {
    config = config_;

    system_->finalize();
  }

  struct config config_;
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
TEST_P(hello_fpga_c_p, print_err) {
  print_err("msg", FPGA_OK);
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When passed an invalid command option,<br>
 *             parse_args prints a message and<br>
 *             returns a value other than FPGA_OK.<br>
 */
TEST_P(hello_fpga_c_p, parse_args0) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_fpga");
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
TEST_P(hello_fpga_c_p, parse_args1) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-B");

  char *argv[] = { zero, one };

  EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(hello_fpga_c_p, parse_args2) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-B");
  strcpy(two, "3");
  strcpy(three, "-s");

  char *argv[] = { zero, one, two, three };

  EXPECT_EQ(parse_args(4, argv), FPGA_OK);
  EXPECT_EQ(config.target.bus, 3);
  EXPECT_EQ(config.open_flags, FPGA_OPEN_SHARED);
}

/**
 * @test       find_fpga0
 * @brief      Test: find_fpga
 * @details    When passed a guid that matches no device,<br>
 *             find_fpga sets *num_matches_accelerators to 0,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(hello_fpga_c_p, find_fpga0) {
  fpga_guid guid;
  fpga_token tok = nullptr;
  uint32_t matches = 0xff;

  config.target.bus = platform_.devices[0].bus;

  ASSERT_EQ(uuid_parse(INVALID_AFU_ID, guid), 0);
  EXPECT_EQ(find_fpga(guid, &tok, &matches), FPGA_OK);
  EXPECT_EQ(tok, nullptr);
  EXPECT_EQ(matches, 0);
}

/**
 * @test       get_bus0
 * @brief      Test: get_bus
 * @details    When passed a valid fpga_token,<br>
 *             get_bus retrieves the associated bus into *bus,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(hello_fpga_c_p, get_bus0) {
  fpga_guid guid;
  fpga_token tok = nullptr;
  uint32_t matches = 0xff;

  config.target.bus = platform_.devices[0].bus;

  ASSERT_EQ(uuid_parse(platform_.devices[0].afu_guid, guid), 0);
  EXPECT_EQ(find_fpga(guid, &tok, &matches), FPGA_OK);
  ASSERT_NE(tok, nullptr);
  ASSERT_GT(matches, 0);

  uint8_t bus = 0xff;
  EXPECT_EQ(get_bus(tok, &bus), FPGA_OK);
  EXPECT_EQ(bus, platform_.devices[0].bus);

  EXPECT_EQ(fpgaDestroyToken(&tok), FPGA_OK);
}

/**
 * @test       main0
 * @brief      Test: hello_fpga_main
 * @details    When given an invalid command option,<br>
 *             hello_fpga_main displays an error message,<br>
 *             and the fn returns non-zero.<br>
 */
TEST_P(hello_fpga_c_p, main0) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_NE(hello_fpga_main(2, argv), 0);
}

INSTANTIATE_TEST_CASE_P(hello_fpga_c, hello_fpga_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));

class mock_hello_fpga_c_p : public hello_fpga_c_p {
 protected:
  mock_hello_fpga_c_p() {}
};

/**
 * @test       main1
 * @brief      Test: hello_fpga_main
 * @details    When given a valid command line,<br>
 *             hello_fpga_main (mock) runs the NLB0 workload.<br>
 *             The workload times out in a mock environment,<br>
 *             causing hello_fpga_main to return FPGA_EXCEPTION.<br>
 */
TEST_P(mock_hello_fpga_c_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);

  EXPECT_EQ(hello_fpga_main(3, argv), FPGA_EXCEPTION);
}

INSTANTIATE_TEST_CASE_P(mock_hello_fpga_c, mock_hello_fpga_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));

class hw_hello_fpga_c_p : public mock_hello_fpga_c_p {
 protected:
  hw_hello_fpga_c_p() {}
};

/**
 * @test       main1
 * @brief      Test: hello_fpga_main
 * @details    When given a valid command line,<br>
 *             hello_fpga_main (hw) runs the NLB0 workload,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(hw_hello_fpga_c_p, main1) {
  char zero[20];
  char one[20];
  char two[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);

  char *argv[] = { zero, one, two };

  EXPECT_EQ(hello_fpga_main(3, argv), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(hw_hello_fpga_c, hw_hello_fpga_c_p,
                        ::testing::ValuesIn(test_platform::hw_platforms({"skx-p","skx-p-v"})));
