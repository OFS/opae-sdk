// Copyright(c) 2017-2022, Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>

#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <argsfilter.h>
#include "mock/opae_std.h"

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
#define CSR_STATUS1                  0x0168
#define CSR_CFG                      0x0140
#define CSR_NUM_LINES                0x0130
#define DSM_STATUS_TEST_COMPLETE     0x40
#define CSR_AFU_DSM_BASEL            0x0110

/* NLB0 AFU_ID */
#define NLB0_AFUID "D8424DC4-A4A3-C413-F89E-433683F9040B"

/* NLB0 AFU_ID for N3000 */
#define N3000_AFUID "9AEFFE5F-8457-0612-C000-C9660D824272"
#define FPGA_NLB0_UUID_H 0xd8424dc4a4a3c413
#define FPGA_NLB0_UUID_L 0xf89e433683f9040b


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
	int open_flags;
	int run_n3000;
}

config = {
	.open_flags = 0,
	.run_n3000 = 0
};

void help(void)
{
	printf("\n"
	       "hello_fpga\n"
	       "OPAE Native Loopback 0 (NLB0) sample\n"
	       "\n"
	       "Usage:\n"
	       "        hello_fpga [-schv] [-S <segment>] [-B <bus>] [-D <device>] [-F <function>] [PCI_ADDR]\n"
	       "\n"
	       "                -s,--shared         Open accelerator in shared mode\n"
	       "                -c,--n3000          Assume N3000 MMIO layout\n"
	       "                -h,--help           Print this help\n"
	       "                -v,--version        Print version info and exit\n"
	       "\n");
}

#define GETOPT_STRING "hscv"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{ "help",    no_argument, NULL, 'h' },
		{ "shared",  no_argument, NULL, 's' },
		{ "n3000",   no_argument, NULL, 'c' },
		{ "version", no_argument, NULL, 'v' },
		{ NULL,      0,           NULL,  0  }
	};

	int getopt_ret;
	int option_index;

	char version[32];
	char build[32];

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
						longopts, &option_index))) {
		const char *tmp_optarg = optarg;
		/* Checks to see if optarg is null and if not it goes to value of optarg */
		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h':
			help();
			return -1;
		case 's':
			config.open_flags |= FPGA_OPEN_SHARED;
			break;
		case 'c':
			config.run_n3000 = 1;
			break;
		case 'v':
			fpgaGetOPAECVersionString(version, sizeof(version));
			fpgaGetOPAECBuildString(build, sizeof(build));
			printf("hello_fpga %s %s\n",
			       version, build);
			return -1;

		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline option \n");
			return FPGA_EXCEPTION;
		}
	}

	return FPGA_OK;
}

fpga_result find_fpga(fpga_properties device_filter,
		      fpga_guid afu_guid,
		      fpga_token *accelerator_token,
		      uint32_t *num_matches_accelerators)
{
	fpga_properties filter = NULL;
	fpga_result res1;
	fpga_result res2 = FPGA_OK;

	res1 = fpgaCloneProperties(device_filter, &filter);
	ON_ERR_GOTO(res1, out, "cloning properties object");

	res1 = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res1, out_destroy, "setting object type");

	res1 = fpgaPropertiesSetGUID(filter, afu_guid);
	ON_ERR_GOTO(res1, out_destroy, "setting GUID");

	res1 = fpgaEnumerate(&filter, 1, accelerator_token, 1, num_matches_accelerators);
	ON_ERR_GOTO(res1, out_destroy, "enumerating accelerators");

out_destroy:
	res2 = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res2, out, "destroying properties object");
out:
	return res1 != FPGA_OK ? res1 : res2;
}

/* Is the FPGA simulated with ASE? */
bool probe_for_ase(void)
{
	fpga_result r = FPGA_OK;
	uint16_t device_id = 0;
	fpga_properties filter = NULL;
	uint32_t num_matches = 1;
	fpga_token fme_token;

	/* Connect to the FPGA management engine */
	fpgaGetProperties(NULL, &filter);
	fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);

	/* Connecting to one is sufficient to find ASE */
	fpgaEnumerate(&filter, 1, &fme_token, 1, &num_matches);
	if (0 != num_matches) {
		/* Retrieve the device ID of the FME */
		fpgaDestroyProperties(&filter);
		fpgaGetProperties(fme_token, &filter);
		r = fpgaPropertiesGetDeviceID(filter, &device_id);
		fpgaDestroyToken(&fme_token);
	}
	fpgaDestroyProperties(&filter);

	/* ASE's device ID is 0xa5e */
	return ((FPGA_OK == r) && (0xa5e == device_id));
}

fpga_result find_nlb_n3000(fpga_handle accelerator_handle,
			   uint64_t *afu_baddr)
{

	fpga_result res1 = FPGA_OK;
	int end_of_list = 0;
	int nlb0_found = 0;
	uint64_t header = 0;
	uint64_t uuid_hi = 0;
	uint64_t uuid_lo = 0;
	uint64_t next_offset = 0;
	uint64_t nlb0_offset = 0;

	/* find NLB0 in AFU */
	do {
		// Read the next feature header
		res1 = fpgaReadMMIO64(accelerator_handle, 0, nlb0_offset, &header);
		ON_ERR_GOTO(res1, out_exit, "fpgaReadMMIO64");
		res1 = fpgaReadMMIO64(accelerator_handle, 0, nlb0_offset+8, &uuid_lo);
		ON_ERR_GOTO(res1, out_exit, "fpgaReadMMIO64");
		res1 = fpgaReadMMIO64(accelerator_handle, 0, nlb0_offset+16, &uuid_hi);
		ON_ERR_GOTO(res1, out_exit, "fpgaReadMMIO64");
		// printf("%zx: %zx %zx %zx\n", nlb0_offset, header, uuid_lo, uuid_hi);
		if ((((header >> 60) & 0xf) == 0x1) &&
			(uuid_lo == FPGA_NLB0_UUID_L) && (uuid_hi == FPGA_NLB0_UUID_H)) {
			nlb0_found = 1;
			break;
		}
		// End of the list flag
		end_of_list = (header >> 40) & 1;
		// Move to the next feature header
		next_offset = (header >> 16) & 0xffffff;
		if ((next_offset == 0xffff) || (next_offset == 0)) {
			nlb0_found = 0;
			break;
		}
		nlb0_offset = nlb0_offset + next_offset;
	} while (!end_of_list);

	if (!nlb0_found) {
		printf("AFU NLB0 not found\n");
		return FPGA_EXCEPTION;
	}

	printf("AFU NLB0 found @ %zx\n", nlb0_offset);
	*afu_baddr = nlb0_offset;
	return FPGA_OK;

out_exit:
	return FPGA_EXCEPTION;
}

int main(int argc, char *argv[])
{
	fpga_token         accelerator_token;
	fpga_handle        accelerator_handle;
	fpga_guid          guid;
	uint32_t           num_matches_accelerators = 0;
	uint32_t           use_ase;

	volatile uint64_t *dsm_ptr    = NULL;
	volatile uint64_t *status_ptr = NULL;
	volatile uint64_t *input_ptr  = NULL;
	volatile uint64_t *output_ptr = NULL;

	uint64_t           dsm_wsid;
	uint64_t           input_wsid;
	uint64_t           output_wsid;
	uint32_t           i;
	uint32_t           timeout;
	fpga_result        res1 = FPGA_OK;
	fpga_result        res2 = FPGA_OK;
	fpga_properties    device_filter = NULL;

	res1 = fpgaGetProperties(NULL, &device_filter);
	if (res1 != FPGA_OK) {
		print_err("failed to allocate properties.\n", res1);
		return 1;
	}

	if (opae_set_properties_from_args(device_filter,
					  &res1,
					  &argc,
					  argv)) {
		print_err("failed arg parse.\n", res1);
		res1 = FPGA_EXCEPTION;
		goto out_exit;
	} else if (res1) {
		print_err("failed to set properties.\n", res1);
		goto out_exit;
	}

	res1 = parse_args(argc, argv);
	if ((int)res1 < 0)
		goto out_exit;
	ON_ERR_GOTO(res1, out_exit, "parsing arguments");

	if (config.run_n3000) {
		if (uuid_parse(N3000_AFUID, guid) < 0)
			res1 = FPGA_EXCEPTION;
	} else {
		if (uuid_parse(NLB0_AFUID, guid) < 0)
			res1 = FPGA_EXCEPTION;
	}

	ON_ERR_GOTO(res1, out_exit, "parsing guid");

	use_ase = probe_for_ase();
	if (use_ase) {
		printf("Running in ASE mode\n");
	}

	/* Look for accelerator with NLB0_AFUID */
	res1 = find_fpga(device_filter,
			 guid,
			 &accelerator_token,
			 &num_matches_accelerators);
	ON_ERR_GOTO(res1, out_exit, "finding FPGA accelerator");

	if (num_matches_accelerators == 0) {
		res1 = FPGA_NOT_FOUND;
	}
	ON_ERR_GOTO(res1, out_exit, "no matching accelerator");

	if (num_matches_accelerators > 1) {
		printf("Found more than one suitable accelerator. ");
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

	uint64_t nlb_base_addr = 0;

	if (config.run_n3000) {
		res1 = find_nlb_n3000(accelerator_handle, &nlb_base_addr);
		ON_ERR_GOTO(res1, out_free_output, "finding nlb in AFU");
	}

	/* Program DMA addresses */
	uint64_t iova = 0;
	res1 = fpgaGetIOAddress(accelerator_handle, dsm_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting DSM IOVA");

	res1 = fpgaWriteMMIO64(accelerator_handle, 0, nlb_base_addr + CSR_AFU_DSM_BASEL, iova);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_AFU_DSM_BASEL");

	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_CTL, 0);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_CTL, 1);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	res1 = fpgaGetIOAddress(accelerator_handle, input_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting input IOVA");
	res1 = fpgaWriteMMIO64(accelerator_handle, 0, nlb_base_addr + CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_SRC_ADDR");

	res1 = fpgaGetIOAddress(accelerator_handle, output_wsid, &iova);
	ON_ERR_GOTO(res1, out_free_output, "getting output IOVA");
	res1 = fpgaWriteMMIO64(accelerator_handle, 0, nlb_base_addr + CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_DST_ADDR");

	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_NUM_LINES");
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_CFG, 0x42000);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/sizeof(uint64_t);


	/* Start the test */
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_CTL, 3);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	/* Wait for test completion */
	timeout = TEST_TIMEOUT;
	while (0 == ((*status_ptr) & 0x1)) {
		usleep(100);
		if (!use_ase && (--timeout == 0)) {
			res1 = FPGA_EXCEPTION;
			ON_ERR_GOTO(res1, out_free_output, "test timed out");
		}
	}

	/* Stop the device */
	res1 = fpgaWriteMMIO32(accelerator_handle, 0, nlb_base_addr + CSR_CTL, 7);
	ON_ERR_GOTO(res1, out_free_output, "writing CSR_CFG");

	/* Wait for the AFU's read/write traffic to complete */
	uint32_t afu_traffic_trips = 0;
	while (afu_traffic_trips < 100) {
		/*
		 * CSR_STATUS1 holds two 32 bit values: num pending reads and writes.
		 * Wait for it to be 0.
		 */
		uint64_t s1;
		res1 = fpgaReadMMIO64(accelerator_handle, 0, nlb_base_addr + CSR_STATUS1, &s1);
		ON_ERR_GOTO(res1, out_free_output, "reading CSR_STATUS1");
		if (s1 == 0) {
			break;
		}

		afu_traffic_trips += 1;
		usleep(1000);
	}

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
	fpgaDestroyProperties(&device_filter);
	return res1 != FPGA_OK ? res1 : res2;
}
