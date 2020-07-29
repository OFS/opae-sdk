// Copyright(c) 2018-2020, Intel Corporation
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

#include <opae/fpga.h>
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "mock/test_utils.h"
extern "C" {
#include <json-c/json.h>
#include <uuid/uuid.h>

struct config {
	struct target {
		int bus;
		bool fme_metrics;
		bool afu_metrics;
		int open_flags;
	} target;
};

struct bdf_info {
	uint8_t bus;
};

extern struct config config;

void print_err(const char *s, fpga_result res);
void FpgaMetricsAppShowHelp(void);
fpga_result parse_args(int argc, char *argv[]);
fpga_result get_bus_info(fpga_token tok, struct bdf_info *finfo);
int fpgametrics_main(int argc, char *argv[]);
void print_bus_info(struct bdf_info *info);
}

#include <config.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
using namespace opae::testing;


class fpga_metrics_c_p : public ::testing::TestWithParam<std::string> {
protected:
	fpga_metrics_c_p() {}

	virtual void SetUp() override {
		std::string platform_key = GetParam();
		ASSERT_TRUE(test_platform::exists(platform_key));
		platform_ = test_platform::get(platform_key);
		system_ = test_system::instance();
		system_->initialize();
		system_->prepare_syfs(platform_);

		EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);
		optind = 0;
		config_ = config;
	}

	virtual void TearDown() override {
		config = config_;
		fpgaFinalize();
		system_->finalize();
	}

	struct config config_;
	test_platform platform_;
	test_system *system_;
};

/**
 * @test       help
 * @brief      Test: help
 * @details    help displays the application help message.<br>
 */
TEST_P(fpga_metrics_c_p, help) {
	FpgaMetricsAppShowHelp();
}

/**
 * @test       print_err
 * @brief      Test: print_err
 * @details    print_err prints the given string and<br>
 *             the decoded representation of the fpga_result<br>
 *             to stderr.<br>
 */
TEST_P(fpga_metrics_c_p, print_err) {
	print_err("msg", FPGA_OK);
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When given an invalid command option,<br>
 *             parse_args returns a negative value.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args0) {
	char zero[20];
	char one[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-Y");

	char *argv[] = { zero, one };
	EXPECT_EQ(parse_args(2, argv), 3);
}

/**
 * @test       parse_args1
 * @brief      Test: parse_args
 * @details    When given a command option that requires a param,<br>
 *             omitting the required param causes parse_args to<br>
 *             return a value other than FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args1) {
	char zero[20];
	char one[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-B");

	char *argv[] = { zero, one };
	EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When given a command option that requires a param,<br>
 *             omitting the required param causes parse_args to<br>
 *             return a value other than FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args2) {
	char zero[20];
	char one[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-v");

	char *argv[] = { zero, one };
	EXPECT_NE(parse_args(2, argv), FPGA_OK);
}

/**
 * @test       parse_args3
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args3) {
	char zero[20];
	char one[20];
	char two[20];
	char three[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-B");
	strcpy(two, "3");
	strcpy(three, "-s");

	char *argv[] = { zero, one, two, three };
	EXPECT_EQ(parse_args(4, argv), FPGA_OK);
	EXPECT_EQ(config.target.bus, 3);
}

/**
 * @test       parse_args4
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args4) {
	char zero[20];
	char one[20];
	char two[20];
	char three[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-B");
	strcpy(two, "3");
	strcpy(three, "-F");

	char *argv[] = { zero, one, two, three };
	EXPECT_EQ(parse_args(4, argv), FPGA_OK);
	EXPECT_EQ(config.target.bus, 3);
}

/**
 * @test       parse_args5
 * @brief      Test: parse_args
 * @details    When given valid command options,<br>
 *             parse_args populates the global config struct,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, parse_args5) {
	char zero[20];
	char one[20];
	char two[20];
	char three[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-B");
	strcpy(two, "3");
	strcpy(three, "-A");

	char *argv[] = { zero, one, two, three };
	EXPECT_EQ(parse_args(4, argv), FPGA_OK);
	EXPECT_EQ(config.target.bus, 3);
}

/**
 * @test       get_bus_info1
 * @brief      Test: get_bus_info,
 *                   print_bus_info
 * @details    When given valid fpga token, <br>
 *             get_bus_info retutns bdf info <br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, get_bus_info1) {
	fpga_properties filter = NULL;
	fpga_token *tokens = NULL;
	uint32_t matches = 0, num_tokens = 0;;

	ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
	ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
	ASSERT_GT(matches, 0);
	tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

	num_tokens = matches;
	ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

	struct bdf_info bdf_info;

	EXPECT_EQ(get_bus_info(tokens[0], &bdf_info), FPGA_OK);
	print_bus_info(&bdf_info);

	for (uint32_t i = 0; i < num_tokens; ++i) {
		fpgaDestroyToken(&tokens[i]);
	}
	free(tokens);
	fpgaDestroyProperties(&filter);
}

/**
 * @test       main1
 * @brief      Test: fpgametrics_main
 * @details    When given a valid command line,<br>
 *             fpgametrics_main prints fpga metrics <br>
 *             return FPGA_OK.<br>
 */
TEST_P(fpga_metrics_c_p, main1) {
	char zero[20];
	char one[20];
	char two[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-B");
	sprintf(two, "%d", platform_.devices[0].bus);

	char *argv[] = { zero, one, two };
	EXPECT_EQ(fpgametrics_main(3, argv), FPGA_OK);
}

/**
 * @test       main2
 * @brief      Test: object_api_main
 * @details    When given a invalid command line,<br>
 *             fpgametrics_main retuns errors<br>
 *             return 2 or 3.<br>
 */
TEST_P(fpga_metrics_c_p, main2) {
	char zero[20];
	char one[20];
	char two[20];
	strcpy(zero, "fpgametrics");
	strcpy(one, "-Y");
	sprintf(two, "%d", platform_.devices[0].bus);

	char *argv1[] = { zero};
	EXPECT_EQ(fpgametrics_main(1, argv1), 1);

	char *argv2[] = { zero, one, two };
	EXPECT_EQ(fpgametrics_main(3, argv2), 2);

}
INSTANTIATE_TEST_CASE_P(fpgametrics_c, fpga_metrics_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"dfl-d5005","dfl-n3000"})));
