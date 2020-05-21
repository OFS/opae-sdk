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

#include <json-c/json.h>
#include <uuid/uuid.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "opae_int.h"

fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len);
fpga_result read_mac_info(fpga_token token, uint32_t afu_channel_num,
	struct ether_addr *mac_addr);
}

#include <opae/fpga.h>
#include "intel-fpga.h"
#include <linux/ioctl.h>
#include <netinet/ether.h>
#include <cstdlib>
#include <string>
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "libboard/board_d5005/board_d5005.h"
#include "libboard/board_common/board_common.h"

using namespace opae::testing;

class board_d5005_c_p : public ::testing::TestWithParam<std::string> {
protected:
	board_d5005_c_p() : tokens_{ {nullptr, nullptr} } {}

	fpga_result write_sysfs_file(const char *file,
		void *buf, size_t count);
	ssize_t eintr_write(int fd, void *buf, size_t count);
	fpga_result delete_sysfs_file(const char *file);

	virtual void SetUp() override {
		ASSERT_TRUE(test_platform::exists(GetParam()));
		std::cout << GetParam();
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
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	printf("pglob.gl_pathv[0]= %s\n", pglob.gl_pathv[0]);
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

fpga_result board_d5005_c_p::delete_sysfs_file(const char *file) {
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
	std::string syspath = system_->get_sysfs_path(pglob.gl_pathv[0]);
	status = remove(syspath.c_str());

	globfree(&pglob);
	if (status < 0) {
		printf("delete failed = %d \n", status);
		return FPGA_NOT_FOUND;
	}

	return res;
}

// test DFL sysfs attributes
class board_dfl_d5005_c_p : public board_d5005_c_p { };

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

	EXPECT_EQ(read_bmcfw_version(tokens_[0], bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);
	EXPECT_EQ(read_bmcfw_version(tokens_[0], NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
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

	EXPECT_EQ(read_max10fw_version(tokens_[0], max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);
	EXPECT_EQ(read_max10fw_version(tokens_[0], NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
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

	EXPECT_EQ(print_sec_info(tokens_[0]), FPGA_OK);
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

	EXPECT_EQ(read_sysfs(tokens_[0], NULL, sysfs_name,len), FPGA_INVALID_PARAM);
	EXPECT_EQ(read_sysfs(tokens_[0], sysfs_path, NULL, len), FPGA_INVALID_PARAM);
	EXPECT_EQ(read_sysfs(tokens_[0], NULL, NULL, len), FPGA_INVALID_PARAM);
	EXPECT_EQ(read_sysfs(tokens_[0], (char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count",
		sysfs_name, 0), FPGA_EXCEPTION);
}

/**
* @test       board_d5005_6
* @brief      Tests: read_sysfs
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_6) {

	char name[SYSFS_PATH_MAX] = { 0 };

	EXPECT_EQ(read_sysfs(tokens_[0], (char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count",
		name, SYSFS_PATH_MAX), FPGA_OK);
	EXPECT_EQ(read_sysfs(tokens_[0], (char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count1",
		name, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

	EXPECT_EQ(read_sysfs(tokens_[0], (char*)"dfl-fme*", NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
	EXPECT_EQ(read_sysfs(tokens_[0], NULL, name, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_d5005_7
* @brief      Tests: print_sec_info
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_7) {

	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/user_flash_count");
	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/bmc_canceled_csks");
	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/bmc_root_hash");
	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/sr_canceled_csks");
	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/sr_root_hash");
	EXPECT_NE(print_sec_info(tokens_[0]), FPGA_OK);
}

/**
* @test       board_d5005_8
* @brief      Tests: print_sec_info
* @details    Validates function with invalid sysfs  <br>
*/
TEST_P(board_dfl_d5005_c_p, board_d5005_8) {

	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/pr_canceled_csks");
	delete_sysfs_file((char*)"dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/pr_root_hash");
	EXPECT_NE(print_sec_info(tokens_[0]), FPGA_OK);
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

	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_OK);
	EXPECT_EQ(print_mac_info(NULL), FPGA_INVALID_PARAM);

	EXPECT_EQ(read_mac_info(tokens_[0],0,NULL), FPGA_INVALID_PARAM);

	struct ether_addr mac_addr;
	EXPECT_EQ(read_mac_info(tokens_[0], 0, &mac_addr), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(baord_d5005_c, board_dfl_d5005_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "dfl-d5005" })));

// test invalid sysfs attributes
class board_d5005_invalid_c_p : public board_d5005_c_p { };

/**
* @test       invalid_board_d5005_1
* @brief      Tests: read_max10fw_version
*             read_bmcfw_version print_sec_info
* @details    Validates function with invalid sysfs <br>
*/
TEST_P(board_d5005_invalid_c_p, invalid_board_d5005_1) {

	char bmcfw_ver[SYSFS_PATH_MAX];
	EXPECT_EQ(read_bmcfw_version(tokens_[0], bmcfw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

	char max10fw_ver[SYSFS_PATH_MAX];
	EXPECT_EQ(read_max10fw_version(tokens_[0], max10fw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

	EXPECT_EQ(print_sec_info(tokens_[0]), FPGA_NOT_FOUND);
	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_NOT_FOUND);

	struct ether_addr mac_addr;
	EXPECT_EQ(read_mac_info(tokens_[0], 0, &mac_addr), FPGA_NOT_FOUND);

}
INSTANTIATE_TEST_CASE_P(board_d5005_invalid_c, board_d5005_invalid_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
