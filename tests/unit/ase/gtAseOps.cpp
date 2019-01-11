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

#include <opae/access.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <opae/error.h>
#include <pthread.h>
#include "types_int.h"
#include "ase_common.h"
#include "gtest/gtest.h"

#define ASE_UNIT
#define FPGA_EVENT_INVALID 0x64
#define ESBADFMT -1
#define ESFMTTYP -2

const uint HUGE_NUM = 1000000000;

static const fpga_guid ASE_GUID = {
	0xd8, 0x42, 0x4d, 0xc4, 0xa4,  0xa3, 0xc4, 0x13, 0xf8,0x9e,
	0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b
};

inline void token_for_afu0(struct _fpga_token* _tok) {
	memcpy(_tok->accelerator_id, ASE_GUID, sizeof(fpga_guid));
	_tok->magic = ASE_TOKEN_MAGIC;
	_tok->ase_objtype = FPGA_ACCELERATOR;
};

/**
* @test       ase_rm_sp
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             remove_spaces() function should remove all the whilte space
*
*/
TEST(LibopaecAseOps, ase_rm_sp) {
	char str1[128] = " This is a test string with spaces";
	char *str2 = NULL;

	remove_spaces(str1);
	EXPECT_STREQ("Thisisateststringwithspaces", str1);

	remove_spaces(str2);
}

/**
* @test       ase_rm_tab
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             remove_tabs() function should remove all the tabs
*
*/
TEST(LibopaecAseOps, ase_rm_tab) {
	char str1[128] = "		This is a test string with tabs		";
	char *str2 = NULL;

	remove_tabs(str1);
	EXPECT_STREQ("This is a test string with tabs", str1);

	remove_tabs(str2);
}

/**
* @test       ase_ops_01
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             ase_buffer_oneline(struct buffer_t *) function should be called
*
*/
TEST(LibopaecAseOps, ase_ops_01) {
	struct buffer_t b1, b2;
	b1.valid = ASE_BUFFER_VALID;
	b2.valid = ASE_BUFFER_INVALID;
	
	ase_buffer_oneline(&b1);
	ase_buffer_oneline(&b2);
}

/**
* @test       ase_ops_02
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             ase_malloc(size_t) function should be called
*
*/
TEST(LibopaecAseOps, ase_ops_02) {
	uint size1 = HUGE_NUM;
	
	ase_malloc(size1);
}


/**
* @test       ase_ops_03
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             register_signal() function should be called
*
*/
TEST(LibopaecAseOps, ase_ops_03) {

	register_signal(SIGINT, NULL);
}

/**
* @test       ase_ops_04
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             ase_read_lock_file(const char *) function should be calle
*
*/
TEST(LibopaecAseOps, ase_ops_04) {
	char * workdir = getenv("ASE_WORKDIR");

	ase_read_lock_file(workdir);
}

/**
* @test       ase_mq_01
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             mqueue_create()/mqueue_destroy()  function should create the
*             named pipes
*
*/
TEST(LibopaecAseMq, ase_mq_01) {
	char name_suffix[] = "testmq";

	mqueue_create(name_suffix);
	mqueue_destroy(name_suffix);
}

/**
* @test       ase_str_01
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*            sscanf_s_ii() function should read from a string
*
*/
TEST(LibopaecAseStr, ase_str_01) {
	char str1[128] = "Read two integers 345 234";
	char format[128] = "Read two integers %d %d";
	int a, b;

	sscanf_s_ii(str1, format, &a, &b);
	EXPECT_EQ(a, 345);
	EXPECT_EQ(b, 234);
}

/**
* @test       ase_str_02
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             parse_format() function should parse the format of a string
*
*/
TEST(LibopaecAseStr, ase_str_02) {
	char format[128];
	char pformatList[32];
	int numformat = 0;

	pformatList[0] = '\0';
	strcpy(format, "Read null %\0");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %%");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %#");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %0");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %-");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  % ");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %+");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %4.2h");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04.3h");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04.5l");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04c");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04L");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04c");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04j");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04z");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04t");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %c");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %d");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %d");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %i");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %o");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %4ho");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %u");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04x");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %04X");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %e");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %f");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %5.2f");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %g");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %a");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %s");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %p");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %n");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(1, numformat);

	pformatList[0] = '\0';
	strcpy(format, "Read  %m");
	numformat = parse_format(format, pformatList, 4);
	EXPECT_EQ(0, numformat);
}

/**
* @test       ase_str_03
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             fscanf_s_i() function should read an integer from a file
*
*/
TEST(LibopaecAseStr, ase_str_03) {
	int a = 0;
	int b = 2018;
	FILE *fp = fopen("str_test.txt", "w+");
	fprintf(fp, "%d ", b);

	rewind(fp);
	fscanf_s_i(fp, "%d", &a);
	EXPECT_EQ(2018, a);

	fclose(fp);
}

/**
* @test    ase_buffer_01
* @brief   Tests: fpgaPrepareBuffer and fpgaReleaseBuffer
*          fpgaGetIOAddress
* @details Buffer functions returns FPGA_INVALID_PARAM for
*..........invalid input
*/
TEST(LibopaecAseApi, ase_buffer_01) {
	uint64_t* buf_addr;
	uint64_t wsid = 1;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(NULL, 0, (void**)&buf_addr, &wsid, 0));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(NULL, 0x10001));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetIOAddress(NULL, 0x10001, NULL));
}

/**
* @test    ase_com_01
* @brief   Tests: fpgaErrStr
*
* @details This function returns different message for
*          different errors
*/
TEST(LibopaecAseApi, ase_com_01) {
	// NULL Handle
	EXPECT_STREQ("success", fpgaErrStr(FPGA_OK));
	EXPECT_STREQ("invalid parameter", fpgaErrStr(FPGA_INVALID_PARAM));
	EXPECT_STREQ("resource busy", fpgaErrStr(FPGA_BUSY));
	EXPECT_STREQ("exception", fpgaErrStr(FPGA_EXCEPTION));
	EXPECT_STREQ("not found", fpgaErrStr(FPGA_NOT_FOUND));
	EXPECT_STREQ("no memory", fpgaErrStr(FPGA_NO_MEMORY));
	EXPECT_STREQ("not supported", fpgaErrStr(FPGA_NOT_SUPPORTED));
	EXPECT_STREQ("no driver available", fpgaErrStr(FPGA_NO_DRIVER));
	EXPECT_STREQ("insufficient privileges", fpgaErrStr(FPGA_NO_ACCESS));
}

/**
* @test    ase_err_01
* @brief   Tests: fpgaReadError fpgaClearError fpgaClearAllErrors
*                 fpgaGetErrorInfo
* @details These functions returns FPGA_NOT_SUPPORTED
*
*/
TEST(LibopaecAseApi, ase_err_01) {
	struct _fpga_token _token;
	fpga_token t = &_token;

	// NULL Handle
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaReadError(t, 0, NULL));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaClearError(t, 0));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaClearAllErrors(t));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaGetErrorInfo(t, 0, NULL));
}

/**
* @test    ase_eve_01
* @brief   Tests: fpgaCreateEventHandle fpgaDestroyEventHandle
*                 fpgaRegisterEvent fpgaUnregisterEvent
* @details These functions returns FPGA_INVALID_PARAM or FPGA_NOT_SUPPORTED
*
*/
TEST(LibopaecAseApi, ase_eve_01) {
	int fd;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaCreateEventHandle(NULL));
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(NULL));
	fpga_event_handle handle = NULL;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(&handle));
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOSObjectFromEventHandle(NULL, &fd));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaRegisterEvent(NULL, (fpga_event_type)FPGA_EVENT_INVALID, NULL, 1));
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(NULL, FPGA_EVENT_INTERRUPT, NULL, MAX_USR_INTRS));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaUnregisterEvent(NULL, (fpga_event_type)FPGA_EVENT_INVALID, NULL));
}

/**
* @test    ase_mmio_01
* @brief   Tests: fpgaWriteMMIO32 fpgaReadMMIO32 fpgaWriteMMIO64
*                 fpgaReadMMIO64 fpgaMapMMIO fpgaUnmapMMIO
* @details These functions returns FPGA_NOT_SUPPORTED
*
*/
TEST(LibopaecAseApi, ase_mmio_01) {
	struct _fpga_handle _handle;
	uint32_t value;
	uint64_t value1;
	_handle.fpgaMMIO_is_mapped = 0;
	fpga_handle handle = &_handle;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaWriteMMIO32(NULL, 0, 0x10, 0));
	EXPECT_EQ(FPGA_NOT_FOUND, fpgaWriteMMIO32(handle, 0, 0x10, 0));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReadMMIO32(NULL, 0, 0x10, &value));
	EXPECT_EQ(FPGA_NOT_FOUND, fpgaReadMMIO32(handle, 0, 0x10, &value));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaWriteMMIO64(NULL, 0, 0x10, 0));
	EXPECT_EQ(FPGA_NOT_FOUND, fpgaWriteMMIO64(handle, 0, 0x10, 0));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReadMMIO64(NULL, 0, 0x10, &value1));
	EXPECT_EQ(FPGA_NOT_FOUND, fpgaReadMMIO64(handle, 0, 0x10, &value1));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaMapMMIO(NULL, 0, NULL));
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnmapMMIO(NULL, 0));
	_handle.magic = FPGA_HANDLE_MAGIC;
	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(handle, 0));
	_handle.magic = FPGA_HANDLE_MAGIC - 1;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnmapMMIO(handle, 0));
}

/**
* @test    ase_open_01
* @brief   Tests: fpgaOpen
* @details These functions returns FPGA_INVALID_PARAM
*
*/
TEST(LibopaecAseApi, ase_open_01) {
	struct _fpga_token _token;
	fpga_token token = (fpga_token)&_token;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(token, NULL, 0));
}


/**
* @test       ase_err_021
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             ase_error_report() function should print out the errors
*
*/
TEST(LibopaecAseErr, ase_err_01) {	

	ase_error_report("ase_init", 1, ASE_USR_CAPCM_NOINIT);
	ase_error_report("ase_init", 1, ASE_OS_MQUEUE_ERR);
	ase_error_report("ase_malloc", 1, ASE_OS_SHM_ERR);
	ase_error_report("ase_buffer_alloc", 1, ASE_OS_FOPEN_ERR);
	ase_error_report("ase_buffer_alloc", 1, ASE_OS_MEMMAP_ERR);
	ase_error_report("ase_init", 1, ASE_OS_MQTXRX_ERR);
	ase_error_report("ase_init", 1, ASE_OS_MALLOC_ERR);
	ase_error_report("ase_init", 1, ASE_IPCKILL_CATERR);
	ase_error_report("ase_init", 1, 100);
	backtrace_handler(SIGSEGV);
}

/**
* @test       ase_app_01
*
* @brief      When the parameters are valid and libopae-ase-c is loaded:
*             failure_cleanup(const char *) function should be called
*
*/
TEST(LibopaecAseOps, ase_app_01) {
	FILE *file = fopen("app_test.txt", "w");
	fprintf(file, "%d\n", getpid() + 1);
	fclose(file);

	remove_existing_lock_file("app_test.txt");

	file = fopen("app_test2.txt", "w");
	fprintf(file, "%s\n", "abab");
	fclose(file);

	remove_existing_lock_file("app_test2.txt");
}

