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
 * \internal_funcs.h
 * \brief Function declarations for DMA not in utils
 */

#ifndef __INTERNAL_FUNCS_H__
#define __INTERNAL_FUNCS_H__

#include <opae/fpga.h>
#include "fpga_dma_types.h"
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "x86-sse2.h"
#include "fpga_common_internal.h"
#include "memcpy_internal.h"
#include "mmio_utils_internal.h"
#include "common_internal.h"

int fpgaDMA_setup_sig_handler(fpga_dma_handle_t *dma_h);

void fpgaDMA_restore_sig_handler(fpga_dma_handle_t *dma_h);

inline fpga_result fpgaDMAQueueInit(fpga_dma_handle_t *dma_h, qinfo_t *q);

inline fpga_result fpgaDMAQueueDestroy(fpga_dma_handle_t *dma_h, qinfo_t *q,
				       bool free_only);

inline fpga_result fpgaDMAEnqueue(qinfo_t *q, fpga_dma_transfer_t *tf);

inline fpga_result fpgaDMADequeue(qinfo_t *q, fpga_dma_transfer_t *tf);

inline sem_pool_item *getFreeSemaphore(handle_common *comm, int pshared,
				       int sem_value);

inline mutex_pool_item *getFreeMutex(handle_common *comm,
				     pthread_mutexattr_t *attr);

inline buffer_pool_item *getFreeBuffer(handle_common *comm);

inline void releasePoolItem(handle_common *comm, void *item);

inline void destroyAllPoolResources(handle_common *comm, bool free_only);

// Bind the calling thread to the NUMA node of the device
int setNUMABindings(fpga_handle fpga_h);

fpga_result fpgaDMAAllocateAndPinBuffers(fpga_dma_handle_t *dma_h);

void *m2sTransactionWorker(void *dma_handle);

void *s2mTransactionWorker(void *dma_handle);

void *m2mTransactionWorker(void *dma_handle);

void *TransactionCompleteWorker(void *dma_handle);

#endif // __INTERNAL_FUNCS_H__
