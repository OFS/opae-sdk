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

#include <opae/access.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;
using namespace std;

class LibopaecOpenFCommonHW : public common_test::BaseFixture,
                              public ::testing::Test {};

class LibopaecCloseFCommonHW : public common_test::BaseFixture,
                               public ::testing::Test {};

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
TEST_F(LibopaecOpenFCommonHW, 05) {
  auto functor = [=]() -> void {
    fpga_handle h = NULL;
    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    fpgaClose(h);
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
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
TEST(LibopaecOpenCommonHW, 07) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  token_for_afu0(&_tok);
  EXPECT_EQ(FPGA_NO_DRIVER, fpgaOpen(tok, &h, 0));
}

/**
 * @test       open_drv_09
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             and the flag FPGA_OPEN_SHARED is not given, fpgaOpen on
 *             an already opened token returns FPGA_BUSY.
 *
 */
TEST_F(LibopaecOpenFCommonHW, open_drv_09) {
  auto functor = [=]() -> void {
    fpga_handle h1, h2;

    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h1, 0));
    EXPECT_EQ(FPGA_BUSY, fpgaOpen(tokens[index], &h2, 0));
    fpgaClose(h1);
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       open_drv_10
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             and the flag FPGA_OPEN_SHARED is given, fpgaOpen on an
 *             already opened token returns FPGA_OK.
 *
 */
TEST_F(LibopaecOpenFCommonHW, open_drv_10) {
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
TEST_F(LibopaecOpenFCommonHW, open_drv_11) {
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
TEST(LibopaecCloseCommonHW, 01) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaClose(NULL));
}

/**
 * @test       02
 *
 * @brief      When the parameters are valid, the drivers are loaded,
 *             and a previous call to fpgaOpen has been made, fpgaClose
 *             returns FPGA_OK.
 */
TEST_F(LibopaecCloseFCommonHW, 02) {
  auto functor = [=]() -> void {
    fpga_handle h;

    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    EXPECT_EQ(FPGA_OK, fpgaClose(h));

  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}
