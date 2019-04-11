// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and use  in source  and  binary  forms,  with  or  without
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
// LIABLE  FOR	ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,	EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,	BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,	DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,	EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <stdlib.h>
#include <getopt.h>

int usleep(unsigned);

#ifndef CL
# define CL(x)			     ((x) * 64)
#endif // CL
#ifndef LOG2_CL
# define LOG2_CL		     6
#endif // LOG2_CL
#ifndef MB
# define MB(x)			     ((x) * 1024 * 1024)
#endif // MB

/**************** BIST #defines ***************/
#define ENABLE_DDRA_BIST	      0x08000000
#define ENABLE_DDRB_BIST	      0x10000000
#define ENABLE_DDRC_BIST              0x20000000
#define ENABLE_DDRD_BIST              0x40000000
#define DDR_BIST_CTRL_ADDR	     0x198
#define DDR_BIST_STATUS_ADDR	     0x200
#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
#define MAX_COUNT	100000
#define DDRA_BIST_PASS 0x200
#define DDRA_BIST_FAIL 0x100
#define DDRA_BIST_TIMEOUT 0x80
#define DDRA_BIST_FATAL_ERROR 0x40
#define DDRB_BIST_PASS 0x1000
#define DDRB_BIST_FAIL 0x800
#define DDRB_BIST_TIMEOUT 0x400
#define DDRB_BIST_FATAL_ERROR 0x20

#define DDRC_BIST_PASS 0x8000
#define DDRC_BIST_FAIL 0x4000
#define DDRC_BIST_TIMEOUT 0x2000
#define DDRC_BIST_FATAL_ERROR 0x20000

#define DDRD_BIST_PASS 0x40000
#define DDRD_BIST_FAIL 0x20000
#define DDRD_BIST_TIMEOUT 0x10000
#define DDRD_BIST_FATAL_ERROR 0x80000

/**************** BIST #defines *****************/


/* BIST AFU_ID */
#define BIST_AFUID "F7DF405C-BD7A-CF72-22F1-44B0B93ACD18"
/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)			 \
	do {					   \
		if ((res) != FPGA_OK) {		   \
			print_err((desc), (res));  \
			goto label;		   \
		}				   \
	} while (0)

/* Type definitions */

typedef struct {
	uint32_t uint[16];
} cache_line;

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

int main(int argc, char *argv[])
{
	fpga_properties    filter = NULL;
	fpga_token	   accelerator_token;
	fpga_handle	   accelerator_handle;
	fpga_guid	   guid;
	uint32_t	   num_matches;

	fpga_result	res = FPGA_OK;

	int opt;
	int open_flags = 0;

	/* Parse command line for exclusive or shared access */
	while ((opt = getopt(argc, argv, "s")) != -1) {
		switch (opt) {
		case 's':
			open_flags |= FPGA_OPEN_SHARED;
			break;
		default:
			printf("USAGE: %s [-s]\n", argv[0]);
			exit(1);
		}
	}

	if (uuid_parse(BIST_AFUID, guid) < 0) {
		fprintf(stderr, "Error parsing guid '%s'\n", BIST_AFUID);
		goto out_exit;
	}

	/* Look for accelerator with MY_ACCELERATOR_ID */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_exit, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

	res = fpgaPropertiesSetGUID(filter, guid);
	ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");
	/* TODO: Add selection via BDF / device ID */

	res = fpgaEnumerate(&filter, 1, &accelerator_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "enumerating accelerators");

	if (num_matches < 1) {
		fprintf(stderr, "accelerator not found.\n");
		res = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}

	/* Open accelerator and map MMIO */
	res = fpgaOpen(accelerator_token, &accelerator_handle, open_flags);
	ON_ERR_GOTO(res, out_destroy_tok, "opening accelerator");

	res = fpgaMapMMIO(accelerator_handle, 0, NULL);
	ON_ERR_GOTO(res, out_close, "mapping MMIO space");

/************* Begin BIST *************/
	/* Perform BIST Check */
	printf("Running BIST Test\n");
	uint64_t data = 0;
	double count = 0;
	unsigned int bist_mask = ENABLE_DDRA_BIST;

	res = fpgaWriteMMIO32(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, bist_mask);
	ON_ERR_GOTO(res, out_close, "write DDR_BIST_CTRL_ADDR");

	res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, &data);
	ON_ERR_GOTO(res, out_close, "write DDR_BIST_CTRL_ADDR");

	while ((CHECK_BIT(data, 27) != 0x08000000)) {
	  printf("Enable Test: reading result #%f: %04lx\n", count, data);
	  if (count >= MAX_COUNT) {
		fprintf(stderr, "BIST not enabled!\n");
		return -1;
	  }
	  count++;
	}
	printf("BIST is enabled.  Reading status register\n");

	count = 0;
	ON_ERR_GOTO(res, out_close, "writing CSR_BIST");
	while ((CHECK_BIT(data, 9) != 0x200) && (CHECK_BIT(data, 8) != 0x100) && (CHECK_BIT(data, 7) != 0x80) &&
		(CHECK_BIT(data, 10) != 0x400) && (CHECK_BIT(data, 11) != 0x800) && (CHECK_BIT(data, 12) != 0x1000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_close, "Reading DDR_BIST_STATUS");
		if (count >= MAX_COUNT) {
			fprintf(stderr, "DDR Bank A BIST Timed Out.\n");
			break;
		}
		count++;
		usleep(100);
	}

	if (CHECK_BIT(data, 9) == DDRA_BIST_PASS) {
		printf("DDR Bank A BIST Test Passed.\n");
	} else if (CHECK_BIT(data, 8) == DDRA_BIST_FAIL) {
		printf("DDR Bank A BIST Test failed.\n");
	} else if (CHECK_BIT(data, 7) == DDRA_BIST_TIMEOUT) {
		printf("DDR Bank A BIST Test timed out.\n");
	} else {
		printf("DDR Bank A Test encountered a fatal error and cannot continue.\n");
	}

	bist_mask = ENABLE_DDRB_BIST;
	count = 0;
	res = fpgaWriteMMIO32(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, bist_mask);
	ON_ERR_GOTO(res, out_close, "writing CSR_BIST");
	while ((CHECK_BIT(data, 10) != 0x400) && (CHECK_BIT(data, 11) != 0x800) && (CHECK_BIT(data, 12) != 0x1000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_close, "Reading DDR_BIST_STATUS_ADDR");
		if (count >= MAX_COUNT) {
			fprintf(stderr, "DDR Bank B BIST Timed Out.\n");
			break;
		}
		count++;
		usleep(100);
	}

	if (CHECK_BIT(data, 12) == DDRB_BIST_PASS) {
		printf("DDR Bank B BIST Test Passed.\n");
	} else if (CHECK_BIT(data, 11) == DDRB_BIST_FAIL) {
		printf("DDR Bank BIST B Test failed.\n");
	} else if (CHECK_BIT(data, 10) == DDRB_BIST_TIMEOUT) {
		printf("DDR Bank B BIST Test timed out.\n");
	} else {
		printf("DDR Bank B Test encountered a fatal error and cannot continue.\n");
	}


       //Bank C
       	bist_mask = ENABLE_DDRC_BIST;
	count = 0;
	res = fpgaWriteMMIO32(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, bist_mask);
	ON_ERR_GOTO(res, out_close, "writing CSR_BIST");
	while ((CHECK_BIT(data, 13) != 0x2000) && (CHECK_BIT(data, 15) != 0x8000) && (CHECK_BIT(data, 14) != 0x4000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_close, "Reading DDR_BIST_STATUS_ADDR");
		if (count >= MAX_COUNT) {
			fprintf(stderr, "DDR Bank C BIST Timed Out.\n");
			break;
		}
		count++;
		usleep(100);
	}

	if (CHECK_BIT(data, 15) == DDRC_BIST_PASS) {
		printf("DDR Bank C BIST Test Passed.\n");
	} else if (CHECK_BIT(data, 14) == DDRC_BIST_FAIL) {
		printf("DDR Bank BIST C Test failed.\n");
	} else if (CHECK_BIT(data, 13) == DDRC_BIST_TIMEOUT) {
		printf("DDR Bank C BIST Test timed out.\n");
	} else {
		printf("DDR Bank C Test encountered a fatal error and cannot continue.\n");
	}


       //Bank D
       	bist_mask = ENABLE_DDRD_BIST;
	count = 0;
	res = fpgaWriteMMIO32(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, bist_mask);
	ON_ERR_GOTO(res, out_close, "writing CSR_BIST");
	while ((CHECK_BIT(data, 16) != 0x10000) && (CHECK_BIT(data, 17) != 0x20000) && (CHECK_BIT(data, 18) != 0x40000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_close, "Reading DDR_BIST_STATUS_ADDR");
		if (count >= MAX_COUNT) {
			fprintf(stderr, "DDR Bank D BIST Timed Out.\n");
			break;
		}
		count++;
		usleep(100);
	}

	if (CHECK_BIT(data, 18) == DDRD_BIST_PASS) {
		printf("DDR Bank D BIST Test Passed.\n");
	} else if (CHECK_BIT(data, 17) == DDRD_BIST_FAIL) {
		printf("DDR Bank BIST D Test failed.\n");
	} else if (CHECK_BIT(data, 16) == DDRD_BIST_TIMEOUT) {
		printf("DDR Bank D BIST Test timed out.\n");
	} else {
		printf("DDR Bank D Test encountered a fatal error and cannot continue.\n");
	}

/**************** End BIST *****************/

	printf("Done Running Test\n");

	/* Release accelerator */
out_close:
	res = fpgaClose(accelerator_handle);
	ON_ERR_GOTO(res, out_destroy_tok, "closing accelerator");

	/* Destroy token */
out_destroy_tok:
	res = fpgaDestroyToken(&accelerator_token);
	ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

	/* Destroy properties object */
out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
	return res;

}
