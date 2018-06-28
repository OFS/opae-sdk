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

using namespace common_test;
using namespace std;

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};


class LibopaecCommonMOCKERRINJ : public BaseFixture, public ::testing::Test {
protected:
	virtual void SetUp() {
		MOCK_enable_ioctl_errinj(true);
	}

	virtual void TearDown() {
		MOCK_enable_ioctl_errinj(false);
	}
};


/**
* @test    fpga_mock_errinj_01
* @brief   Tests: fpgaReset
* @details fpgaReset resets fpga afu
*          Then the return error code
*/
TEST_F(LibopaecCommonMOCKERRINJ, fpga_mock_errinj_01) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Reset
	EXPECT_NE(FPGA_OK, fpgaReset(h));

	// close port
	ASSERT_EQ(FPGA_OK, fpgaClose(h));

}

/**
* @test    fpga_mock_errinj_02
* @brief   Tests: fpgaMapMMIO
* @details fpgaMapMMIO maps fpga afu mmio region
*          Then the return error code
*/
TEST_F(LibopaecCommonMOCKERRINJ, fpga_mock_errinj_02) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint32_t mmio_num;
	uint64_t *mmio_ptr;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// mmap 
	mmio_num = 0;
	EXPECT_NE(FPGA_OK, fpgaMapMMIO(h, mmio_num, &mmio_ptr));

	// close port
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}


/**
* @test    fpga_mock_errinj_03
* @brief   Tests: fpgaPrepareBuffer and fpgaReleaseBuffer
* @details API allcocats buffer and Release buffer
*          Then the return error code
*/
TEST_F(LibopaecCommonMOCKERRINJ, fpga_mock_errinj_03) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t* buf_addr;
	uint64_t wsid = 1;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate a buffer
	buf_len = 1024;
	EXPECT_NE(FPGA_OK,
		fpgaPrepareBuffer(h, buf_len, (void**) &buf_addr, &wsid, 0));

	// Release buffer
	EXPECT_NE(FPGA_OK, fpgaReleaseBuffer(h, wsid));

	// Prepare buffer successful 
	// Relase buffer fails
	MOCK_enable_ioctl_errinj(false);

	// Allocate a buffer
	buf_len = 1024;
	EXPECT_EQ(FPGA_OK,
		fpgaPrepareBuffer(h, buf_len, (void**) &buf_addr, &wsid, 0));

	MOCK_enable_ioctl_errinj(true);
	// Release buffer
	EXPECT_NE(FPGA_OK, fpgaReleaseBuffer(h, wsid));

	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}


/**
* @test    fpga_mock_errinj_04
* @brief   Tests:fpgaGetNumUmsg,fpgaSetUmsgAttributes
*...........fpgaGetUmsgPtr and fpgaTriggerUmsg
* @details API Set,Get and Trigger UMSG
*          Then the return error code
*/
TEST_F(LibopaecCommonMOCKERRINJ, fpga_mock_errinj_04) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t *value = 0;
	uint64_t *umsg_ptr;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Get Number of UMSG
	EXPECT_NE(FPGA_OK, fpgaGetNumUmsg(h, value));

	// Set UMSG
	EXPECT_NE(FPGA_OK, fpgaSetUmsgAttributes(h, 0));

	// Get UMSG pointer
	EXPECT_NE(FPGA_OK, fpgaGetUmsgPtr(h, &umsg_ptr));

	// Trigger UMSG
	EXPECT_NE(FPGA_OK, fpgaTriggerUmsg(h, 0));

	// Close port handle
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
* @test    fpga_mock_errinj_05
* @brief   Tests:fpgaAssignPortToInterface
* @details fpgaAssignPortToInterface Assign and Release port
*          Then the return error code
*/
TEST_F(LibopaecCommonMOCKERRINJ, fpga_mock_errinj_05) {

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;

	// Open port device
	token_for_fme0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	EXPECT_NE(FPGA_OK,
		fpgaAssignPortToInterface(h, 0, 0, 0));

	EXPECT_NE(FPGA_OK,
		fpgaAssignPortToInterface(h, 1, 0, 0));

	// Close port handle
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
