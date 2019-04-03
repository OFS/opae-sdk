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
#include <opae/umsg.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;

/**
 * @test       Umsg_drv_01
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaUmsgGetNumber returns number of umsgs supported by
 *             slot.
 *
 */
TEST(LibopaecUmsgCommonMOCK, Umsg_drv_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t Umsg_num = 0;

  // Open  port device
  token_for_afu0(&_tok);
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // get umsg number
  EXPECT_EQ(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));
  EXPECT_GT(Umsg_num, 0);

  // close
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_drv_02
 *
 * @brief      When the parameters are invalid and the drivers are
 *             loaded, fpgaUmsgGetNumber returns error.
 *
 */
TEST(LibopaecUmsgCommonMOCK, Umsg_drv_02) {
  struct _fpga_token _tok;
  fpga_handle h = NULL;
  fpga_token tok = &_tok;
  uint64_t Umsg_num = 0;
  int fddev = -1;

  token_for_afu0(&_tok);

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(NULL, &Umsg_num));

  // Invalid Magic Number
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  struct _fpga_handle* _handle = (struct _fpga_handle*)h;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));

  _handle->magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  _handle = (struct _fpga_handle*)h;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, &Umsg_num));

  _handle->fddev = fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invlaid Input Paramter
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, NULL));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_drv_03
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaUmsgSetAttributes sets umsg hit  Enable / Disable.
 *
 */
TEST(LibopaecUmsgCommonMOCK, Umsg_drv_03) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t Umsghit_Enable = 0xffff;
  uint64_t Umsghit_Disble = 0;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Set umsg hint
  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Enable));
  EXPECT_EQ(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

  // close
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_drv_04
 *
 * @brief      When the parameters are Invalid and the drivers are
 *             loaded, fpgaUmsgSetAttributes retuns error.
 *
 */
TEST(LibopaecUmsgCommonMOCK, Umsg_drv_04) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t Umsghit_Disble = 0;
  int fddev = -1;

  // Open  port device
  token_for_afu0(&_tok);

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(NULL, Umsghit_Disble));

  // Invalid Magic Number
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  struct _fpga_handle* _handle = (struct _fpga_handle*)h;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

  _handle->magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  _handle = (struct _fpga_handle*)h;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, Umsghit_Disble));

  _handle->fddev = fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invlaid Input Paramter
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, 0xFFFFFFFF));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_drv_05
 *
 * @brief      When the parameters are valid and the drivers are loaded,
 *             fpgaGetUmsgPtr returns umsg address.
 *
 */
TEST(LibopaecUmsgCommonHW, Umsg_drv_05) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* umsg_ptr = NULL;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  // Get umsg buffer
  EXPECT_EQ(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));
  EXPECT_TRUE(umsg_ptr != NULL);
  printf("umsg_ptr %p", umsg_ptr);

  // Close
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_drv_06
 *
 * @brief      When the parameters are invalid and the drivers are
 *             loaded, fpgaGetUmsgPtr returns uerror.
 *
 */
TEST(LibopaecUmsgCommonMOCK, Umsg_drv_06) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* umsg_ptr = NULL;
  int fddev = -1;

  // Open  port device
  token_for_afu0(&_tok);

  // NULL Driver hnadle
  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(NULL, &umsg_ptr));

  // Invalid Magic Number
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  struct _fpga_handle* _handle = (struct _fpga_handle*)h;
  _handle->magic = 0x123;

  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));

  _handle->magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Driver handle
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
  _handle = (struct _fpga_handle*)h;

  fddev = _handle->fddev;
  _handle->fddev = -1;

  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));

  _handle->fddev = fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));

  // Invalid Input Parameter
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, NULL));

  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       Umsg_07
 *
 * @brief      When the handle parameter to fpgaTriggerUmsg<br>
 *             is NULL, the function returns FPGA_INVALID_PARAM.<br>
 *
 */
TEST(LibopaecUmsgCommonALL, Umsg_07) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaTriggerUmsg(NULL, 0));
}

/**
 * @test       Umsg_08
 *
 * @brief      When the handle parameter to fpgaTriggerUmsg<br>
 *             has an invalid fddev,<br>
 *             Then the function returns FPGA_INVALID_PARAM.<br>
 *
 */
TEST(LibopaecUmsgCommonALL, Umsg_08) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  struct _fpga_handle *_h;

  // Open  port device
  token_for_afu0(&_tok);
  EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

  _h = (struct _fpga_handle *) h;
  int save_fddev = _h->fddev;

  _h->fddev = -1;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaTriggerUmsg(h, 0));

  _h->fddev = save_fddev;
  EXPECT_EQ(FPGA_OK, fpgaClose(h));
}
