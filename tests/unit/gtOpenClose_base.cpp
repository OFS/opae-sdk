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

#ifdef __cplusplus
extern "C" {
#endif
#include <opae/access.h>

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;

/**
 * @test       open_01
 *
 * @brief      When the fpga_handle * parameter to fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecOpenCommonALL, open_01) {
  fpga_token tok = NULL;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, NULL, 0));
}

/**
 * @test       open_06
 *
 * @brief      When the fpga_token parameter to fpgaOpen is NULL, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecOpenCommonALL, open_06) {
  fpga_handle h;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(NULL, &h, 0));
}

/**
 * @test       open_08
 *
 * @brief      When the flags parameter to fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecOpenCommonALL, open_08) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h;

  token_for_afu0(&_tok);
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, &h, 42));
}
