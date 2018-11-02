// Copyright(c) 2018, Intel Corporation
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

extern "C" {

typedef fpga_result (*filter_fn)(fpga_properties *, int, char **);
typedef fpga_result (*command_fn)(fpga_token *, int, int, char **);
typedef void (*help_fn)(void);

typedef enum metrics_inquiry { ALL, POWER, THERMAL } metrics_inquiry;

struct command_handler {
    const char *command;
    filter_fn filter;
    command_fn run;
    help_fn help;
};
extern struct command_handler *cmd_array;

void help(void);

int parse_args(int argc, char *argv[]);

struct command_handler *get_command(char *cmd);

fpga_result errors_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result errors_command(fpga_token *tokens, int num_tokens, int argc,
			   char *argv[]);
void errors_help(void);

fpga_result fme_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result fme_command(fpga_token *tokens, int num_tokens, int argc,
			char *argv[]);
void fme_help(void);

void fpgainfo_print_common(const char *hdr, fpga_properties props);

void fpgainfo_print_err(const char *s, fpga_result res);

fpga_result port_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result port_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[]);
void port_help(void);

fpga_result power_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result power_command(fpga_token *tokens, int num_tokens, int argc,
			  char *argv[]);
void power_help(void);

fpga_result temp_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result temp_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[]);
void temp_help(void);

fpga_result bmc_filter(fpga_properties *filter, int argc, char *argv[]);

fpga_result bmc_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[]);
void bmc_help(void);

fpga_result perf_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[]);
void perf_help(void);

fpga_result get_metrics(fpga_token token, metrics_inquiry inquiry,
                        fpga_metric_info *metrics_info, fpga_metric *metrics,
                        uint64_t *num_metrics);

void replace_chars(char *str, char match, char rep);

void upcase_pci(char *str, size_t len);

void upcase_first(char *str);

int str_in_list(const char *key, const char *list[], size_t size);

int fpgainfo_main(int argc, char *argv[]);

}

#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgainfo_c_p : public ::testing::TestWithParam<std::string> {
    protected:
        fpgainfo_c_p() {}

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
            system_->finalize();
        }

        test_platform platform_;
        test_system *system_;
};

/**
 * @test       help
 * @brief      Test: help
 * @details    help displays the application help message.<br>
 */
TEST_P(fpgainfo_c_p, help) {
    help();
}

/**
 * @test       parse_args0
 * @brief      Test: parse_args
 * @details    When passed with valid command options, <br>
 *             the fn returns 0. <br>
 */
TEST_P(fpgainfo_c_p, parse_args0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    EXPECT_EQ(parse_args(2, argv), 0);

    strcpy(one, "port");
    EXPECT_EQ(parse_args(2, argv), 0);

    strcpy(one, "power");
    EXPECT_EQ(parse_args(2, argv), 0);

    strcpy(one, "temp");
    EXPECT_EQ(parse_args(2, argv), 0);
}

/**
 * @test       parse_args1
 * @brief      Test: parse_args
 * @details    When passed with '-h' or '--help', the fn <br>
 *             prints help message and return non-zero. <br>
 */
TEST_P(fpgainfo_c_p, parse_args1) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    strcpy(zero, "fpgainfo");
    strcpy(one, "-h");
    EXPECT_NE(parse_args(2, argv), 0);

    optind = 0;
    strcpy(one, "--help");
    EXPECT_NE(parse_args(2, argv), 0);
}

/**
 * @test       parse_args2
 * @brief      Test: parse_args
 * @details    When passed with invalid options, the fn <br>
 *             prints error message and return zero. <br>
 */
TEST_P(fpgainfo_c_p, parse_args2) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    strcpy(zero, "fpgainfo");
    strcpy(one, "?");
    EXPECT_EQ(parse_args(2, argv), 0);
}


/**
 * @test       get_command0
 * @brief      Test: get_command
 * @details    When passed with valid commands, the fn <br>
 *             returns non-nullptr <br>
 */
TEST_P(fpgainfo_c_p, get_command0) {
    char cmd[20];

    strcpy(cmd, "errors");
    EXPECT_NE(get_command(cmd), nullptr);

    strcpy(cmd, "power");
    EXPECT_NE(get_command(cmd), nullptr);

    strcpy(cmd, "temp");
    EXPECT_NE(get_command(cmd), nullptr);

    strcpy(cmd, "fme");
    EXPECT_NE(get_command(cmd), nullptr);

    strcpy(cmd, "port");
    EXPECT_NE(get_command(cmd), nullptr);

    /*
    strcpy(cmd, "bmc");
    EXPECT_NE(get_command(cmd), nullptr);
    */
}

/**
 * @test       get_command1
 * @brief      Test: get_command
 * @details    When passed with invalid commands, the fn <br>
 *             returns nullptr <br>
 */
TEST_P(fpgainfo_c_p, get_command1) {
    char cmd[20];

    strcpy(cmd, "???");
    EXPECT_EQ(get_command(cmd), nullptr);
}


/**
 * @test       errors_filter0
 * @brief      Test: errors_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, errors_filter0) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv3[] = { zero, one, two };
    char *argv4[] = { zero, one, two, three };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "errors");
    strcpy(two, "fme");
    EXPECT_EQ(errors_filter(&filter, 3, argv3), FPGA_OK);

    strcpy(two, "port");
    EXPECT_EQ(errors_filter(&filter, 3, argv3), FPGA_OK);

    strcpy(two, "all");
    EXPECT_EQ(errors_filter(&filter, 3, argv3), FPGA_OK);

    strcpy(two, "fme");
    strcpy(three, "-c");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    strcpy(two, "port");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    strcpy(two, "all");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    strcpy(two, "fme");
    strcpy(three, "--force");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    strcpy(two, "port");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    strcpy(two, "all");
    EXPECT_EQ(errors_filter(&filter, 4, argv4), FPGA_OK);

    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       errors_filter1
 * @brief      Test: errors_filter
 * @details    When passed with invalid arguments, the function <br>
 *             prints help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, errors_filter1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "errors");
    strcpy(two, "???");
    EXPECT_EQ(errors_filter(&filter, 3, argv), FPGA_OK);

    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       errors_filter2
 * @brief      Test: errors_filter
 * @details    When passed with argument that requires additional <br>
 *             option, missing the option causes the fn to print <br>
 *             help messages and return FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, errors_filter2) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv2[] = { zero, one };
    char *argv3[] = { zero, one, two };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "errors");
    EXPECT_EQ(errors_filter(&filter, 2, argv2), FPGA_OK);

    strcpy(two, "-c");
    EXPECT_EQ(errors_filter(&filter, 3, argv3), FPGA_OK);

    strcpy(two, "--force");
    EXPECT_EQ(errors_filter(&filter, 3, argv3), FPGA_OK);

    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       errors_command0
 * @brief      Test: errors_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, errors_command0) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv3[] = { zero, one, two };
    char *argv4[] = { zero, one, two, three };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "errors");
    strcpy(two, "all");
    EXPECT_EQ(errors_command(tokens, num_tokens, 3, argv3), FPGA_OK);

    strcpy(two, "fme");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 3, argv3), FPGA_OK);

    strcpy(two, "port");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_ACCELERATOR), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 3, argv3), FPGA_OK);

    strcpy(two, "all");
    strcpy(three, "-h");
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "fme");
    strcpy(three, "-h");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "port");
    strcpy(three, "-h");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_ACCELERATOR), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "all");
    strcpy(three, "-c");
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "fme");
    strcpy(three, "-c");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "port");
    strcpy(three, "-c");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_ACCELERATOR), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "all");
    strcpy(three, "--force");
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "fme");
    strcpy(three, "--force");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    strcpy(two, "port");
    strcpy(three, "--force");
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_ACCELERATOR), FPGA_OK);
    EXPECT_EQ(errors_command(tokens, num_tokens, 4, argv4), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       errors_help
 * @brief      Test: errors_help
 * @details    The function prints help message for errors <br>
 *             subcommand.<br>
 */
TEST_P(fpgainfo_c_p, errors_help) {
    errors_help();
}

/**
 * @test       fme_filter0
 * @brief      Test: fme_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, fme_filter0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    EXPECT_EQ(fme_filter(&filter, 2, argv), FPGA_OK);
    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       fme_command0
 * @brief      Test: fme_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, fme_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    EXPECT_EQ(fme_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       fme_command1
 * @brief      Test: fme_command
 * @details    When passed with '-h', the fn prints <br>
 *             fme help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, fme_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-h");
    EXPECT_EQ(fme_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       fme_help
 * @brief      Test: fme_help
 * @details    The function prints help message for fme subcommand.<br>
 */
TEST_P(fpgainfo_c_p, fme_help) {
    fme_help();
}

/**
 * @test       port_filter0
 * @brief      Test: port_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, port_filter0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "port");
    EXPECT_EQ(port_filter(&filter, 2, argv), FPGA_OK);
    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       port_command0
 * @brief      Test: port_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, port_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "port");
    EXPECT_EQ(port_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       port_command1
 * @brief      Test: port_command
 * @details    When passed with '-h', the fn prints <br>
 *             port help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, port_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "port");
    strcpy(two, "-h");
    EXPECT_EQ(port_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       port_help
 * @brief      Test: port_help
 * @details    The function prints help message for port subcommand.<br>
 */
TEST_P(fpgainfo_c_p, port_help) {
    port_help();
}

/**
 * @test       power_filter0
 * @brief      Test: power_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, power_filter0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "power");
    EXPECT_EQ(power_filter(&filter, 2, argv), FPGA_OK);
    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       power_command0
 * @brief      Test: power_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, power_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "power");
    EXPECT_EQ(power_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       power_command1
 * @brief      Test: power_command
 * @details    When passed with '-h', the fn prints <br>
 *             power help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, power_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "power");
    strcpy(two, "-h");
    EXPECT_EQ(power_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       power_help
 * @brief      Test: power_help
 * @details    The function prints help message for power subcommand.<br>
 */
TEST_P(fpgainfo_c_p, power_help) {
    power_help();
}

/**
 * @test       temp_filter0
 * @brief      Test: temp_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, temp_filter0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "temp");
    EXPECT_EQ(temp_filter(&filter, 2, argv), FPGA_OK);
    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       temp_command0
 * @brief      Test: temp_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, temp_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "temp");
    EXPECT_EQ(temp_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       temp_command1
 * @brief      Test: temp_command
 * @details    When passed with '-h', the fn prints <br>
 *             temp help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, temp_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "temp");
    strcpy(two, "-h");
    EXPECT_EQ(temp_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       temp_help
 * @brief      Test: temp_help
 * @details    The function prints help message for temp subcommand.<br>
 */
TEST_P(fpgainfo_c_p, temp_help) {
    temp_help();
}

/**
 * @test       bmc_filter0
 * @brief      Test: bmc_filter
 * @details    When passed with valid arguments, the function <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, bmc_filter0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "bmc");
    EXPECT_EQ(bmc_filter(&filter, 2, argv), FPGA_OK);
    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       bmc_command0
 * @brief      Test: bmc_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, bmc_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "bmc");
    EXPECT_EQ(bmc_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       bmc_command1
 * @brief      Test: bmc_command
 * @details    When passed with '-h', the fn prints <br>
 *             bmc help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, bmc_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "bmc");
    strcpy(two, "-h");
    EXPECT_EQ(bmc_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       perf_command0
 * @brief      Test: perf_command
 * @details    When passed with valid arguments, the fn prints <br>
 *             relevant information and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, perf_command0) {
    char zero[20];
    char one[20];
    char *argv[] = { zero, one };

    fpga_properties filter = NULL;
    fpga_token *tokens = NULL;
    uint32_t matches = 0, num_tokens = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, NULL, 0, &matches), FPGA_OK);
    ASSERT_GT(matches, 0);
    tokens = (fpga_token *)malloc(matches * sizeof(fpga_token));

    num_tokens = matches;
    ASSERT_EQ(fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "perf");
    EXPECT_EQ(bmc_command(tokens, num_tokens, 2, argv), FPGA_OK);

    for (uint32_t i = 0; i < num_tokens; ++i) {
        fpgaDestroyToken(&tokens[i]);
    }
    free(tokens);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       perf_command1
 * @brief      Test: perf_command
 * @details    When passed with '-h', the fn prints <br>
 *             bmc help message and returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, perf_command1) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };

    fpga_token *tokens = NULL;

    strcpy(zero, "fpgainfo");
    strcpy(one, "perf");
    strcpy(two, "-h");
    EXPECT_EQ(bmc_command(tokens, 0, 3, argv), FPGA_OK);
}

/**
 * @test       bmc_help
 * @brief      Test: bmc_help
 * @details    The function prints help message for bmc subcommand.<br>
 */
TEST_P(fpgainfo_c_p, bmc_help) {
    bmc_help();
}

/**
 * @test       perf_help
 * @brief      Test: perf_help
 * @details    The function prints help message for perf subcommand.<br>
 */
TEST_P(fpgainfo_c_p, perf_help) {
    perf_help();
}

/**
 * @test       get_metrics0
 * @brief      Test: get_metrics
 * @details    When passed with valid arguments, the fn <br>
 *             retrieve required information from BMC and <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, get_metrics0) {
    fpga_properties filter = NULL;
    fpga_token token;
    fpga_metric_info metrics_info[64];
    fpga_metric metrics[64];
    uint64_t num_metrics;
    uint32_t matches = 0;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, &token, 1, &matches), FPGA_OK);

    EXPECT_EQ(get_metrics(token, ALL, metrics_info, metrics, &num_metrics), FPGA_OK);

    fpgaDestroyToken(&token);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       get_metrics1
 * @brief      Test: get_metrics
 * @details    When passed with valid arguments, the fn <br>
 *             retrieve required information from BMC and <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, get_metrics1) {
    fpga_properties filter = NULL;
    fpga_token token;
    fpga_metric_info metrics_info[64];
    fpga_metric metrics[64];
    uint64_t num_metrics;
    uint32_t matches = 0;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, &token, 1, &matches), FPGA_OK);

    EXPECT_EQ(get_metrics(token, POWER, metrics_info, metrics, &num_metrics), FPGA_OK);

    fpgaDestroyToken(&token);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       get_metrics2
 * @brief      Test: get_metrics
 * @details    When passed with valid arguments, the fn <br>
 *             retrieve required information from BMC and <br>
 *             returns FPGA_OK. <br>
 */
TEST_P(fpgainfo_c_p, get_metrics2) {
    fpga_properties filter = NULL;
    fpga_token token;
    fpga_metric_info metrics_info[64];
    fpga_metric metrics[64];
    uint64_t num_metrics;
    uint32_t matches = 0;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, &token, 1, &matches), FPGA_OK);

    EXPECT_EQ(get_metrics(token, THERMAL, metrics_info, metrics, &num_metrics), FPGA_OK);

    fpgaDestroyToken(&token);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       fpgainfo_print_common
 * @brief      Test: fpgainfo_print_common
 */
TEST_P(fpgainfo_c_p, fpgainfo_print_common) {
    fpga_properties filter = NULL;
    fpga_token token = nullptr;
    uint32_t matches = 0;;

    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter,FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter, 1, &token, 1, &matches), FPGA_OK);

    fpgainfo_print_common("//****** HEADER ******//", filter);

    fpgaDestroyToken(&token);
    fpgaDestroyProperties(&filter);
}

/**
 * @test       fpgainfo_print_err
 * @brief      Test: fpgainfo_print_err
 */
TEST_P(fpgainfo_c_p, fpgainfo_print_err) {
    fpgainfo_print_err("ERROR:", FPGA_INVALID_PARAM);
    fpgainfo_print_err("ERROR:", FPGA_BUSY);
    fpgainfo_print_err("ERROR:", FPGA_EXCEPTION);
    fpgainfo_print_err("ERROR:", FPGA_NOT_FOUND);
    fpgainfo_print_err("ERROR:", FPGA_NO_MEMORY);
    fpgainfo_print_err("ERROR:", FPGA_NOT_SUPPORTED);
    fpgainfo_print_err("ERROR:", FPGA_NO_DRIVER);
    fpgainfo_print_err("ERROR:", FPGA_NO_DAEMON);
    fpgainfo_print_err("ERROR:", FPGA_NO_ACCESS);
    fpgainfo_print_err("ERROR:", FPGA_NO_ACCESS);
}


/**
 * @test       replace_chars0
 * @brief      Test: replace_chars
 */
TEST(fpgainfo_c, replace_chars0) {
    char input[256];
    strcpy(input, "one_two_three_four");
    replace_chars(input, '_', ' ');
    EXPECT_STREQ(input, "one two three four");
}

/**
 * @test       replace_chars1
 * @brief      Test: replace_chars
 */
TEST(fpgainfo_c, replace_chars1) {
    char input[256];
    strcpy(input, "one_two_three_four");
    replace_chars(input, ':', ' ');
    EXPECT_STREQ(input, "one_two_three_four");
}

/**
 * @test       upcase_pci0
 * @brief      Test: upcase_pci
 */
TEST(fpgainfo_c, upcase_pci0) {
    char input[256];
    strcpy(input, "pcie 0");
    upcase_pci(input, strnlen(input, 256));
    EXPECT_STREQ(input, "PCIe 0");
}

/**
 * @test       upcase_pci1
 * @brief      Test: upcase_pci
 */
TEST(fpgainfo_c, upcase_pci1) {
    char input[256];
    strcpy(input, "  pcie 0 pcie 1 pcie 2  ");
    upcase_pci(input, strnlen(input, 256));
    EXPECT_STREQ(input, "  PCIe 0 PCIe 1 PCIe 2  ");
}

/**
 * @test       upcase_pci2
 * @brief      Test: upcase_pci
 */
TEST(fpgainfo_c, upcase_pci2) {
    char input[256];
    strcpy(input, " pc ");
    upcase_pci(input, strnlen(input, 256));
    EXPECT_STREQ(input, " pc ");
}

/**
 * @test       upcase_first0
 * @brief      Test: upcase_first
 */
TEST(fpgainfo_c, upcase_first0) {
    char input[256];
    strcpy(input, "one two three four");
    upcase_first(input);
    EXPECT_STREQ(input, "One Two Three Four");
}

/**
 * @test       upcase_first1
 * @brief      Test: upcase_first
 */
TEST(fpgainfo_c, upcase_first1) {
    char input[256];
    strcpy(input, "One Two Three Four");
    upcase_first(input);
    EXPECT_STREQ(input, "One Two Three Four");
}

/**
 * @test       upcase_first2
 * @brief      Test: upcase_first
 */
TEST(fpgainfo_c, upcase_first2) {
    char input[256];
    strcpy(input, "");
    upcase_first(input);
    EXPECT_STREQ(input, "");
}

/**
 * @test       str_in_list0
 * @brief      Test: upcase_first
 */
TEST(fpgainfo_c, str_in_list0) {
    const char *slist[] = { "one", "two", "two", "three", "four" };
    int idx = str_in_list("one", slist, 5);
    EXPECT_EQ(idx, 0);

    idx = str_in_list("two", slist, 5);
    EXPECT_EQ(idx, 1);

    idx = str_in_list("three", slist, 5);
    EXPECT_EQ(idx, 3);

    idx = str_in_list("four", slist, 5);
    EXPECT_EQ(idx, 4);

    idx = str_in_list("five", slist, 5);
    EXPECT_EQ(idx, INT_MAX);
}


INSTANTIATE_TEST_CASE_P(fpgainfo_c, fpgainfo_c_p,
        ::testing::ValuesIn(test_platform::keys(true)));
