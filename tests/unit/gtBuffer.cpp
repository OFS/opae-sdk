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

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;

/**
* @test    fpga_buffer_01
* @brief   Tests: fpgaPrepareBuffer and fpgaReleaseBuffer
*          fpgaGetIOAddress
* @details Buffer functions returns FPGA_INVALID_PARAM for
*..........invalid input
*/
TEST(LibopaecBufferCommonMOCK, fpga_buffer_01) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len = 1024;
	uint64_t* buf_addr;
	uint64_t wsid = 1;
	int flags = 0;
	uint64_t *ioaddr = NULL;
	uint64_t* invalid_buf_addr = NULL;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// NULL Handle
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(NULL, 0, (void**) &buf_addr, &wsid, 0));

	// NULL Workspaceid
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, buf_len, (void**) &buf_addr, NULL, 0));

	// Invlaid Flags
	flags = 0x100;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, buf_len, (void**) &buf_addr, &wsid, flags));

	// Buffer lenth is zero
	flags = FPGA_BUF_PREALLOCATED;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, 0, (void**) &buf_addr, &wsid, flags));

	// Not Page aligned buffer
	buf_len = 11247;
	flags = FPGA_BUF_PREALLOCATED;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, buf_len, (void**) &buf_addr, &wsid, flags));

	// Invalid input buffer pointer
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, buf_len, (void**) &invalid_buf_addr, &wsid, flags));

	// special test case
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, 0, (void**) NULL, &wsid, flags));

	// Buffer lenth is zero
	flags = FPGA_BUF_QUIET;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h, 0, (void**) NULL, &wsid, flags));

	// Invalid Handle
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetIOAddress(NULL, wsid, ioaddr));

	// Invalid workspace id
	EXPECT_NE(FPGA_OK, fpgaGetIOAddress(h, 0x10000, ioaddr));

	// NULL Handle
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(NULL, wsid));

	// Invalid workspace id
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(h, 0x10001));

	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}