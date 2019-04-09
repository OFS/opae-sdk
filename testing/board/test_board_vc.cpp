// Copyright(c) 2019, Intel Corporation
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "opae_int.h"
#include "safe_string/safe_string.h"
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
#include "libboard/board_vc/board_vc.h"


using namespace opae::testing;

class board_vc_c_p : public ::testing::TestWithParam<std::string> {
protected:
	board_vc_c_p() : tokens_{ {nullptr, nullptr} } {}

	fpga_result write_sysfs_file(const char *file,
		void *buf, size_t count);
	ssize_t eintr_write(int fd, void *buf, size_t count);
	fpga_result delete_sysfs_file(const char *file);

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

ssize_t board_vc_c_p::eintr_write(int fd, void *buf, size_t count)
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
fpga_result board_vc_c_p::write_sysfs_file(const char *file,
	void *buf, size_t count) {
	fpga_result res = FPGA_OK;
	char sysfspath[256];
	int fd = 0;

	snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s", "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", file);

	glob_t pglob;
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		printf("-----------FAIL ---gres = %d\n", gres);
		return FPGA_NOT_FOUND;
	}
	fd = open(pglob.gl_pathv[0], O_WRONLY);
	globfree(&pglob);
	if (fd < 0) {
		printf("open faild \n");
		return FPGA_NOT_FOUND;
	}

	ssize_t total_written = eintr_write(fd, buf, count);
	if (total_written == 0) {
		close(fd);
		printf("total_written faild \n");
		return FPGA_INVALID_PARAM;
	}

	close(fd);
	return res;
}

fpga_result board_vc_c_p::delete_sysfs_file(const char *file) {
	fpga_result res = FPGA_OK;
	char sysfspath[256];
	int status = 0;

	snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s", "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", file);

	glob_t pglob;
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	status = remove(pglob.gl_pathv[0]);

	globfree(&pglob);
	if (status < 0) {
		printf("deleter faild = %d \n", status);
		return FPGA_NOT_FOUND;
	}

	return res;
}


/**
 * @test       pr
 * @brief      Test: fpgaReconfigureSlot
 * @details    When fpgaReconfigureSlot is called with invalid params,<br>
 *             then the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(board_vc_c_p, board_vc_1) {
	
	char bmcfw_ver[256];
	EXPECT_EQ(read_bmcfw_version(tokens_[0], bmcfw_ver), FPGA_OK);

	EXPECT_NE(read_bmcfw_version(tokens_[0], NULL), FPGA_OK);
	EXPECT_NE(read_bmcfw_version(NULL, bmcfw_ver), FPGA_OK);
}
TEST_P(board_vc_c_p, board_vc_2) {

	char max10fw_ver[256];
	EXPECT_EQ(read_max10fw_version(tokens_[0], max10fw_ver), FPGA_OK);

	EXPECT_NE(read_max10fw_version(tokens_[0], NULL), FPGA_OK);
	EXPECT_NE(read_max10fw_version(NULL, max10fw_ver), FPGA_OK);
}

TEST_P(board_vc_c_p, board_vc_3) {

	char pcb_info[256];
	EXPECT_EQ(read_pcb_info(tokens_[0], pcb_info), FPGA_OK);

	EXPECT_NE(read_pcb_info(tokens_[0], NULL), FPGA_OK);
	EXPECT_NE(read_pcb_info(NULL, pcb_info), FPGA_OK);
}

fpga_result read_pkvl_info(fpga_token token, struct fpga_pkvl_info *pkvl_info, int *fpga_mode);

TEST_P(board_vc_c_p, board_vc_4) {

	struct fpga_pkvl_info pkvl_info;
	int fpga_mode;
	EXPECT_EQ(read_pkvl_info(tokens_[0], &pkvl_info, &fpga_mode), FPGA_OK);


	EXPECT_NE(read_pkvl_info(tokens_[0], &pkvl_info, NULL), FPGA_OK);
	EXPECT_NE(read_pkvl_info(tokens_[0], NULL, &fpga_mode), FPGA_OK);

	EXPECT_NE(read_pkvl_info(NULL, &pkvl_info, &fpga_mode), FPGA_OK);

}

TEST_P(board_vc_c_p, board_vc_5) {

	unsigned char buf[8] = { 0 };
	EXPECT_EQ(read_mac_info(tokens_[0], buf), FPGA_OK);

	EXPECT_NE(read_mac_info(NULL, buf), FPGA_OK);

	EXPECT_NE(read_mac_info(tokens_[0], NULL), FPGA_OK);
}

TEST_P(board_vc_c_p, board_vc_6) {

	int group_num = 0;
	EXPECT_EQ(read_phy_group_info(tokens_[0], NULL, &group_num), FPGA_OK);

	EXPECT_NE(read_phy_group_info(tokens_[0], NULL, NULL), FPGA_OK);

	EXPECT_NE(read_phy_group_info(NULL, NULL, &group_num), FPGA_OK);
}


TEST_P(board_vc_c_p, board_vc_7) {


	EXPECT_EQ(print_board_info(tokens_[0]), FPGA_OK);
	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_OK);

	EXPECT_EQ(print_phy_info(tokens_[0]), FPGA_OK);


}


TEST_P(board_vc_c_p, board_vc_8) {



	char buf[10] = { 0 };
	write_sysfs_file((const char *)"spi-altera.0.auto/spi_master/spi0/spi0.0/bmcfw_flash_ctrl/bmcfw_version", (void*)buf, sizeof(buf));

	char bmcfw_ver[256];
	EXPECT_NE(read_bmcfw_version(tokens_[0], bmcfw_ver), FPGA_OK);


	write_sysfs_file((const char *)"spi-altera.0.auto/spi_master/spi0/spi0.0/max10_version", (void*)buf, sizeof(buf));

	char max10fw_ver[256];
	EXPECT_NE(read_max10fw_version(tokens_[0], max10fw_ver), FPGA_OK);


}


INSTANTIATE_TEST_CASE_P(baord_vc_c, board_vc_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "dcp-vc" })));


class board_vc_invalid_c_p : public board_vc_c_p { };


TEST_P(board_vc_invalid_c_p, board_vc_20) {

	char bmcfw_ver[256];
	EXPECT_NE(read_bmcfw_version(tokens_[0], bmcfw_ver), FPGA_OK);

	char max10fw_ver[256];
	EXPECT_NE(read_max10fw_version(tokens_[0], max10fw_ver), FPGA_OK);

	char pcb_info[256];
	EXPECT_NE(read_pcb_info(tokens_[0], pcb_info), FPGA_OK);

	struct fpga_pkvl_info pkvl_info;
	int fpga_mode;
	EXPECT_NE(read_pkvl_info(tokens_[0], &pkvl_info, &fpga_mode), FPGA_OK);


	unsigned char buf[8] = { 0 };
	EXPECT_NE(read_mac_info(tokens_[0], buf), FPGA_OK);

	int group_num = 0;
	EXPECT_NE(read_phy_group_info(tokens_[0], NULL, &group_num), FPGA_OK);

	EXPECT_NE(print_board_info(tokens_[0]), FPGA_OK);
	EXPECT_NE(print_mac_info(tokens_[0]), FPGA_OK);

	EXPECT_NE(print_phy_info(tokens_[0]), FPGA_OK);
}
INSTANTIATE_TEST_CASE_P(board_vc_invalid_c, board_vc_invalid_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
