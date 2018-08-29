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

#include "ase_common.h"
#include "gtest/gtest.h"

#define ASE_UNIT

const uint HUGE_NUM = 10000000000000000000;

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
*             ase_read_lock_file(const char *) function should be called
*
*/
TEST(LibopaecAseOps, ase_ops_04) {
	char * workdir = getenv("ASE_WORKDIR");

	ase_read_lock_file(workdir);
}

TEST(LibopaecAseStr, ase_str_01) {
	char str1[128] = "Read two integers 345 234";
	char format[128] = "Read two integers %d %d";
	int a, b;

	sscanf_s_ii(str1, format, &a, &b);
	EXPECT_EQ(a, 345);
	EXPECT_EQ(b, 234);
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

