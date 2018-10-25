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
 * \fpga_dma_st.c
 * \brief FPGA Streaming DMA User-mode driver (Stub)
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

inline fpga_result fpgaDMAGetChannelType(fpga_dma_channel_handle _dma,
					 fpga_dma_channel_type_t *ch_type)
{
	fpga_dma_handle_t *dma = (fpga_dma_handle_t *)_dma;
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)_dma;
	assert(&m2m->header == &dma->main_header);
	if (!dma || !IS_CHANNEL_HANDLE(m2m)) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!ch_type) {
		FPGA_DMA_ST_ERR("Invalid pointer to channel type");
		return FPGA_INVALID_PARAM;
	}

	*ch_type = m2m->header.ch_type;
	return FPGA_OK;
}

inline fpga_result fpgaDMATransferInit(fpga_dma_channel_handle _dma_h,
				       fpga_dma_transfer *transfer)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *tmp;
	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)_dma_h;
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)_dma_h;
	assert(&m2m->header == &dma_h->main_header);

	if (!dma_h || !IS_CHANNEL_HANDLE(m2m)) {
		FPGA_DMA_ST_ERR("Invalid channel handle");
		return FPGA_INVALID_PARAM;
	}

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	tmp = (fpga_dma_transfer_t *)calloc(1, sizeof(fpga_dma_transfer_t));
	if (!tmp) {
		FPGA_DMA_ST_ERR("Out of memory allocating small transfer");
		res = FPGA_NO_MEMORY;
		return res;
	}

	*transfer = NULL;

	// Initialize default transfer attributes
	tmp->src = 0;
	tmp->dst = 0;
	tmp->len = 0;
	tmp->transfer_type = HOST_MM_TO_FPGA_ST;
	tmp->tx_ctrl = TX_NO_PACKET; // deterministic length
	tmp->rx_ctrl = RX_NO_PACKET; // deterministic length
	tmp->cb = NULL;
	tmp->context = NULL;
	tmp->rx_bytes = 0;
	tmp->eop_status = 0;

	tmp->tf_mutex = getFreeMutex(&m2m->header.dma_h->main_header, NULL);
	tmp->tf_semaphore = getFreeSemaphore(&m2m->header.dma_h->main_header, 0,
					     TRANSFER_NOT_IN_PROGRESS);
	tmp->ch_type = m2m->header.ch_type;

	if (!tmp->tf_mutex) {
		free(tmp);
		return FPGA_EXCEPTION;
	}

	*transfer = tmp;
	return res;
}

inline fpga_result fpgaDMATransferReset(fpga_dma_channel_handle dma,
					fpga_dma_transfer _transfer)
{
	(void)dma;
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	transfer->src = 0;
	transfer->dst = 0;
	transfer->len = 0;
	transfer->transfer_type = HOST_MM_TO_FPGA_ST;
	transfer->tx_ctrl = TX_NO_PACKET; // deterministic length
	transfer->rx_ctrl = RX_NO_PACKET; // deterministic length
	transfer->cb = NULL;
	transfer->context = NULL;
	transfer->rx_bytes = 0;
	transfer->eop_status = 0;

	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return res;
}

inline fpga_result fpgaDMATransferDestroy(fpga_dma_channel_handle _dma_h,
					  fpga_dma_transfer *_transfer)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)_dma_h;
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)dma_h;
	assert(&m2m->header == &dma_h->main_header);

	if (!_transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer pointer");
		return FPGA_INVALID_PARAM;
	}

	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)*_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	if (transfer->buffers) {
		uint32_t i;
		for (i = 0; i < transfer->num_buffers; i++) {
			releasePoolItem(&m2m->header.dma_h->main_header,
					transfer->buffers[i]);
			transfer->buffers[i] = NULL;
		}

		transfer->buffers = NULL;
		transfer->num_buffers = 0;
		free(transfer->buffers);
	}

	if (transfer->small_buffer) {
		releasePoolItem(&m2m->header.dma_h->main_header,
				transfer->small_buffer);
		transfer->small_buffer = NULL;
	}

	transfer->tf_mutex->header.destroyed = 1;

	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	releasePoolItem(&m2m->header.dma_h->main_header, transfer->tf_mutex);
	releasePoolItem(&m2m->header.dma_h->main_header,
			transfer->tf_semaphore);

	free(transfer);
	*_transfer = NULL;

	return res;
}

inline fpga_result fpgaDMATransferSetSrc(fpga_dma_transfer _transfer,
					 uint64_t src)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	int errr = pthread_mutex_lock(&transfer->tf_mutex->m_mutex);
	if (errr) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		printf("Mutex is %p, error is %d\n",
		       &transfer->tf_mutex->m_mutex, errr);
		return FPGA_EXCEPTION;
	}
	transfer->src = src;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return res;
}

inline fpga_result fpgaDMATransferSetDst(fpga_dma_transfer _transfer,
					 uint64_t dst)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}
	transfer->dst = dst;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return res;
}

inline fpga_result fpgaDMATransferSetLen(fpga_dma_transfer _transfer,
					 uint64_t len)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}
	transfer->len = len;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}
	return res;
}

inline fpga_result fpgaDMATransferSetTransferType(fpga_dma_transfer _transfer,
						  fpga_dma_transfer_type_t type)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (type >= FPGA_MAX_TRANSFER_TYPE) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer type");
		return FPGA_INVALID_PARAM;
	}

	if ((type == FPGA_MM_TO_FPGA_ST) || (type == FPGA_ST_TO_FPGA_MM)) {
		FPGA_DMA_ST_ERR("Transfer unsupported");
		return FPGA_NOT_SUPPORTED;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	transfer->transfer_type = type;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}
	return res;
}

inline fpga_result fpgaDMATransferSetTxControl(fpga_dma_transfer _transfer,
					       fpga_dma_tx_ctrl_t tx_ctrl)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (tx_ctrl >= FPGA_MAX_TX_CTRL) {
		FPGA_DMA_ST_ERR("Invalid TX Ctrl");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}
	transfer->tx_ctrl = tx_ctrl;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return res;
}

inline fpga_result fpgaDMATransferSetRxControl(fpga_dma_transfer _transfer,
					       fpga_dma_rx_ctrl_t rx_ctrl)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (rx_ctrl >= FPGA_MAX_RX_CTRL) {
		FPGA_DMA_ST_ERR("Invalid RX Ctrl");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}
	transfer->rx_ctrl = rx_ctrl;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return res;
}

inline fpga_result
fpgaDMATransferSetTransferCallback(fpga_dma_transfer _transfer,
				   fpga_dma_transfer_cb cb, void *ctxt)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	transfer->cb = cb;
	transfer->context = ctxt;
	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}
	return res;
}

inline fpga_result
fpgaDMATransferGetBytesTransferred(fpga_dma_transfer _transfer,
				   size_t *rx_bytes)
{
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (!rx_bytes) {
		FPGA_DMA_ST_ERR("Invalid pointer to transferred bytes");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	*rx_bytes = transfer->rx_bytes;

	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

inline fpga_result fpgaDMATransferCheckEopArrived(fpga_dma_transfer _transfer,
						  bool *eop_arrived)
{
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (!eop_arrived) {
		FPGA_DMA_ST_ERR("Invalid pointer to eop status");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	*eop_arrived = transfer->eop_status;

	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

inline fpga_result fpgaDMATransferStart(fpga_dma_channel_handle _dma,
					fpga_dma_transfer _transfer)
{
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t *transfer = (fpga_dma_transfer_t *)_transfer;
	fpga_dma_handle_t *dma = (fpga_dma_handle_t *)_dma;
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)_dma;
	s2m_dma_handle_t *s2m = (s2m_dma_handle_t *)_dma;
	m2s_dma_handle_t *m2s = (m2s_dma_handle_t *)_dma;
	assert(&m2m->header == &s2m->header);
	assert(&m2m->header == &m2s->header);
	assert(&m2m->header == &dma->main_header);

	if (!dma || !IS_CHANNEL_HANDLE(m2m)) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if (transfer->transfer_type == FPGA_MM_TO_FPGA_ST
	    || transfer->transfer_type == FPGA_ST_TO_FPGA_MM) {
		FPGA_DMA_ST_ERR("Transfer unsupported");
		return FPGA_NOT_SUPPORTED;
	}

	if (m2m->header.ch_type == RX_ST
	    && transfer->transfer_type == HOST_MM_TO_FPGA_ST) {
		FPGA_DMA_ST_ERR(
			"Incompatible transfer on stream to memory DMA channel");
		return FPGA_INVALID_PARAM;
	}

	if (m2m->header.ch_type == TX_ST
	    && transfer->transfer_type == FPGA_ST_TO_HOST_MM) {
		FPGA_DMA_ST_ERR(
			"Incompatible transfer on memory to stream DMA channel");
		return FPGA_INVALID_PARAM;
	}

	if (((transfer->tx_ctrl == TX_NO_PACKET && m2m->header.ch_type == TX_ST)
	     || (transfer->rx_ctrl == RX_NO_PACKET
		 && m2m->header.ch_type == RX_ST))
	    && ((transfer->len % 64) != 0)) {
		FPGA_DMA_ST_ERR(
			"Incompatible transfer length for transfer type NO_PKT");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	// Transfer in progress
	if (sem_wait(&transfer->tf_semaphore->m_semaphore)) {
		FPGA_DMA_ST_ERR("sem_wait failed");
		return FPGA_EXCEPTION;
	}

	// Enqueue the transfer, transfer will be processed in worker thread
	do {
		switch (transfer->ch_type) {
		case TX_ST:
			res = fpgaDMAEnqueue(&m2s->header.transferRequestq,
					     transfer);
			break;

		case RX_ST:
			res = fpgaDMAEnqueue(&s2m->header.transferRequestq,
					     transfer);
			break;

		case MM:
			res = fpgaDMAEnqueue(&m2m->header.transferRequestq,
					     transfer);
			break;

		default:
			FPGA_DMA_ST_ERR("Invalid channel type in transfer");
			break;
		}
	} while (res == FPGA_BUSY);

	if (res != FPGA_OK) {
		if (sem_post(&transfer->tf_semaphore->m_semaphore))
			FPGA_DMA_ST_ERR("sem_post failed");
		return FPGA_EXCEPTION;
	}

	if (pthread_mutex_unlock(&transfer->tf_mutex->m_mutex)) {
		FPGA_DMA_ST_ERR("pthread_mutex_unlock failed");
		return FPGA_EXCEPTION;
	}

	// Blocking transfer
	if (!transfer->cb) {
		if (sem_wait(&transfer->tf_semaphore->m_semaphore)) {
			FPGA_DMA_ST_ERR("sem_wait failed");
			return FPGA_EXCEPTION;
		}
	}

	return res;
}

inline fpga_result fpgaDMATransferInitSmall(fpga_dma_channel_handle _dma,
					    uint64_t *size, void **buf_ptr,
					    fpga_dma_transfer *transfer)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)_dma;
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)_dma;
	assert(&m2m->header == &dma_h->main_header);

	if (!dma_h || !IS_CHANNEL_HANDLE(m2m)) {
		FPGA_DMA_ST_ERR("Invalid channel handle");
		return FPGA_INVALID_PARAM;
	}

	if (!buf_ptr) {
		FPGA_DMA_ST_ERR("Invalid buffer pointer");
		return FPGA_INVALID_PARAM;
	}

	if (!transfer) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA small transfer");
		return FPGA_INVALID_PARAM;
	}

	if (!size || (*size > FPGA_DMA_BUF_SIZE)) {
		FPGA_DMA_ST_ERR("Invalid size");
		return FPGA_INVALID_PARAM;
	}

	if (m2m->header.dma_h->num_smalls >= FPGA_DMA_MAX_SMALL_BUFFERS) {
		FPGA_DMA_ST_ERR("Attempt to allocate too many small transfers");
		return FPGA_NO_MEMORY;
	}

	res = fpgaDMATransferInit(_dma, transfer);
	if (FPGA_OK != res) {
		FPGA_DMA_ST_ERR("Cannot initialize transfer");
		return res;
	}

	fpga_dma_transfer_t *xfer = (fpga_dma_transfer_t *)*transfer;

	xfer->small_buffer = getFreeBuffer(&m2m->header.dma_h->main_header);
	xfer->num_buffers = 1;

	buffer_pool_item *bp = xfer->small_buffer;
	*size = bp->size;
	*buf_ptr = (void *)bp->dma_buf_ptr;

	return res;
}
