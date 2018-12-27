// Copyright(c) 2017, Intel Corporation
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

#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <time.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "safe_string/safe_string.h"
#ifndef USE_ASE
#include <hwloc.h>
#endif
#include "fpga_dma.h"
/**
 * \fpga_dma_test.c
 * \brief User-mode DMA test
 */

#include <stdlib.h>
#include <assert.h>

#define HELLO_AFU_ID "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (10 * 1024 * 1024)
#define ASE_TEST_BUF_SIZE (4 * 1024)
#define TEST_TOTAL_SIZE (uint64_t)4 * 1024 * 1024 * 1024

#ifdef CHECK_DELAYS
extern double poll_wait_count;
extern double buf_full_count;
char cbuf[2048];
#endif

static char *verify_buf = NULL;
static uint64_t verify_buf_size = 0;

static int err_cnt = 0;

// Options determining various optimization attempts
bool use_malloc = true;
bool use_memcpy = true;
bool use_advise = false;
bool do_not_verify = false;
bool cpu_affinity = true;
bool memory_affinity = true;

/*
 * macro for checking return codes
 */
#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			err_cnt++;                                             \
			fprintf(stderr, "Error %s: %s\n", (desc),              \
				fpgaErrStr(res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)

/*
 *  *  * Global configuration of bus, set during parse_args()
 *   *   * */
struct config {
    struct target {
        int bus;
        int dma;
        uint64_t size;
        char guid[48];
    } target;
}
config = {
    .target = {
        .bus = -1,
        .dma = 0,
        .size = TEST_TOTAL_SIZE,
        .guid = {0}
    }
};

/*
 *  *  * Parse command line arguments
 *   *   */
#define GETOPT_STRING ":B:D:S:G:mpc2nayCM"
fpga_result parse_args(int argc, char *argv[])
{
    struct option longopts[] = {
        {"bus", required_argument, NULL, 'B'},
        {"dma", required_argument, NULL, 'D'},
        {"size", required_argument, NULL, 'S'},
        {"guid", required_argument, NULL, 'G'}
    };

	int getopt_ret;
	int option_index;
	char *endptr = NULL;
    int buf_size = sizeof(config.target.guid);

	memcpy_s(config.target.guid, buf_size, HELLO_AFU_ID, strlen(HELLO_AFU_ID));

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING, longopts,
					    &option_index))) {
		const char *tmp_optarg = optarg;
		/* Checks to see if optarg is null and if not goes to value of
		 * optarg */
		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'B': /* bus */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.bus =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
				fprintf(stderr, "invalid bus: %s\n",
					tmp_optarg);
				return FPGA_EXCEPTION;
			}
			break;
        case 'D':   /* dma */
            if (NULL == tmp_optarg)
                break;
            endptr = NULL;
            config.target.dma =
			    (int)strtoul(tmp_optarg, &endptr, 10);
            if (endptr != tmp_optarg + strnlen(tmp_optarg, 16)) {
                fprintf(stderr, "invalid dma: %s\n", tmp_optarg);
                return FPGA_EXCEPTION;
            }
            break;
        case 'S':   /* size */
            if (NULL == tmp_optarg)
                break;
            endptr = NULL;
            config.target.size = (unsigned long)strtoul(tmp_optarg, &endptr, 0);
            if (endptr != tmp_optarg + strnlen(tmp_optarg, 16)) {
                fprintf(stderr, "invalid size: %s\n", tmp_optarg);
                return FPGA_EXCEPTION;
            }
            break;
        case 'G':   /* AFU ID */
            if (tmp_optarg)
                memcpy_s(config.target.guid, buf_size, tmp_optarg, buf_size);
            break;
		case 'm':
			use_malloc = true;
			break;
		case 'p':
			use_malloc = false;
			break;
		case 'c':
			use_memcpy = true;
			break;
		case '2':
			use_memcpy = false;
			break;
		case 'n':
			use_advise = false;
			break;
		case 'a':
			use_advise = true;
			break;
		case 'y':
			do_not_verify = true;
			break;
		case 'C':
			cpu_affinity = true;
			break;
		case 'M':
			memory_affinity = true;
			break;

		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			return -1;
		}
	}

	/* first non-option argument as hardware or simulation*/
	if (optind == argc) {
		fprintf(stderr, "Hardware (0) or simulation (1)?\n");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

int find_fpga(fpga_guid interface_id, fpga_token *fpga, uint32_t *num_matches)
{
	fpga_properties filter = NULL;
	fpga_result res;

	/* Get number of FPGAs in system*/
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting interface ID");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy, "fpgaPropertiesSetObjectType");

	res = fpgaPropertiesSetGUID(filter, interface_id);
	ON_ERR_GOTO(res, out_destroy, "fpgaPropertiesSetGUID");

	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}

	res = fpgaEnumerate(&filter, 1, fpga, 1, num_matches);
	ON_ERR_GOTO(res, out, "enumerating FPGAs");

out_destroy:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out, "destroying properties object");
out:
	return err_cnt;
}

// Aligned malloc
static inline void *malloc_aligned(uint64_t align, size_t size)
{
	assert(align
	       && ((align & (align - 1)) == 0)); // Must be power of 2 and not 0
	assert(align >= 2 * sizeof(void *));
	void *blk = NULL;
	if (use_malloc) {
		blk = malloc(size + align + 2 * sizeof(void *));
	} else {
		align = getpagesize();
		blk = mmap(NULL, size + align + 2 * sizeof(void *),
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
	}
	void **aptr =
		(void **)(((uint64_t)blk + 2 * sizeof(void *) + (align - 1))
			  & ~(align - 1));
	aptr[-1] = blk;
	aptr[-2] = (void *)(size + align + 2 * sizeof(void *));
	return aptr;
}

// Aligned free
static inline void free_aligned(void *ptr)
{
	void **aptr = (void **)ptr;
	if (use_malloc) {
		free(aptr[-1]);
	} else {
		munmap(aptr[-1], (size_t)aptr[-2]);
	}
	return;
}

static inline void fill_buffer(char *buf, size_t size)
{
	char *data = NULL;

	if (do_not_verify)
		return;
	size_t i = 0;

	if (verify_buf_size < size) {
		if (verify_buf)
			free(verify_buf);
		verify_buf = (char *)malloc(size);
		if (NULL == verify_buf) {
			verify_buf_size = 0;
			data = buf;
		} else {
			verify_buf_size = size;
			data = verify_buf;
		}

		// use a deterministic seed to generate pseudo-random numbers
		srand(99);

		for (i = 0; i < size; i++) {
			*data = rand() % 256;
			data++;
		}
	}

	if (verify_buf)
		memcpy(buf, verify_buf, size);
}

static inline fpga_result verify_buffer(char *buf, size_t size)
{
	if (do_not_verify)
		return FPGA_OK;

	assert(NULL != verify_buf);

	if (!memcmp(buf, verify_buf, size)) {
		printf("Buffer Verification Success!\n");
	} else {
		size_t i, rnum = 0;
		srand(99);

		for (i = 0; i < size; i++) {
			rnum = rand() % 256;
			if ((*buf & 0xFF) != rnum) {
				printf("Invalid data at %zx Expected = %zx Actual = %x\n",
				       i, rnum, (*buf & 0xFF));
				return FPGA_INVALID_PARAM;
			}
			buf++;
		}
	}

	return FPGA_OK;
}

static inline void clear_buffer(char *buf, size_t size)
{
	if (do_not_verify)
		return;
	memset_s(buf, size, 0);
}

static inline char *showDelays(char *buf)
{
#ifdef CHECK_DELAYS
	sprintf(buf,
		"Avg per iteration: Poll delays: %g, Descriptor buffer full delays: %g",
		poll_wait_count, buf_full_count);
#else
	buf[0] = '\0';
#endif
	return buf;
}

static inline void report_bandwidth(size_t size, double seconds)
{
	char buf[2048];
	double throughput = (double)size / ((double)seconds * 1000 * 1000);
	printf("\rMeasured bandwidth = %lf Megabytes/sec %s\n", throughput,
	       showDelays(buf));

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif
}

// return elapsed time
static inline double getTime(struct timespec start, struct timespec end)
{
	uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec
			- start.tv_nsec;
	return (double)diff / (double)1000000000L;
}

/* functions to get the bus number when there are multiple buses */
struct bus_info {
	uint8_t bus;
};

fpga_result get_bus_info(fpga_token tok, struct bus_info *finfo)
{
	fpga_result res = FPGA_OK;
	fpga_properties props;
	res = fpgaGetProperties(tok, &props);
	ON_ERR_GOTO(res, out, "reading properties from Token");

	res = fpgaPropertiesGetBus(props, &finfo->bus);
	ON_ERR_GOTO(res, out_destroy, "Reading bus from properties");

	if (res != FPGA_OK) {
		return FPGA_EXCEPTION;
	}

out_destroy:
	res = fpgaDestroyProperties(&props);
	ON_ERR_GOTO(res, out, "fpgaDestroyProps");

out:
	return res;
}

void print_bus_info(struct bus_info *info)
{
	printf("Running on bus 0x%02X. \n", info->bus);
}


fpga_result ddr_sweep(fpga_dma_handle dma_h, uint64_t mem_size, uint64_t ptr_align,
		      uint64_t siz_align)
{
	int res;
	struct timespec start, end;

	uint64_t total_mem_size = mem_size;

	uint64_t *dma_buf_ptr = malloc_aligned(getpagesize(), total_mem_size);
	if (dma_buf_ptr == NULL) {
		printf("Unable to allocate %ld bytes of memory",
		       total_mem_size);
		return FPGA_NO_MEMORY;
	}

	if (use_advise) {
		if (0 != madvise(dma_buf_ptr, total_mem_size, MADV_SEQUENTIAL))
			perror("Warning: madvise returned error");
	}

	uint64_t *buf_to_free_ptr = dma_buf_ptr;
	dma_buf_ptr = (uint64_t *)((uint64_t)dma_buf_ptr + ptr_align);
	total_mem_size -= ptr_align + siz_align;

	printf("Buffer pointer = %p, size = 0x%lx (%p through %p)\n",
	       dma_buf_ptr, total_mem_size, dma_buf_ptr,
	       (uint64_t *)((uint64_t)dma_buf_ptr + total_mem_size));

	printf("Allocated test buffer\n");
	printf("Fill test buffer\n");
	fill_buffer((char *)dma_buf_ptr, total_mem_size);

	uint64_t src = (uint64_t)dma_buf_ptr;
	uint64_t dst = 0x0;

	double tot_time = 0.0;
	int i;

	printf("DDR Sweep Host to FPGA\n");

#define ITERS 1

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	for (i = 0; i < ITERS; i++) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		res = fpgaDmaTransferSync(dma_h, dst, src, total_mem_size,
					  HOST_TO_FPGA_MM);
		clock_gettime(CLOCK_MONOTONIC, &end);
		if (res != FPGA_OK) {
			printf(" fpgaDmaTransferSync Host to FPGA failed with error %s",
			       fpgaErrStr(res));
			free_aligned(buf_to_free_ptr);
			return FPGA_EXCEPTION;
		}
		tot_time += getTime(start, end);
	}

#ifdef CHECK_DELAYS
	poll_wait_count /= (double)ITERS;
	buf_full_count /= (double)ITERS;
#endif

	report_bandwidth(total_mem_size * ITERS, tot_time);
	tot_time = 0.0;

	printf("\rClear buffer\n");
	clear_buffer((char *)dma_buf_ptr, total_mem_size);

	src = 0x0;
	dst = (uint64_t)dma_buf_ptr;

	printf("DDR Sweep FPGA to Host\n");

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	for (i = 0; i < ITERS; i++) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		res = fpgaDmaTransferSync(dma_h, dst, src, total_mem_size,
					  FPGA_TO_HOST_MM);
		clock_gettime(CLOCK_MONOTONIC, &end);

		if (res != FPGA_OK) {
			printf(" fpgaDmaTransferSync FPGA to Host failed with error %s",
			       fpgaErrStr(res));
			free_aligned(buf_to_free_ptr);
			return FPGA_EXCEPTION;
		}
		tot_time += getTime(start, end);
	}

#ifdef CHECK_DELAYS
	poll_wait_count /= (double)ITERS;
	buf_full_count /= (double)ITERS;
#endif

	report_bandwidth(total_mem_size * ITERS, tot_time);
	tot_time = 0.0;

	printf("Verifying buffer..\n");
	verify_buffer((char *)dma_buf_ptr, total_mem_size);

	free_aligned(buf_to_free_ptr);
	return FPGA_OK;
}

static void usage(void)
{
	printf("Usage: fpga_dma_test <use_ase = 1 (simulation only), 0 (hardware)> [options]\n");
	printf("Options are:\n");
	printf("\t-m\tUse malloc (default)\n");
	printf("\t-p\tUse mmap (Incompatible with -m)\n");
	printf("\t-c\tUse builtin memcpy (default)\n");
	printf("\t-2\tUse SSE2 memcpy (Incompatible with -c)\n");
	printf("\t-n\tDo not provide OS advice (default)\n");
	printf("\t-a\tUse madvise (Incompatible with -n)\n");
	printf("\t-y\tDo not verify buffer contents - faster (default is to verify)\n");
	printf("\t-C\tDo not restrict process to CPUs attached to DCP NUMA node\n");
	printf("\t-M\tDo not restrict process memory allocation to DCP NUMA node\n");
	printf("\t-B\tSet a target bus number\n");
	printf("\t-D\tSelect DMA to test\n");
	printf("\t-S\tSet memory test size\n");
	printf("\t-G\tSet AFU GUID\n");
}

int main(int argc, char *argv[])
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle dma_h;
	uint64_t count;
	// fpga_properties filter = NULL;
	fpga_token afc_token;
	fpga_handle afc_h;
	fpga_guid guid;
	uint32_t num_matches = 1;
	volatile uint64_t *mmio_ptr = NULL;
	uint64_t *dma_buf_ptr = NULL;
	uint32_t use_ase;
	struct bus_info info;
	fpga_properties props = NULL;

	if (argc < 2) {
		usage();
		return 1;
	}

	res = parse_args(argc, argv);
	if (res == FPGA_EXCEPTION) {
		return 1;
	}

	use_ase = atoi(argv[optind]);
	if (use_ase == 0) {
		printf("Running test in HW mode\n");
	} else {
		printf("Running test in ASE mode\n");
	}


	// enumerate the afc
	if (uuid_parse(config.target.guid, guid) < 0) {
		return 1;
	}

	res = find_fpga(guid, &afc_token, &num_matches);
	if (num_matches == 0) {
		fprintf(stderr, "No suitable slots found.\n");
		return 1;
	}
	if (num_matches > 1) {
		fprintf(stderr, "Found more than one suitable slot. ");
		res = get_bus_info(afc_token, &info);
		ON_ERR_GOTO(res, out, "getting bus num");
		print_bus_info(&info);
	}

	if (num_matches < 1) {
		printf("Error: Number of matches < 1");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out, "num_matches<1");
	}


	// open the AFC
	res = fpgaOpen(afc_token, &afc_h, 0);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaOpen");

#ifndef USE_ASE
	// Set up proper affinity if requested
	if (cpu_affinity || memory_affinity) {
		unsigned dom = 0, bus = 0, dev = 0, func = 0;
		int retval;
#ifdef FPGA_DMA_DEBUG
		char str[4096];
#endif
		res = fpgaGetProperties(afc_token, &props);
		ON_ERR_GOTO(res, out_close, "fpgaGetProperties");
		res = fpgaPropertiesGetBus(props, (uint8_t *)&bus);
		ON_ERR_GOTO(res, out_close, "fpgaPropertiesGetBus");
		res = fpgaPropertiesGetDevice(props, (uint8_t *)&dev);
		ON_ERR_GOTO(res, out_close, "fpgaPropertiesGetDevice");
		res = fpgaPropertiesGetFunction(props, (uint8_t *)&func);
		ON_ERR_GOTO(res, out_close, "fpgaPropertiesGetFunction");

		// Find the device from the topology
		hwloc_topology_t topology;
		hwloc_topology_init(&topology);
		hwloc_topology_set_flags(topology,
					 HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
		hwloc_topology_load(topology);
		hwloc_obj_t obj = hwloc_get_pcidev_by_busid(topology, dom, bus,
							    dev, func);
		hwloc_obj_t obj2 = hwloc_get_non_io_ancestor_obj(topology, obj);
#ifdef FPGA_DMA_DEBUG
		hwloc_obj_type_snprintf(str, 4096, obj2, 1);
		printf("%s\n", str);
		hwloc_obj_attr_snprintf(str, 4096, obj2, " :: ", 1);
		printf("%s\n", str);
		hwloc_bitmap_taskset_snprintf(str, 4096, obj2->cpuset);
		printf("CPUSET is %s\n", str);
		hwloc_bitmap_taskset_snprintf(str, 4096, obj2->nodeset);
		printf("NODESET is %s\n", str);
#endif
		if (memory_affinity) {
#if HWLOC_API_VERSION > 0x00020000
			retval = hwloc_set_membind(
				topology, obj2->nodeset, HWLOC_MEMBIND_THREAD,
				HWLOC_MEMBIND_MIGRATE
					| HWLOC_MEMBIND_BYNODESET);
#else
			retval = hwloc_set_membind_nodeset(
				topology, obj2->nodeset, HWLOC_MEMBIND_THREAD,
				HWLOC_MEMBIND_MIGRATE);
#endif
			ON_ERR_GOTO(retval, out_close,
				    "hwloc_set_membind");
		}
		if (cpu_affinity) {
			retval = hwloc_set_cpubind(topology, obj2->cpuset,
						   HWLOC_CPUBIND_STRICT);
			ON_ERR_GOTO(retval, out_close,
				    "hwloc_set_cpubind");
		}
	}
#endif

	if (!use_ase) {
		res = fpgaMapMMIO(afc_h, 0, (uint64_t **)&mmio_ptr);
		ON_ERR_GOTO(res, out_close, "fpgaMapMMIO");
	}
	// reset AFC
	res = fpgaReset(afc_h);
	ON_ERR_GOTO(res, out_unmap, "fpgaReset");

    res = fpgaDmaOpen(afc_h, config.target.dma, &dma_h);
    ON_ERR_GOTO(res, out_dma_close, "fpgaDmaOpen");
    if (!dma_h) {
        res = FPGA_EXCEPTION;
        ON_ERR_GOTO(res, out_dma_close, "Invaid DMA Handle");
    }

	if (use_ase)
		count = ASE_TEST_BUF_SIZE;
	else
		count = TEST_BUF_SIZE;

	dma_buf_ptr = (uint64_t *)malloc_aligned(getpagesize(), count);
	if (!dma_buf_ptr) {
		res = FPGA_NO_MEMORY;
		ON_ERR_GOTO(res, out_dma_close, "Error allocating memory");
	}

	if (use_advise) {
		int rr = madvise(dma_buf_ptr, count, MADV_SEQUENTIAL);
		ON_ERR_GOTO((rr == 0) ? FPGA_OK : FPGA_EXCEPTION, out_dma_close,
			    "Error madvise");
	}

	fill_buffer((char *)dma_buf_ptr, count);

	// Test procedure
	// - Fill host buffer with pseudo-random data
	// - Copy from host buffer to FPGA buffer at address 0x0
	// - Clear host buffer
	// - Copy from FPGA buffer to host buffer
	// - Verify host buffer data
	// - Clear host buffer
	// - Copy FPGA buffer at address 0x0 to FPGA buffer at addr "count"
	// - Copy data from FPGA buffer at addr "count" to host buffer
	// - Verify host buffer data

	// copy from host to fpga

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = fpgaDmaTransferSync(dma_h, 0x0 /*dst */,
				  (uint64_t)dma_buf_ptr /*src */, count,
				  HOST_TO_FPGA_MM);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync HOST_TO_FPGA_MM");
	clear_buffer((char *)dma_buf_ptr, count);

#ifdef CHECK_DELAYS
	printf("H->F size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	// copy from fpga to host
	res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst */,
				  0x0 /*src */, count, FPGA_TO_HOST_MM);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_HOST_MM");

#ifdef CHECK_DELAYS
	printf("F->H size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = verify_buffer((char *)dma_buf_ptr, count);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	clear_buffer((char *)dma_buf_ptr, count);

	// copy from fpga to fpga
	res = fpgaDmaTransferSync(dma_h, count /*dst */, 0x0 /*src */, count,
				  FPGA_TO_FPGA_MM);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_FPGA_MM");

#ifdef CHECK_DELAYS
	printf("F->F size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	// copy from fpga to host
	res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr /*dst */,
				  count /*src */, count, FPGA_TO_HOST_MM);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDmaTransferSync FPGA_TO_HOST_MM");

#ifdef CHECK_DELAYS
	printf("F->H size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = verify_buffer((char *)dma_buf_ptr, count);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	if (!use_ase) {
		printf("Running DDR sweep test\n");
		res = ddr_sweep(dma_h, config.target.size, 0, 0);
		if (config.target.dma >= 0 && config.target.dma < 3) {
			printf("DDR sweep with unaligned pointer and size\n");
			res |= ddr_sweep(dma_h, config.target.size, 61, 5);
			res |= ddr_sweep(dma_h, config.target.size, 3, 0);
			res |= ddr_sweep(dma_h, config.target.size, 7, 3);
			res |= ddr_sweep(dma_h, config.target.size, 0, 3);
			res |= ddr_sweep(dma_h, config.target.size, 0, 61);
			res |= ddr_sweep(dma_h, config.target.size, 0, 7);
		}
		ON_ERR_GOTO(res, out_dma_close, "ddr_sweep");
	}

	free(verify_buf);

out_dma_close:
	if (dma_buf_ptr)
		free_aligned(dma_buf_ptr);
	if (dma_h) {
		res = fpgaDmaClose(dma_h);
		ON_ERR_GOTO(res, out_unmap, "fpgaDmaClose");
	}
out_unmap:
	if (!use_ase) {
		res = fpgaUnmapMMIO(afc_h, 0);
		ON_ERR_GOTO(res, out_close, "fpgaUnmapMMIO");
	}
out_close:
	res = fpgaClose(afc_h);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaClose");

out_destroy_tok:
	if (props)
		fpgaDestroyProperties(&props);
	res = fpgaDestroyToken(&afc_token);
	ON_ERR_GOTO(res, out, "fpgaDestroyToken");

out:
	return err_cnt;
}
