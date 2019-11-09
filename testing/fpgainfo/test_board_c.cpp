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
#include <opae/fpga.h>
#include <limits.h>
#include <config.h>

#include <iostream>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include "safe_string/safe_string.h"

extern "C" {

#include <json-c/json.h>
#include <stdlib.h>
#include "opae_int.h"
#include "types_int.h"
fpga_result load_board_plugin(fpga_token token, void** dl_handle);
int unload_board_plugin(void);
int parse_mac_args(int argc, char *argv[]);
fpga_result mac_filter(fpga_properties * filter, int arc, char *argv[]);
fpga_result mac_command(fpga_token *tokens, int num_tokens, int argc, char *argv[]);
int parse_phy_args(int argc, char *argv[]);
fpga_result phy_filter(fpga_properties *filter, int argc, char *argv[]);
fpga_result phy_command(fpga_token *tokens, int num_tokens, int argc, char *argv[]);
fpga_result mac_info(fpga_token token);
fpga_result phy_group_info(fpga_token token);
}


using namespace opae::testing;

class fpgainfo_board_c_p : public ::testing::TestWithParam<std::string> {
  protected:
    fpgainfo_board_c_p() {}

    virtual void SetUp() override 
    {
        std::string platform_key = GetParam();
        ASSERT_TRUE(test_platform::exists(platform_key));
        platform_ = test_platform::get(platform_key);
        system_ = test_system::instance();
        system_->initialize();
        system_->prepare_syfs(platform_);
    
        EXPECT_EQ(fpgaInitialize(nullptr), FPGA_OK);
    
        optind = 0;
    }
    
    virtual void TearDown() override {
        fpgaFinalize();
        system_->finalize();
    }
    
    test_platform platform_;
    test_system *system_;
};

/**
 * @test       invalid_loading_tests
 * @brief      Test: load_board_plugin, unload_board_plugin
 * @detail:    Given invalid params to load_board_plugin,
 *                     the fn returns FPGA_INVALID_PARAM
 *                     unload_board_plugin return FPGA_OK
 */  
TEST(fpgainfo_board_c_p, invalid_loading_tests) {
    EXPECT_EQ(load_board_plugin(NULL, NULL), FPGA_INVALID_PARAM);

    EXPECT_EQ(unload_board_plugin(), FPGA_OK);
}

/**
 * @test       invalid_loading_tests
 * @brief      Test: load_board_plugin
 * @detail     Given valid filter and invalid token upon enumeration, 
 *             load_board_plugin returns FPGA_INVALID_PARAM
 */
TEST_P(fpgainfo_board_c_p, load_board_plugin) {
    void* dl_handle = NULL;
    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    EXPECT_EQ(load_board_plugin(tokens, &dl_handle), FPGA_INVALID_PARAM);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       parse_mac_args0
 * @brief      Test: parse_mac_args
 * @details    When passed with valid command options, <br>
 *             the fn returns 0. For invalid options, fn returns -1. <br>
 */
TEST_P(fpgainfo_board_c_p, parse_mac_args0) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    strcpy(zero, "fpgainfo");
    strcpy(one, "mac");
    EXPECT_EQ(parse_mac_args(2, argv), 0);

    strcpy(two, "-h");
    EXPECT_EQ(parse_mac_args(3, argv), -1);

    strcpy(two, "-z");
    EXPECT_EQ(parse_mac_args(3, argv), -1);
}

/**
 * @test       mac_filter
 * @brief      Test: mac_filter
 * @details    When passed with invalid command options, <br>
 *             the fn returns FPGA_INVALID_PARAM. <br>
 */
TEST_P(fpgainfo_board_c_p, mac_filter) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    strcpy(zero, "fpgainfo");
    strcpy(one, "mac");
    EXPECT_EQ(mac_filter(&filter, 2, argv), FPGA_INVALID_PARAM);
}

/**
* @test       mac_command
* @brief      Test: mac_command
* @details    When passed with invalid token, <br>
*             the fn returns FPGA_INVALID_PARAM. <br>
*/
TEST_P(fpgainfo_board_c_p, mac_command) {
    char *argv[] = { };

    fpga_token tokens = NULL;
    EXPECT_EQ(mac_command(&tokens, 0, 0, argv), FPGA_OK);
}

/**
* @test       mac_command
* @brief      Test: mac_command
* @details    When passed with valid token and fails to get properties, <br>
*             the fn returns FPGA_INVALID_PARAM. <br>
*/
TEST_P(fpgainfo_board_c_p, mac_command1) {
    char *argv[] = { };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    EXPECT_EQ(mac_command(tokens, num_tokens, 0, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
* @test       mac_info
* @brief      Test: mac_info
* @details    When passed with invalid token, <br>
*             the fn returns FPGA_INVALID_PARAM. <br>
*/
TEST_P(fpgainfo_board_c_p, mac_info) {
    fpga_token tokens = NULL;
    EXPECT_EQ(mac_info(tokens), FPGA_INVALID_PARAM);
}

/**
 * @test       parse_phy_args0
 * @brief      Test: parse_phy_args
 * @details    When passed with valid command options, <br>
 *             the fn returns 0. For invalid options, fn returns -1. <br>
 */
TEST_P(fpgainfo_board_c_p, parse_phy_args0) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    strcpy(zero, "fpgainfo");
    strcpy(one, "phy");
    EXPECT_EQ(parse_phy_args(2, argv), 0);

    strcpy(two, "-h");
    EXPECT_EQ(parse_phy_args(3, argv), -1);

    strcpy(two, "-z");
    EXPECT_EQ(parse_phy_args(3, argv), -1);
}

/**
 * @test       parse_phy_args0
 * @brief      Test: parse_phy_args
 * @details    When passed with valid command options, <br>
 *             the fn returns 0. 
 *             For invalid options, fn returns -1. <br>
 */
TEST_P(fpgainfo_board_c_p, parse_phy_args1) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three};

    strcpy(zero, "fpgainfo");
    strcpy(one, "phy");
    strcpy(two, "-G");
    strcpy(three, "all");
    EXPECT_EQ(parse_phy_args(4, argv), 0);

    strcpy(three, "0");
    EXPECT_EQ(parse_phy_args(4, argv), 0);

    strcpy(three, "1");
    EXPECT_EQ(parse_phy_args(4, argv), 0);

    strcpy(three, "99");
    EXPECT_EQ(parse_phy_args(4, argv), -1);
}

/**
 * @test       phy_filter
 * @brief      Test: phy_filter
 * @details    When passed with invalid command options, <br>
 *             the fn returns FPGA_INVALID_PARAM. <br>
 */
TEST_P(fpgainfo_board_c_p, phy_filter) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    strcpy(zero, "fpgainfo");
    strcpy(one, "phy");
    EXPECT_EQ(phy_filter(&filter, 2, argv), FPGA_INVALID_PARAM);
}

/**
* @test       phy_command
* @brief      Test: phy_command
* @details    The phy_command fn always returen FPGA_OK <br>
*/
TEST_P(fpgainfo_board_c_p, phy_command) {
    char *argv[] = { };

    fpga_token tokens = NULL;
    EXPECT_EQ(phy_command(&tokens, 0, 0, argv), FPGA_OK);
}

/**
* @test       phy_command
* @brief      Test: phy_command
* @details    The phy_command fn always returen FPGA_OK <br>
*/
TEST_P(fpgainfo_board_c_p, phy_command1) {
    char *argv[] = { };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    EXPECT_EQ(phy_command(tokens, num_tokens, 0, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);

}

/**
* @test       phy_group_info
* @brief      Test: phy_group_info
* @details    Given an invalid option to phy_grou_info, the <br>
*             fn returns FPGA_INVALID_PARAM. <br>
*/
TEST_P(fpgainfo_board_c_p, phy_group_info) {
    fpga_token tokens = NULL;
    EXPECT_EQ(phy_group_info(tokens), FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(fpgainfo_c, fpgainfo_board_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc","dcp-vc" })));

