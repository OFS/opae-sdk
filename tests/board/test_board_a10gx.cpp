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

#include "libboard/board_a10gx/board_a10gx.h"
#define NO_OPAE_C
#include "mock/opae_fixtures.h"

#define SYSFS_FME_PATH "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0"

#define SDR_HEADER_LEN 3
#define SDR_MSG_LEN 4

typedef struct _bmc_powerdown_cause {
  uint8_t _header[SDR_HEADER_LEN];
  uint8_t completion_code;
  uint8_t iana[SDR_HEADER_LEN];
  uint8_t count;
  uint8_t message[SDR_MSG_LEN];
} bmc_powerdown_cause;

typedef struct _bmc_reset_cause {
  uint8_t _header[SDR_HEADER_LEN];
  uint8_t completion_code;
  uint8_t iana[SDR_HEADER_LEN];
  uint8_t reset_cause;
} bmc_reset_cause;

typedef enum {
  CHIP_RESET_CAUSE_POR = 0x01,
  CHIP_RESET_CAUSE_EXTRST = 0x02,
  CHIP_RESET_CAUSE_BOD_IO = 0x04,
  CHIP_RESET_CAUSE_WDT = 0x08,
  CHIP_RESET_CAUSE_OCD = 0x10,
  CHIP_RESET_CAUSE_SOFT = 0x20,
  CHIP_RESET_CAUSE_SPIKE = 0x40,
} bmc_ResetCauses;

typedef struct _bmc_device_id {
  uint8_t _header[SDR_HEADER_LEN];
  uint8_t completion_code;
  uint8_t device_id;
  union {
    struct {
      uint8_t device_revision : 3;
      uint8_t _unused : 3;
      uint8_t provides_sdrs : 2;
    } bits;
    uint8_t _value;
  } device_revision;
  union {
    struct {
      uint8_t device_available : 7;
      uint8_t major_fw_revision : 1;
    } bits;
    uint8_t _value;
  } firmware_revision_1;
  uint8_t firmware_revision_2;
  uint8_t ipmi_version;
  union {
    struct {
      uint8_t sensor_device : 1;
      uint8_t sdr_repository_device : 1;
      uint8_t sel_device : 1;
      uint8_t fru_inventory_device : 1;
      uint8_t ipmb_event_receiver : 1;
      uint8_t ipmb_event_generator : 1;
      uint8_t bridge : 1;
      uint8_t chassis_device : 1;
    } bits;
    uint8_t _value;
  } additional_device_support;
  uint8_t manufacturer_id_0_7;
  uint8_t manufacturer_id_8_15;
  uint8_t manufacturer_id_16_23;
  uint8_t product_id_0_7;
  uint8_t product_id_8_15;
  uint8_t aux_fw_rev_0_7;
  uint8_t aux_fw_rev_8_15;
  uint8_t aux_fw_rev_16_23;
  uint8_t aux_fw_rev_24_31;
} bmc_device_id;

using namespace opae::testing;

class board_a10gx_c_p : public opae_device_p<> {
 protected:
  fpga_result write_sysfs_file(const char *file, void *buf, size_t count);
  ssize_t eintr_write(int fd, void *buf, size_t count);
};

ssize_t board_a10gx_c_p::eintr_write(int fd, void *buf, size_t count) {
  ssize_t bytes_written = 0, total_written = 0;
  char *ptr = (char *)buf;

  if (!buf) {
    return -1;
  }

  while (total_written < (ssize_t)count) {
    bytes_written = write(fd, ptr + total_written, count - total_written);
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

fpga_result board_a10gx_c_p::write_sysfs_file(const char *file, void *buf,
                                              size_t count) {
  fpga_result res = FPGA_OK;
  char sysfspath[SYSFS_PATH_MAX];
  int fd = 0;

  snprintf(sysfspath, sizeof(sysfspath), "%s/%s", SYSFS_FME_PATH, file);
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

/**
 * @test       board_a10gx_1
 * @brief      Tests: read_bmc_version
 * @details    Validates bmc version  <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_1) {
  int version;

  EXPECT_EQ(read_bmc_version(device_token_, &version), FPGA_OK);

  EXPECT_EQ(read_bmc_version(device_token_, NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_version(NULL, &version), FPGA_OK);
}

/**
 * @test       board_a10gx_2
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc power down root cause <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_2) {
  char pwr_down_cause[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmc_pwr_down_cause(device_token_, pwr_down_cause), FPGA_OK);

  EXPECT_EQ(read_bmc_pwr_down_cause(device_token_, NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_pwr_down_cause(NULL, pwr_down_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_3
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc reset root cause <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_3) {
  char reset_cause[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  EXPECT_EQ(read_bmc_reset_cause(device_token_, NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_reset_cause(NULL, reset_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_4
 * @brief      Tests: print_board_info
 * @details    Validates print board information <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_4) {
  EXPECT_EQ(print_board_info(device_token_), FPGA_OK);
  EXPECT_NE(print_board_info(NULL), FPGA_OK);
}

/**
 * @test       board_a10gx_5
 * @brief      Tests: read_bmc_reset_cause
 * @details    Validates bmc reset root cause with invalid completion code <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_5) {
  bmc_reset_cause bmc_a10gx;
  bmc_a10gx.completion_code = 1;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  char reset_cause[SYSFS_PATH_MAX];

  EXPECT_NE(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_6
 * @brief      Tests: read_bmc_reset_cause
 * @details    Validates bmc reset root cause with different root cause<br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_6) {
  bmc_reset_cause bmc_a10gx;
  bmc_a10gx.completion_code = 0;
  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_POR;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));

  char reset_cause[SYSFS_PATH_MAX];
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_EXTRST;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_BOD_IO;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_WDT;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_OCD;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_SOFT;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_SPIKE;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_7
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_7) {
  bmc_powerdown_cause bmc_pd;
  bmc_pd.completion_code = 1;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/power_down_cause",
                   (void *)&bmc_pd, sizeof(bmc_powerdown_cause));

  char pwr_down_cause[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmc_pwr_down_cause(device_token_, pwr_down_cause), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_a10gx_c_p);
INSTANTIATE_TEST_SUITE_P(
    board_a10gx_c, board_a10gx_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"dcp-rc"})));

class board_a10gx_invalid_c_p : public board_a10gx_c_p {};

/**
 * @test       board_a10gx_8
 * @brief      Tests: read_bmc_version
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_8) {
  int version;
  EXPECT_NE(read_bmc_version(device_token_, &version), FPGA_OK);
}

/**
 * @test       board_a10gx_9
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_9) {
  char pwr_down_cause[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmc_pwr_down_cause(device_token_, pwr_down_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_9
 * @brief      Tests: read_bmc_reset_cause
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_10) {
  char reset_cause[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmc_reset_cause(device_token_, reset_cause), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_a10gx_invalid_c_p);
INSTANTIATE_TEST_SUITE_P(
    board_a10gx_invalid_c, board_a10gx_invalid_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));
