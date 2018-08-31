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
#include <opae/fpga.h>

#ifdef __cplusplus

extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <opae/access.h>
#include <opae/umsg.h>
#include "types_int.h"
#include "intel-fpga.h"
#include <cstdarg>
#include <linux/ioctl.h>

#include "gtest/gtest.h"
#include "test_system.h"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

using namespace opae::testing;

int umsg_ioctl(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    static bool gEnableIRQ = false;

    struct fpga_port_info *pinfo = va_arg(argp, struct fpga_port_info *);
    if (!pinfo) {
        FPGA_MSG("pinfo is NULL");
        goto out_EINVAL;
    }
    if (pinfo->argsz != sizeof(*pinfo)) {
        FPGA_MSG("wrong structure size");
        goto out_EINVAL;
    }
    pinfo->flags = 0;
    pinfo->num_regions = 1;
    pinfo->num_umsgs = 8;
    if (gEnableIRQ) {
        pinfo->capability = FPGA_PORT_CAP_ERR_IRQ | FPGA_PORT_CAP_UAFU_IRQ;
        pinfo->num_uafu_irqs = 1;
    } else {
        pinfo->capability = 0;
        pinfo->num_uafu_irqs = 0;
    }
    retval = 0;
    errno = 0;
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;

}

int umsg_enable_ioctl(mock_object * m, int request, va_list argp){
    return 0;
}

class umsg_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  umsg_c_p() : tmpsysfs_("mocksys-XXXXXX"), handle_(nullptr) {}

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
    ASSERT_EQ(fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_INFO, umsg_ioctl);
    system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, umsg_enable_ioctl);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
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
 * @test       umsg_c_p
 * @brief      test_umsg_drv_01
 * @details    When the parameters are valid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns number of umsgs supported by
 *             slot.
 *
 */
TEST_P (umsg_c_p, test_umsg_drv_01) {
  uint64_t Umsg_num = 0;

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(handle_, NULL));
  // get umsg number
  EXPECT_EQ(FPGA_OK, fpgaGetNumUmsg(handle_, &Umsg_num));
  EXPECT_GT(Umsg_num, 0);
}

/**
 * @test       umsg_c_p
 * @brief      test_umsg_drv_02
 * @details    When the parameters are invalid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns error.
 *
 */
TEST_P (umsg_c_p, test_umsg_drv_02) {
  uint64_t Umsg_num = 0;
  int fddev = -1;

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(NULL, &Umsg_num));


  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(_handle, &Umsg_num));

  _handle->magic = FPGA_HANDLE_MAGIC;
}



/**
 * @test       umsg_c_p
 * @brief      test_umsg_drv_03
 * @details    When the parameters are invalid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns error.
 *
 */
TEST_P (umsg_c_p, test_umsg_drv_03) {
  uint64_t Umsg_num = 0;
  int fddev = -1;

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(NULL, &Umsg_num));

  // Invlaid Input Paramter
  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(handle_, NULL));

  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(handle_, &Umsg_num));

  _handle->fddev = fddev;

}



/**
 * @test       umsg_c_p
 * @brief      Test: test_set_attribute
 * @details    When the parameters are invalid and the drivers are
 *             loaded, fpgaUmsgSetAttributes sets umsg hit Enable/Disable.
 *
 */

TEST_P(umsg_c_p, test_set_attribute) {
  uint64_t Umsghit_Enable = 0xffff;
  uint64_t Umsghit_Disble = 0;

  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(NULL, Umsghit_Disble));
  // Set umsg hint
  EXPECT_EQ(FPGA_OK, fpgaSetUmsgAttributes(handle_, Umsghit_Enable));
  EXPECT_EQ(FPGA_OK, fpgaSetUmsgAttributes(handle_, Umsghit_Disble));
}

/**
 * @test       umsg_c_p
 * @brief      Test: test_get_ptr_01
 * @details    When the parameters are valid and the drivers are loaded,
 *             fpgaGetUmsgPtr returns umsg address.
 *
 */
TEST_P(umsg_c_p, test_get_ptr_01) {
  uint64_t* umsg_ptr = NULL;
  int fddev = -1;

  // NULL Driver handler
  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(nullptr, &umsg_ptr));
  EXPECT_EQ(FPGA_OK, fpgaGetUmsgPtr(handle_, &umsg_ptr));
  EXPECT_TRUE(umsg_ptr != NULL) << "\tThe content of umsg_ptr is " << umsg_ptr;
}



/**
 * @test       umsg_c_p
 * @brief      Test: test_get_ptr
 * @details    When the parameters are valid and the drivers are loaded,
 *             fpgaGetUmsgPtr returns umsg address.
 *
 */
//TEST_P(umsg_c_p, test_get_ptr) {
//  uint64_t* umsg_ptr = NULL;
//  int fddev = -1;
//
//  // NULL Driver handler
//  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(nullptr, &umsg_ptr));
//
//  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;
//  _handle->magic = 0x123;
//
//  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(handle_, &umsg_ptr));
//
//  _handle->magic = FPGA_HANDLE_MAGIC;
//}



INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_p, ::testing::ValuesIn(test_platform::keys(true)));
