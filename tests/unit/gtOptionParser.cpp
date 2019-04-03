// Copyright(c) 2017, Intel Corporation
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
TEST(LibopaecCppOptionsCommonMOCKHW, default_option_01) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_json_02) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_json_03) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_arg_04) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_arg_05) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_arg_06) {
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
TEST(LibopaecCppOptionsCommonMOCKHW, parse_arg_07) {
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
