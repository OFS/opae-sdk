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
#include <opae/manage.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

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
