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
 * \fpga_dma.c
 * \brief FPGA DMA User-mode driver
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"
#include "globals.h"

void *TransactionCompleteWorker(void *dma_handle)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)dma_handle;

	// At this point, we're alive.  Let the main thread know
	sem_post(&dma_h->completion_thread_sem);

	// Wait until the main thread knows.
	int sval;
	do {
		if (sem_getvalue(&dma_h->completion_thread_sem, &sval)) {
			FPGA_DMA_ST_ERR("sem_getvalue failed");
			break;
		}
		pthread_yield();
	} while (sval > 0);

	while (1) {
		fpga_dma_transfer_t xfer;
		res = fpgaDMADequeue(&dma_h->transferCompleteq, &xfer);
		if (res == FPGA_NO_ACCESS) {
			// FPGA_DMA_ST_ERR("Completion thread termination");
			break;
		}
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMADequeue failed");
			return NULL;
		}
		debug_print(
			"TransferComplete --- src_addr = %08lx, dst_addr = %08lx\n",
			xfer.src, xfer.dst);

		//.  When a worker has finished a transfer, it enqueues it onto
		// the completion queue.  A separate thread will be dequeueing
		// completions, releasing the buffers back to the pool, calling
		// the callback function (if specified), posting the completion
		// notice to the semaphore, and releasing the transfer back to
		// the transfer pool

		// transfer_complete, if a callback was registered, invoke it
		if (xfer.cb) {
			xfer.cb(xfer.context);
		}

		// Mark transfer complete
		if (sem_post(&xfer.tf_semaphore->m_semaphore)) {
			FPGA_DMA_ST_ERR("sem_post failed");
			return NULL;
		}

		if (xfer.buffers) {
			uint32_t i;
			m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)dma_h;
			assert(&m2m->header == &dma_h->main_header);
			for (i = 0; i < xfer.num_buffers; i++) {
				releasePoolItem(&m2m->header,
						xfer.buffers[i]);
				xfer.buffers[i] = NULL;
			}

			xfer.buffers = NULL;
			xfer.num_buffers = 0;
			free(xfer.buffers);
		}

		if (!xfer.cb) {
			int sval;
			do {
				if (sem_getvalue(
					    &xfer.tf_semaphore->m_semaphore,
					    &sval)) {
					FPGA_DMA_ST_ERR("sem_getvalue failed");
					break;
				}
				pthread_yield();
			} while (sval > 0);

			if (sem_post(&xfer.tf_semaphore->m_semaphore)) {
				FPGA_DMA_ST_ERR("sem_post failed");
				return NULL;
			}
		}
	}

	return dma_handle;
}
