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

#include "gtest/gtest.h"
#include <opae/cxx/core/except.h>

using namespace opae::fpga::types;

/**
 * @test except_01
 * Given a src_location object<br>
 * When the object is constructed with OPAECXX_HERE<br>
 * Then it represents the current location in the source file.<br>
 */
TEST(except_cxx_core, except_01) {
  src_location loc(OPAECXX_HERE);

  EXPECT_STREQ("test_except_cxx_core.cpp", loc.file());
  EXPECT_STREQ("TestBody", loc.fn());
  EXPECT_EQ(39, loc.line());
}

/**
 * @test except_02
 * Given an except object<br>
 * When the object is constructed with a src_location only<br>
 * Then then the fpga_result value is FPGA_EXCEPTION.<br>
 */
TEST(except_cxx_core, except_02) {
  except e(OPAECXX_HERE);

  EXPECT_EQ(FPGA_EXCEPTION, e);
}

/**
 * @test except_03
 * Given an except object<br>
 * When the object is constructed with an fpga_result and src_location<br>
 * Then then the fpga_result value matches the value passed to the
 * constructor<br> And the string returned by what() represents the fpga_result
 * and src_location.<br>
 */
TEST(except_cxx_core, except_03) {
  except e(FPGA_INVALID_PARAM, OPAECXX_HERE);

  EXPECT_EQ(FPGA_INVALID_PARAM, e);
  EXPECT_STREQ("failed with error invalid parameter at: "
               "test_except_cxx_core.cpp:TestBody():67",
               e.what());
}

/**
 * @test except_04
 * Given a src_location<br>
 * When the object is copy-assigned<br>
 * Then the new object is a copy of the original.<br>
 */
TEST(except_cxx_core, except_04) {
  src_location locA(__FILE__, __func__, __LINE__);
  src_location locB("someotherfile.txt", "some_other_fn", __LINE__);

  locB = locA;
  EXPECT_STREQ(locB.file(), locA.file());
  EXPECT_STREQ(locB.fn(), locA.fn());
  EXPECT_EQ(locB.line(), locA.line());
}

/**
 * @test assert_ok_invalid_param
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_INVALID_PARAM
 * Then an exception of type invalid_param is thrown
 */
TEST(except_cxx_core, assert_ok_invalid_param) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_INVALID_PARAM), invalid_param);
}

/**
 * @test assert_ok_busy
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_BUSY
 * Then an exception of type busy is thrown
 */
TEST(except_cxx_core, assert_ok_busy) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_BUSY), busy);
}

/**
 * @test assert_ok_exception
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_EXCEPTION
 * Then an exception of type exception is thrown
 */
TEST(except_cxx_core, assert_ok_exception) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_EXCEPTION), exception);
}

/**
 * @test assert_ok_not_found
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NOT_FOUND
 * Then an exception of type not_found is thrown
 */
TEST(except_cxx_core, assert_ok_not_found) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NOT_FOUND), not_found);
}

/**
 * @test assert_ok_no_memory
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NO_MEMORY
 * Then an exception of type no_memory is thrown
 */
TEST(except_cxx_core, assert_ok_no_memory) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NO_MEMORY), no_memory);
}

/**
 * @test assert_ok_not_supported
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NOT_SUPPORTED
 * Then an exception of type not_supported is thrown
 */
TEST(except_cxx_core, assert_ok_not_supported) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NOT_SUPPORTED), not_supported);
}

/**
 * @test assert_ok_no_driver
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NO_DRIVER
 * Then an exception of type no_driver is thrown
 */
TEST(except_cxx_core, assert_ok_no_driver) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NO_DRIVER), no_driver);
}

/**
 * @test assert_ok_no_daemon
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NO_DAEMON
 * Then an exception of type no_daemon is thrown
 */
TEST(except_cxx_core, assert_ok_no_daemon) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NO_DAEMON), no_daemon);
}

/**
 * @test assert_ok_no_access
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_NO_ACCESS
 * Then an exception of type no_access is thrown
 */
TEST(except_cxx_core, assert_ok_no_access) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_NO_ACCESS), no_access);
}

/**
 * @test assert_ok_reconf_error
 * Given an assertion macro, ASSERT_FPGA_OK
 * When I use the macro with a result code of FPGA_RECONF_ERROR
 * Then an exception of type reconf_error is thrown
 */
TEST(except_cxx_core, assert_ok_reconf_error) {
  EXPECT_THROW(ASSERT_FPGA_OK(FPGA_RECONF_ERROR), reconf_error);
}
