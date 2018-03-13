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

