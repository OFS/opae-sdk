// Copyright(c) 2021-2022, Intel Corporation
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

extern "C" {
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
fpga_result intel_fpga_version(int fd);
fpga_result intel_fme_port_pr(int fd, uint32_t flags, uint32_t port_id,
                          uint32_t sz, uint64_t addr, uint64_t *status);
}

#include "mock/opae_fixtures.h"

using namespace opae::testing;

class sdl_c_p : public opae_p<> {};

/**
 * @test       sdl_c_p
 * @brief      test_bmc_Get_Threshold_for_invalid_fd
 * @details    When the _bmcGetThreshold() is called with invalid fd,
 *             this method returns FPGA_INVALID_PARAM.
 */
TEST_P(sdl_c_p, test_bmc_Get_Threshold_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_bmc_Set_Threshold_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_get_port_info_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_get_port_region_info_for_invalid_fd) {
  opae_port_region_info rinfo;
  int fd = -1;
  EXPECT_EQ(opae_get_port_region_info(fd, 0, &rinfo), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_get_port_region_info_for_invalid_fd
 * @details    When the opae_get_port_region_info() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P(sdl_c_p, test_opae_get_fme_info_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_dfl_port_get_err_irq_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_dfl_port_get_user_irq_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_dfl_fme_get_err_irq_for_invalid_fd) {
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
TEST_P(sdl_c_p, test_opae_dfl_fme_set_err_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(accel_);
  EXPECT_EQ(opae_dfl_fme_set_err_irq(fd, 0, 1, &event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_set_user_irq_for_invalid_fd
 * @details    When the opae_dfl_port_set_user_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_opae_dfl_port_set_user_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(accel_);
  EXPECT_EQ(opae_dfl_port_set_user_irq(fd, 0, 1, &event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_dfl_port_err_user_irq_for_invalid_fd
 * @details    When the opae_dfl_port_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_opae_dfl_port_err_user_irq_for_invalid_fd) {
  int fd = -1;
  int event_handle = FILE_DESCRIPTOR(accel_);
  EXPECT_EQ(opae_dfl_port_set_err_irq(fd, 0, 1, &event_handle), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_set_err_irq_for_invalid_fd
 * @details    When the opae_port_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P(sdl_c_p, test_opae_port_set_err_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_port_set_err_irq(fd, 0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_set_user_irq_for_invalid_fd
 * @details    When the opae_port_set_user_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P(sdl_c_p, test_opae_port_set_user_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_port_set_user_irq(fd, 0, 0, 1, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_fme_set_err_irq_for_invalid_fd
 * @details    When the opae_fme_set_err_irq() is called with invalid fd,
 *             this method returns FPGA_NOT_SUPPORTED.
 */
TEST_P(sdl_c_p, test_opae_fme_set_err_irq_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(opae_fme_set_err_irq(fd, 0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_map_for_invalid_fd
 * @details    When the opae_port_map() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_opae_port_map_for_invalid_fd) {
  uint64_t buf_len;
  uint64_t wsid;
  uint64_t* buf_addr = nullptr;
  uint64_t io_addr = 0;
  int fd = -1;

  // Allocate a buffer
  buf_len = 1024;
  EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(accel_, buf_len, (void**) &buf_addr, &wsid, 0));
  EXPECT_EQ(opae_port_map(fd, buf_addr, buf_len, 0, &io_addr), FPGA_EXCEPTION);

  // Release buffer
  EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(accel_, wsid));
}

/**
 * @test       sdl_c_p
 * @brief      test_opae_port_unmap_for_invalid_fd
 * @details    When the opae_port_unmap() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_opae_port_unmap_for_invalid_fd) {
  uint64_t io_addr = 0;
  int fd = -1;
  EXPECT_EQ(opae_port_unmap(fd, io_addr), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_intel_fpga_version_for_invalid_fd
 * @details    When the intel_fpga_version() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_intel_fpga_version_for_invalid_fd) {
  int fd = -1;
  EXPECT_EQ(intel_fpga_version(fd), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      test_intel_fme_port_pr_for_invalid_fd
 * @details    When the intel_fme_port_pr() is called with invalid fd,
 *             this method returns FPGA_EXCEPTION.
 */
TEST_P(sdl_c_p, test_intel_fme_port_pr_for_invalid_fd) {
  int fd = -1;
  uint64_t status = 0;
  EXPECT_EQ(intel_fme_port_pr(fd, 0, 0, 0, 0, &status), FPGA_EXCEPTION);
}

/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaHandleGetObject_for_null_byte
 * @details    When fpgaHandleGetObject is called with a name that has a null
 *             byte, the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaHandleGetObject_for_null_byte) {
  fpga_object obj = nullptr;
  const char *bad_name = "err\0rs";

  EXPECT_EQ(fpgaHandleGetObject(accel_, bad_name, &obj, 0), FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaHandleGetObject_for_too_short
 * @details    When fpgaHandleGetObject is called with too short name,
 *             the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaHandleGetObject_for_too_short) {
  fpga_object obj = nullptr;
  const char *too_short_name = "a";

  EXPECT_EQ(fpgaHandleGetObject(accel_, too_short_name, &obj, 0), FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaHandleGetObject_for_too_long_name
 * @details    When fpgaHandleGetObject is called with too long name,
 *             the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaHandleGetObject_for_too_long_name) {
  fpga_object obj = nullptr;
  const char *too_long_name = "This/is/invalid/path/with/maximim/255/\
                               characterssssssssssssssssssssssssssssss\
                               ssssssssssssssssssssssa/lengthhhhhhhhhhh\
                               hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\
                               /so/opaeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\
                               eeeeeeeeeeeeeeeeeee/api/should/return/with/\
                               errorrrrrrrrrrrrrrrrr/for/SDL testing/";

  EXPECT_EQ(fpgaHandleGetObject(accel_, too_long_name, &obj, 0), FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}
/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaTokenGetObject_for_null_byte
 * @details    When fpgaTokenGetObject is called with a name that has a null
 *             byte, the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaTokenGetObject_for_null_byte) {
  fpga_object obj = nullptr;
  const char *bad_name = "err\0rs";

  EXPECT_EQ(fpgaTokenGetObject(accel_token_, bad_name, &obj, 0),
                                FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaTokenGetObject_for_too_short
 * @details    When fpgaTokenGetObject is called with too short path name,
 *             the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaTokenGetObject_for_too_short) {
  fpga_object obj = nullptr;
  const char *too_short_path = "a";

  EXPECT_EQ(fpgaTokenGetObject(accel_token_, too_short_path, &obj, 0),
                                FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaTokenGetObject_for_long_path
 * @details    When fpgaTokenGetObject is called with too long path name,
 *             the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(sdl_c_p, test_fpgaTokenGetObject_for_long_path) {
  fpga_object obj = nullptr;
  const char *too_long_path = "This/is/invalid/path/with/maximim/255/\
                               characterssssssssssssssssssssssssssssss\
                               ssssssssssssssssssssssa/lengthhhhhhhhhhh\
                               hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\
                               /so/opaeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\
                               eeeeeeeeeeeeeeeeeee/api/should/return/with/\
                               errorrrrrrrrrrrrrrrrr/for/SDL testing/";

  EXPECT_EQ(fpgaTokenGetObject(accel_token_, too_long_path, &obj, 0),
                                FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}



/**
 * @test       sdl_c_p
 * @brief      Test: test_fpgaClose_for_null_object
 * @details
 *             When fpgaClose is called with NULL object<br>
 *             the function returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(sdl_c_p, test_fpgaClose_for_null_object) {
  EXPECT_EQ(fpgaClose(nullptr), FPGA_INVALID_PARAM);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(sdl_c_p);
INSTANTIATE_TEST_SUITE_P(sdl_c, sdl_c_p, 
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000"
                                                                      })));
