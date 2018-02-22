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

#include "any_value.h"

using namespace intel::utils;

/**
 * @test       any_value_01
 *
 * @brief      Given an any_value variable assigned from a uint64_t value<br>
 *             When I get the value as a uint64_t type<br>
 *             Then the value returned is equal to the original value<br>
 *
 */
TEST(CxxCommonAll, any_value_01) {
  any_value a = uint64_t(0x100);
  auto b = a.value<uint64_t>();
  EXPECT_EQ(0x100, b);
}

/**
 * @test       any_value_02
 *
 * @brief      Given an any_value variable assigned from a uint64_t value<br>
 *             When I get the value as a string type<br>
 *             Then an exception of type any_value_cast_error is thrown<br>
 *
 */
TEST(CxxCommonAll, any_value_02) {
  any_value a = uint64_t(0x100);
  EXPECT_THROW(std::string b = a.value<std::string>(), any_value_cast_error);
}

/**
 * @test       any_value_03
 *
 * @brief      Given an any_value variable assigned from a string value<br>
 *             When I get the value as a string type<br>
 *             Then the value returned is equal to the original value<br>
 *
 */
TEST(CxxCommonAll, any_value_03) {
  any_value a = std::string("any");
  auto b = a.value<std::string>();
  EXPECT_STREQ("any", b.c_str());
}

/**
 * @test       any_value_04
 *
 * @brief      Given an any_value variable assigned from a const char* value<br>
 *             When I get the value as a const char* type<br>
 *             Then the value returned is equal to the original value<br>
 *
 */
TEST(CxxCommonAll, any_value_04) {
  any_value a = static_cast<const char*>("any");
  auto b = a.value<const char*>();
  EXPECT_STREQ("any", b);
}

/**
 * @test       any_value_05
 *
 * @brief      Given an any_value variable assigned from an int value<br>
 *             And I assign that any_value to another variable of type any_value<br>
 *             When I get the value as an int type<br>
 *             Then the value returned is equal to the original value<br>
 *
 */
TEST(CxxCommonAll, any_value_05) {
  any_value a = static_cast<int>(100);
  any_value b = a;
  int c = b.value<int>();
  EXPECT_EQ(100, c);
}

/**
 * @test       any_value_06
 *
 * @brief      Given an any_value variable assigned from an int value<br>
 *             When I check if the type is int
 *             Then the result is true<br>
 *
 */
TEST(CxxCommonAll, any_value_06) {
  any_value a = static_cast<int>(100);
  EXPECT_TRUE(a.is_type<int>());
}
