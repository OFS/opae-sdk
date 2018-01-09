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
#include <opae/manage.h>

#include "common_test.h"
#include "gtest/gtest.h"
#ifdef BUILD_ASE
#include "ase/api/src/types_int.h"
#else
#include "types_int.h"
#endif

using namespace common_test;

/**
 * @test       Port_drv_reset_01
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaReset Resets fpga slot.
 *
 */
TEST(LibopaecResetCommonALL, Port_drv_reset_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reset slot
  EXPECT_EQ(FPGA_OK, fpgaReset(h));

  // close
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Port_drv_reset_02
 *
 * @brief      When the parameters are invalid and the drivers are
 *             loaded, fpgaReset return error.
 *
 */
TEST(LibopaecResetCommonALL, Port_drv_reset_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;
  int fddev = -1;

  // Open port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Reset slot
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReset(NULL));

  // close
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Magic Number
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  struct _fpga_handle* _handle = (struct _fpga_handle*)h;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaReset(h));

  _handle->magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  _handle = (struct _fpga_handle*)h;

#ifndef BUILD_ASE
  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaReset(h));
#else
  EXPECT_EQ(FPGA_OK, fpgaReset(h));
#endif
  _handle->fddev = fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}
