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

#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <time.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"
#include <unistd.h>
#ifndef USE_ASE
#include <hwloc.h>
#include <assert.h>
#endif
/**
 * \fpga_dma_st_test.c
 * \brief Streaming DMA test
 */

#include <stdlib.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>

#define ST_DMA_AFU_ID "EB59BF9D-B211-4A4E-B3E3-753CE68634BA"
#define MM_DMA_AFU_ID "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (20 * 1024 * 1024)
#define ASE_TEST_BUF_SIZE (4 * 1024)
// Single pattern is represented as 64Bytes
#define PATTERN_WIDTH 64
// No. of Patterns
#define PATTERN_LENGTH 32

int err_cnt = 0;

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

typedef struct _pool_metrics {
	uint32_t batch_size;
	uint32_t num_items;
	double enq_new_items_per_sec;
	double enq_batch_items_per_sec;
	double deq_new_items_per_sec;
	double deq_batch_items_per_sec;
} queue_metrics;

typedef struct _thread_context {
	fpga_dma_handle_t *dma_h;
	fpga_dma_channel_handle dma_ch;
	sem_t alive_sem;
	sem_t go_sem;
	queue_metrics metrics;
	uint32_t thread_num;
	pthread_t thread_id;
} thread_context;

static int NUM_THREADS;
thread_context *threads;
qinfo_t gq;
qinfo_t *queue = &gq;

fpga_result verify_buffer(uint8_t *buf, size_t payload_size)
{
	size_t i, j;
	uint8_t test_word = 0;
	while (payload_size) {
		test_word = 0x01;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH / sizeof(test_word));
			     j++) {
				if (!payload_size)
					goto out;
				if ((*buf) != test_word) {
					printf("Invalid data at %zx Expected = %x Actual = %x\n",
					       i, test_word, (*buf));
					return FPGA_INVALID_PARAM;
				}
				payload_size -= sizeof(test_word);
				buf++;
				test_word += 0x01;
				if (test_word == 0xfd)
					test_word = 0x01;
			}
		}
	}
out:
	printf("S2M: Data Verification Success!\n");
	return FPGA_OK;
}

void clear_buffer(uint32_t *buf, size_t size)
{
	memset(buf, 0, size);
}

#define UNUSED(x) (void)(x)

void report_bandwidth(size_t size, double seconds)
{
	double throughput = (double)size / ((double)seconds * 1000 * 1000);
	printf("\rMeasured bandwidth = %lf Megabytes/sec\n", throughput);
}

// return elapsed time
double getTime(struct timespec start, struct timespec end)
{
	uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec
			- start.tv_nsec;
	return (double)diff / (double)1000000000L;
}

void *queue_thread(void *ctx)
{
#define ITERATIONS 1000
#define NUM_TRANSFERS 1000
#define BATCH_SIZE 20
	assert((NUM_TRANSFERS / BATCH_SIZE)
	       == ((double)NUM_TRANSFERS / (double)BATCH_SIZE));

	thread_context *th = (thread_context *)ctx;
	queue_metrics *metrics = &th->metrics;
	struct timespec start, end;
	fpga_result res = FPGA_OK;

	metrics->batch_size = BATCH_SIZE;
	metrics->num_items = NUM_TRANSFERS;

	fpga_dma_transfer_t *xfers[NUM_TRANSFERS];
	fpga_dma_transfer_t xfers_ret[NUM_TRANSFERS];

	int i;
	double tot_enq_time = 0.0;
	double tot_deq_time = 0.0;

	for (i = 0; i < NUM_TRANSFERS; i++) {
		if (FPGA_OK
		    != fpgaDMATransferInit(th->dma_ch,
					   (fpga_dma_transfer *)&xfers[i])) {
			fprintf(stderr, "TransferInit failed.\n");
		}
	}

	// Synchronize
	sem_post(&th->alive_sem);
	sem_wait(&th->go_sem);

	// m2m_dma_handle_t *dma_ch = (m2m_dma_handle_t *)th->dma_ch;

	// Time initial creation:
	// Enqueue a bunch, then dequeue them
	for (i = 0; i < ITERATIONS; i++) {
		int j;

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (j = 0; j < NUM_TRANSFERS; j++) {
			fpgaDMAEnqueue(queue, xfers[i]);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		tot_enq_time += getTime(start, end);

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (j = 0; j < NUM_TRANSFERS; j++) {
			fpgaDMADequeue(queue, &xfers_ret[i]);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		tot_deq_time += getTime(start, end);

		res = fpgaDMAQueueDestroy(th->dma_h, queue, true);
		if (FPGA_OK != res) {
			fprintf(stderr, "fpgaDMAQueueDestroy failure.\n");
		}

#if 0
		res = fpgaDMAQueueInit(th->dma_h, queue);
		if (FPGA_OK != res) {
			fprintf(stderr, "fpgaDMAQueueInit failure.\n");
		}
#endif
	}

	metrics->enq_new_items_per_sec =
		(double)NUM_TRANSFERS / tot_enq_time * (double)ITERATIONS;
	tot_enq_time = 0.0;

	metrics->deq_new_items_per_sec =
		(double)NUM_TRANSFERS / tot_deq_time * (double)ITERATIONS;
	tot_deq_time = 0.0;

	// Time getting free:
	// Create BATCH_SIZE items, get them in a block
	for (i = 0; i < BATCH_SIZE; i++) {
		fpgaDMAEnqueue(queue, xfers[i]);
	}

	for (i = 0; i < BATCH_SIZE; i++) {
		fpgaDMADequeue(queue, &xfers_ret[i]);
	}

	for (i = 0; i < ITERATIONS; i++) {
		int j;
		int k;

		for (k = 0; k < NUM_TRANSFERS; k += BATCH_SIZE) {
			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 2 * BATCH_SIZE; j < 3 * BATCH_SIZE; j++) {
				fpgaDMAEnqueue(queue, xfers[j]);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_enq_time += getTime(start, end);

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 2 * BATCH_SIZE; j < 3 * BATCH_SIZE; j++) {
				fpgaDMADequeue(queue, &xfers_ret[j]);
			}
			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_deq_time += getTime(start, end);
		}
	}

	for (i = 0; i < NUM_TRANSFERS; i++) {
		fpgaDMATransferDestroy(th->dma_ch, (void **)&xfers[i]);
	}

	metrics->enq_batch_items_per_sec =
		(double)NUM_TRANSFERS / tot_enq_time * (double)ITERATIONS;
	tot_enq_time = 0.0;

	metrics->deq_batch_items_per_sec =
		(double)NUM_TRANSFERS / tot_deq_time * (double)ITERATIONS;
	tot_deq_time = 0.0;

	return NULL;
}

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	fpga_result res = FPGA_OK;
	fpga_dma_handle dma_handle;
	int i;
	fpga_properties filter[2] = {NULL, NULL};
	fpga_token afc_token;
	fpga_handle afc_h;
	fpga_guid guid;
	fpga_guid mm_guid;
	uint32_t num_matches;
	uint32_t count;
#ifndef USE_ASE
	volatile uint64_t *mmio_ptr = NULL;
#endif

	if (argc == 2) {
		NUM_THREADS = atoi(argv[1]);
	} else {
		NUM_THREADS = 1;
	}

	threads = (thread_context *)calloc(NUM_THREADS, sizeof(thread_context));

	// enumerate the afc
	if (uuid_parse(ST_DMA_AFU_ID, guid) < 0) {
		return 1;
	}
	if (uuid_parse(MM_DMA_AFU_ID, mm_guid) < 0) {
		return 1;
	}

	res = fpgaGetProperties(NULL, &filter[0]);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");
	res = fpgaGetProperties(NULL, &filter[1]);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");

	res = fpgaPropertiesSetObjectType(filter[0], FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");
	res = fpgaPropertiesSetObjectType(filter[1], FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");

	res = fpgaPropertiesSetGUID(filter[0], guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");
	res = fpgaPropertiesSetGUID(filter[1], mm_guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");

	res = fpgaEnumerate(&filter[0], 2, &afc_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaEnumerate");

	if (num_matches < 1) {
		printf("Error: Number of matches < 1\n");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out_destroy_prop,
			    "num_matches<1");
	}

	// open the AFC
	res = fpgaOpen(afc_token, &afc_h, 0);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaOpen");

#ifndef USE_ASE
	res = fpgaMapMMIO(afc_h, 0, (uint64_t **)&mmio_ptr);
	ON_ERR_GOTO(res, out_close, "fpgaMapMMIO");
#endif

	// reset AFC
	res = fpgaReset(afc_h);
	ON_ERR_GOTO(res, out_unmap, "fpgaReset");

	res = fpgaDMAOpen(afc_h, &dma_handle);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAOpen");

	// Enumerate DMA handles
	res = fpgaDMAEnumerateChannels(dma_handle, 0, NULL, &count);
	ON_ERR_GOTO(res, dma_close, "fpgaDMAEnumerateChannels");

	if (count < 1) {
		printf("Error: DMA channels not found\n");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, dma_close, "count<1");
	}
	printf("No of DMA channels = %08x\n", count);

	fpga_dma_channel_desc *descs = (fpga_dma_channel_desc *)malloc(
		sizeof(fpga_dma_channel_desc) * count);
	res = fpgaDMAEnumerateChannels(dma_handle, count, descs, &count);
	ON_ERR_GOTO(res, free_descs, "fpgaDMAEnumerateChannels");
	res = fpgaDMAOpenChannel(dma_handle, 0, &threads[0].dma_ch);
	ON_ERR_GOTO(res, free_descs, "fpgaDMAEnumerateChannels");

free_descs:
	free(descs);

	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)dma_handle;
	res = fpgaDMAQueueInit(dma_h, queue);
	if (FPGA_OK != res) {
		fprintf(stderr, "QueueInit failure.\n");
	}

	// Spawn threads to pound on the various pools
	for (i = 0; i < NUM_THREADS; i++) {
		threads[i].dma_h = dma_h;
		threads[i].dma_ch = threads[0].dma_ch;
		sem_init(&threads[i].alive_sem, 0, 0);
		sem_init(&threads[i].go_sem, 0, 0);
		threads[i].thread_num = i;
		pthread_create(&threads[i].thread_id, NULL, queue_thread,
			       (void *)&threads[i]);
	}

	// Wait for threads to come alive
	for (i = 0; i < NUM_THREADS; i++) {
		sem_wait(&threads[i].alive_sem);
	}

	// Let threads start processing
	for (i = 0; i < NUM_THREADS; i++) {
		sem_post(&threads[i].go_sem);
	}

	// Wait for threads to finish
	for (i = 0; i < NUM_THREADS; i++) {
		void *ret;
		pthread_join(threads[i].thread_id, &ret);
	}

	fpgaDMAQueueDestroy(dma_h, queue, false);
	res = fpgaDMACloseChannel(&threads[0].dma_ch);
	ON_ERR_GOTO(res, free_descs, "fpgaDMAEnumerateChannels");

	printf("Queue performance\n\n");

	queue_metrics avg = {0};

	for (i = 0; i < NUM_THREADS; i++) {
		queue_metrics *mets = &threads[i].metrics;
		printf("Thread %d:  Batch size: %d items\n",
		       threads[i].thread_num, mets->batch_size);
		printf("\t %15.3lf new enqueues/sec\n",
		       mets->enq_new_items_per_sec);
		printf("\t %15.3lf new dequeues/sec\n",
		       mets->deq_new_items_per_sec);
		printf("\t %15.3lf steady-state enqueues/sec\n",
		       mets->enq_batch_items_per_sec);
		printf("\t %15.3lf steady-state dequeues/sec\n\n",
		       mets->deq_batch_items_per_sec);

		avg.deq_batch_items_per_sec += mets->deq_batch_items_per_sec;
		avg.deq_new_items_per_sec += mets->deq_new_items_per_sec;
		avg.enq_batch_items_per_sec += mets->enq_batch_items_per_sec;
		avg.enq_new_items_per_sec += mets->enq_new_items_per_sec;
	}

	printf("Average per thread (%d threads):\n", NUM_THREADS);
	printf("\t %15.3lf new enqueues/sec\n",
	       avg.enq_new_items_per_sec / NUM_THREADS);
	printf("\t %15.3lf new dequeues/sec\n",
	       avg.deq_new_items_per_sec / NUM_THREADS);
	printf("\t %15.3lf steady-state enqueues/sec\n",
	       avg.enq_batch_items_per_sec / NUM_THREADS);
	printf("\t %15.3lf steady-state dequeues/sec\n\n",
	       avg.deq_batch_items_per_sec / NUM_THREADS);


dma_close:
	res = fpgaDMAClose(&dma_handle);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAClose");

out_unmap:
#ifndef USE_ASE
	res = fpgaUnmapMMIO(afc_h, 0);
	ON_ERR_GOTO(res, out_close, "fpgaUnmapMMIO");

out_close:
#endif
	res = fpgaClose(afc_h);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaClose");

out_destroy_tok:
	res = fpgaDestroyToken(&afc_token);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaDestroyToken");

out_destroy_prop:
	res = fpgaDestroyProperties(&filter[0]);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");
	res = fpgaDestroyProperties(&filter[1]);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");

out:
	return err_cnt;
}
