// Copyright(c) 2018-2021, Intel Corporation
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
//#include <uuid/uuid.h>
#include "config.h"
#include "bmc_ioctl.h"
#include "opae_int.h"
#include "types_int.h"
#include "opae_drv.h"
fpga_result _bmcGetThreshold(int fd, uint32_t sensor,
        bmc_get_thresh_response *resp);
fpga_result _bmcSetThreshold(int fd, uint32_t sensor,
        bmc_set_thresh_request *req);
fpga_result opae_get_port_info(int fd, opae_port_info *info);
fpga_result opae_get_port_region_info(int fd, uint32_t index,
                                      opae_port_region_info *info);
fpga_result opae_get_fme_info(int fd, opae_fme_info *info);
fpga_result opae_dfl_port_get_err_irq(int fd, uint32_t *num_irqs);
fpga_result opae_dfl_port_get_user_irq(int fd, uint32_t *num_irqs);
fpga_result opae_dfl_fme_get_err_irq(int fd, uint32_t *num_irqs);
fpga_result opae_dfl_fme_set_err_irq(int fd, uint32_t start,
        uint32_t count, int32_t *eventfd);
fpga_result opae_dfl_port_set_user_irq(int fd, uint32_t start,
        uint32_t count, int32_t *eventfd);
fpga_result opae_dfl_port_set_err_irq(int fd, uint32_t start,
        uint32_t count, int32_t *eventfd);
fpga_result opae_port_set_err_irq(int fd, uint32_t flags, int32_t eventfd);
fpga_result opae_port_set_user_irq(int fd, uint32_t flags, uint32_t start,
                                   uint32_t count, int32_t *eventfd);
fpga_result opae_fme_set_err_irq(int fd, uint32_t flags, int32_t eventfd);
fpga_result opae_port_map(int fd, void *addr, uint64_t len, uint32_t flags,
                          uint64_t *io_addr);
fpga_result opae_port_unmap(int fd, uint64_t io_addr);
}

#include <opae/fpga.h>
#include <linux/ioctl.h>
#include <string>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class sdl_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  sdl_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    handle_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) {
        EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
        handle_ = nullptr;
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle handle_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       sdl_c_p
 * @brief      test_bmc_Get_Threshold_for_invalid_fd
 * @details    When the _bmcGetThreshold() is called with invalid fd,
 *             this method returns FPGA_INVALID_PARAM.
 */
TEST_P (sdl_c_p, test_bmc_Get_Threshold_for_invalid_fd) {
  // Get  threshold with invalid parameter.
  bmc_get_thresh_response thres;
  int fd = -1;
  EXPECT_EQ(_bmcGetThreshold(fd, 1, &thres), FPGA_INVALID_PARAM);
}
/**
 * @test       sdl_c_p
 * @brief      test_bmc_Set_Threshold_for_invalid_fd
 * @details    When the _bmcSetThreshold() is called with invalid fd,
 *             this method returns FPGA_INVALID_PARAM.
 */
TEST_P (sdl_c_p, test_bmc_Set_Threshold_for_invalid_fd) {
  // Set  threshold with invalid parameter.
  bmc_set_thresh_request req;
  int fd = -1;
  EXPECT_EQ(_bmcSetThreshold(fd, 1, &req), FPGA_INVALID_PARAM);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_get_port_info_for_invalid_fd
 * @details    When the opae_get_fme_info() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_get_port_info_for_invalid_fd) {
  opae_port_info info = { 0, 0, 0, 0, 0 };
  int fd = -1;

  EXPECT_EQ(opae_get_port_info(fd,&info), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_get_port_region_info_for_invalid_fd
 * @details    When the opae_get_port_region_info() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_get_port_region_info_for_invalid_fd) {
  opae_port_region_info rinfo;
  int fd = -1;
  EXPECT_EQ(opae_get_port_region_info(fd,0,&rinfo), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_get_port_region_info_for_invalid_fd
 * @details    When the opae_get_port_region_info() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P (sdl_c_p, test_opae_get_fme_info_for_invalid_fd) {
  opae_fme_info info;
  int fd = -1;
  EXPECT_EQ(opae_get_fme_info(fd,&info), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_get_err_irq_for_invalid_fd
 * @details    When the opae_dfl_port_get_err_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_port_get_err_irq_for_invalid_fd) {
  uint32_t num_irqs = 0;
  int fd = -1;
  EXPECT_EQ(opae_dfl_port_get_err_irq(fd,&num_irqs), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_get_user_irq_for_invalid_fd
 * @details    When the opae_dfl_port_get_user_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_port_get_user_irq_for_invalid_fd) {
  uint32_t num_irqs = 0;
  int fd = -1;
  EXPECT_EQ(opae_dfl_port_get_user_irq(fd,&num_irqs), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_fme_get_err_irq_for_invalid_fd
 * @details    When the opae_dfl_fme_get_err_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_fme_get_err_irq_for_invalid_fd) {
  uint32_t num_irqs = 0;
  int fd = -1;
  EXPECT_EQ(opae_dfl_fme_get_err_irq(fd,&num_irqs), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_fme_set_err_irq_for_invalid_fd
 * @details    When the opae_dfl_fme_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_fme_set_err_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(handle_);
  EXPECT_EQ(opae_dfl_fme_set_err_irq(fd,0,1,&event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_set_user_irq_for_invalid_fd
 * @details    When the opae_dfl_port_set_user_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_port_set_user_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(handle_);
  EXPECT_EQ(opae_dfl_port_set_user_irq(fd,0,1,&event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_err_user_irq_for_invalid_fd
 * @details    When the opae_dfl_port_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_dfl_port_err_user_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(handle_);
  EXPECT_EQ(opae_dfl_port_set_err_irq(fd,0,1,&event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_set_err_irq_for_invalid_fd
 * @details    When the opae_port_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P (sdl_c_p, test_opae_port_set_err_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_port_set_err_irq(fd,0,0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_set_user_irq_for_invalid_fd
 * @details    When the opae_port_set_user_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P (sdl_c_p, test_opae_port_set_user_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_port_set_user_irq(fd,0,0,1,0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_fme_set_err_irq_for_invalid_fd
 * @details    When the opae_fme_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P (sdl_c_p, test_opae_fme_set_err_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_fme_set_err_irq(fd,0,0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_map_for_invalid_fd
 * @details    When the opae_port_map() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_port_map_for_invalid_fd) {
  uint64_t buf_len;
  uint64_t wsid;
  uint64_t* buf_addr = nullptr;
  uint64_t io_addr = 0;
  int fd = -1;

  // Allocate a buffer
  buf_len = 1024;
  EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, 0));
  EXPECT_EQ(opae_port_map(fd,buf_addr,buf_len,0,&io_addr), FPGA_EXCEPTION);

  // Release buffer
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(handle_, wsid));
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_unmap_for_invalid_fd
 * @details    When the opae_port_unmap() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P (sdl_c_p, test_opae_port_unmap_for_invalid_fd) {
  uint64_t io_addr = 0;
  int fd = -1;
  EXPECT_EQ(opae_port_unmap(fd,io_addr), FPGA_EXCEPTION);
}

INSTANTIATE_TEST_CASE_P(sdl_c, sdl_c_p, 
                        ::testing::ValuesIn(test_platform::platforms({})));
