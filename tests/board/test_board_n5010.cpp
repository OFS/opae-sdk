// Original work Copyright(c) 2019-2022, Intel Corporation
// Modifications Copyright(c) 2021, Silicom Denmark A/S
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
#include <net/ethernet.h>
#include "opae_int.h"
}

#include "gtest/gtest.h"
#include "mock/test_system.h"

#include "libboard/board_common/board_common.h"
#include "libboard/board_n5010/board_n5010.h"

using namespace opae::testing;

class board_n5010_c_p : public ::testing::TestWithParam<std::string> {
protected:
	board_n5010_c_p() : tokens_{ { nullptr, nullptr } } {}

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

// test DFL sysfs attributes
class board_dfl_n5010_c_p : public board_n5010_c_p { };

/**
* @test       board_n5010_0
* @brief      Tests: print_board_info
* @details    Validates fpga board info  <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_0) {

	EXPECT_EQ(print_board_info(tokens_[0]), FPGA_OK);
}

/**
* @test       board_n5010_1
* @brief      Tests: read_max10fw_version
* @details    Validates max10 firmware version  <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_1) {

	char max10fw_ver[SYSFS_PATH_MAX];

	EXPECT_EQ(read_max10fw_version(tokens_[0], max10fw_ver, SYSFS_PATH_MAX), FPGA_OK);

	EXPECT_EQ(read_max10fw_version(tokens_[0], NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

	EXPECT_EQ(read_max10fw_version(NULL, max10fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_n5010_2
* @brief      Tests: read_bmcfw_version
* @details    Validates bmc firmware version  <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_2) {

	char bmcfw_ver[SYSFS_PATH_MAX];

	EXPECT_EQ(read_bmcfw_version(tokens_[0], bmcfw_ver, SYSFS_PATH_MAX), FPGA_OK);

	EXPECT_EQ(read_bmcfw_version(tokens_[0], NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

	EXPECT_EQ(read_bmcfw_version(NULL, bmcfw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
}

/**
* @test       board_n5010_3
* @brief      Tests: parse_fw_ver
* @details    Validates parse fw version  <br>
*/
TEST_P(board_dfl_n5010_c_p, board_n5010_3) {

	char buf[SYSFS_PATH_MAX];
	char fw_ver[SYSFS_PATH_MAX];

	EXPECT_EQ(parse_fw_ver(buf, NULL, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);
	EXPECT_EQ(parse_fw_ver(NULL, fw_ver, SYSFS_PATH_MAX), FPGA_INVALID_PARAM);

}

/**
* @test       board_n5010_4
* @brief      Tests: print_mac_info
* @details    Validates prints mac info <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_4) {

	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_OK);
}

/**
* @test       board_n5010_5
* @brief      Tests: print_sec_info
* @details    Validates fpga board info  <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_5) {

	EXPECT_EQ(print_sec_info(tokens_[0]), FPGA_OK);
}

/**
* @test       board_n5010_6
* @brief      Tests: print_mac_info
* @details    Validates prints mac info  <br>
*/
TEST_P(board_dfl_n5010_c_p, DISABLED_board_n5010_6) {

	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(board_dfl_n5010_c_p);
INSTANTIATE_TEST_CASE_P(board_n5010_c, board_dfl_n5010_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "dfl-n6000" })));

// test invalid sysfs attributes
class board_n5010_invalid_c_p : public board_n5010_c_p { };

/**
* @test       board_n5010_9
* @brief      Tests: read_max10fw_version
*             read_max10fw_version,read_pcb_info
*             read_pkvl_info,read_mac_info
*             read_phy_group_info,print_board_info
*             print_phy_info,print_mac_info
* @details    Validates function with invalid sysfs <br>
*/
TEST_P(board_n5010_invalid_c_p, board_n5010_9) {

	char bmcfw_ver[SYSFS_PATH_MAX];
	EXPECT_EQ(read_bmcfw_version(tokens_[0], bmcfw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

	char max10fw_ver[SYSFS_PATH_MAX];
	EXPECT_EQ(read_max10fw_version(tokens_[0], max10fw_ver, SYSFS_PATH_MAX), FPGA_NOT_FOUND);

	EXPECT_EQ(print_board_info(tokens_[0]), FPGA_NOT_FOUND);

	EXPECT_EQ(print_mac_info(tokens_[0]), FPGA_NOT_FOUND);

	EXPECT_EQ(print_sec_info(tokens_[0]), FPGA_NOT_FOUND);
}
INSTANTIATE_TEST_CASE_P(board_n5010_invalid_c, board_n5010_invalid_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
