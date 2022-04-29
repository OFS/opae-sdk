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

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

#include "libboard/board_common/board_common.h"
#include "libboard/board_n3000/board_n3000.h"

using namespace opae::testing;

class board_n3000_c_p : public opae_device_p<> {
 protected:
  fpga_result write_sysfs_file(const char *file,
                               void *buf, size_t count);
  ssize_t eintr_write(int fd, void *buf, size_t count);
  fpga_result delete_sysfs_file(const char *file);
};

ssize_t board_n3000_c_p::eintr_write(int fd, void *buf, size_t count)
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

fpga_result board_n3000_c_p::write_sysfs_file(const char *file,
                                              void *buf, size_t count) {
  fpga_result res = FPGA_OK;
  char sysfspath[SYSFS_PATH_MAX];
  int fd = 0;

  snprintf(sysfspath, sizeof(sysfspath),
           "%s/%s", "/sys/class/fpga_region/region*/dfl-fme*", file);

  glob_t pglob;
  int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
  if ((gres) || (1 != pglob.gl_pathc)) {
    globfree(&pglob);
    return FPGA_NOT_FOUND;
  }

  fd = open(pglob.gl_pathv[0], O_WRONLY);
  globfree(&pglob);
  if (fd < 0) {
    printf("open failed \n");
    return FPGA_NOT_FOUND;
  }

  ssize_t total_written = eintr_write(fd, buf, count);
  if (total_written == 0) {
    close(fd);
    printf("total_written failed \n");
    return FPGA_INVALID_PARAM;
  }

  close(fd);
  return res;
}

fpga_result board_n3000_c_p::delete_sysfs_file(const char *file) {
  fpga_result res = FPGA_OK;
  char sysfspath[SYSFS_PATH_MAX];
  int status = 0;

  snprintf(sysfspath, sizeof(sysfspath),
           "%s/%s", "/sys/class/fpga_region/region*/dfl-fme*", file);

  glob_t pglob;
  int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
  if ((gres) || (1 != pglob.gl_pathc)) {
    globfree(&pglob);
    return FPGA_NOT_FOUND;
  }

  status = remove(pglob.gl_pathv[0]);

  globfree(&pglob);
  if (status < 0) {
    printf("delete failed = %d \n", status);
    return FPGA_NOT_FOUND;
  }

  return res;
}

// test DFL sysfs attributes
class board_dfl_n3000_c_p : public board_n3000_c_p {};

/**
* @test       board_n3000_1
* @brief      Tests: read_bmcfw_version
* @details    Validates bmc firmware version  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_1) {
  char bmcfw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);

  EXPECT_EQ(read_bmcfw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_bmcfw_version(NULL, bmcfw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_n3000_2
* @brief      Tests: read_max10fw_version
* @details    Validates max10 firmware version  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_2) {
  char max10fw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);

  EXPECT_EQ(read_max10fw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_max10fw_version(NULL, max10fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_n3000_3
* @brief      Tests: parse_fw_ver
* @details    Validates parse fw version  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_3) {
  char buf[SYSFS_PATH_MAX];
  char fw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(parse_fw_ver(buf, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
  EXPECT_EQ(parse_fw_ver(NULL, fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_n3000_10
* @brief      Tests: print_sec_info
* @details    Validates fpga board info  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_10) {
  EXPECT_EQ(print_sec_info(device_token_), FPGA_OK);
}

/**
* @test       board_n3000_11
* @brief      Tests: print_mac_info
* @details    Validates prints mac info  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_11) {
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);
}

/**
* @test       board_n3000_7
* @brief      Tests: print_board_info
* @details    Validates fpga board info  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_7) {
  EXPECT_EQ(print_pkvl_version(device_token_), FPGA_OK);
}

/**
* @test       board_n3000_8
* @brief      Tests: read_max10fw_version
*             read_bmcfw_version,
* @details    Validates fpga invalid fpga firmware version  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_8) {
  char buf[10] = { 0 };
  write_sysfs_file((const char *)"dfl_dev*/*spi*/spi_master/spi*/spi*/bmcfw_version", (void*)buf, sizeof(buf));

  char bmcfw_ver[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);

  write_sysfs_file((const char *)"dfl_dev*/*spi*/spi_master/spi*/spi*/bmc_version", (void*)buf, sizeof(buf));

  char max10fw_ver[SYSFS_PATH_MAX];
  EXPECT_NE(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);
}

/**
* @test       board_n3000_12
* @brief      Tests: print_eth_interface_info
* @details    Validates fpga eth group info  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_12) {
  EXPECT_NE(print_eth_interface_info(device_token_, "npac"), FPGA_OK);
}

/**
* @test       board_n3000_13
* @brief      Tests: enum_eth_group_feature
* @details    Validates enum fpga eth group feature <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_13) {
  char eth_feature_dev[2][SYSFS_PATH_MAX];

  EXPECT_EQ(enum_eth_group_feature(device_token_,
                                   eth_feature_dev,
                                   2), FPGA_OK);

  EXPECT_EQ(enum_eth_group_feature(NULL,
                                   eth_feature_dev,
                                   2), FPGA_NOT_FOUND);

  EXPECT_EQ(enum_eth_group_feature(device_token_,
                                   eth_feature_dev,
                                   1), FPGA_OK);
}

/**
* @test       board_n3000_14
* @brief      Tests: enum_pkvl_sysfs_path
* @details    Validates enum pkvl sysfs path <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_14) {
  char path[SYSFS_PATH_MAX];

  EXPECT_EQ(enum_pkvl_sysfs_path(device_token_, path), FPGA_OK);
  EXPECT_EQ(enum_pkvl_sysfs_path(device_token_, NULL), FPGA_INVALID_PARAM);
  EXPECT_EQ(enum_pkvl_sysfs_path(NULL, path), FPGA_NOT_FOUND);
}

/**
* @test       board_n3000_15
* @brief      Tests: read_regmap
* @details    Validates regmap <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_15) {
  char path[SYSFS_PATH_MAX];
  uint64_t index = 0;
  uint32_t value = 0;

  EXPECT_EQ(read_regmap(path, index, &value), FPGA_EXCEPTION);

  snprintf(path, sizeof(path),
           "%s", "/sys/kernel/debug/regmap/spi4.0/registers");
  index = 0x300800 + 0x164;
  value = 0;
  EXPECT_EQ(read_regmap(path, index, &value), FPGA_OK);

  EXPECT_EQ(read_regmap(path, 0xabcdabcd, &value), FPGA_NOT_FOUND);

  EXPECT_EQ(read_regmap(path, index, NULL), FPGA_INVALID_PARAM);
  EXPECT_EQ(read_regmap(NULL, index, &value), FPGA_INVALID_PARAM);
}

/**
* @test       board_n3000_16
* @brief      Tests: print_retimer_info
* @details    Validates print retimer info <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_16) {
  EXPECT_EQ(print_retimer_info(device_token_, 40), FPGA_OK);

  EXPECT_EQ(print_retimer_info(NULL, 40), FPGA_NOT_FOUND);

  EXPECT_EQ(print_retimer_info(device_token_, 200), FPGA_OK);
}

/**
* @test       board_n3000_17
* @brief      Tests: print_pkvl_version
* @details    Validates print pkvl info <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_17) {
  EXPECT_EQ(print_pkvl_version(device_token_), FPGA_OK);
  EXPECT_EQ(print_pkvl_version(NULL), FPGA_NOT_FOUND);
}

/**
* @test       board_n3000_18
* @brief      Tests: print_phy_info
* @details    Validates print phy info <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_18) {
  EXPECT_NE(print_phy_info(device_token_), FPGA_OK);
  EXPECT_NE(print_phy_info(NULL), FPGA_OK);
}

/**
* @test       board_n3000_19
* @brief      Tests: get_fpga_sbdf
* @details    Validates fpga sbdf <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_19) {
  uint8_t bus = (uint8_t)-1;
  uint16_t segment = (uint16_t)-1;
  uint8_t device = (uint8_t)-1;
  uint8_t function = (uint8_t)-1;

  EXPECT_EQ(get_fpga_sbdf(device_token_,
                          &segment,
                          &bus,
                          &device,
                          &function), FPGA_OK);

  EXPECT_EQ(get_fpga_sbdf(device_token_,
                          NULL,
                          &bus,
                          &device,
                          &function), FPGA_INVALID_PARAM);

  EXPECT_EQ(get_fpga_sbdf(NULL,
                          &segment,
                          &bus,
                          &device,
                          &function), FPGA_NOT_FOUND);
}

/**
* @test       board_n3000_20
* @brief      Tests: print_eth_interface_info
* @details    Validates fpga eth group info  <br>
*/
TEST_P(board_dfl_n3000_c_p, board_n3000_20) {
  char path[SYSFS_PATH_MAX];

  uint64_t value = 0;
  snprintf(path, sizeof(path),
           "%s", "/sys/class/fpga_region/region0/dfl-fme.0/bitstream_id");
  EXPECT_EQ(sysfs_read_u64(path, &value), FPGA_OK);
  EXPECT_EQ(sysfs_read_u64(NULL, &value), FPGA_INVALID_PARAM);
  EXPECT_EQ(sysfs_read_u64(path, NULL), FPGA_INVALID_PARAM);

  EXPECT_EQ(sysfs_read_u64("/sys/class/fpga_region1/region*/dfl-fme*",
                           &value), FPGA_NOT_FOUND);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_dfl_n3000_c_p);
INSTANTIATE_TEST_SUITE_P(board_dfl_n3000_c, board_dfl_n3000_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "dfl-n3000" })));

// test invalid sysfs attributes
class board_n3000_invalid_c_p : public board_n3000_c_p {};

/**
* @test       board_n3000_9
* @brief      Tests: read_max10fw_version
*             read_max10fw_version,read_pcb_info
*             read_pkvl_info,read_mac_info
*             read_phy_group_info,print_board_info
*             print_phy_info,print_mac_info
* @details    Validates function with invalid sysfs <br>
*/
TEST_P(board_n3000_invalid_c_p, board_n3000_9) {
  char bmcfw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  char max10fw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  EXPECT_EQ(print_board_info(device_token_), FPGA_NOT_FOUND);

  EXPECT_EQ(print_mac_info(device_token_), FPGA_NOT_FOUND);

  EXPECT_EQ(print_sec_info(device_token_), FPGA_NOT_FOUND);
  char eth_feature_dev[2][SYSFS_PATH_MAX];
  EXPECT_EQ(enum_eth_group_feature(device_token_,
                                   eth_feature_dev,
                                   2), FPGA_NOT_FOUND);

  char path[SYSFS_PATH_MAX];
  uint64_t index = 0;
  uint32_t value = 0;

  EXPECT_EQ(enum_pkvl_sysfs_path(device_token_, path), FPGA_NOT_FOUND);
  EXPECT_EQ(read_regmap(path, index, &value), FPGA_EXCEPTION);
  EXPECT_EQ(print_retimer_info(device_token_, 40), FPGA_NOT_FOUND);
  EXPECT_EQ(print_pkvl_version(device_token_), FPGA_NOT_FOUND);
  EXPECT_NE(print_phy_info(device_token_), FPGA_EXCEPTION);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_n3000_invalid_c_p);
INSTANTIATE_TEST_SUITE_P(board_n3000_invalid_c, board_n3000_invalid_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
