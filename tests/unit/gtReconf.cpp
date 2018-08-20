// Copyright(c) 2018, Intel Corporation
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
#include <opae/enum.h>
#include <opae/properties.h>


extern fpga_result set_afu_userclock(fpga_handle handle,
									uint64_t usrlclock_high,
									uint64_t usrlclock_low);

extern 	fpga_result set_fpga_pwr_threshold(fpga_handle handle,
										uint64_t gbs_power);

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;


/**
* @test    gbs_reconf_01
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Then the return value  FPGA_OK if set or
*..........Returns error code
*/
TEST(LibopaecReconfCommonMOCK, gbs_reconf_01) {

	fpga_handle h = NULL;
	uint64_t usrlclock_high = 0;
	uint64_t usrlclock_low = 0;
	struct _fpga_token _tok;
	fpga_token tok = &_tok;


	//EXPECT_EQ(FPGA_INVALID_PARAM, set_afu_userclock(h, usrlclock_high, usrlclock_low));

	// Open  port device
	token_for_fme0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));


		
	EXPECT_EQ(FPGA_INVALID_PARAM, set_afu_userclock(h, usrlclock_high, usrlclock_low));

	usrlclock_high = 300;

	EXPECT_NE(FPGA_OK, set_afu_userclock(h, usrlclock_high, usrlclock_low));

	ASSERT_EQ(FPGA_OK, fpgaClose(h));

}

/**
* @test    gbs_reconf_02
* @brief   Tests: set_fpga_pwr_threshold
* @details set_fpga_pwr_threshold sets power threshold
*          Then the return value  FPGA_OK if set or
*..........Returns error code
*/
TEST(LibopaecReconfCommonMOCK, gbs_reconf_02) {

	fpga_handle h;
	struct _fpga_token _tok;
	fpga_token tok = &_tok;

	uint64_t gbs_power = 40;

	// NULL handle
	EXPECT_EQ(FPGA_INVALID_PARAM, set_fpga_pwr_threshold(NULL, gbs_power));

	// Open  port device
	token_for_fme0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Zero GBS power
	gbs_power = 0;
	EXPECT_EQ(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));

	// Max GBS power
	gbs_power = 200;
	EXPECT_NE(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));

	gbs_power = 65;
	EXPECT_NE(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));

	gbs_power = 60;
	EXPECT_EQ(FPGA_OK, set_fpga_pwr_threshold(h, gbs_power));

	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
