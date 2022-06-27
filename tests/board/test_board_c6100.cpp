// Copyright(c) 2022, Intel Corporation
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
#include <glob.h>
#include <regex>

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

#include "libboard/board_common/board_common.h"
#include "libboard/board_c6100/board_c6100.h"

using namespace opae::testing;

class board_c6100_c_p : public opae_device_p<> {
 protected:
  fpga_result write_sysfs_file(const char *file,
                               void *buf, size_t count);
  ssize_t eintr_write(int fd, void *buf, size_t count);
  fpga_result delete_sysfs_file(const char *file);
};

ssize_t board_c6100_c_p::eintr_write(int fd, void *buf, size_t count)
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

fpga_result board_c6100_c_p::write_sysfs_file(const char *file,
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

fpga_result board_c6100_c_p::delete_sysfs_file(const char *file) {
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

  status = remove(pglob.gl_pathv[0]);

  opae_globfree(&pglob);
  if (status < 0) {
    printf("delete failed = %d \n", status);
    return FPGA_NOT_FOUND;
  }

  return res;
}

// test DFL sysfs attributes
class board_dfl_c6100_c_p : public board_c6100_c_p {
protected:
	void erase_bom_info(
		const char * const bom_info_nvmem,
		char * const bom_info,
		size_t bom_info_size);
	void test_bom_info(
		const fpga_token token,
		const char * const bom_info_nvmem,
		char * const bom_info_in,
		const char * const bom_info_out);
};

/**
* @test       board_c6100_1
* @brief      Tests: read_bmcfw_version
* @details    Validates bmc firmware version  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_1) {
  char bmcfw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);

  EXPECT_EQ(read_bmcfw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_bmcfw_version(NULL, bmcfw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_c6100_2
* @brief      Tests: read_max10fw_version
* @details    Validates max10 firmware version  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_2) {
  char max10fw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);

  EXPECT_EQ(read_max10fw_version(device_token_, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

  EXPECT_EQ(read_max10fw_version(NULL, max10fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_c6100_3
* @brief      Tests: parse_fw_ver
* @details    Validates parse fw version  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_3) {
  char buf[SYSFS_PATH_MAX];
  char fw_ver[SYSFS_PATH_MAX];

  EXPECT_EQ(parse_fw_ver(buf, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
  EXPECT_EQ(parse_fw_ver(NULL, fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_c6100_4
* @brief      Tests: print_sec_info
* @details    Validates fpga security info  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_4) {
  EXPECT_EQ(print_sec_info(device_token_), FPGA_OK);
}

/**
* @test       board_c6100_5
* @brief      Tests: print_mac_info
* @details    Validates prints mac info  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_5) {
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);
}

/**
* @test       board_c6100_6
* @brief      Tests: print_board_info
* @details    Validates fpga board info  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_6) {
  EXPECT_EQ(print_board_info(device_token_), FPGA_OK);
}

/**
* @test       board_c6100_8
* @brief      Tests: print_phy_info
* @details    Validates fpga phy group info  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_8) {
  EXPECT_EQ(print_phy_info(device_token_), FPGA_NOT_SUPPORTED);
}

/**
* @test       board_c6100_9
* @brief      Tests: read_max10fw_version
*             read_bmcfw_version,
* @details    Validates fpga invalid fpga firmware version  <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_9) {
  char buf[10] = { 0 };
  memset(&buf, 0xf, sizeof(buf));

  ASSERT_EQ(write_sysfs_file((const char *)"dfl_dev*/bmcfw_version", (void*)buf, sizeof(buf)), FPGA_OK);
  char bmcfw_ver[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);

  ASSERT_EQ(write_sysfs_file((const char *)"dfl_dev*/bmc_version", (void*)buf, sizeof(buf)), FPGA_OK);
  char max10fw_ver[SYSFS_PATH_MAX];
  EXPECT_NE(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);
}

/**
* @test       board_c6100_10
* @brief      Tests: print_phy_info
* @details    invalid print phy info input <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_10) {
  EXPECT_NE(print_phy_info(device_token_), FPGA_OK);
  EXPECT_NE(print_phy_info(NULL), FPGA_OK);
}

void board_dfl_c6100_c_p::erase_bom_info(
	const char * const bom_info_nvmem,
	char * const bom_info,
	const size_t bom_info_size)
{
  EXPECT_NE(bom_info_nvmem, (char *)NULL);
  EXPECT_NE(bom_info, (char *)NULL);

  // Fill whole BOM Critical Components with 0xFF:
  memset(bom_info, 0xFF, bom_info_size);
  ASSERT_EQ(write_sysfs_file(bom_info_nvmem, bom_info, bom_info_size), FPGA_OK);
}

void board_dfl_c6100_c_p::test_bom_info(
	const fpga_token token,
	const char * const bom_info_nvmem,
	char * const bom_info_in,
	const char * const bom_info_out)
{
  EXPECT_NE(bom_info_nvmem, (char *)NULL);
  EXPECT_NE(bom_info_in, (char * )NULL);
  EXPECT_NE(bom_info_out, (char *)NULL);

  size_t bom_info_in_length = strlen(bom_info_in);
  if (bom_info_in_length > 0) {
    ASSERT_EQ(write_sysfs_file(bom_info_nvmem, bom_info_in, bom_info_in_length), FPGA_OK);
  }

  testing::internal::CaptureStdout();
  EXPECT_EQ(print_board_info(token), FPGA_OK);
  std::string stdout = testing::internal::GetCapturedStdout();
  std::regex reg(bom_info_out, std::regex::extended);
  bool match_found = std::regex_match(stdout.c_str(), reg);
  EXPECT_EQ(match_found, true);
}

/**
* @test       board_c6100_12
* @brief      Tests: print_mac_info
* @details    prints mac address <br>
*/
TEST_P(board_dfl_c6100_c_p, board_c6100_12) {
  char mac_buf[18] = { 0 };
  strncpy(mac_buf, "ff:ff:ff:ff:ff:ff", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  char mac_count[2] = { 0 };
  strncpy(mac_count, "8", 2);
  write_sysfs_file((char*)"dfl_dev*/mac_count",
                   (void*)mac_count, 2);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_EXCEPTION);


  strncpy(mac_buf, "78:56:34:12:AB:90", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);


  strncpy(mac_buf, "78:56:34:12:AB:FE", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "78:56:34:12:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "78:56:34:FE:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);


  strncpy(mac_buf, "78:56:34:FF:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "78:56:FF:FF:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "78:FF:FF:FF:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "FE:FF:FF:FF:FF:FF", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);

  strncpy(mac_buf, "00:00:00:00:00:ff", 18);
  write_sysfs_file((char*)"dfl_dev*/mac_address",
                   (void*)mac_buf, 18);
  EXPECT_EQ(print_mac_info(device_token_), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_dfl_c6100_c_p);
INSTANTIATE_TEST_SUITE_P(board_dfl_c6100_c, board_dfl_c6100_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-c6100"
                                                                           })));

// test invalid sysfs attributes
class board_c6100_invalid_c_p : public board_c6100_c_p {};

/**
* @test       board_c6100_11
* @brief      Tests: read_max10fw_version
*             read_bmcfw_version
*             read_mac_info
*             print_board_info
*             print_phy_info
*             print_mac_info
* @details    Validates function with invalid sysfs <br>
*/
TEST_P(board_c6100_invalid_c_p, board_c6100_11) {
  char bmcfw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_bmcfw_version(device_token_, bmcfw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  char max10fw_ver[SYSFS_PATH_MAX];
  EXPECT_EQ(read_max10fw_version(device_token_, max10fw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

  EXPECT_EQ(print_board_info(device_token_), FPGA_NOT_FOUND);

  EXPECT_EQ(print_mac_info(device_token_), FPGA_NOT_FOUND);
  EXPECT_EQ(print_sec_info(device_token_), FPGA_NOT_FOUND);

  EXPECT_NE(print_phy_info(device_token_), FPGA_EXCEPTION);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_c6100_invalid_c_p);
INSTANTIATE_TEST_SUITE_P(board_c6100_invalid_c, board_c6100_invalid_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
