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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "opae_int.h"

}

#include <opae/fpga.h>
#include "intel-fpga.h"
#include <linux/ioctl.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
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

class umsg_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  umsg_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    dev_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &dev_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_INFO, umsg_port_info);
    system_->register_ioctl_handler(FPGA_PORT_UMSG_SET_MODE, umsg_set_mode);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (dev_) {
        EXPECT_EQ(fpgaClose(dev_), FPGA_OK);
        dev_ = nullptr;
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle dev_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       get_num
 * @brief      Test: fpgaGetNumUmsg
 * @details    When fpgaGetNumUmsg retrieves the number of UMsgs,<br>
 *             the number of Umsgs is greater than 0,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(umsg_c_p, get_num) {
  uint64_t num = 0;
  EXPECT_EQ(fpgaGetNumUmsg(dev_, &num), FPGA_OK);
  EXPECT_GT(num, 0);
}

/**
 * @test       set_attr
 * @brief      Test: fpgaSetUmsgAttributes
 * @details    When fpgaSetUmsgAttributes is called with valid values,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(umsg_c_p, set_attr) {
  uint64_t enable  = 0xff;
  uint64_t disable = 0;
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, enable), FPGA_OK);
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, disable), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_p, 
                        ::testing::ValuesIn(test_platform::platforms({})));

class umsg_c_mock_p: public umsg_c_p{
  protected:
    umsg_c_mock_p() {};
};
/**
 * @test       trigger
 * @brief      Test: fpgaTriggerUmsg
 * @details    When Umsgs are enabled and<br>
 *             fpgaTriggerUmsg is called with a valid value,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(umsg_c_mock_p, trigger) {
  uint64_t enable  = 0xff;
  uint64_t disable = 0;
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, enable), FPGA_OK);
  EXPECT_EQ(fpgaTriggerUmsg(dev_, 1), FPGA_OK);
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, disable), FPGA_OK);
}

/**
 * @test       get_ptr
 * @brief      Test: fpgaGetUmsgPtr
 * @details    When Umsgs are enabled and<br>
 *             fpgaGetUmsgPtr is called with a valid value,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(umsg_c_mock_p, get_ptr) {
  uint64_t enable  = 0xff;
  uint64_t disable = 0;
  uint64_t *umsg_ptr = nullptr;
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, enable), FPGA_OK);
  EXPECT_EQ(fpgaGetUmsgPtr(dev_, &umsg_ptr), FPGA_OK);
  EXPECT_NE(umsg_ptr, nullptr);
  EXPECT_EQ(fpgaSetUmsgAttributes(dev_, disable), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(umsg_c, umsg_c_mock_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({})));

