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
#include <inttypes.h>

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

enum _etype {
	TYPE_MUTEX = 0,
	TYPE_SPIN,
	TYPE_PTHREAD_SPIN,
	TYPE_SEM,
	TYPE_ATOMIC,
	TYPE_LAST,
} etype;

typedef struct _lock_metrics {
	double mutex_per_sec;
	double spinlock_per_sec;
	double pthread_spinlock_per_sec;
	double semaphore_per_sec;
	double atomic_per_sec;
} lock_metrics;

typedef struct _thread_context {
	sem_t alive_sem;
	sem_t go_sem;
	lock_metrics metrics;
	uint32_t thread_num;
	pthread_t thread_id;
	uint32_t type;
	uint64_t iterations;
} thread_context;

static volatile uint64_t prot_var;
static pthread_spinlock_t g_spin;
static pthread_mutex_t g_mutex;
static sem_t g_sem;
static volatile uint8_t g_lock2;

static int NUM_THREADS;
thread_context *threads;

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

void *lock_thread(void *ctx)
{
#define ITERATIONS 1000
	thread_context *th = (thread_context *)ctx;
	uint64_t iterations = th->iterations;
	uint32_t type = th->type;
	lock_metrics *metrics = &th->metrics;
	struct timespec start, end;

	int i;
	double tot_time = 0.0;

	// Synchronize
	sem_post(&th->alive_sem);
	sem_wait(&th->go_sem);

	switch (type) {
	case TYPE_MUTEX:
		for (i = 0; i < ITERATIONS; i++) {
			uint64_t j;

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 0; j < iterations; j++) {
				pthread_mutex_lock(&g_mutex);
				prot_var++;
				pthread_mutex_unlock(&g_mutex);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_time += getTime(start, end);
		}

		metrics->mutex_per_sec =
			(double)iterations / tot_time * (double)ITERATIONS;
		break;

	case TYPE_SPIN:
		for (i = 0; i < ITERATIONS; i++) {
			uint64_t j;

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 0; j < iterations; j++) {
				while (!__sync_bool_compare_and_swap(&g_lock2,
								     0, 1))
					;
				prot_var++;
				__sync_bool_compare_and_swap(&g_lock2, 1, 0);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_time += getTime(start, end);
		}

		metrics->spinlock_per_sec =
			(double)iterations / tot_time * (double)ITERATIONS;
		break;

	case TYPE_PTHREAD_SPIN:
		for (i = 0; i < ITERATIONS; i++) {
			uint64_t j;

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 0; j < iterations; j++) {
				pthread_spin_lock(&g_spin);
				prot_var++;
				pthread_spin_unlock(&g_spin);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_time += getTime(start, end);
		}

		metrics->pthread_spinlock_per_sec =
			(double)iterations / tot_time * (double)ITERATIONS;
		break;

	case TYPE_SEM:
		for (i = 0; i < ITERATIONS; i++) {
			uint64_t j;

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 0; j < iterations; j++) {
				sem_wait(&g_sem);
				prot_var++;
				sem_post(&g_sem);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_time += getTime(start, end);
		}

		metrics->semaphore_per_sec =
			(double)iterations / tot_time * (double)ITERATIONS;
		break;

	case TYPE_ATOMIC:
		for (i = 0; i < ITERATIONS; i++) {
			uint64_t j;

			clock_gettime(CLOCK_MONOTONIC, &start);
			for (j = 0; j < iterations; j++) {
				__sync_fetch_and_add(&prot_var, 1);
			}

			clock_gettime(CLOCK_MONOTONIC, &end);
			tot_time += getTime(start, end);
		}

		metrics->atomic_per_sec =
			(double)iterations / tot_time * (double)ITERATIONS;
		break;

	default:
		return NULL;
		break;
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	int i;
	int type = 0;
	pthread_attr_t attr;

	lock_metrics avg;
	memset(&avg, 0, sizeof(avg));

	if (argc == 2) {
		NUM_THREADS = atoi(argv[1]);
	} else {
		NUM_THREADS = 1;
	}

	pthread_spin_init(&g_spin, 0);
	pthread_mutex_init(&g_mutex, NULL);
	sem_init(&g_sem, 0, 1);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (type = 0; type < TYPE_LAST; type++) {
		threads = (thread_context *)calloc(NUM_THREADS,
						   sizeof(thread_context));
		prot_var = 0ULL;

		// Spawn threads
		for (i = 0; i < NUM_THREADS; i++) {
			sem_init(&threads[i].alive_sem, 0, 0);
			sem_init(&threads[i].go_sem, 0, 0);
			threads[i].thread_num = i;
			threads[i].type = type;
			threads[i].iterations = 0x10000ULL;
			pthread_create(&threads[i].thread_id, NULL, lock_thread,
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

		uint64_t good_val = NUM_THREADS * threads[0].iterations * ITERATIONS;

		if (prot_var == good_val) {
			printf("Validation succeeded!\n");
		} else {
			printf("Validation failed!  Expected %" PRIu64
			       ", got %" PRIu64 "\n",
			       good_val, prot_var);
		}

		printf("Lock performance for ");
		switch (type) {
		case TYPE_MUTEX:
			printf("pthread_mutex\n\n");
			break;

		case TYPE_SPIN:
			printf("spinlock\n\n");
			break;

		case TYPE_PTHREAD_SPIN:
			printf("pthread_spin\n\n");
			break;

		case TYPE_SEM:
			printf("semaphore\n\n");
			break;

		case TYPE_ATOMIC:
			printf("atomic\n\n");
			break;

		default:
			return -1;
			break;
		}

		double *avg_val = (double *)&avg;

		for (i = 0; i < NUM_THREADS; i++) {
			lock_metrics *mets = &threads[i].metrics;
			double *val = (double *)mets;
			printf("Thread %d, %" PRIu64 " iterations:\n",
			       threads[i].thread_num, threads[i].iterations);
			printf("\t %15.3lf lock/unlock pairs/sec\n", val[type]);

			avg_val[type] += val[type];
		}

		printf("Average per thread (%d threads):\n", NUM_THREADS);
		printf("\t %15.3lf lock/unlock pairs/sec\n\n",
		       avg_val[type] / NUM_THREADS);

		free(threads);
	}

	return err_cnt;
}
