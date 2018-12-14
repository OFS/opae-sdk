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
extern "C"{
#include "types_int.h"
fpga_result free_umsg_buffer(fpga_handle);
}

#include "xfpga.h"
#include "intel-fpga.h"
#include <cstdarg>
#include <linux/ioctl.h>

#include "gtest/gtest.h"
#include "test_system.h"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

using namespace opae::testing;

int umsg_port_info(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    static bool gEnableIRQ = false;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
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

int umsg_set_mode(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct fpga_port_umsg_cfg *ucfg = va_arg(argp, struct fpga_port_umsg_cfg *);
    if (!ucfg) {
    	FPGA_MSG("ucfg is NULL");
    	goto out_EINVAL;
    }
    if (ucfg->argsz != sizeof(*ucfg)) {
    	FPGA_MSG("wrong structure size");
    	goto out_EINVAL;
    }
    if (ucfg->flags != 0) {
    	FPGA_MSG("unexpected flags %u", ucfg->flags);
    	goto out_EINVAL;
    }
    /* TODO: check hint_bitmap */
    if (ucfg->hint_bitmap >> 8) {
    	FPGA_MSG("invalid hint_bitmap 0x%x", ucfg->hint_bitmap);
    	goto out_EINVAL;
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

int umsg_set_base_addr(mock_object * m, int request, va_list argp){
    int retval = -1;
    errno = EINVAL;
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    struct fpga_port_umsg_base_addr *ubase = va_arg(argp, struct fpga_port_umsg_base_addr *);
    if (!ubase) {
    	FPGA_MSG("ubase is NULL");
    	goto out_EINVAL;
    }
    if (ubase->argsz != sizeof(*ubase)) {
    	FPGA_MSG("wrong structure size");
    	goto out_EINVAL;
    }
    if (ubase->flags != 0) {
    	FPGA_MSG("unexpected flags %u", ubase->flags);
    	goto out_EINVAL;
    }
    /* TODO: check iova */
    retval = 0;
    errno = 0;
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;
}

class umsg_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  umsg_c_p()
  : handle_(nullptr),
    tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_,
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_INFO, umsg_port_info);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) { EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); }

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&t));
        t = nullptr;
      }
    }
    system_->finalize();
  }

  fpga_handle handle_;
  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       umsg_c_p
 * @brief      test_umsg_drv_02
 * @details    When the parameters are invalid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns error.
 *
 */
TEST_P (umsg_c_p, test_umsg_drv_02) {
  uint64_t Umsg_num = 0;

  // NULL Driver handle 
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(NULL, &Umsg_num));

  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(_handle, &Umsg_num));

  // Reset handle magic 
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

  // NULL Driver handle
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(NULL, &Umsg_num));

  // Invalid Input Parameter
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(handle_, NULL));

  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(handle_, &Umsg_num));

  // Reset handle fd
  _handle->fddev = fddev;
}

/**
 * @test       Umsg_drv_04
 *
 * @brief      When the parameters are Invalid and the drivers are
 *             loaded, fpgaUmsgSetAttributes retuns error.
 *
 */
TEST_P(umsg_c_p, test_umsg_drv_05) {
  uint64_t Umsghit_Disble = 0;
  int fddev = -1;

  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE,umsg_set_mode);
  // NULL Driver handle
  EXPECT_NE(FPGA_OK, xfpga_fpgaSetUmsgAttributes(NULL, Umsghit_Disble));

  // Invalid handle magic
  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, xfpga_fpgaSetUmsgAttributes(handle_, Umsghit_Disble));

  // Valid handle magic
  _handle->magic = FPGA_HANDLE_MAGIC;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  _handle = (struct _fpga_handle*)handle_;

  // Invalid handle fd
  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, xfpga_fpgaSetUmsgAttributes(handle_, Umsghit_Disble));

  // Valid handle fd
  _handle->fddev = fddev;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_));

  // Invlaid Input Paramter
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  EXPECT_NE(FPGA_OK, xfpga_fpgaSetUmsgAttributes(handle_, 0xFFFFFFFF));
}

/**
 * @test       Umsg_drv_07
 *
 * @brief      When the parameters are invalid and the drivers are
 *             loaded, xfpga_fpgaGetUmsgPtr returns fpga error.
 *
 */
TEST_P(umsg_c_p, test_umsg_drv_07) {
  uint64_t* umsg_ptr = NULL;
  int fddev = -1;

  // NULL Driver handle
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetUmsgPtr(NULL, &umsg_ptr));

  // Invalid handle magic
  struct _fpga_handle* _handle = (struct _fpga_handle*)handle_;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr));

  // Valid handle magic
  _handle->magic = FPGA_HANDLE_MAGIC;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));
  _handle = (struct _fpga_handle*)handle_;

  // Invalid handle fd
  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr));

  // Valid handle fd
  _handle->fddev = fddev;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaClose(handle_));

  // Invalid Input Parameter
  ASSERT_EQ(FPGA_OK, xfpga_fpgaOpen(tokens_[0], &handle_, 0));

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetUmsgPtr(handle_, NULL));
}

/**
 * @test       Umsg_08
 *
 * @brief      When the handle parameter to xfpga_fpgaTriggerUmsg
 *             is NULL, the function returns FPGA_INVALID_PARAM.
 *
 */
TEST_P(umsg_c_p, test_umsg_drv_08) {
  int fddev = -1;
  auto _handle = (struct _fpga_handle*)handle_;

  // Null handle
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaTriggerUmsg(NULL, 0));

  // Invalid handle fd
  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaTriggerUmsg(handle_, 0));

  // Reset fd for fpgaClose
  _handle->fddev = fddev;
}

INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_p, ::testing::ValuesIn(test_platform::platforms({})));

class umsg_c_mcp_p : public umsg_c_p {
};

/**
 * @test       umsg_c_p
 * @brief      test_umsg_drv_01
 * @details    When the parameters are valid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns number of umsgs supported by
 *             slot.
 *
 */
TEST_P (umsg_c_mcp_p, test_umsg_drv_01) {
  uint64_t Umsg_num = 0;

  EXPECT_NE(FPGA_OK, xfpga_fpgaGetNumUmsg(handle_, NULL));
  // Get umsg number
  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetNumUmsg(handle_, &Umsg_num));
  EXPECT_GT(Umsg_num, 0);
}

/**
 * @test       Umsg_drv_04
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaUmsgSetAttributes sets umsg hit  Enable / Disable.
 *
 */
TEST_P(umsg_c_mcp_p, test_umsg_drv_04) {
  uint64_t Umsghit_Enable = 0xffff;
  uint64_t Umsghit_Disble = 0;

  // Set umsg hint
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE,umsg_set_mode);
  EXPECT_NE(FPGA_OK, xfpga_fpgaSetUmsgAttributes(handle_, Umsghit_Enable));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaSetUmsgAttributes(handle_, Umsghit_Disble));
}

INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_mcp_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));

class umsg_c_mock_p : public umsg_c_p {
};

 /**
 * @test       Umsg_08
 *
 * @brief      When the handle parameter to xfpga_fpgaTriggerUmsg<br>
 *             is valid, Then the function returns FPGA_OK when <br>
 *             hugepages is allocated. <br>
 *
 */
TEST_P(umsg_c_mock_p, test_umsg_08) {
  auto res = xfpga_fpgaTriggerUmsg(handle_, 0);
  EXPECT_EQ(FPGA_OK, res) << "\t return value is " << res;
}

/**
 * @test       umsg_c_mock_p
 * @brief      get_num_umsg_ioctl_err
 * @details    When the parameters are valid and the drivers are loaded,
 *             but the ioctl fails,
 *             fpgaGetNumUmsg returns FPGA_INVALID_PARAM/FPGA_EXCEPTION
 *
 */
TEST_P(umsg_c_mock_p, get_num_umsg_ioctl_err) {
  uint64_t num = 0;

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_GET_INFO, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetNumUmsg(handle_, &num));

  // register an ioctl handler that will return -1 and set errno to EFAULT
  system_->register_ioctl_handler(FPGA_PORT_GET_INFO, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetNumUmsg(handle_, &num));

  // register an ioctl handler that will return -1 and set errno to ENOTSUP
  system_->register_ioctl_handler(FPGA_PORT_GET_INFO, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaGetNumUmsg(handle_, &num));
}

/**
 * @test       umsg_c_mock_p
 * @brief      set_umsg_attr_ioctl_err
 * @details    When the parameters are valid and the drivers are loaded,
 *             but the ioctl fails,
 *             fpgaSetUmsgAttributes returns FPGA_INVALID_PARAM/FPGA_EXCEPTION
 *
 */
TEST_P(umsg_c_mock_p, set_umsg_attr_ioctl_err) {
  uint64_t value = 0;
  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaSetUmsgAttributes(handle_, value));

  // register an ioctl handler that will return -1 and set errno to EFAULT
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaSetUmsgAttributes(handle_, value));


  // register an ioctl handler that will return -1 and set errno to ENOTSUP
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaSetUmsgAttributes(handle_, value));
}

/**
 * @test       umsg_c_mock_p
 * @brief      get_umsg_ptr_ioctl_err
 * @details    When the parameters are valid and the drivers are loaded,
 *             but the ioctl fails on FPGA_PORT_UMSG_ENABLE and FPGA_PORT_DMA_UNMAP
 *             fpgaGetUmsgPtr returns FPGA_INVALID_PARAM/FPGA_EXCEPTION
 *
 */
TEST_P(umsg_c_mock_p, get_umsg_ptr_ioctl_err) {
  uint64_t *value = 0;

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<-1,EINVAL>);
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetUmsgPtr(handle_, &value));

  // register an ioctl handler that will return -1 and set errno to EFAULT
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<-1,EFAULT>);
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetUmsgPtr(handle_, &value));

  // register an ioctl handler that will return -1 and set errno to ENOTSUP
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<-1,ENOTSUP>);
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaGetUmsgPtr(handle_, &value));
}

/**
 * @test       umsg_c_mock_p
 * @brief      get_umsg_ptr_ioctl_err_02
 * @details    When the parameters are valid and the drivers are loaded,
 *             but the ioctl fails on FPGA_PORT_UMSG_SET_BASE_ADDR
 *             and FPGA_PORT_DMA_UNMAP. fpgaGetUmsgPtr returns
 *             FPGA_INVALID_PARAM/FPGA_EXCEPTION
 *
 */
TEST_P(umsg_c_mock_p, get_umsg_ptr_ioctl_err_02) {
  uint64_t *value = 0;

  // register an ioctl handler that will return -1 and set errno to ENOTSUP
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, dummy_ioctl<-1,ENOTSUP>);
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaGetUmsgPtr(handle_, &value));

  // register an ioctl handler that will return -1 and set errno to EFAULT
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, dummy_ioctl<-1,EFAULT>);
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetUmsgPtr(handle_, &value));
}

/**
 * @test       umsg_c_mock_p
 * @brief      get_umsg_ptr_ioctl_err_03
 * @details    When the parameters are valid and the drivers are loaded,
 *             but the ioctl fails on FPGA_PORT_DMA_MAP. fpgaGetUmsgPtr returns
 *             FPGA_INVALID_PARAM/FPGA_EXCEPTION
 *
 */
TEST_P(umsg_c_mock_p, get_umsg_ptr_ioctl_err_03) {
  uint64_t *value = 0;

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetUmsgPtr(handle_, &value));

  // register an ioctl handler that will return -1 and set errno to EFAULT
  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetUmsgPtr(handle_, &value));
}

/**
 * @test       umsg_c_mock_p
 * @brief      invalid_free_umsg_buffer
 * @details    When the drivers are loaded and handle umsg_virt is mapped,
 *             but ioctl fails on FPGA_PORT_UMSG_DISABLE, FPGA_ERR outputs
 *             "Failed to disable UMSG" and returns FPGA_OK
 *             When ioctl fails on FPGA_PORT_UMSG_SET_BASE_ADDR, FPGA_ERR outputs
 *             "led to zero UMSG address" and returns FPGA_OK
 *
 */
TEST_P(umsg_c_mock_p, invalid_free_umsg_buffer) {
  uint64_t* umsg_ptr = NULL;
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, umsg_set_base_addr);
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<0,EINVAL>);
  auto res = xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr);
  EXPECT_EQ(FPGA_OK, res);

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_UMSG_DISABLE, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_OK, free_umsg_buffer(handle_));

  // register an ioctl handler that will return -1 and set errno to EINVAL
  res = xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr);
  EXPECT_EQ(FPGA_OK, res);
  system_->register_ioctl_handler(FPGA_PORT_UMSG_DISABLE, dummy_ioctl<0,EINVAL>);
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_OK, free_umsg_buffer(handle_));
}

/**
 * @test       umsg_c_mock_p
 * @brief      invalid_free_umsg_buffer
 * @details    When the drivers are loaded and handle umsg_virt is mapped,
 *             but ioctl fails on FPGA_PORT_DMA_UNMAP, FPGA_ERR outputs
 *             "Failed to unmap UMSG Buffer" and returns FPGA_OK
 *
 */
TEST_P(umsg_c_mock_p, invalid_free_umsg_buffer_02) {
  uint64_t* umsg_ptr = NULL;
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, umsg_set_base_addr);
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<0,EINVAL>);
  auto res = xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr);
  EXPECT_EQ(FPGA_OK, res);

  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_OK, free_umsg_buffer(handle_));
}

/**
 * @test       Umsg_drv_06
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             xfpga_fpgaGetUmsgPtr returns umsg address.
 *
 */
TEST_P(umsg_c_mock_p, test_umsg_drv_06) {
  uint64_t* umsg_ptr = NULL;
  fpga_result res;

  // Get umsg buffer
  system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_BASE_ADDR, umsg_set_base_addr);
  system_->register_ioctl_handler(FPGA_PORT_UMSG_ENABLE, dummy_ioctl<0,EINVAL>);
  res = xfpga_fpgaGetUmsgPtr(handle_, &umsg_ptr);
  EXPECT_EQ(FPGA_OK, res);
  EXPECT_TRUE(umsg_ptr != NULL) << "\t this is umsg:" << res;
  printf("umsg_ptr %p", umsg_ptr);
}

/**
 * @test       Umsg_09
 *
 * @brief      When the handle parameter to xfpga_fpgaTriggerUmsg<br>
 *             is invalid, Then the function returns FPGA_EXCEPTION.<br>
 *
 */
TEST_P(umsg_c_mock_p, test_umsg_09) {
  // register an ioctl handler that will return -1 and set errno to EINVAL
  system_->register_ioctl_handler(FPGA_PORT_GET_INFO, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaTriggerUmsg(handle_, 0));
}

INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({})));

