// Copyright(c) 2017-2019, Intel Corporation
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

#define CONFIG_UNINIT (0)

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
#define CSR_AFU_DSM_BASEH	     0x0114
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

static void printUsage()
{
	printf(
"Usage:\n"
"     bist [-h] [-B <bus>] [-D <device>] [-F <function>] \n"
"         -h,--help           Print this help\n"
"         -v,--version        Print version and exit\n"
"         -B,--bus            Set target bus number\n"
"         -D,--device         Set target device number\n"
"         -F,--function       Set target function number\n\n"
);

	exit(1);
}

/* Type definitions */

typedef struct {
	uint32_t uint[16];
} cache_line;

struct config {
	int bus;
	int device;
	int function;
};

/* Helper function definitions */

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

int find_accelerator(const char *afu_id, struct config *config,
			    fpga_token *afu_tok) {
	fpga_result res;
	fpga_guid guid;
	uint32_t num_matches = 0;
	fpga_properties filter = NULL;

	if(uuid_parse(afu_id, guid) < 0){
	}
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");

	res = fpgaPropertiesSetGUID(filter, guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");

	if (CONFIG_UNINIT != config->bus) {
		res = fpgaPropertiesSetBus(filter, config->bus);
		ON_ERR_GOTO(res, out_destroy_prop, "setting bus");
	}

	if (CONFIG_UNINIT != config->device) {
		res = fpgaPropertiesSetDevice(filter, config->device);
		ON_ERR_GOTO(res, out_destroy_prop, "setting device");
	}

	if (CONFIG_UNINIT != config->function) {
		res = fpgaPropertiesSetFunction(filter, config->function);
		ON_ERR_GOTO(res, out_destroy_prop, "setting function");
	}

	res = fpgaEnumerate(&filter, 1, afu_tok, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaEnumerate");

out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");

out:
	if (num_matches > 0)
		return (int)num_matches;
	else
		return 0;
}

static void parse_args(struct config *config, int argc, char *argv[])
{
	int c;
	if(argc <= 1) {
		//TODO: REmove this
		printf("too few args");
		printUsage();
		return;
	}
	do {
		static const struct option options[] = {
			{"help", no_argument, 0, 'h'},
			{"bus", required_argument, NULL, 'B'},
			{"device", required_argument, NULL, 'D'},
			{"function", required_argument, NULL, 'F'},
			{"version", no_argument, NULL, 'v'},
			{0, 0, 0, 0}
		};
		char *endptr;
		const char *tmp_optarg;

		c = getopt_long(argc, argv, "hB:D:F:v", options, NULL);
		if (c == -1) {
			break;
		}

		endptr = NULL;
		tmp_optarg = optarg;
		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (c) {
		case 'h':
			printUsage();
			break;

		case 'B':    /* bus */
			if (NULL == tmp_optarg)
				break;
			config->bus = (int) strtoul(tmp_optarg, &endptr, 0);
			//debug_print("bus = %x\n", config->bus);
			break;

		case 'D':    /* device */
			if (NULL == tmp_optarg)
				break;
			config->device = (int) strtoul(tmp_optarg, &endptr, 0);
			//debug_print("device = %x\n", config->device);
			break;

		case 'F':    /* function */
			if (NULL == tmp_optarg)
				break;
			config->function = (int)strtoul(tmp_optarg, &endptr, 0);
			//debug_print("function = %x\n", config->function);
			break;
		case 'v':    /* version */
			printf("bist %s %s%s\n",
			       OPAE_VERSION,
			       OPAE_GIT_COMMIT_HASH,
			       OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			exit(0);

		default:
			fprintf(stderr, "unknown op %c\n", c);
			printUsage();
			break;
		} //end case
	} while(1);
}

int main(int argc, char *argv[])
{
	fpga_token	   accelerator_token;
	fpga_handle	   accelerator_handle;
	int open_flags = 0;

	
	volatile uint64_t *dsm_ptr    = NULL;
	volatile uint64_t *status_ptr = NULL;
	volatile uint64_t *input_ptr  = NULL;
	volatile uint64_t *output_ptr = NULL;

	uint64_t	dsm_wsid;
	uint64_t	input_wsid;
	uint64_t	output_wsid;

	fpga_result	res = FPGA_OK;

	struct config config = {
		.bus = CONFIG_UNINIT,
		.device = CONFIG_UNINIT,
		.function = CONFIG_UNINIT,
	};

	parse_args(&config, argc, argv);

	int ret = find_accelerator(BIST_AFUID, &config, &accelerator_token);
	if (ret < 0) {
		ON_ERR_GOTO(ret, out_exit, "failed to find accelerator");
	} else if (ret > 1){
		ON_ERR_GOTO(ret, out_exit, "Found more than one suitable slot");
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
	memset((void *)dsm_ptr,    0,	 LPBK1_DSM_SIZE);
	memset((void *)input_ptr,  0xAF, LPBK1_BUFFER_SIZE);
	memset((void *)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

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
	ON_ERR_GOTO(res, out_free_output, "writing CSR_BIST");
	while ((CHECK_BIT(data, 13) != 0x2000) && (CHECK_BIT(data, 15) != 0x8000) && (CHECK_BIT(data, 14) != 0x4000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_free_output, "Reading DDR_BIST_STATUS_ADDR");
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
	ON_ERR_GOTO(res, out_free_output, "writing CSR_BIST");
	while ((CHECK_BIT(data, 16) != 0x10000) && (CHECK_BIT(data, 17) != 0x20000) && (CHECK_BIT(data, 18) != 0x40000)) {
		res = fpgaReadMMIO64(accelerator_handle, 0, DDR_BIST_STATUS_ADDR,
			&data);
		ON_ERR_GOTO(res, out_free_output, "Reading DDR_BIST_STATUS_ADDR");
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
	ON_ERR_GOTO(res, out_exit, "destroying token");



out_exit:
	return res;

}
