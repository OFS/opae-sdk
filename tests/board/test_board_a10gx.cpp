// Copyright(c) 2019-2020, Intel Corporation
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
#include <fcntl.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
}

#include <linux/ioctl.h>
#include <opae/fpga.h>
#include <array>
#include <cstdarg>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "intel-fpga.h"
#include "libboard/board_a10gx/board_a10gx.h"
#include "opae_int.h"
#include "mock/test_system.h"

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

class board_a10gx_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  board_a10gx_c_p() : tokens_{{nullptr, nullptr}} {}

  fpga_result write_sysfs_file(const char *file, void *buf, size_t count);
  ssize_t eintr_write(int fd, void *buf, size_t count);

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
                            &num_matches_),
              FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    dev_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &dev_, 0), FPGA_OK);
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
    fpgaFinalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle dev_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
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

/**
 * @test       board_a10gx_1
 * @brief      Tests: read_bmc_version
 * @details    Validates bmc version  <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_1) {
  int version;

  EXPECT_EQ(read_bmc_version(tokens_[0], &version), FPGA_OK);

  EXPECT_EQ(read_bmc_version(tokens_[0], NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_version(NULL, &version), FPGA_OK);
}

/**
 * @test       board_a10gx_2
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc power down root cause <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_2) {
  char pwr_down_cause[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmc_pwr_down_cause(tokens_[0], pwr_down_cause), FPGA_OK);

  EXPECT_EQ(read_bmc_pwr_down_cause(tokens_[0], NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_pwr_down_cause(NULL, pwr_down_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_3
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc reset root cause <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_3) {
  char reset_cause[SYSFS_PATH_MAX];

  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], NULL), FPGA_INVALID_PARAM);

  EXPECT_NE(read_bmc_reset_cause(NULL, reset_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_4
 * @brief      Tests: print_board_info
 * @details    Validates print board information <br>
 */
TEST_P(board_a10gx_c_p, board_a10gx_4) {
  EXPECT_EQ(print_board_info(tokens_[0]), FPGA_OK);
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

  EXPECT_NE(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);
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
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_EXTRST;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_BOD_IO;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_WDT;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_OCD;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_SOFT;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);

  bmc_a10gx.reset_cause = CHIP_RESET_CAUSE_SPIKE;
  write_sysfs_file((const char *)"avmmi-bmc.3.auto/bmc_info/reset_cause",
                   (void *)&bmc_a10gx, sizeof(bmc_reset_cause));
  EXPECT_EQ(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);
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
  EXPECT_NE(read_bmc_pwr_down_cause(tokens_[0], pwr_down_cause), FPGA_OK);
}
INSTANTIATE_TEST_CASE_P(
    baord_a10gx_c, board_a10gx_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"dcp-a10gx"})));

class board_a10gx_invalid_c_p : public board_a10gx_c_p {};

/**
 * @test       board_a10gx_8
 * @brief      Tests: read_bmc_version
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_8) {
  int version;
  EXPECT_NE(read_bmc_version(tokens_[0], &version), FPGA_OK);
}

/**
 * @test       board_a10gx_9
 * @brief      Tests: read_bmc_pwr_down_cause
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_9) {
  char pwr_down_cause[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmc_pwr_down_cause(tokens_[0], pwr_down_cause), FPGA_OK);
}

/**
 * @test       board_a10gx_9
 * @brief      Tests: read_bmc_reset_cause
 * @details    Validates bmc power down root cause with invalid completion code
 * <br>
 */
TEST_P(board_a10gx_invalid_c_p, board_a10gx_10) {
  char reset_cause[SYSFS_PATH_MAX];
  EXPECT_NE(read_bmc_reset_cause(tokens_[0], reset_cause), FPGA_OK);
}
INSTANTIATE_TEST_CASE_P(
    board_a10gx_invalid_c, board_a10gx_invalid_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));
