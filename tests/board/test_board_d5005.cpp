// Copyright(c) 2019-2022, Intel Corporation
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/ether.h>

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

#include "libboard/board_d5005/board_d5005.h"
#include "libboard/board_common/board_common.h"

extern "C" {
fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len);
fpga_result read_mac_info(fpga_token token, uint32_t afu_channel_num,
                          struct ether_addr *mac_addr);
}

using namespace opae::testing;

class board_d5005_c_p : public opae_device_p<> {
 protected:
  fpga_result write_sysfs_file(const char *file,
                               void *buf, size_t count);
  ssize_t eintr_write(int fd, void *buf, size_t count);
  fpga_result delete_sysfs_file(const char *file);
};

ssize_t board_d5005_c_p::eintr_write(int fd, void *buf, size_t count)
{
  ssize_t bytes_written = 0, total_written = 0;
  char *ptr = (char*)buf;

  if (!buf) {
    return -1;
  }

  while (total_written < (ssize_t)count) {
    bytes_written =
      write(fd, ptr + total_written, count - total_written);
    if (bytes_written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return bytes_written;
    }
    total_written += bytes_written;
  }

  return total_written;
}

fpga_result board_d5005_c_p::write_sysfs_file(const char *file,
                                              void *buf, size_t count) {
  fpga_result res = FPGA_OK;
  char sysfspath[SYSFS_PATH_MAX];
  int fd = 0;

  snprintf(sysfspath, sizeof(sysfspath),
           "%s/%s", "/sys/class/fpga_region/region*/dfl-fme*", file);

  glob_t pglob;
  int gres = opae_glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
  if ((gres) || (1 != pglob.gl_pathc)) {
    opae_globfree(&pglob);
    return FPGA_NOT_FOUND;
  }

  printf("pglob.gl_pathv[0]= %s\n", pglob.gl_pathv[0]);
  fd = opae_open(pglob.gl_pathv[0], O_WRONLY);
  opae_globfree(&pglob);
  if (fd < 0) {
    printf("open failed \n");
    return FPGA_NOT_FOUND;
  }

  ssize_t total_written = eintr_write(fd, buf, count);
  if (total_written == 0) {
    opae_close(fd);
    printf("total_written failed \n");
    return FPGA_INVALID_PARAM;
  }

  opae_close(fd);
  return res;
}

fpga_result board_d5005_c_p::delete_sysfs_file(const char *file) {
  fpga_result res = FPGA_OK;
  char sysfspath[SYSFS_PATH_MAX];
  int status = 0;

  snprintf(sysfspath, sizeof(sysfspath),
           "%s/%s", "/sys/class/fpga_region/region*/dfl-fme*", file);

  glob_t pglob;
  int gres = opae_glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
  if ((gres) || (1 != pglob.gl_pathc)) {
    opae_globfree(&pglob);
    return FPGA_NOT_FOUND;
  }
  std::string syspath = system_->get_sysfs_path(pglob.gl_pathv[0]);
  status = remove(syspath.c_str());

  opae_globfree(&pglob);
  if (status < 0) {
    printf("delete failed = %d \n", status);
    return FPGA_NOT_FOUND;
  }

  return res;
}

// test DFL sysfs attributes
class board_dfl_d5005_c_p : public board_d5005_c_p {};

/**
* @test       board_d5005_1
* @brief      Tests: read_bmcfw_version
* @details    When given a valid input to function <br>
*             return FPGA_OK.<br>
*             When given a invalid input to function.<br>
*             return FPGA_INVALID_PARAM.<br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_1) {
  char bmcfw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);
  EXPECT_EQ(read_bmcfw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_bmcfw_version(NULL, bmcfw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_d5005_2
* @brief      Tests: read_max10fw_version
* @details    When given a valid input to function <br>
*             return FPGA_OK.<br>
*             When given a invalid input to function.<br>
*             return FPGA_INVALID_PARAM.<br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_2) {
  char max10fw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);
  EXPECT_EQ(read_max10fw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_max10fw_version(NULL, max10fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_d5005_3
* @brief      Tests: parse_fw_ver
* @details    Validates parse fw version  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_3) {
  char buf[FPGA_VAR_BUF_LEN] = { 0 };
  char fw_ver[FPGA_VAR_BUF_LEN] = { 0 };

  EXPECT_EQ(parse_fw_ver(NULL, fw_ver, 0), FPGA_INVALID_PARAM);
  EXPECT_EQ(parse_fw_ver(buf, NULL, 0), FPGA_INVALID_PARAM);
}

/**
* @test       board_d5005_4
* @brief      Tests: print_sec_info
* @details    Validates fpga sec info  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_4) {
  EXPECT_EQ(print_sec_info(device_token_), FPGA_OK);
}

/**
* @test       board_d5005_5
* @brief      Tests: read_sysfs
* @details    Validates read sysfs function  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_5) {
  char sysfs_name[SYSFS_PATH_MAX];
  char sysfs_path[SYSFS_PATH_MAX];
  size_t len = 0;

  EXPECT_EQ(read_sysfs(device_token_, NULL, sysfs_name,len), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_sysfs(device_token_, sysfs_path, NULL, len), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_sysfs(device_token_, NULL, NULL, len), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_sysfs(device_token_,
                       (char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count",
                       sysfs_name, 0), FPGA_EXCEPTION);
}

/**
* @test       board_d5005_6
* @brief      Tests: read_sysfs
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_6) {
  char name[SYSFS_PATH_MAX] = { 0 };

  EXPECT_EQ(read_sysfs(device_token_,
                       (char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count",
                       name, SYSFS_PATH_MAX), FPGA_OK);
  EXPECT_EQ(read_sysfs(device_token_,
                       (char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count1",
                       name, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  EXPECT_EQ(read_sysfs(device_token_, (char*)"dfl-fme*", NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_sysfs(device_token_, NULL, name, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_d5005_7
* @brief      Tests: print_sec_info
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_7) {
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count");
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/bmc_canceled_csks");
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/bmc_root_entry_hash");
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/sr_canceled_csks");
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/sr_root_entry_hash");
  EXPECT_NE(print_sec_info(device_token_), FPGA_OK);
}

/**
* @test       board_d5005_8
* @brief      Tests: print_sec_info
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_8) {
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/pr_canceled_csks");
  delete_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/pr_root_entry_hash");
  EXPECT_NE(print_sec_info(device_token_), FPGA_OK);
}

/**
* @test       board_d5005_9
* @brief      Tests: print_mac_info
* @details    When given a valid input to function <br>
*             return FPGA_OK.<br>
*             When given a invalid input to function.<br>
*             return FPGA_INVALID_PARAM.<br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_9) {
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);
  EXPECT_EQ(print_mac_info(NULL), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_mac_info(device_token_,0,NULL), FPGA_INVALID_PARAM);

  struct ether_addr mac_addr;
  EXPECT_EQ(read_mac_info(device_token_, 0, &mac_addr), FPGA_OK);

  char mac_buf[18] = { 0 };
  strncpy(mac_buf, "ff:ff:ff:ff:ff:ff", 18);
  write_sysfs_file((char*)"dfl-fme*/*spi*/spi_master/spi*/spi*.*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(read_mac_info(device_token_, 0, &mac_addr), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_mac_info(device_token_, 100, &mac_addr), FPGA_INVALID_PARAM);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_dfl_d5005_c_p);
INSTANTIATE_TEST_SUITE_P(board_d5005_c, board_dfl_d5005_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "dfl-d5005" })));

// test invalid sysfs attributes
class board_d5005_invalid_c_p : public board_d5005_c_p {};

/**
* @test       invalid_board_d5005_1
* @brief      Tests: read_max10fw_version
*             read_bmcfw_version print_sec_info
* @details    Validates function with invalid sysfs <br>
*/
TEST_P(board_d5005_invalid_c_p, invalid_board_d5005_1) {
  char bmcfw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  char max10fw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  EXPECT_EQ(print_sec_info(device_token_), FPGA_NOT_FOUND);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_NOT_FOUND);

  struct ether_addr mac_addr;
  EXPECT_EQ(read_mac_info(device_token_, 0, &mac_addr), FPGA_NOT_FOUND);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_d5005_invalid_c_p);
INSTANTIATE_TEST_SUITE_P(board_d5005_invalid_c, board_d5005_invalid_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
