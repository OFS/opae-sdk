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

/**
 * \queue_mgmt.c
 * \brief Manage transfer queue(s)
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <safe_string/safe_string.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

// Internal Functions

static inline fpga_dma_transfer_t *getFreeTransfer(qinfo_t *q,
						   fpga_dma_transfer_t *tf)
{
	fpga_dma_transfer_t *new_t = NULL;

	if (q->num_free) {
		q->num_free--;
		new_t = q->free_queue[q->num_free];
	} else {
		new_t = (fpga_dma_transfer_t *)malloc(
			sizeof(fpga_dma_transfer_t));
		if (!new_t) {
			FPGA_DMA_ST_ERR("transfer alloc failed");
		}
	}

	local_memcpy(new_t, tf, sizeof(fpga_dma_transfer_t));

	return new_t;
}

inline fpga_result fpgaDMAQueueInit(fpga_dma_handle_t *dma_h, qinfo_t *q)
{
	memset(q, 0, sizeof(qinfo_t));
	q->read_index = q->write_index = -1;
	if (pthread_spin_init(&q->q_mutex, 0)) {
		FPGA_DMA_ST_ERR("pthread_spin_init failed");
		return FPGA_EXCEPTION;
	}

	q->q_semaphore = getFreeSemaphore(&dma_h->main_header, 0, 0);
	if (!q->q_semaphore) {
		FPGA_DMA_ST_ERR("sem_init failed");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

inline fpga_result fpgaDMAQueueDestroy(fpga_dma_handle_t *dma_h, qinfo_t *q,
				       bool free_only)
{
	int i = 0;

	if (!q->q_semaphore) {
		return FPGA_OK;
	}

	if (pthread_spin_lock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return FPGA_EXCEPTION;
	}

	for (i = 0; i < q->num_free; i++) {
		if (q->free_queue[i]->tf_mutex) {
			releasePoolItem(&dma_h->main_header,
					q->free_queue[i]->tf_mutex);
			releasePoolItem(&dma_h->main_header,
					q->free_queue[i]->tf_semaphore);
		}
		free(q->free_queue[i]);
	}
	q->num_free = 0;

	for (i = 0; !free_only && (i < FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS);
	     i++) {
		if (q->queue[i]) {
			if (q->queue[i]->tf_mutex) {
				releasePoolItem(&dma_h->main_header,
						q->queue[i]->tf_mutex);
				releasePoolItem(&dma_h->main_header,
						q->queue[i]->tf_semaphore);
			}
			free(q->queue[i]);
		}
	}

	if (!free_only) {
		q->read_index = q->write_index = -1;
	}

	if (pthread_spin_unlock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return FPGA_EXCEPTION;
	}

	if (!free_only) {
		releasePoolItem(&dma_h->main_header, q->q_semaphore);

		if (pthread_spin_destroy(&q->q_mutex)) {
			FPGA_DMA_ST_ERR("pthread_spin_destroy failed");
		}
		q->q_semaphore = NULL;
	}

	return FPGA_OK;
}

inline fpga_result fpgaDMAEnqueue(qinfo_t *q, fpga_dma_transfer_t *tf)
{
	int value = 0;

	if (pthread_spin_lock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return FPGA_EXCEPTION;
	}

	if (sem_getvalue(&q->q_semaphore->m_semaphore, &value)) {
		FPGA_DMA_ST_ERR("sem_getvalue failed");
		if (pthread_spin_unlock(&q->q_mutex))
			FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return FPGA_EXCEPTION;
	}

	if (value == FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS) {
		FPGA_DMA_ST_ERR(
			"Maximum inflight transactions reached in queue");
		if (pthread_spin_unlock(&q->q_mutex)) {
			FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
			return FPGA_EXCEPTION;
		}
		return FPGA_BUSY;
	}

	// Increment tail index
	q->write_index =
		(q->write_index + 1) % FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS;
	// Add the item to the Queue
	q->queue[q->write_index] = getFreeTransfer(q, tf);
	q->queue[q->write_index]->tf_mutex = NULL;

	if (sem_post(&q->q_semaphore->m_semaphore)) {
		FPGA_DMA_ST_ERR("sem_post failed");
		if (pthread_spin_unlock(&q->q_mutex))
			FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return FPGA_EXCEPTION;
	}

	if (pthread_spin_unlock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

inline fpga_result fpgaDMADequeue(qinfo_t *q, fpga_dma_transfer_t *tf)
{
	// Wait till have an entry
	if (sem_wait(&q->q_semaphore->m_semaphore)) {
		FPGA_DMA_ST_ERR("sem_wait failed");
		return FPGA_EXCEPTION;
	}

	if (pthread_spin_lock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return FPGA_EXCEPTION;
	}

	q->read_index =
		(q->read_index + 1) % FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS;

	if (q->queue[q->read_index]->transfer_type == TERMINATE_THREAD) {
		if (pthread_spin_unlock(&q->q_mutex)) {
			FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
			return FPGA_EXCEPTION;
		}
		return FPGA_NO_ACCESS;
	}

	local_memcpy(tf, q->queue[q->read_index], sizeof(fpga_dma_transfer_t));

	q->free_queue[q->num_free++] = q->queue[q->read_index];
	q->queue[q->read_index] = NULL;

	if (pthread_spin_unlock(&q->q_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}
