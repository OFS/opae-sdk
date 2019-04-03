// Copyright(c) 2017-2018, Intel Corporation
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

#include <opae/access.h>

#include "common_utils.h"
#include "common_sys.h"
#include "gtest/gtest.h"


using namespace common_utils;
using namespace std;

class LibopaecOpenFCommonHW : public common_utils::BaseFixture,
                              public ::testing::Test {};

class LibopaecCloseFCommonHW : public common_utils::BaseFixture,
                               public ::testing::Test {};

class LibopaecOpenFCommonALL : public common_utils::BaseFixture,
                               public ::testing::Test {};

class LibopaecCloseFCommonALL : public common_utils::BaseFixture,
                                public ::testing::Test {};

/**
 * @test       02
 *
 * @brief      Calling fpgaOpen() after fpgaClose() using the same token
 *             returns FPGA_OK.
 */
TEST_F(LibopaecOpenFCommonHW, 02) {
  auto functor = [=]() -> void {
    fpga_handle h = NULL;
    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    fpgaClose(h);
    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    fpgaClose(h);
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

TEST_F(LibopaecOpenFCommonHW, 03) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;
    fpga_properties props = NULL;
    uint8_t obus = 0;

    // open exclusive in gtest process
    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(fpgaGetProperties(tokens[index], &props),
                       LINE(__LINE__));

      checkReturnCodes(fpgaPropertiesGetBus(props, &obus), LINE(__LINE__));

      ASSERT_LT(
          0,
          tryOpen(
              false,
              obus));  // open exclusive (shared == false) in external process
                       // expect fail
      ASSERT_LT(
          0,
          tryOpen(true,
                  obus));  // open shared (shared == true) in external process
                           // expect fail

      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    // open shared in gtest process
    if (checkReturnCodes(fpgaOpen(tokens[index], &h, FPGA_OPEN_SHARED),
                         LINE(__LINE__))) {
      checkReturnCodes(fpgaGetProperties(tokens[index], &props),
                       LINE(__LINE__));

      checkReturnCodes(fpgaPropertiesGetBus(props, &obus), LINE(__LINE__));

      ASSERT_LT(
          0,
          tryOpen(
              false,
              obus));  // open exclusive (shared == false) in external process
                       // expect fail
      ASSERT_EQ(
          FPGA_OK,
          tryOpen(true,
                  obus));  // open shared (shared == true) in external process
                           // expect PASS

      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       05
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaOpen returns FPGA_OK.
 */
TEST_F(LibopaecOpenFCommonALL, 05) {

#ifdef BUILD_ASE
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  fpgaClose(h);

#else
  auto functor = [=]() -> void {
    fpga_handle h = NULL;
    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    fpgaClose(h);
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
#endif
}

/**
 * @test       06
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             but the user lacks sufficient privilege for the device,
 *             fpgaOpen returns FPGA_NO_ACCESS.
 */
TEST_F(LibopaecOpenFCommonHW, 06) {
  auto functor = [=]() -> void {
    fpga_handle h;

    EXPECT_EQ(FPGA_NO_ACCESS, fpgaOpen(tokens[index], &h, 0));
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       07
 *
 * @brief      When the parameters are valid but the drivers are not
 *             loaded, fpgaOpen returns FPGA_NO_DRIVER.
 */
TEST(LibopaecOpenCommonALL, 07) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  token_for_afu0(&_tok);
#ifdef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#else
  EXPECT_EQ(FPGA_NO_DRIVER, fpgaOpen(tok, &h, 0));
#endif
}

/**
 * @test       01
 *
 * @brief      When the fpga_handle parameter to fpgaClose is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecCloseCommonALL, 01) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaClose(NULL));
}

/**
 * @test       02
 *
 * @brief      When the parameters are valid, the drivers are loaded,
 *             and a previous call to fpgaOpen has been made, fpgaClose
 *             returns FPGA_OK.
*/
TEST_F(LibopaecCloseFCommonALL, 02) {

#ifdef BUILD_ASE
  fpga_handle h;

  struct _fpga_token _tok;
  fpga_token tok = &_tok;

  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

#else
  auto functor = [=]() -> void {
    fpga_handle h;

    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    EXPECT_EQ(FPGA_OK, fpgaClose(h));

  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
#endif
}

/**
 * @test       03
 *
 * @brief      When MMIO spaces have been mapped by an open handle,
 *             and there is no explicit call to unmap them,
 *             fpgaClose will unmap all MMIO spaces.
*/
#ifndef BUILD_ASE
TEST_F(LibopaecCloseFCommonALL, 03) {

  auto functor = [=]() -> void {
    fpga_handle h;
    struct _fpga_handle *p;

    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));

    p = (struct _fpga_handle *)h;
    EXPECT_EQ((void *)NULL, p->mmio_root);

    EXPECT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, NULL));

    EXPECT_NE((void *)NULL, p->mmio_root);

    EXPECT_EQ(FPGA_OK, fpgaClose(h));

    EXPECT_EQ((void *)NULL, p->mmio_root);
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}
#endif // BUILD_ASE


