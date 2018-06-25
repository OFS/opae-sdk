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
#undef  _GNU_SOURCE
#include "usrclk/user_clk_pgm_uclock.h"

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;

/**
* @test    afu_usrclk_01
* @brief   Tests: fpac_GetErrMsg and fv_BugLog
* @details fpac_GetErrMsg returns error string
*          fv_BugLog sets bug log
*/
TEST(LibopaecUsrclkCommonMOCKHW, afu_usrclk_01) {

	//Get error string
	const char * pmsg = fpac_GetErrMsg(1);
	EXPECT_EQ(NULL, !pmsg);

	//Get error string
	pmsg = fpac_GetErrMsg(5);
	EXPECT_EQ(NULL, !pmsg);
	
	//Get error string
	pmsg = fpac_GetErrMsg(16);
	EXPECT_EQ(NULL, !pmsg);

	//Get error string for invlaid index
	pmsg = NULL;
	pmsg = fpac_GetErrMsg(17);
	EXPECT_STREQ("ERROR: MSG INDEX OUT OF RANGE", pmsg);

	//Get error string for invlaid index
	pmsg = NULL;
	pmsg = fpac_GetErrMsg(-1);
	EXPECT_STREQ("ERROR: MSG INDEX OUT OF RANGE", pmsg);


	fv_BugLog(1);

	fv_BugLog(2);

}
