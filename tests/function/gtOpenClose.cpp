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

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"


using namespace common_test;
using namespace std;

class LibopaecOpenFCommonMOCKHW : public common_test::BaseFixture,
                              public ::testing::Test {};

class LibopaecOpenFCommonMOCK : public common_test::BaseFixture,
                              public ::testing::Test {};

class LibopaecCloseFCommonMOCKHW : public common_test::BaseFixture,
                               public ::testing::Test {};

class LibopaecOpenFCommonALL : public common_test::BaseFixture,
                               public ::testing::Test {};

class LibopaecCloseFCommonALL : public common_test::BaseFixture,
                                public ::testing::Test {};

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
 * @test       open_drv_10
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             and the flag FPGA_OPEN_SHARED is given, fpgaOpen on an
 *             already opened token returns FPGA_OK.
 *
 */
TEST_F(LibopaecOpenFCommonALL, open_drv_10) {

#ifdef BUILD_ASE
  fpga_handle h1, h2;
  struct _fpga_token _tok;
  fpga_token tok = &_tok;

  token_for_afu0(&_tok);
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h1, FPGA_OPEN_SHARED));
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h2, FPGA_OPEN_SHARED));
  fpgaClose(h1);
  fpgaClose(h2);

#else

  auto functor = [=]() -> void {
    fpga_handle h1, h2;

    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h1, FPGA_OPEN_SHARED));
    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h2, FPGA_OPEN_SHARED));
    fpgaClose(h1);
    fpgaClose(h2);

  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
#endif
}

/**
 * @test       open_drv_11
 *
 * @brief      Invalid parameter verification.  fpgaOpen returns
 *             FPGA_INVALID_PARAM for a NULL token.  fpgaOpen returns
 *             FPGA_INVALID_PARAM for a NULL handle pointer.  fpgaOpen
 *             returns FPGA_INVALID_PARAM for invalid flags.  fpgaOpen
 *             returns FPGA_INVALID_PARAM for a corrupted token.
 *
 */
TEST_F(LibopaecOpenFCommonMOCK, open_drv_11) {
  auto functor = [=]() -> void {
    fpga_handle h;

    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(NULL, &h, 0));
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tokens[index], NULL, 0));
    EXPECT_EQ(FPGA_INVALID_PARAM,
              fpgaOpen(tokens[index], &h, ~FPGA_OPEN_SHARED));

    ((_fpga_token*)tokens[index])->magic = 0;
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tokens[index], &h, 0));
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
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
 * @test       04
 *
 * @brief      When the handle parameter to fpgaClose has
 *             an fddev member equal to -1,
 *             fpgaClose will fail with FPGA_INVALID_PARAM.
 */
#ifndef BUILD_ASE
TEST_F(LibopaecCloseFCommonALL, 04) {

  auto functor = [=]() -> void {
    fpga_handle h;
    struct _fpga_handle *p;

    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    
    p = (struct _fpga_handle *)h;
    int save_fddev = p->fddev;

    p->fddev = -1;
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaClose(h));

    p->fddev = save_fddev;
    EXPECT_EQ(FPGA_OK, fpgaClose(h));
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}
#endif // BUILD_ASE
