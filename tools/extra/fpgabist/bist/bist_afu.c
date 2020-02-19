// Copyright(c) 2017-2019, Intel Corporation
//
// Redistribution  and	use  in source	and  binary  forms,  with  or  without
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <stdlib.h>
#include <getopt.h>
#include "safe_string/safe_string.h"

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

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

#define LPBK1_BUFFER_SIZE	     MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE MB(2)
#define LPBK1_DSM_SIZE		     MB(2)
#define CSR_SRC_ADDR		     0x0120
#define CSR_DST_ADDR		     0x0128
#define CSR_CTL			     0x0138
#define CSR_CFG			     0x0140
#define CSR_NUM_LINES		     0x0130
#define DSM_STATUS_TEST_COMPLETE     0x40
#define CSR_AFU_DSM_BASEL	     0x0110

/**************** BIST #defines ***************/
#define ENABLE_DDRA_BIST	      0x08000000
#define ENABLE_DDRB_BIST	      0x10000000
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
/**************** BIST #defines *****************/


/* BIST AFU_ID */
#define BIST_AFUID "9caef53d-2fcf-43ea-84b9-aad98993fe41"
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
	fpga_properties		filter;
	fpga_token		*accelerator_token;
	fpga_handle		accelerator_handle;
	fpga_guid		guid;
	uint64_t		res;
	int			open_flags;
} fpga_struct;

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

	volatile uint64_t *dsm_ptr    = NULL;
	volatile uint64_t *status_ptr = NULL;
	volatile uint64_t *input_ptr  = NULL;
	volatile uint64_t *output_ptr = NULL;

	uint64_t	dsm_wsid;
	uint64_t	input_wsid;
	uint64_t	output_wsid;
	fpga_result	res = FPGA_OK;

	int opt;
	int open_flags = 0;

	/* Parse command line for exclusive or shared access */
	while ((opt = getopt(argc, argv, "sv")) != -1) {
		switch (opt) {
		case 's':
			open_flags |= FPGA_OPEN_SHARED;
			break;
		case 'v':
			printf("bist_app %s %s%s\n",
			       OPAE_VERSION,
			       OPAE_GIT_COMMIT_HASH,
			       OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			exit(0);
		default:
			printf("USAGE: %s [-s] [-v]\n", argv[0]);
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

	/* Allocate buffers */
	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_DSM_SIZE,
				(void **)&dsm_ptr, &dsm_wsid, 0);
	ON_ERR_GOTO(res, out_close, "allocating DSM buffer");

	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&input_ptr, &input_wsid, 0);
	ON_ERR_GOTO(res, out_free_dsm, "allocating input buffer");

	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&output_ptr, &output_wsid, 0);
	ON_ERR_GOTO(res, out_free_input, "allocating output buffer");

	/* Initialize buffers */
	memset_s((void *)dsm_ptr,    LPBK1_DSM_SIZE,    0);
	memset_s((void *)input_ptr,  LPBK1_BUFFER_SIZE, 0xAF);
	memset_s((void *)output_ptr, LPBK1_BUFFER_SIZE, 0xBE);

	cache_line *cl_ptr = (cache_line *)input_ptr;
	for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
		cl_ptr[i].uint[15] = i+1; /* set the last uint in every cacheline */
	}

	/* Reset accelerator */
	res = fpgaReset(accelerator_handle);
	ON_ERR_GOTO(res, out_free_output, "resetting accelerator");

	/* Program DMA addresses */
	uint64_t iova;
	res = fpgaGetIOAddress(accelerator_handle, dsm_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting DSM IOVA");

	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_AFU_DSM_BASEL, iova);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_AFU_DSM_BASEL");

	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 0);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 1);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	res = fpgaGetIOAddress(accelerator_handle, input_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting input IOVA");
	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_SRC_ADDR");

	res = fpgaGetIOAddress(accelerator_handle, output_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting output IOVA");
	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_DST_ADDR");
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_NUM_LINES");
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CFG, 0x42000);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/8;

	/* Start the test */
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 3);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	/* Wait for test completion */
	while (0 == ((*status_ptr) & 0x1)) {
		usleep(100);
	}


/************* Begin BIST *************/
	/* Perform BIST Check */
	printf("Running BIST Test\n");
	uint64_t data = 0;
	double count = 0;
	unsigned int bist_mask = ENABLE_DDRA_BIST;

	res = fpgaWriteMMIO32(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, bist_mask);
	ON_ERR_GOTO(res, out_free_output, "write DDR_BIST_CTRL_ADDR");

	res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_CTRL_ADDR, &data);
	ON_ERR_GOTO(res, out_free_output, "write DDR_BIST_CTRL_ADDR");

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
	ON_ERR_GOTO(res, out_free_output, "writing CSR_BIST");
	while ((CHECK_BIT(data, 9) != 0x200) && (CHECK_BIT(data, 8) != 0x100) && (CHECK_BIT(data, 7) != 0x80) &&
		(CHECK_BIT(data, 10) != 0x400) && (CHECK_BIT(data, 11) != 0x800) && (CHECK_BIT(data, 12) != 0x1000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_free_output, "Reading DDR_BIST_STATUS");
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
	ON_ERR_GOTO(res, out_free_output, "writing CSR_BIST");
	while ((CHECK_BIT(data, 10) != 0x400) && (CHECK_BIT(data, 11) != 0x800) && (CHECK_BIT(data, 12) != 0x1000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_free_output, "Reading DDR_BIST_STATUS_ADDR");
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
		printf("DDR Bank BIST Test failed.\n");
	} else if (CHECK_BIT(data, 10) == DDRB_BIST_TIMEOUT) {
		printf("DDR Bank B BIST Test timed out.\n");
	} else {
		printf("DDR Bank B Test encountered a fatal error and cannot continue.\n");
	}
/**************** End BIST *****************/


	/* Stop the device */
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 7);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	/* Check output buffer contents */
	for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE; i++) {
		if (((uint8_t *)output_ptr)[i] != ((uint8_t *)input_ptr)[i]) {
			fprintf(stderr, "Output does NOT match input "
				"at offset %i!\n", i);
			break;
		}
	}

	printf("Done Running Test\n");

	/* Release buffers */
out_free_output:
	res = fpgaReleaseBuffer(accelerator_handle, output_wsid);
	ON_ERR_GOTO(res, out_free_input, "releasing output buffer");
out_free_input:
	res = fpgaReleaseBuffer(accelerator_handle, input_wsid);
	ON_ERR_GOTO(res, out_free_dsm, "releasing input buffer");
out_free_dsm:
	res = fpgaReleaseBuffer(accelerator_handle, dsm_wsid);
	ON_ERR_GOTO(res, out_unmap, "releasing DSM buffer");

	/* Unmap MMIO space */
out_unmap:
	res = fpgaUnmapMMIO(accelerator_handle, 0);
	ON_ERR_GOTO(res, out_close, "unmapping MMIO space");

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
