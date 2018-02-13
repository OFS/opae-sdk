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
#include "types_int.h"

using namespace common_test;

/**
 * @test       Hostif_drv_01
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaAssignPortToInterface Release port to a host interface.
 *
 */
TEST(LibopaecHostifCommonMOCK, Hostif_drv_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Release Port Interface
  EXPECT_EQ(FPGA_OK, fpgaAssignPortToInterface(h, 1, 0, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Hostif_drv_02
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaAssignPortToInterface Assign port to a host interface.
 *
 */
TEST(LibopaecHostifCommonMOCK, Hostif_drv_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  // open FME device
  token_for_fme0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Assign Port Interface
  EXPECT_EQ(FPGA_OK, fpgaAssignPortToInterface(h, 0, 0, 0));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

