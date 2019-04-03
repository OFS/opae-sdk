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
 *             When I check if the type is int<br>
 *             Then the result is true<br>
 *
 */
TEST(CxxCommonAll, any_value_06) {
  any_value a = static_cast<int>(100);
  EXPECT_TRUE(a.is_type<int>());
}

/**
 * @test       any_value_07
 *
 * @brief      Given an any_value variable assigned from an int value<br>
 *             When I check if the type is a type other than int<br>
 *             Then the result is false<br>
 *
 */
TEST(CxxCommonAll, any_value_07) {
  any_value a = static_cast<int>(100);
  EXPECT_FALSE(a.is_type<std::string>());
}
