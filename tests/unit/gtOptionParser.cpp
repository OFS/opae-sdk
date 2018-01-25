/*++

INTEL CONFIDENTIAL
Copyright 2016 - 2017 Intel Corporation

The source code contained or described  herein and all documents related to
the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
Corporation or  its suppliers  and licensors.  The Material  contains trade
secrets  and  proprietary  and  confidential  information  of Intel  or its
suppliers and licensors.  The Material is protected  by worldwide copyright
and trade secret laws and treaty provisions. No part of the Material may be
used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
transmitted,  distributed, or  disclosed in  any way  without Intel's prior
express written permission.

No license under any patent, copyright,  trade secret or other intellectual
property  right  is  granted to  or conferred  upon  you by  disclosure  or
delivery of the  Materials, either  expressly, by  implication, inducement,
estoppel or otherwise. Any license  under such intellectual property rights
must be express and approved by Intel in writing.

--*/

#include "gtest/gtest.h"
#include "option_map.h"
#include "option_parser.h"

extern int optind;

using namespace intel::utils;

/**
 * @test       default_option_01
 *
 * @brief      Given an options map with one option (with no default
 *             value) When I try to get the value The option value is
 *             the default value for that type
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, default_option_01) {
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument,
                            "test option help");
  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(0, opts["answer"]->value<uint32_t>());
}

/**
 * @test       parse_json_02
 *
 * @brief      Given an options map with one option (with a default
 *             value) And a JSON string with the value in it When I
 *             parse the string The option value is updated to the value
 *             in the JSON string
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_json_02) {
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument, "test option help",
                            42);
  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
  option_parser parser;
  ASSERT_TRUE(parser.parse_json("{\"answer\": 43}", opts));

  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}

/**
 * @test       parse_json_03
 *
 * @brief      Given an options map with one option (with a default
 *             value) And a JSON string with the value not in it When I
 *             parse the string The option value is not updated to the
 *             value in the JSON string And the is_set function returns
 *             false
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_json_03) {
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument, "test option help",
                            42);
  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
  option_parser parser;
  ASSERT_TRUE(parser.parse_json("{\"not_answer\": 43}", opts));

  EXPECT_NE(43, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
  EXPECT_FALSE(opts["answer"]->is_set());
}

/**
 * @test       parse_arg_04
 *
 * @brief      Given an options map with one option (with a default
 *             value) And the option has no short option And a command
 *             line argument vector with the long option When I parse
 *             the arguments vector The option value is updated to the
 *             value in the arguments vector
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_arg_04) {
  optind = 1;
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument, "test option help",
                            42);
  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
  option_parser parser;
  const char* args[] = {"program", "--answer=43", nullptr};

  ASSERT_TRUE(parser.parse_args(2, const_cast<char**>(args), opts));

  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}

/**
 * @test       parse_arg_05
 *
 * @brief      Given an options map with one option (with a default
 *             value) And the option has a short option And a command
 *             line argument vector with the short option When I parse
 *             the arguments vector The option value is updated to the
 *             value in the arguments vector
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_arg_05) {
  optind = 1;
  option_map opts;
  opts.add_option<uint32_t>("answer", 'a', option::with_argument,
                            "test option help", 42);
  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
  option_parser parser;
  const char* args[] = {"program", "-a", "43", nullptr};

  ASSERT_TRUE(parser.parse_args(3, const_cast<char**>(args), opts));

  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}

/**
 * @test       parse_arg_06
 *
 * @brief      Given an options map with options And an arguments vector
 *             with non-option strings after the options When I parse
 *             the arguments vector The leftover arguments vector in the
 *             parser is equal to the non-options
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_arg_06) {
  optind = 1;
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument, "test option help",
                            42);
  option_parser parser;
  const char* args[] = {"program", "--answer=43", "one",
                        "two",     "three",       nullptr};

  EXPECT_EQ(0, parser.leftover().size());
  ASSERT_TRUE(parser.parse_args(5, const_cast<char**>(args), opts));

  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
  EXPECT_STREQ("one", parser.leftover()[0].c_str());
  EXPECT_STREQ("two", parser.leftover()[1].c_str());
  EXPECT_STREQ("three", parser.leftover()[2].c_str());
}

/**
 * @test       parse_arg_07
 *
 * @brief      Given an options map with options And an arguments vector
 *             with non-option strings intermixed with the options When
 *             I parse the arguments vector The leftover arguments
 *             vector in the parser is equal to the non-options
 *
 */
TEST(LibopaecCppOptionsCommonMOCK, parse_arg_07) {
  optind = 1;
  option_map opts;
  opts.add_option<uint32_t>("answer", option::with_argument, "test option help",
                            42);
  option_parser parser;
  const char* args[] = {"program", "one",   "--answer=43",
                        "two",     "three", nullptr};

  EXPECT_EQ(0, parser.leftover().size());
  ASSERT_TRUE(parser.parse_args(5, const_cast<char**>(args), opts));

  ASSERT_NE(nullptr, opts["answer"]);
  EXPECT_NE(42, opts["answer"]->value<uint32_t>());
  EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
  ASSERT_EQ(3, parser.leftover().size());
  EXPECT_STREQ("one", parser.leftover()[0].c_str());
  EXPECT_STREQ("two", parser.leftover()[1].c_str());
  EXPECT_STREQ("three", parser.leftover()[2].c_str());
}
