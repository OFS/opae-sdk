// Copyright(c) 2017-2018, Intel Corporation
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

/**
 * @file hello_fpga.c
 * @brief A code sample illustrates the basic usage of the OPAE C API.
 *
 * The sample is a host application that demonstrates the basic steps of
 * interacting with FPGA using the OPAE library. These steps include:
 *
 *  - FPGA enumeration
 *  - Resource acquiring and releasing
 *  - Managing shared memory buffer
 *  - MMIO read and write
 *
 * The sample also demonstrates OPAE's object model, such as tokens, handles,
 * and properties.
 *
 * The sample requires a native loopback mode (NLB) test image to be loaded on
 * the FPGA. Refer to
 * <a href="https://opae.github.io/docs/fpga_api/quick_start/readme.html">Quick
 * Start Guide</a> for full instructions on building, configuring, and running
 * this code sample.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>

#include <uuid/uuid.h>
#include <opae/fpga.h>

#include "safe_string/safe_string.h"

int usleep(unsigned);

#ifndef TEST_TIMEOUT
#define TEST_TIMEOUT 30000
#endif // TEST_TIMEOUT

#ifndef CL
# define CL(x)                       ((x) * 64)
#endif // CL
#ifndef LOG2_CL
# define LOG2_CL                     6
#endif // LOG2_CL
#ifndef MB
# define MB(x)                       ((x) * 1024 * 1024)
#endif // MB

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

#define LPBK1_BUFFER_SIZE            MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE MB(2)
#define LPBK1_DSM_SIZE               MB(2)
#define CSR_SRC_ADDR                 0x0120
#define CSR_DST_ADDR                 0x0128
#define CSR_CTL                      0x0138
#define CSR_CFG                      0x0140
#define CSR_NUM_LINES                0x0130
#define DSM_STATUS_TEST_COMPLETE     0x40
#define CSR_AFU_DSM_BASEL            0x0110
#define CSR_AFU_DSM_BASEH            0x0114

/* NLB0 AFU_ID */
#define NLB0_AFUID "D8424DC4-A4A3-C413-F89E-433683F9040B"


/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)              \
	do {                                       \
		if ((res) != FPGA_OK) {            \
			print_err((desc), (res));  \
			goto label;                \
		}                                  \
	} while (0)

/* Type definitions */
typedef struct {
	uint32_t uint[16];
} cache_line;

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

/*
 * Global configuration of bus, set during parse_args()
 * */
struct config {
	struct target {
		int bus;
	} target;
	int open_flags;
}

config = {
	.target = {
		.bus = -1,
	},
	.open_flags = 0
};

#define GETOPT_STRING "B:s"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{ "bus",    required_argument, NULL, 'B' },
		{ "shared", no_argument,       NULL, 's' },
		{ NULL,     0,                 NULL,  0  }
	};

	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
						longopts, &option_index))) {
		const char *tmp_optarg = optarg;
		/* Checks to see if optarg is null and if not it goes to value of optarg */
		if ((optarg) && ('=' == *tmp_optarg)){
			++tmp_optarg;
		}

		switch (getopt_ret){
		case 'B': /* bus */
			if (NULL == tmp_optarg){
				return FPGA_EXCEPTION;
			}
			endptr = NULL;
			config.target.bus = (int) strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
				fprintf(stderr, "invalid bus: %s\n", tmp_optarg);
				return FPGA_EXCEPTION;
			}
			break;
		case 's':
			config.open_flags |= FPGA_OPEN_SHARED;
			break;

		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline option \n");
			return FPGA_EXCEPTION;
		}
	}

	return FPGA_OK;
}

fpga_result find_fpga(fpga_guid afu_guid,
		      fpga_token *accelerator_token,
		      uint32_t *num_matches_accelerators)
{
	fpga_properties filter = NULL;
	fpga_result res1;
	fpga_result res2 = FPGA_OK;

	res1 = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res1, out, "creating properties object");

	res1 = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res1, out_destroy, "setting object type");

	res1 = fpgaPropertiesSetGUID(filter, afu_guid);
	ON_ERR_GOTO(res1, out_destroy, "setting GUID");

	if (-1 != config.target.bus) {
		res1 = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res1, out_destroy, "setting bus");
	}

	res1 = fpgaEnumerate(&filter, 1, accelerator_token, 1, num_matches_accelerators);
	ON_ERR_GOTO(res1, out_destroy, "enumerating accelerators");

out_destroy:
	res2 = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res2, out, "destroying properties object");
out:
	return res1 != FPGA_OK ? res1 : res2;
}

/* function to get the bus number when there are multiple accelerators */
fpga_result get_bus(fpga_token tok, uint8_t *bus)
{
	fpga_result res1;
	fpga_result res2 = FPGA_OK;
	fpga_properties props = NULL;

	res1 = fpgaGetProperties(tok, &props);
	ON_ERR_GOTO(res1, out, "reading properties from Token");

	res1 = fpgaPropertiesGetBus(props, bus);
	ON_ERR_GOTO(res1, out_destroy, "Reading bus from properties");

out_destroy:
	res2 = fpgaDestroyProperties(&props);
	ON_ERR_GOTO(res2, out, "fpgaDestroyProps");
out:
	return res1 != FPGA_OK ? res1 : res2;
}

int main(int argc, char *argv[])
{
	char               library_version[FPGA_VERSION_STR_MAX];
	char               library_build[FPGA_BUILD_STR_MAX];
	fpga_token         accelerator_token;
	fpga_handle        accelerator_handle;
	fpga_guid          guid;
	uint32_t           num_matches_accelerators = 0;

	volatile uint64_t *dsm_ptr    = NULL;
	volatile uint64_t *status_ptr = NULL;
	volatile uint64_t *input_ptr  = NULL;
	volatile uint64_t *output_ptr = NULL;

	uint64_t           dsm_wsid;
	uint64_t           input_wsid;
	uint64_t           output_wsid;
	uint8_t            bus = 0xff;
	uint32_t           i;
	uint32_t           timeout;
	fpga_result        res1 = FPGA_OK;
	fpga_result        res2 = FPGA_OK;


	/* Print version information of the underlying library */
	fpgaGetOPAECVersionString(library_version, sizeof(library_version));
	fpgaGetOPAECBuildString(library_build, sizeof(library_build));
	printf("Using OPAE C library version '%s' build '%s'\n", library_version,
	       library_build);

	res1 = parse_args(argc, argv);
	ON_ERR_GOTO(res1, out_exit, "parsing arguments");

	if (uuid_parse(NLB0_AFUID, guid) < 0) {
		res1 = FPGA_EXCEPTION;
	}
	ON_ERR_GOTO(res1, out_exit, "parsing guid");


	/* Look for accelerator with NLB0_AFUID */
	res1 = find_fpga(guid, &accelerator_token, &num_matches_accelerators);
	ON_ERR_GOTO(res1, out_exit, "finding FPGA accelerator");

	if (num_matches_accelerators <= 0) {
		res1 = FPGA_NOT_FOUND;
	}
	ON_ERR_GOTO(res1, out_exit, "no matching accelerator");

	if (num_matches_accelerators > 1) {
		printf("Found more than one suitable accelerator. ");
		res1 = get_bus(accelerator_token, &bus);
		ON_ERR_GOTO(res1, out_exit, "getting bus num");
		printf("Running on bus 0x%02x.\n", bus);
	}


	/* Open accelerator and map MMIO */
	res1 = fpgaOpen(accelerator_token, &accelerator_handle, config.open_flags);
	ON_ERR_GOTO(res1, out_destroy_tok, "opening accelerator");

	res1 = fpgaMapMMIO(accelerator_handle, 0, NULL);
	ON_ERR_GOTO(res1, out_close, "mapping MMIO space");


	/* Allocate buffers */
	res1 = fpgaPrepareBuffer(accelerator_handle, LPBK1_DSM_SIZE,
				(void **)&dsm_ptr, &dsm_wsid, 0);
	ON_ERR_GOTO(res1, out_close, "allocating DSM buffer");

	res1 = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&input_ptr, &input_wsid, 0);
	ON_ERR_GOTO(res1, out_free_dsm, "allocating input buffer");

	res1 = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&output_ptr, &output_wsid, 0);
	ON_ERR_GOTO(res1, out_free_input, "allocating output buffer");

	printf("Running Test\n");

	bus = 0xff;
	res1 = get_bus(accelerator_token, &bus);
	ON_ERR_GOTO(res1, out_free_output, "getting bus num");
	printf("Running on bus 0x%02x.\n", bus);


	/* Initialize buffers */
	memset((void *)dsm_ptr,    0,    LPBK1_DSM_SIZE);
	memset((void *)input_ptr,  0xAF, LPBK1_BUFFER_SIZE);
	memset((void *)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

	cache_line *cl_ptr = (cache_line *)input_ptr;
	for (i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
		cl_ptr[i].uint[15] = i+1; /* set the last uint in every cacheline */
	}


	/* Reset accelerator */
	res1 = fpgaReset(accelerator_handle);
	ON_ERR_GOTO(res1, out_free_output, "resetting accelerator");


	/* Program DMA addresses */
	uint64_t iova = 0;
	res1 = fpgaGetIOAddress(accelerator_handle, dsm_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting DSM IOVA");

	res1 = fpgaWriteMMIO64(accelerator_handle, 0, CSR_AFU_DSM_BASEL, iova);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_AFU_DSM_BASEL");

	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 0);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 1);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	res1 = fpgaGetIOAddress(accelerator_handle, input_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting input IOVA");
	res1 = fpgaWriteMMIO64(accelerator_handle, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_SRC_ADDR");

	res1 = fpgaGetIOAddress(accelerator_handle, output_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting output IOVA");
	res1 = fpgaWriteMMIO64(accelerator_handle, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_DST_ADDR");

	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_NUM_LINES");
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CFG, 0x42000);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/sizeof(uint64_t);


	/* Start the test */
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 3);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	/* Wait for test completion */
	timeout = TEST_TIMEOUT;
	while (0 == ((*status_ptr) & 0x1)) {
		usleep(100);
		if (--timeout == 0) {
			res1 = FPGA_EXCEPTION;
			ON_ERR_GOTO(res1, out_free_output, "test timed out");
		}
	}

	/* Stop the device */
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 7);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");


	/* Check output buffer contents */
	for (i = 0; i < LPBK1_BUFFER_SIZE; i++) {
		if (((uint8_t *)output_ptr)[i] != ((uint8_t *)input_ptr)[i]) {
			fprintf(stderr, "Output does NOT match input "
				"at offset %i!\n", i);
			break;
		}
	}

	printf("Done Running Test\n");


	/* Release buffers */
out_free_output:
	res2 = fpgaReleaseBuffer(accelerator_handle, output_wsid);
	ON_ERR_GOTO(res2, out_free_input, "releasing output buffer");
out_free_input:
	res2 = fpgaReleaseBuffer(accelerator_handle, input_wsid);
	ON_ERR_GOTO(res2, out_free_dsm, "releasing input buffer");
out_free_dsm:
	res2 = fpgaReleaseBuffer(accelerator_handle, dsm_wsid);
	ON_ERR_GOTO(res2, out_unmap, "releasing DSM buffer");

	/* Unmap MMIO space */
out_unmap:
	res2 = fpgaUnmapMMIO(accelerator_handle, 0);
	ON_ERR_GOTO(res2, out_close, "unmapping MMIO space");

	/* Release accelerator */
out_close:
	res2 = fpgaClose(accelerator_handle);
	ON_ERR_GOTO(res2, out_destroy_tok, "closing accelerator");

	/* Destroy token */
out_destroy_tok:
	res2 = fpgaDestroyToken(&accelerator_token);
	ON_ERR_GOTO(res2, out_exit, "destroying token");

out_exit:
	return res1 != FPGA_OK ? res1 : res2;
}
