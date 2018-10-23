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
 * \sem_mutex_mgmt.c
 * \brief Manage semaphore / mutex pairs (cannot be copied)
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

//#define DBG_POOLS

static void print_lists(const char *who, handle_common *comm)
{
#ifndef DBG_POOLS
	(void)who;
	(void)comm;
#else
	sem_pool_item *sem_in_use_head = comm->dma_h->sem_in_use_head;
	sem_pool_item *sem_free_head = comm->dma_h->sem_free_head;
	mutex_pool_item *mut_in_use_head = comm->dma_h->mutex_in_use_head;
	mutex_pool_item *mut_free_head = comm->dma_h->mutex_free_head;
	buffer_pool_item *buf_in_use_head = comm->dma_h->buffer_in_use_head;
	buffer_pool_item *buf_free_head = comm->dma_h->buffer_free_head;

	printf("*** %s ***\n", who);
	printf("Semaphores:\n\tIn use: ");
	while (sem_in_use_head) {
		printf("%p ", sem_in_use_head);
		sem_in_use_head = sem_in_use_head->next;
	}
	printf("\n\tFree: ");
	while (sem_free_head) {
		printf("%p ", sem_free_head);
		sem_free_head = sem_free_head->next;
	}
	printf("\nMutexes:\n\tIn use: ");
	while (mut_in_use_head) {
		printf("%p ", mut_in_use_head);
		mut_in_use_head = mut_in_use_head->next;
	}
	printf("\n\tFree: ");
	while (mut_free_head) {
		printf("%p ", mut_free_head);
		mut_free_head = mut_free_head->next;
	}
	printf("\nBuffers:\n\tIn use: ");
	while (buf_in_use_head) {
		printf("%p ", buf_in_use_head);
		buf_in_use_head = buf_in_use_head->next;
	}
	printf("\n\tFree: ");
	while (buf_free_head) {
		printf("%p ", buf_free_head);
		buf_free_head = buf_free_head->next;
	}
	printf("\n");
#endif
}

// Get and initialize a semaphore
inline sem_pool_item *getFreeSemaphore(handle_common *comm, int pshared,
				       int sem_value)
{
	sem_pool_item *new_sem = NULL;

	if (pthread_spin_lock(&comm->dma_h->sem_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		new_sem = NULL;
		goto out_unlock;
	}

	sem_pool_item *in_use_head = comm->dma_h->sem_in_use_head;
	sem_pool_item *free_head = comm->dma_h->sem_free_head;

	if (free_head) {
		new_sem = free_head;
		assert(free_head->header.type == POOL_SEMA);

		if (sem_destroy(&new_sem->m_semaphore)) {
			FPGA_DMA_ST_ERR("getFreeSemaphore sem_destroy failed");
			new_sem = NULL;
			goto out_unlock;
		}

		if (sem_init(&new_sem->m_semaphore, pshared, sem_value)) {
			FPGA_DMA_ST_ERR("getFreeSemaphore sem_init failed");
			new_sem = NULL;
			goto out_unlock;
		}
		comm->dma_h->sem_free_head = free_head->next;
	} else {
		new_sem = (sem_pool_item *)calloc(1, sizeof(sem_pool_item));
		if (!new_sem) {
			FPGA_DMA_ST_ERR("getFreeSemaphore alloc failed");
			new_sem = NULL;
			goto out_unlock;
		}

		if (sem_init(&new_sem->m_semaphore, pshared, sem_value)) {
			free(new_sem);
			new_sem = NULL;
			goto out_unlock;
		}

		new_sem->header.type = POOL_SEMA;
	}

	new_sem->header.destroyed = 0;

	new_sem->next = in_use_head;
	comm->dma_h->sem_in_use_head = new_sem;

out_unlock:
	if (pthread_spin_unlock(&comm->dma_h->sem_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		new_sem = NULL;
	}

	print_lists("getFreeSemaphore", comm);

	return new_sem;
}

// Get and initialize a mutex
inline mutex_pool_item *getFreeMutex(handle_common *comm,
				     pthread_mutexattr_t *attr)
{
	mutex_pool_item *new_mutex = NULL;
	(void)attr;

	if (pthread_spin_lock(&comm->dma_h->mutex_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		new_mutex = NULL;
		goto out_unlock;
	}

	mutex_pool_item *in_use_head = comm->dma_h->mutex_in_use_head;
	mutex_pool_item *free_head = comm->dma_h->mutex_free_head;

	if (free_head) {
		new_mutex = free_head;
		comm->dma_h->mutex_free_head = free_head->next;
		if (pthread_mutex_trylock(&free_head->m_mutex)) {
			FPGA_DMA_ST_ERR("Mutex on free list still locked!!");
			goto out_unlock;
		}
		pthread_mutex_unlock(&free_head->m_mutex);
	} else {
		new_mutex =
			(mutex_pool_item *)calloc(1, sizeof(mutex_pool_item));
		if (!new_mutex) {
			new_mutex = NULL;
			goto out_unlock;
		}

		pthread_mutexattr_t attr2;
		pthread_mutexattr_init(&attr2);
		pthread_mutexattr_settype(&attr2, PTHREAD_MUTEX_ERRORCHECK);

		int err = pthread_mutex_init(&new_mutex->m_mutex, &attr2);
		pthread_mutexattr_destroy(&attr2);
		if (err) {
			free(new_mutex);
			new_mutex = NULL;
			FPGA_DMA_ST_ERR(
				"getFreeMutex pthread_mutex_init failed");
			fprintf(stderr, "ret is %d\n", err);
			goto out_unlock;
		}

		new_mutex->header.type = POOL_MUTEX;
	}

	new_mutex->header.destroyed = 0;

	new_mutex->next = in_use_head;
	comm->dma_h->mutex_in_use_head = new_mutex;

out_unlock:
	if (pthread_spin_unlock(&comm->dma_h->mutex_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		new_mutex = NULL;
	}

	print_lists("getFreeMutex", comm);

	return new_mutex;
}

// Get and initialize a pinned buffer
inline buffer_pool_item *getFreeBuffer(handle_common *comm)
{
	buffer_pool_item *new_buffer = NULL;
	fpga_result res = FPGA_OK;

	if (pthread_spin_lock(&comm->dma_h->buffer_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		new_buffer = NULL;
		goto out_unlock;
	}

	buffer_pool_item *in_use_head = comm->dma_h->buffer_in_use_head;
	buffer_pool_item *free_head = comm->dma_h->buffer_free_head;

	if (free_head) {
		new_buffer = free_head;
		comm->dma_h->buffer_free_head = free_head->next;
	} else {
		uint64_t siz = FPGA_DMA_BUF_SIZE;
		new_buffer =
			(buffer_pool_item *)calloc(1, sizeof(buffer_pool_item));
		if (!new_buffer) {
			new_buffer = NULL;
			goto out_unlock;
		}

		res = fpgaPrepareBuffer(comm->fpga_h, siz,
					(void **)&(new_buffer->dma_buf_ptr),
					&new_buffer->dma_buf_wsid, 0);
		ON_ERR_GOTO(res, out_unlock, "fpgaPrepareBuffer");

		res = fpgaGetIOAddress(comm->fpga_h, new_buffer->dma_buf_wsid,
				       &new_buffer->dma_buf_iova);
		ON_ERR_GOTO(res, rel_buf, "fpgaGetIOAddress");


		new_buffer->header.type = POOL_BUFFERS;
		new_buffer->size = siz;
	}

	new_buffer->header.destroyed = 0;

	new_buffer->next = in_use_head;
	comm->dma_h->buffer_in_use_head = new_buffer;

out_unlock:
	if (pthread_spin_unlock(&comm->dma_h->buffer_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		new_buffer = NULL;
	}

	print_lists("getFreeBuffer", comm);

	return new_buffer;

rel_buf:
	res = fpgaReleaseBuffer(comm->fpga_h, new_buffer->dma_buf_wsid);
	ON_ERR_GOTO(res, out_unlock, "fpgaReleaseBuffer");
	goto out_unlock;
}

// Put a sem/mutex pair onto the free list
static inline void releasePoolInt(void *item, void **in_use_head,
				  void **free_head)
{
	sem_pool_item *item_ptr = (sem_pool_item *)item;
	sem_pool_item *prev_ptr = (sem_pool_item *)*in_use_head;

	if (prev_ptr == item_ptr) {
		*in_use_head = item_ptr->next;
	} else {

		while ((prev_ptr) && (prev_ptr->next != item)) {
			prev_ptr = prev_ptr->next;
		}

		if (!prev_ptr) {
			// Not in use, already freed
			return;
		}

		prev_ptr->next = item_ptr->next;
	}

	item_ptr->next = *free_head;
	*free_head = item;

	return;
}

// Release (free) a pooled item (external Ifc)
inline void releasePoolItem(handle_common *comm, void *item)
{
	pool_header *hdr =
		(pool_header *)((char *)item + offsetof(sem_pool_item, header));
	void **in_use_head = NULL;
	void **free_head = NULL;
	pthread_spinlock_t *pmutex = NULL;

	switch (hdr->type) {
	case POOL_SEMA:
		in_use_head = (void **)&comm->dma_h->sem_in_use_head;
		free_head = (void **)&comm->dma_h->sem_free_head;
		pmutex = &comm->dma_h->sem_mutex;
		break;

	case POOL_MUTEX:
		in_use_head = (void **)&comm->dma_h->mutex_in_use_head;
		free_head = (void **)&comm->dma_h->mutex_free_head;
		pmutex = &comm->dma_h->mutex_mutex;
		break;

	case POOL_BUFFERS:
		in_use_head = (void **)&comm->dma_h->buffer_in_use_head;
		free_head = (void **)&comm->dma_h->buffer_free_head;
		pmutex = &comm->dma_h->buffer_mutex;
		break;

	default:
		FPGA_DMA_ST_ERR("releasePoolItem: invalid type");
		return;
	}

	if (pthread_spin_lock(pmutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return;
	}

	releasePoolInt(item, in_use_head, free_head);

	print_lists("releasePoolItem", comm);

	if (pthread_spin_unlock(pmutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return;
	}

	return;
}
// Free all semaphores
static inline void destroySemaphores(handle_common *comm, bool free_only)
{
	if (pthread_spin_lock(&comm->dma_h->sem_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return;
	}

	sem_pool_item *curr = comm->dma_h->sem_in_use_head;
	sem_pool_item **in_use_head = &comm->dma_h->sem_in_use_head;
	sem_pool_item **free_head = &comm->dma_h->sem_free_head;

	print_lists("destroySemaphores head", comm);

	if (!free_only && curr) {
		FPGA_DMA_ST_WARN("Destroying semaphores while marked in-use");
	}

	while (!free_only && curr) {
		releasePoolInt(curr, (void **)in_use_head, (void **)free_head);
		curr = curr->next;
	}

	curr = comm->dma_h->sem_free_head;

	while (curr) {
		sem_pool_item *tmp = curr->next;

		// pthread_mutex_unlock(&curr->m_mutex);
		sem_destroy(&curr->m_semaphore);

		free(curr);
		curr = tmp;
	}

	*free_head = NULL;

	print_lists("destroySemaphores tail", comm);

	if (pthread_spin_unlock(&comm->dma_h->sem_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return;
	}

	return;
}

// Free all mutexes
static inline void destroyMutexes(handle_common *comm, bool free_only)
{
	if (pthread_spin_lock(&comm->dma_h->mutex_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return;
	}

	mutex_pool_item *curr = comm->dma_h->mutex_in_use_head;
	mutex_pool_item **in_use_head = &comm->dma_h->mutex_in_use_head;
	mutex_pool_item **free_head = &comm->dma_h->mutex_free_head;

	if (!free_only && curr) {
		FPGA_DMA_ST_WARN("Destroying mutex while marked in-use");
	}

	print_lists("destroyMutexes head", comm);

	while (!free_only && curr) {
		releasePoolInt(curr, (void **)in_use_head, (void **)free_head);
		curr = curr->next;
	}

	curr = comm->dma_h->mutex_free_head;

	while (curr) {
		// int err;
		mutex_pool_item *tmp = curr->next;

#if 1
		int err = 0;
		if ((err = pthread_mutex_trylock(&curr->m_mutex))) {
			switch (err) {
			case EBUSY:
				perror("Mutex still locked!!");
				break;

			case EINVAL:
				perror("Mutex not initialized!!");
				break;
			case EAGAIN:
				perror("Mutex max recurse exceeded!!");
				break;
			default:
				perror("Mutex trylock failure!!");
				break;
			}
			fprintf(stderr, "Mutex: %p\n", &curr->m_mutex);
		}
		pthread_mutex_unlock(&curr->m_mutex);
#endif
		pthread_mutex_destroy(&curr->m_mutex);

		free(curr);
		curr = tmp;
	}

	*free_head = NULL;

	if (pthread_spin_unlock(&comm->dma_h->mutex_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return;
	}

	print_lists("destroyMutexes tail", comm);

	return;
}

// Free all buffers
static inline void destroyBuffers(handle_common *comm, bool free_only)
{
	if (pthread_spin_lock(&comm->dma_h->buffer_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_lock failed");
		return;
	}

	buffer_pool_item *curr = comm->dma_h->buffer_in_use_head;
	buffer_pool_item **in_use_head = &comm->dma_h->buffer_in_use_head;
	buffer_pool_item **free_head = &comm->dma_h->buffer_free_head;

	if (!free_only && curr) {
		FPGA_DMA_ST_WARN("Destroying buffer while marked in-use");
	}

	print_lists("destroyBuffers head", comm);

	while (!free_only && curr) {
		releasePoolInt(curr, (void **)in_use_head, (void **)free_head);
		curr = curr->next;
	}

	curr = comm->dma_h->buffer_free_head;

	while (curr) {
		fpga_result res = FPGA_OK;
		buffer_pool_item *tmp = curr->next;

		res = fpgaReleaseBuffer(comm->fpga_h, curr->dma_buf_wsid);
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("Failure releasing pinned buffer");
		}

		free(curr);
		curr = tmp;
	}

	*free_head = NULL;

	if (pthread_spin_unlock(&comm->dma_h->buffer_mutex)) {
		FPGA_DMA_ST_ERR("pthread_spin_unlock failed");
		return;
	}

	print_lists("destroyBuffers tail", comm);

	return;
}

// Free all pooled resources
inline void destroyAllPoolResources(handle_common *comm, bool free_only)
{
	destroySemaphores(comm, free_only);
	destroyMutexes(comm, free_only);
	destroyBuffers(comm, free_only);

	return;
}
