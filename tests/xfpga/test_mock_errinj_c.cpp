// Copyright(c) 2017-2022, Intel Corporation
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
KEEP_XFPGA_SYMBOLS

#include <linux/ioctl.h>

extern "C" {
#include "types_int.h"
#include "xfpga.h"
#include "fpga-dfl.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

#include <opae/enum.h>
#include <opae/properties.h>
#include <opae/mmio.h>

using namespace opae::testing;

int dfl_port_release_ioctl(mock_object * m, int request, va_list argp){
  UNUSED_PARAM(m);
  UNUSED_PARAM(request);
  UNUSED_PARAM(argp);
  int retval = -1;
  errno = EINVAL;
  int *port_id = va_arg(argp,int*);

  if (*port_id != 0) {
    OPAE_MSG("unexpected port ID %u", *port_id);
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
  int *port_id = va_arg(argp, int*);

  if (*port_id != 0) {
	  OPAE_MSG("unexpected port ID %u", *port_id);
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

class err_inj_c_p : public opae_device_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

};

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
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaAssignPortToInterface(device_, 1, 0, 0));

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_ASSIGN, dummy_ioctl<-1, ENOTSUP>);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaAssignPortToInterface(device_, 0, 0, 0));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(err_inj_c_usd_p);
INSTANTIATE_TEST_SUITE_P(err_inj_c, err_inj_c_usd_p, 
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0"
                                                                           })));

class err_inj_c_mock_p : public err_inj_c_p {};

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
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPrepareBuffer(device_, buf_len, (void**) &buf_addr, &wsid, 0));
  
  // Release buffer
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReleaseBuffer(device_, wsid));
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
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReset(device_));
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
  EXPECT_NE(FPGA_OK, xfpga_fpgaMapMMIO(device_, mmio_num, &mmio_ptr));
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
  res = xfpga_fpgaAssignPortToInterface(device_, 1, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  system_->register_ioctl_handler(DFL_FPGA_FME_PORT_ASSIGN, dummy_ioctl<-1,EINVAL>);
  res = xfpga_fpgaAssignPortToInterface(device_, 0, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(err_inj_c_mock_p);
INSTANTIATE_TEST_SUITE_P(err_inj_c, err_inj_c_mock_p, 
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0"
                                                                           })));

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
