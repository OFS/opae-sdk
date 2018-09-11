// Copyright(c) 2017-2018, Intel Corporation
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

#include <opae/enum.h>
#include <opae/properties.h>
#include "intel-fpga.h"
#include "gtest/gtest.h"
#include "types_int.h"
#include "test_system.h"
#include <opae/mmio.h>
#include <cstdarg>
#include <linux/ioctl.h>

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

using namespace opae::testing;


static int port_release_ioctl(mock_object * m, int request, va_list argp){
  int retval = -1;
  errno = EINVAL;
  struct fpga_fme_port_release *port_release =
         va_arg(argp, struct fpga_fme_port_release *);
  if (!port_release) {
    FPGA_MSG("port_release is NULL");
    goto out_EINVAL;
  }
  if (port_release->argsz != sizeof(*port_release)) {
    FPGA_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (port_release->flags != 0) {
    FPGA_MSG("unexpected flags %u", port_release->flags);
    goto out_EINVAL;
  }
  if (port_release->port_id != 0) {
    FPGA_MSG("unexpected port ID %u", port_release->port_id);
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

static int port_assign_ioctl(mock_object * m, int request, va_list argp){
  int retval = -1;
  errno = EINVAL;
  struct fpga_fme_port_assign *port_assign =
      va_arg(argp, struct fpga_fme_port_assign *);
  if (!port_assign) {
      FPGA_MSG("port_assign is NULL");
      goto out_EINVAL;
  }
  if (port_assign->argsz != sizeof(*port_assign)) {
      FPGA_MSG("wrong structure size");
      goto out_EINVAL;
  }
  if (port_assign->flags != 0) {
      FPGA_MSG("unexpected flags %u", port_assign->flags);
      goto out_EINVAL;
  }
  if (port_assign->port_id != 0) {
      FPGA_MSG("unexpected port ID %u", port_assign->port_id);
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



class mock_err_inj_c_p : public ::testing::TestWithParam<std::string> {
 protected:
   mock_err_inj_c_p() : tmpsysfs_("mocksys-XXXXXX") {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};


/**
* @test    fpga_mock_errinj_01
* @brief   Tests: fpgaReset
* @details fpgaReset resets fpga afu
*          Then the return error code
*/
TEST_P(mock_err_inj_c_p, fpga_mock_errinj_01) {
  // Open port device
  ASSERT_EQ(FPGA_OK, fpgaOpen(tokens_[0], &handle_, 0));
  
  // Reset
  system_->register_ioctl_handler(FPGA_PORT_RESET,dummy_ioctl<0,ENOTSUP>);
  EXPECT_EQ(FPGA_OK, fpgaReset(handle_));
  
  // close port
  ASSERT_EQ(FPGA_OK, fpgaClose(handle_));

}

/**
* @test    fpga_mock_errinj_02
* @brief   Tests: fpgaMapMMIO
* @details fpgaMapMMIO maps fpga afu mmio region
*          Then the return error code
*/
//TEST_P(mock_err_inj_c_p, fpga_mock_errinj_02) {
//  uint32_t mmio_num;
//  uint64_t *mmio_ptr;
//  
//  // Open port device
//  ASSERT_EQ(FPGA_OK, fpgaOpen(tokens_[0], &handle_, 0));
//  
//  // mmap 
//  mmio_num = 0;
//  EXPECT_EQ(FPGA_OK, fpgaMapMMIO(handle_, mmio_num, &mmio_ptr));
//  
//  // close port
//  ASSERT_EQ(FPGA_OK, fpgaClose(handle_));
//}


/**
* @test    fpga_mock_errinj_03
* @brief   Tests: fpgaPrepareBuffer and fpgaReleaseBuffer
* @details API allcocats buffer and Release buffer
*          Then the return error code
*/
//TEST_P(mock_err_inj_c_p, fpga_mock_errinj_03) {
//  uint64_t buf_len;
//  uint64_t* buf_addr;
//  uint64_t wsid = 1;
//  
//  // Open port device
//  ASSERT_EQ(FPGA_OK, fpgaOpen(tokens_[0], &handle_, 0));
//  
//  // Allocate a buffer
//  buf_len = 1024;
//  EXPECT_EQ(FPGA_OK,
//  	fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, 0));
//  
//  // Release buffer
//  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(handle_, wsid));
//  
//}
//
//
///**
//* @test    fpga_mock_errinj_04
//* @brief   Tests:fpgaGetNumUmsg,fpgaSetUmsgAttributes
//*...........fpgaGetUmsgPtr and fpgaTriggerUmsg
//* @details API Set,Get and Trigger UMSG
//*          Then the return error code
//*/
//TEST_P(mock_err_inj_c_p, fpga_mock_errinj_04) {
//
//	struct _fpga_token _tok;
//	fpga_token tok = &_tok;
//	fpga_handle h;
//	uint64_t *value = 0;
//	uint64_t *umsg_ptr;
//
//	// Open port device
//	token_for_afu0(&_tok);
//	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
//
//	// Get Number of UMSG
//	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, value));
//
//	// Set UMSG
//	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, 0));
//
//	// Get UMSG pointer
//	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));
//
//	// Trigger UMSG
//	EXPECT_NE(FPGA_OK, fpgaTriggerUmsg(h, 0));
//
//	// Close port handle
//	ASSERT_EQ(FPGA_OK, fpgaClose(h));
//}
//
/**
* @test    fpga_mock_errinj_05
* @brief   Tests:fpgaAssignPortToInterface
* @details fpgaAssignPortToInterface Assign and Release port
*          Then the return error code
*/
TEST_P(mock_err_inj_c_p, fpga_mock_errinj_05) {
  fpga_result res;
  // Open port device
  res = fpgaOpen(tokens_[0], &handle_, 0);
  ASSERT_EQ(FPGA_OK, res);
   
  res = fpgaAssignPortToInterface(handle_, 2, 0, 0);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  system_->register_ioctl_handler(FPGA_FME_PORT_RELEASE, port_release_ioctl);
  res = fpgaAssignPortToInterface(handle_, 0, 0, 0);
  EXPECT_EQ(FPGA_OK, res);

  system_->register_ioctl_handler(FPGA_FME_PORT_ASSIGN, port_assign_ioctl);
  res = fpgaAssignPortToInterface(handle_, 1, 0, 0);
  EXPECT_EQ(FPGA_OK, res);

  ASSERT_EQ(FPGA_OK, fpgaClose(handle_));
}

TEST_P(mock_err_inj_c_p, port_to_interface_err) {
  fpga_result res;
  // Open port device
  res = fpgaOpen(tokens_[0], &handle_, 0);
  ASSERT_EQ(FPGA_OK, res);
   
  system_->register_ioctl_handler(FPGA_FME_PORT_RELEASE, dummy_ioctl<-1,EINVAL>);
  res = fpgaAssignPortToInterface(handle_, 1, 0, 0);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, res);

  system_->register_ioctl_handler(FPGA_FME_PORT_ASSIGN, dummy_ioctl<-1,EINVAL>);
  res = fpgaAssignPortToInterface(handle_, 0, 0, 0);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, res);

  ASSERT_EQ(FPGA_OK, fpgaClose(handle_));
}

INSTANTIATE_TEST_CASE_P(mock_err_c, mock_err_inj_c_p, ::testing::ValuesIn(test_platform::keys(true)));
