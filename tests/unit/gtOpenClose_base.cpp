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
