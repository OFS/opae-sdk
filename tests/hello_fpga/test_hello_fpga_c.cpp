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

extern "C" {
void print_err(const char *s, fpga_result res);

struct config {
  int open_flags;
  int run_n3000;
};
extern struct config config;

fpga_result parse_args(int argc, char *argv[]);

fpga_result find_fpga(fpga_properties device_filter,
		      fpga_guid afu_guid,
                      fpga_token *accelerator_token,
                      uint32_t *num_matches_accelerators);

int hello_fpga_main(int argc, char *argv[]);
}

#include <linux/ioctl.h>
#include "intel-fpga.h"
#include "fpga-dfl.h"

using namespace opae::testing;

int mmio_ioctl(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
    if (!rinfo) {
      OPAE_MSG("rinfo is NULL");
      goto out_EINVAL;
    }
    if (rinfo->argsz != sizeof(*rinfo)) {
      OPAE_MSG("wrong structure size");
      goto out_EINVAL;
    }
    if (rinfo->index > 1 ) {
      OPAE_MSG("unsupported MMIO index");
      goto out_EINVAL;
    }
    if (rinfo->padding != 0) {
      OPAE_MSG("unsupported padding");
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

class hello_fpga_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    optind = 0;
    config_ = config;
  }

  virtual void TearDown() override {
    config = config_;

    opae_base_p<>::TearDown();
  }

  struct config config_;
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
TEST_P(hello_fpga_c_p, parse_args1) {
  char zero[20];
  char one[20];
  strcpy(zero, "hello_fpga");
  strcpy(one, "-B");

  char *argv[] = { zero, one, NULL };

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
  strcpy(zero, "hello_fpga");
  strcpy(one, "-s");

  char *argv[] = { zero, one, NULL };

  EXPECT_EQ(parse_args(2, argv), FPGA_OK);
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
  test_device device = platform_.devices[0];
  if (device.has_afu) {
    fpga_guid guid;
    fpga_token tok = nullptr;
    uint32_t matches = 0xff;

    fpga_properties filter = NULL;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    ASSERT_EQ(fpgaPropertiesSetSegment(filter, platform_.devices[0].segment), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetBus(filter, platform_.devices[0].bus), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDevice(filter, platform_.devices[0].device), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetFunction(filter, platform_.devices[0].function), FPGA_OK);

    ASSERT_EQ(uuid_parse(INVALID_AFU_ID, guid), 0);
    EXPECT_EQ(find_fpga(filter, guid, &tok, &matches), FPGA_OK);
    EXPECT_EQ(tok, nullptr);
    EXPECT_EQ(matches, 0);

    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
  }
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

  char *argv[] = { zero, one, NULL };

  EXPECT_NE(hello_fpga_main(2, argv), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hello_fpga_c_p);
INSTANTIATE_TEST_SUITE_P(hello_fpga_c, hello_fpga_c_p,
                         ::testing::ValuesIn(test_platform::platforms({})));

class mock_hello_fpga_c_p : public hello_fpga_c_p {};

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

  char *argv[] = { zero, one, two, NULL };

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
  system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);

  EXPECT_EQ(hello_fpga_main(3, argv), FPGA_EXCEPTION);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(mock_hello_fpga_c_p);
INSTANTIATE_TEST_SUITE_P(mock_hello_fpga_c, mock_hello_fpga_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"skx-p", "dcp-rc"})));

class hw_hello_fpga_c_p : public mock_hello_fpga_c_p {};

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

  char *argv[] = { zero, one, two, NULL };

  EXPECT_EQ(hello_fpga_main(3, argv), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hw_hello_fpga_c_p);
INSTANTIATE_TEST_SUITE_P(hw_hello_fpga_c, hw_hello_fpga_c_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({"skx-p", "dcp-rc"})));
