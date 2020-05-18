// Copyright(c) 2017-2020, Intel Corporation
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

#include <opae/enum.h>
#include <opae/properties.h>
#include "gtest/gtest.h"
#include "types_int.h"
#include "mock/test_system.h"
#include <opae/mmio.h>
#include <cstdarg>
#include <linux/ioctl.h>
#include "xfpga.h"
#include "fpga-dfl.h"
#include "sysfs_int.h"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}


using namespace opae::testing;

int dfl_port_release_ioctl(mock_object * m, int request, va_list argp){
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  UNUSED_PARAM(argp);
  int retval = -1;
  errno = EINVAL;
  int port_id = va_arg(argp,int);

  if (port_id != 0) {
    FPGA_MSG("unexpected port ID %u", port_id);
    goto out_EINVAL;
  }
  retval = 0;
  errno = 0;

out:
  va_end(argp);
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}

int dfl_port_assign_ioctl(mock_object * m, int request, va_list argp){
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  UNUSED_PARAM(argp);
  int retval = -1;
  errno = EINVAL;
  errno = EINVAL;
  int port_id = va_arg(argp, int);

  if (port_id != 0) {
	  FPGA_MSG("unexpected port ID %u", port_id);
	  goto out_EINVAL;
  }

  retval = 0;
  errno = 0;
out:
  va_end(argp);
  return retval;

out_EINVAL:
  retval = -1;
  errno = EINVAL;
  goto out;
}

class err_inj_c_p : public ::testing::TestWithParam<std::string> {
 protected:
   err_inj_c_p() : tokens_{{nullptr, nullptr}},
                   handle_ {nullptr} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);
    // Open port device
    ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) { 
      ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_)); 
      handle_ = nullptr;
    }

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&t));
        t = nullptr;
      }
    }
    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};


/**
* @test    fpga_mock_errinj_03
* @brief   Tests:fpgaAssignPortToInterface
* @details fpgaAssignPortToInterface given invalid param
*          Then the return error code
*/
TEST_P(err_inj_c_p, fpga_mock_errinj_03) {
  int fddev = -1;

  struct _fpga_handle* h = (struct _fpga_handle*)handle_;
  fddev = h->fddev;
  h->fddev = -1;

  auto res = xfpga_fpgaAssignPortToInterface(handle_, 1, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  h->fddev = fddev;
}

/**
* @test    fpga_mock_errinj_02
* @brief   Tests:fpgaAssignPortToInterface
* @details fpgaAssignPortToInterface Assign and Release port
*          Then the return FPGA_OK 
*/
TEST_P(err_inj_c_p, fpga_mock_errinj_02) {
  fpga_result res;
   
  res = xfpga_fpgaAssignPortToInterface(handle_, 2, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_RELEASE, dfl_port_release_ioctl);
  res = xfpga_fpgaAssignPortToInterface(handle_, 1, 0, 0);
  EXPECT_EQ(FPGA_OK, res);

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_ASSIGN, dfl_port_assign_ioctl);
  res = xfpga_fpgaAssignPortToInterface(handle_, 0, 0, 0);
  EXPECT_EQ(FPGA_OK, res);
}

/**
 * @test       invalid_max_interface_num
 *
 * @brief      When the interface_num parameter to fpgaAssignPortToInterface
 *             is greater than FPGA_MAX_INTERFACE_NUM,
 *             then the function returns FPGA_INVALID_PARAM.
 */
TEST_P(err_inj_c_p, invalid_max_interface_num) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaAssignPortToInterface(handle_, 99, 0, 0));
}

INSTANTIATE_TEST_CASE_P(err_inj_c, err_inj_c_p, 
                        ::testing::ValuesIn(test_platform::platforms({ "dfl-n3000","dfl-d5005" })));


class err_inj_c_usd_p : public err_inj_c_p {};

/**
 * @test       dfl_tests
 *
 * @brief      fpgaAssignPortToInterface Assign and Release port are not yet
 *             supported by the latest upstream-drv. The API calls will return
 *             FPGA_NOT_SUPPORTED.
 */

TEST_P(err_inj_c_usd_p, dfl_tests_neg) {
  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_RELEASE, dummy_ioctl<-1, ENOTSUP>);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaAssignPortToInterface(handle_, 1, 0, 0));

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_ASSIGN, dummy_ioctl<-1, ENOTSUP>);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaAssignPortToInterface(handle_, 0, 0, 0));
}

INSTANTIATE_TEST_CASE_P(err_inj_c, err_inj_c_usd_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({ "dfl-n3000","dfl-d5005" })));

class err_inj_c_mock_p : public err_inj_c_p {
 protected:
   err_inj_c_mock_p() {}
};

/**
* @test    fpga_mock_errinj_01
* @brief   Tests: fpgaPrepareBuffer and fpgaReleaseBuffer
* @details API allcocats buffer and Release buffer
*          Then the return error code
*/
TEST_P(err_inj_c_mock_p, fpga_mock_errinj_01) {
  uint64_t buf_len;
  uint64_t* buf_addr;
  uint64_t wsid = 1;
 
  // Allocate a buffer
  buf_len = 1024;
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, 0));
  
  // Release buffer
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReleaseBuffer(handle_, wsid));
}

/**
* @test    fpga_mock_errinj_04
* @brief   Tests: fpgaReset
* @details fpgaReset resets fpga afu
*          Then the return error code
*/
TEST_P(err_inj_c_mock_p, fpga_mock_errinj_04) {
  // Reset
  system_->register_ioctl_handler(DFL_FPGA_PORT_RESET,dummy_ioctl<0,ENOTSUP>);
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReset(handle_));
}

/**
* @test    fpga_mock_errinj_05
* @brief   Tests: fpgaMapMMIO
* @details fpgaMapMMIO maps fpga afu mmio region
*          Then the return error code
*/
TEST_P(err_inj_c_mock_p, fpga_mock_errinj_05) {
  uint32_t mmio_num;
  uint64_t *mmio_ptr;
  
  // mmap 
  mmio_num = 0;

  system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO,dummy_ioctl<0,EINVAL>);
  EXPECT_NE(FPGA_OK, xfpga_fpgaMapMMIO(handle_, mmio_num, &mmio_ptr));
}


/**
* @test    port_to_interface_err
* @brief   Tests:fpgaAssignPortToInterface
* @details fpgaAssignPortToInterface Assign and Release port
*          Then the return error code
*/
TEST_P(err_inj_c_mock_p, port_to_interface_err) {
  fpga_result res;
   
  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_RELEASE, dummy_ioctl<-1,EINVAL>);
  res = xfpga_fpgaAssignPortToInterface(handle_, 1, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_ASSIGN, dummy_ioctl<-1,EINVAL>);
  res = xfpga_fpgaAssignPortToInterface(handle_, 0, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);
}

INSTANTIATE_TEST_CASE_P(err_inj_c, err_inj_c_mock_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({ "dfl-n3000","dfl-d5005" })));

/**
 * @test       invalid_handle
 *
 * @brief      When the handle parameter to fpgaAssignPortToInterface
 *             is NULL,
 *             then the function returns FPGA_INVALID_PARAM.
 */
TEST(err_inj_c, invalid_handle) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaAssignPortToInterface(NULL, 0, 0, 0));
}
