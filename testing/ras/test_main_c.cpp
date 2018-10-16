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

// RAS Command line struct
struct  RASCommandLine
{
  uint32_t flags;
  int segment;
  int bus;
  int device;
  int function;
  int socket;
  bool print_error;
  bool catast_error;
  bool fatal_error;
  bool nonfatal_error;
  bool clear_injerror;
  bool mwaddress_error;
  bool mraddress_error;
  bool mwlength_error;
  bool mrlength_error;
  bool pagefault_error;
};

extern struct RASCommandLine rasCmdLine;

void RASAppShowHelp(void);
void print_err(const char*, fpga_result);
int ras_main(int argc, char *argv[]);
fpga_result print_errors(fpga_token, const char*, const char*, int);
fpga_result print_ras_errors(fpga_token);
fpga_result print_port_errors(fpga_token);
fpga_result clear_port_errors(fpga_token);
fpga_result print_pwr_temp(fpga_token);
fpga_result mmio_error(fpga_handle, struct RASCommandLine*);
fpga_result page_fault_errors();
fpga_result inject_ras_errors(fpga_token, struct RASCommandLine*);
fpga_result clear_inject_ras_errors(fpga_token); 
}

#include "intel-fpga.h"
#include "types_int.h"
#include <iostream>
#include <fstream>
#include <opae/mmio.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <string>
#include <cstdarg>
#include "gtest/gtest.h"
#include "test_system.h"
#define OPAE_WRAPPED_HANDLE_MAGIC 0x6e616877
using namespace opae::testing;

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
        printf("MOCK " fmt "\n", ## __VA_ARGS__)

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

class ras_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  ras_c_p()
      : tokens_dev_{{nullptr, nullptr}},
        tokens_accel_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
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
    cmd_line_ = rasCmdLine;

    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_dev_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_dev_, FPGA_DEVICE), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_dev_, 1, tokens_dev_.data(), tokens_dev_.size(),
                            &num_matches_),
                            FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    handle_dev_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_dev_[0], &handle_dev_, 0), FPGA_OK);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_accel_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_accel_, FPGA_DEVICE), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_accel_, 1, tokens_accel_.data(), tokens_accel_.size(),
                            &num_matches_),
                            FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    handle_accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_accel_[0], &handle_accel_, 0), FPGA_OK);

  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_dev_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyProperties(&filter_accel_), FPGA_OK);
    if (handle_dev_) {
      EXPECT_EQ(fpgaClose(handle_dev_), FPGA_OK);
      handle_dev_ = nullptr;
    }

    if (handle_accel_) {
      EXPECT_EQ(fpgaClose(handle_accel_), FPGA_OK);
      handle_accel_ = nullptr;
    }
 
    for (auto &t : tokens_dev_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

    rasCmdLine = cmd_line_;
    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmp_gbs_);
    }
  }

  fpga_properties filter_dev_;
  fpga_properties filter_accel_;
  std::array<fpga_token, 2> tokens_dev_;
  std::array<fpga_token, 2> tokens_accel_;
  fpga_handle handle_dev_;
  fpga_handle handle_accel_;
  uint32_t num_matches_;
  struct RASCommandLine cmd_line_;
  char tmp_gbs_[20];
  test_platform platform_;
  test_system *system_;

};

/**
 * @test       help
 * @brief      Test: show_help
 * @details    RASAppShowHelp displays the application help message.<br>
 */
TEST(ras_c, show_help){
  RASAppShowHelp();
}

/**
 * @test       print_err
 * @brief      Test: invalid_print_error
 * @details    When params to print_err is valid, it displays the std error.<br>
 */
TEST(ras_c, invalid_print_error){
  const std::string des = "/sys/class/fpga/intel-fpga-dev.01";
  print_err(des.c_str(), FPGA_INVALID_PARAM);
}

/**
 * @test       page_fault_errors
 * @brief      Test: test_page_fault_errors
 * @details    When mmio ioctl FPGA_PORT_GET_REGION_INFO is valid, 
 *             page_fault_errors returns FPGA_OK.<br>
 */
TEST_P(ras_c_p, test_page_fault_errors){
  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
  EXPECT_EQ(FPGA_OK, page_fault_errors());
}

/**
 * @test       page_fault_errors
 * @brief      Test: invalid_page_fault_errors
 * @details    When mmio ioctl fails on FPGA_PORT_GET_REGION_INFO, 
 *             page_fault_errors returns FPGA_INVALID_PARAM.<br>
 */
TEST(ras_c, invalid_page_fault_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, page_fault_errors());
}

/**
 * @test       print_token_errors
 * @brief      Test: invalid_print_token_errors
 * @details    When fpga_token is invalid, print_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_print_token_errors){
  fpga_properties filter = NULL;
  uint32_t num_matches = 0;
  fpga_token tok = nullptr;
  const std::string sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors";

  EXPECT_EQ(FPGA_INVALID_PARAM, print_errors(tok, sysfs_port.c_str(), nullptr, 0)); 
}

/**
 * @test       inject_ras_errors
 * @brief      Test: test_inject_ras_errors
 * @details    When fpga_token is invalid, inject_ras_error returns 
 *             FPGA_INVALID_PARAM. Else, it returns FPGA_OK.<br>
 */
TEST_P(ras_c_p, test_inject_ras_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, inject_ras_errors(nullptr, &rasCmdLine)); 

  cmd_line_ = { 0, -1, -1, -1, -1, -1, false,
               false, false, true, false,
               false, false, false, false, true};
  struct _fpga_token * tok = static_cast<_fpga_token*>(tokens_dev_[0]);

  auto current_magic = tok->magic;
  tok->magic = OPAE_WRAPPED_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, inject_ras_errors(handle_dev_, &cmd_line_)); 

  tok->magic = current_magic;
}

/**
 * @test       clear_inject_ras_errors
 * @brief      Test: invalid_clear_inject_ras_errors
 * @details    When fpga_token is invalid, clear_inject_ras_error returns 
 *             FPGA_INVALID_PARAM. Else, it returns FPGA_OK.<br>
 */
TEST_P(ras_c_p, invalid_clear_inject_ras_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, clear_inject_ras_errors(nullptr)); 
}

/**
 * @test       print_pwr_temp
 * @brief      Test: test_print_pwr_temp
 * @details    When fpga_token is invalid, print_pwr_temp returns 
 *             FPGA_INVALID_PARAM. Else, it returns FPGA_OK.<br>
 */
TEST_P(ras_c_p, invalid_print_pwr_temp){
  EXPECT_EQ(FPGA_INVALID_PARAM, print_pwr_temp(nullptr)); 

  EXPECT_EQ(FPGA_OK, print_pwr_temp(tokens_dev_[0])); 
}

/**
 * @test       test_mmio_error
 * @brief      Test: mmio_error 
 * @details    When mmio_erro is called with an valid afu handle,<br>
 *             injecting mmio errors, it returns FPGA_EXCEPTION because<br>
 *             FPGA PCIE BAR2 doesn't exist.<br>
 */
TEST_P(ras_c_p, test_mmio_error){
  cmd_line_ = { 0, -1, -1, -1, -1, -1, false,
                false, false, false,false,
                false, false, true, true, false};
  EXPECT_EQ(FPGA_OK, mmio_error(handle_accel_, &cmd_line_));
}

/**
 * @test       mmio_errors
 * @brief      Test: invalid_mmio_errors
 * @details    When RASCommandLine is invalid, mmio_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_mmio_errors){
  struct RASCommandLine invalid_rasCmdLine;
  EXPECT_EQ(FPGA_INVALID_PARAM, mmio_error(nullptr, &invalid_rasCmdLine)); 
}

/**
 * @test       print_ras_errors
 * @brief      Test: invalid_ras_errors
 * @details    When fpga_token is invalid, print_ras_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_ras_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, print_ras_errors(nullptr));
}

/**
 * @test       print_port_errors
 * @brief      Test: invalid_port_errors
 * @details    When fpga_token is invalid, print_port_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_print_port_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, print_port_errors(nullptr));
}

/**
 * @test       clear_port_errors
 * @brief      Test: invalid_port_errors
 * @details    When fpga_token is invalid, clear_port_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_clear_port_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, clear_port_errors(nullptr));
}

/**
 * @test       main_params_01
 * @brief      Test: ras_main
 * @details    When ras_main is called with an valid command,<br>
 *             injecting cast error option, ras_main returns zero.<br>
 */
TEST_P(ras_c_p, main_params_01){
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

  strcpy(zero, "ras");
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
  strcpy(eleven, "-P");
  strcpy(twelve, "-Q");
  strcpy(thirteen, "-R");
  strcpy(fourteen, "-O");
  strcpy(fifteen, "-N");
  strcpy(sixteen, "-C");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen,
                   fourteen, fifteen, sixteen };

  EXPECT_EQ(ras_main(17, argv), 0);
}

/**
 * @test       main_params_05
 * @brief      Test: ras_main
 * @details    When ras_main is called with an valid command option,<br>
 *             injecting mmio error, ras_main retunrs zeros.<br>
 */
TEST_P(ras_c_p, main_params_05){
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  char six[20];
  char seven[20];
  char eight[20];

  strcpy(zero, "ras");
  strcpy(one, "-B");
  sprintf(two, "%d", platform_.devices[0].bus);
  strcpy(three, "-P");
  strcpy(four, "-E");
  strcpy(five, "-G");
  strcpy(six, "-H");
  strcpy(seven, "-I");
  strcpy(eight, "-C");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight };

  EXPECT_EQ(ras_main(9, argv), 0);
}

/**
 * @test       invalid_cmd_01
 * @brief      Test: ras_main
 * @details    When ras_main is called with only 1 arg,<br>
 *             main returns 1.<br>
 */
TEST_P(ras_c_p, invalid_cmd_01){
  char zero[20];
  strcpy(zero, "ras");

  char *argv[] = { zero }; 
  EXPECT_EQ(ras_main(1, argv), 1);
}

/**
 * @test       invalid_cmd_02
 * @brief      Test: ras_main
 * @details    When ras_main is called with an invalid command option,<br>
 *             main returns 2.<br>
 */
TEST_P(ras_c_p, invalid_cmd_02){
  char zero[20];
  char one[20];
  
  strcpy(zero, "ras");
  strcpy(one, "-h");

  char *argv[] = { zero, one };
  EXPECT_EQ(ras_main(2, argv), 2);
}

INSTANTIATE_TEST_CASE_P(ras_c, ras_c_p,
                        ::testing::Values(std::string("skx-p-1s")));
