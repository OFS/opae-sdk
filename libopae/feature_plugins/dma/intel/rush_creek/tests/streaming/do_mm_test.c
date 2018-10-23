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

#define __STDC_FORMAT_MACROS

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
#include <inttypes.h>
#include <sched.h>
#include "safe_string/safe_string.h"
#ifndef USE_ASE
#include <hwloc.h>
#endif
#include "fpga_dma.h"
/**
 * \fpga_dma_test.c
 * \brief User-mode DMA test
 */

#define DO_SHORT_TEST

#include <stdlib.h>
#include <assert.h>

#define HELLO_AFU_ID "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (10 * 1024 * 1024)
#define ASE_TEST_BUF_SIZE (4 * 1024)

#ifdef CHECK_DELAYS
extern double poll_wait_count;
extern double buf_full_count;
char cbuf[2048];
#endif

static char *verify_buf = NULL;
static uint64_t verify_buf_size = 0;

static int err_cnt = 0;

static volatile uint32_t async_buf_count = 0;

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
	} target;
}

config = {.target = {.bus = -1}};

/*
 *  *  * Parse command line arguments
 *   *   */

static void async_cb(void *context)
{
	(void)context;
	async_buf_count++;
}

// Aligned malloc
static inline void *malloc_aligned(uint64_t align, size_t size)
{
	assert(align
	       && ((align & (align - 1)) == 0)); // Must be power of 2 and not 0
	assert(align >= 2 * sizeof(void *));
	void *blk = NULL;
	align = getpagesize();
	blk = mmap(NULL, size + align + 2 * sizeof(void *),
		   PROT_READ | PROT_WRITE,
		   MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
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
	munmap(aptr[-1], (size_t)aptr[-2]);
	return;
}

static inline void fill_buffer(char *buf, size_t size)
{
	size_t i = 0;

	if (verify_buf_size < size) {
		free(verify_buf);
		verify_buf = (char *)malloc(size);
		verify_buf_size = size;
		char *buf = verify_buf;

		// use a deterministic seed to generate pseudo-random numbers
		srand(99);

		for (i = 0; i < size; i++) {
			*buf = rand() % 256;
			buf++;
		}
	}

	memcpy(buf, verify_buf, size);
}

static inline fpga_result verify_buffer(char *buf, size_t size)
{
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
	memset(buf, 0, size);
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

static fpga_result ddr_sweep(fpga_dma_channel_handle dma_ch, uint64_t ptr_align,
			     uint64_t siz_align, uint64_t size,
			     uint64_t dst_addr)
{
	int res;
	struct timespec start, end;

	fpga_dma_transfer transfer;
	fpgaDMATransferInit(dma_ch, &transfer);

	ssize_t total_mem_size = size;

	int64_t ITERS = ((uint64_t)4096 * 5) / size;
	ITERS = 64;

#ifdef DO_SHORT_TEST
	ITERS = 10;
#endif

	uint64_t *dma_buf_ptr = malloc_aligned(getpagesize(), total_mem_size);
	if (dma_buf_ptr == NULL) {
		printf("Unable to allocate %ld bytes of memory",
		       total_mem_size);
		return FPGA_NO_MEMORY;
	}

	if (0 != madvise(dma_buf_ptr, total_mem_size, MADV_SEQUENTIAL))
		perror("Warning: madvise returned error");

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
	uint64_t dst = dst_addr;

	double tot_time = 0.0;
	int i;

	printf("DDR Sweep Host to FPGA, %lld iterations of size %lld\n",
	       (long long)ITERS, (long long)total_mem_size);

	//#define ITERS 3

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif
	fpgaDMATransferSetSrc(transfer, src);
	fpgaDMATransferSetDst(transfer, dst);
	fpgaDMATransferSetLen(transfer, total_mem_size);
	fpgaDMATransferSetTransferType(transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);

	for (i = 0; i < ITERS; i++) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		res = fpgaDMATransferStart(dma_ch, transfer);
		clock_gettime(CLOCK_MONOTONIC, &end);
		if (res != FPGA_OK) {
			printf(" fpgaDMATransferSync Host to FPGA failed with error %s",
			       fpgaErrStr(res));
			free_aligned(buf_to_free_ptr);
			fpgaDMATransferDestroy(dma_ch, &transfer);
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

	src = dst_addr;
	dst = (uint64_t)dma_buf_ptr;

	printf("DDR Sweep FPGA to Host, %lld iterations of size %lld\n",
	       (long long)ITERS, (long long)total_mem_size);

#ifdef CHECK_DELAYS
	poll_wait_count = 0;
	buf_full_count = 0;
#endif
	fpgaDMATransferSetSrc(transfer, src);
	fpgaDMATransferSetDst(transfer, dst);
	fpgaDMATransferSetLen(transfer, total_mem_size);
	fpgaDMATransferSetTransferType(transfer, FPGA_TO_HOST_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);

	for (i = 0; i < ITERS; i++) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		res = fpgaDMATransferStart(dma_ch, transfer);
		clock_gettime(CLOCK_MONOTONIC, &end);

		if (res != FPGA_OK) {
			printf(" fpgaDMATransferSync FPGA to Host failed with error %s",
			       fpgaErrStr(res));
			free_aligned(buf_to_free_ptr);
			fpgaDMATransferDestroy(dma_ch, &transfer);
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
	fpgaDMATransferDestroy(dma_ch, &transfer);
	return FPGA_OK;
}

void do_mm_test(fpga_dma_handle dma_handle, uint32_t channel, uint32_t use_ase)
{
	fpga_result res = FPGA_OK;
	fpga_dma_channel_handle dma_ch;
	uint64_t count;
	uint64_t *dma_buf_ptr = NULL;

	res = fpgaDMAOpenChannel(dma_handle, channel, &dma_ch);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMAOpenChannel");
	if (!dma_ch) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, out_dma_close, "Invaid DMA Handle");
	}

	if (use_ase) {
		count = ASE_TEST_BUF_SIZE;
	} else {
		count = TEST_BUF_SIZE;
	}

	dma_buf_ptr = (uint64_t *)malloc_aligned(getpagesize(), count);
	if (!dma_buf_ptr) {
		res = FPGA_NO_MEMORY;
		ON_ERR_GOTO(res, out_dma_close, "Error allocating memory");
	}

	int rr = madvise(dma_buf_ptr, count, MADV_SEQUENTIAL);
	ON_ERR_GOTO((rr == 0) ? FPGA_OK : FPGA_EXCEPTION, out_dma_close,
		    "Error madvise");

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

	fpga_dma_transfer transfer;
	fpgaDMATransferInit(dma_ch, &transfer);

	fpgaDMATransferSetSrc(transfer, (uint64_t)dma_buf_ptr);
	fpgaDMATransferSetDst(transfer, 0);
	fpgaDMATransferSetLen(transfer, count);
	fpgaDMATransferSetTransferType(transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync HOST_TO_FPGA_MM");
	clear_buffer((char *)dma_buf_ptr, count);

#ifdef CHECK_DELAYS
	printf("H->F size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	// copy from fpga to host
	fpgaDMATransferSetSrc(transfer, 0);
	fpgaDMATransferSetDst(transfer, (uint64_t)dma_buf_ptr);
	fpgaDMATransferSetLen(transfer, count);
	fpgaDMATransferSetTransferType(transfer, FPGA_TO_HOST_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync FPGA_TO_HOST_MM");

#ifdef CHECK_DELAYS
	printf("F->H size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = verify_buffer((char *)dma_buf_ptr, count);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	clear_buffer((char *)dma_buf_ptr, count);

	// copy from fpga to fpga
	fpgaDMATransferSetSrc(transfer, 0);
	fpgaDMATransferSetDst(transfer, count);
	fpgaDMATransferSetLen(transfer, count);
	fpgaDMATransferSetTransferType(transfer, FPGA_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync FPGA_TO_FPGA_MM");

#ifdef CHECK_DELAYS
	printf("F->F size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	// copy from fpga to host
	fpgaDMATransferSetSrc(transfer, count);
	fpgaDMATransferSetDst(transfer, (uint64_t)dma_buf_ptr);
	fpgaDMATransferSetLen(transfer, count);
	fpgaDMATransferSetTransferType(transfer, FPGA_TO_HOST_MM);
	fpgaDMATransferSetTransferCallback(transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync FPGA_TO_HOST_MM");

#ifdef CHECK_DELAYS
	printf("F->H size 0x%lx, %s\n", count, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = verify_buffer((char *)dma_buf_ptr, count);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	printf("Starting small buffer test\n");

	// Try small transfer
	fpga_dma_transfer sm_transfer;
	uint64_t siz_buf = 2 * 1024 * 1024;
	char *buf_ptr;
	fpgaDMATransferInitSmall(dma_ch, &siz_buf, (void **)&buf_ptr,
				 &sm_transfer);
	fill_buffer(&buf_ptr[2048], 0x3800);

	fpgaDMATransferSetSrc(sm_transfer, (uint64_t)&buf_ptr[2048]);
	fpgaDMATransferSetDst(sm_transfer, 0x200);
	fpgaDMATransferSetLen(sm_transfer, 0x3800);
	fpgaDMATransferSetTransferType(sm_transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, sm_transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync HOST_TO_FPGA_MM");
	clear_buffer(buf_ptr, siz_buf);

#ifdef CHECK_DELAYS
	printf("H->F size 0x%lx, %s\n", 0x3800, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	// copy from fpga to host
	fpgaDMATransferSetSrc(sm_transfer, 0x200);
	fpgaDMATransferSetDst(sm_transfer, (uint64_t)&buf_ptr[1024]);
	fpgaDMATransferSetLen(sm_transfer, 0x3800);
	fpgaDMATransferSetTransferType(sm_transfer, FPGA_TO_HOST_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, NULL, NULL);
	res = fpgaDMATransferStart(dma_ch, sm_transfer);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferSync FPGA_TO_HOST_MM");

#ifdef CHECK_DELAYS
	printf("F->H size 0x%lx, %s\n", 0x3800, showDelays(cbuf));
	poll_wait_count = 0;
	buf_full_count = 0;
#endif

	res = verify_buffer((char *)&buf_ptr[1024], 0x3800);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	uint64_t sb_src = (uint64_t)buf_ptr;
	uint64_t sb_dst = 0x200;
	uint64_t sb_size = 0x1000;
	uint64_t sb_num_xfers = siz_buf / sb_size;
	uint64_t sb_i;
	struct timespec start, end, end2;
	double sb_time = 0.0;
	double sb_send_time = 0.0;
	int ii;

	printf("Starting asynchronous small transfer bandwidth tests\n");
	async_buf_count = 0;

	fill_buffer(buf_ptr, siz_buf);

	// Set up transfer to copy full buffer in 4K chunks
	sb_src = (uint64_t)buf_ptr;
	sb_dst = 0x200;
	sb_size = 0x1000;
	sb_num_xfers = siz_buf / sb_size;

	fpgaDMATransferSetLen(sm_transfer, sb_size);
	fpgaDMATransferSetTransferType(sm_transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, async_cb, NULL);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = (uint64_t)buf_ptr;
		sb_dst = 0x200;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync HOST_TO_FPGA_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		clock_gettime(CLOCK_MONOTONIC, &end2);
		sb_send_time += getTime(start, end2);

		while (async_buf_count < sb_num_xfers)
			sched_yield();
		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers Host to FPGA - %" PRIu64 " %" PRIu64
	       "-byte asynchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);
	printf("Send-only bandwidth:\n");
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_send_time);
	printf("Overhead waiting per transfer: %10.4g seconds\n",
	       ((sb_time - sb_send_time) / 1000.0) / sb_num_xfers);

	sb_time = 0.0;
	sb_send_time = 0.0;

	clear_buffer(buf_ptr, siz_buf);
	async_buf_count = 0;
	fpgaDMATransferSetTransferType(sm_transfer, FPGA_TO_HOST_MM);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = 0x200;
		sb_dst = (uint64_t)buf_ptr;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync FPGA_TO_HOST_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		while (async_buf_count < sb_num_xfers)
			sched_yield();
		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers FPGA to Host - %" PRIu64 " %" PRIu64
	       "-byte asynchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	res = verify_buffer(buf_ptr, siz_buf);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	sb_time = 0.0;
	sb_send_time = 0.0;

	async_buf_count = 0;

	fill_buffer(buf_ptr, siz_buf);

	// Set up transfer to copy full buffer in 1M chunks
	sb_size = siz_buf / 2;
	sb_num_xfers = siz_buf / sb_size;

	fpgaDMATransferSetLen(sm_transfer, sb_size);
	fpgaDMATransferSetTransferType(sm_transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, async_cb, NULL);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = (uint64_t)buf_ptr;
		sb_dst = 0x0;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync HOST_TO_FPGA_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		while (async_buf_count < sb_num_xfers)
			sched_yield();
		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers Host to FPGA - %" PRIu64 " %" PRIu64
	       "-byte asynchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	sb_time = 0.0;
	sb_send_time = 0.0;

	clear_buffer(buf_ptr, siz_buf);
	async_buf_count = 0;
	fpgaDMATransferSetTransferType(sm_transfer, FPGA_TO_HOST_MM);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = 0x0;
		sb_dst = (uint64_t)buf_ptr;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync FPGA_TO_HOST_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		while (async_buf_count < sb_num_xfers)
			sched_yield();
		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers FPGA to Host - %" PRIu64 " %" PRIu64
	       "-byte asynchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	sb_time = 0.0;
	sb_send_time = 0.0;

	res = verify_buffer(buf_ptr, siz_buf);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");


	printf("Starting synchronous small transfer bandwidth tests\n");
	async_buf_count = 0;

	fill_buffer(buf_ptr, siz_buf);

	// Set up transfer to copy full buffer in 4K chunks
	sb_src = (uint64_t)buf_ptr;
	sb_dst = 0x200;
	sb_size = 0x1000;
	sb_num_xfers = siz_buf / sb_size;

	fpgaDMATransferSetLen(sm_transfer, sb_size);
	fpgaDMATransferSetTransferType(sm_transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, NULL, NULL);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = (uint64_t)buf_ptr;
		sb_dst = 0x200;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync HOST_TO_FPGA_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers Host to FPGA - %" PRIu64 " %" PRIu64
	       "-byte synchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	sb_time = 0.0;
	sb_send_time = 0.0;

	clear_buffer(buf_ptr, siz_buf);
	async_buf_count = 0;
	fpgaDMATransferSetTransferType(sm_transfer, FPGA_TO_HOST_MM);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = 0x200;
		sb_dst = (uint64_t)buf_ptr;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync FPGA_TO_HOST_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers FPGA to Host - %" PRIu64 " %" PRIu64
	       "-byte synchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	res = verify_buffer(buf_ptr, siz_buf);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	sb_time = 0.0;
	sb_send_time = 0.0;

	async_buf_count = 0;

	fill_buffer(buf_ptr, siz_buf);

	// Set up transfer to copy full buffer in 1M chunks
	sb_size = siz_buf / 2;
	sb_num_xfers = siz_buf / sb_size;

	fpgaDMATransferSetLen(sm_transfer, sb_size);
	fpgaDMATransferSetTransferType(sm_transfer, HOST_TO_FPGA_MM);
	fpgaDMATransferSetTransferCallback(sm_transfer, NULL, NULL);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = (uint64_t)buf_ptr;
		sb_dst = 0x0;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync HOST_TO_FPGA_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers Host to FPGA - %" PRIu64 " %" PRIu64
	       "-byte synchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	sb_time = 0.0;
	sb_send_time = 0.0;

	clear_buffer(buf_ptr, siz_buf);
	async_buf_count = 0;
	fpgaDMATransferSetTransferType(sm_transfer, FPGA_TO_HOST_MM);

	for (ii = 0; ii < 1000; ii++) {
		sb_src = 0x0;
		sb_dst = (uint64_t)buf_ptr;
		async_buf_count = 0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (sb_i = 0; sb_i < sb_num_xfers; sb_i++) {
			fpgaDMATransferSetSrc(sm_transfer, sb_src);
			fpgaDMATransferSetDst(sm_transfer, sb_dst);
			res = fpgaDMATransferStart(dma_ch, sm_transfer);
			ON_ERR_GOTO(res, out_dma_close,
				    "fpgaDMATransferSync FPGA_TO_HOST_MM");
			sb_src += sb_size;
			sb_dst += sb_size;
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		sb_time += getTime(start, end);
	}

	printf("Small transfers FPGA to Host - %" PRIu64 " %" PRIu64
	       "-byte synchronous transfers\n",
	       sb_num_xfers, sb_size);
	report_bandwidth(sb_size * sb_num_xfers * 1000, sb_time);

	sb_time = 0.0;
	sb_send_time = 0.0;

	res = verify_buffer(buf_ptr, siz_buf);
	ON_ERR_GOTO(res, out_dma_close, "verify_buffer");

	res = fpgaDMATransferDestroy(dma_ch, &sm_transfer);
	ON_ERR_GOTO(res, out_dma_close, "destroy small transfer");

	if (!use_ase) {
		printf("Running DDR sweep test\n");
#define MB(x) ((uint64_t)(x)*1024 * 1024)
		res = ddr_sweep(dma_ch, 0, 0, MB(2096), 0);
#ifndef DO_SHORT_TEST
		res |= ddr_sweep(dma_ch, 0, 0, MB(10), 0);
#endif
		res |= ddr_sweep(dma_ch, 0, 0, MB(1), 200);
		res |= ddr_sweep(dma_ch, 0, 0, 4096, 200);
#ifndef DO_SHORT_TEST
		printf("DDR sweep with unaligned pointer and size\n");
		res |= ddr_sweep(dma_ch, 61, 5, MB(3), 3);
		res |= ddr_sweep(dma_ch, 3, 0, MB(22), 6);
		res |= ddr_sweep(dma_ch, 7, 3, MB(1024), 0xfc00);
		res |= ddr_sweep(dma_ch, 0, 3, MB(2048), 0);
		res |= ddr_sweep(dma_ch, 0, 61, MB(4096), 1);
		res |= ddr_sweep(dma_ch, 0, 7, MB(4096), 0);
#endif
		ON_ERR_GOTO(res, out_dma_close, "ddr_sweep");
	}

	free(verify_buf);
	fpgaDMATransferDestroy(dma_ch, &transfer);

out_dma_close:
	free_aligned(dma_buf_ptr);
	if (dma_ch)
		res = fpgaDMACloseChannel(&dma_ch);
	ON_ERR_GOTO(res, out, "fpgaDMACloseChannel");

out:
	return;
}
