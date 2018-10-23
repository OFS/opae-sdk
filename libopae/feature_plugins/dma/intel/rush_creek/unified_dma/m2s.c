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

// Internal Functions

#define UNUSED(x) (void)(x)

/**
 * getTxCtrl
 *
 * @brief                Sets appropriate tx_ctrl property based on desc being
 *                       worked on
 * @param[in] index      Index of descriptor being worked on
 * @param[in] max_desc   Descriptor count
 * @param[in] tx_ctrl    Transfer control assigned to the transfer
 * @return fpga_dma_tx_ctrl_t Transfer control assigned to specific descriptor
 *
 */
static inline fpga_dma_tx_ctrl_t getTxCtrl(uint64_t index, uint64_t max_desc,
					   fpga_dma_tx_ctrl_t tx_ctrl)
{
	if (max_desc == 1) {
		return tx_ctrl;
	} else {
		if ((index == 0) /*first desc*/
		    && ((tx_ctrl == GENERATE_SOP)
			|| (tx_ctrl == GENERATE_SOP_AND_EOP))) {
			return GENERATE_SOP;
		} else if ((index == (max_desc - 1)) /*last desc*/
			   && ((tx_ctrl == GENERATE_EOP)
			       || (tx_ctrl == GENERATE_SOP_AND_EOP))) {
			return GENERATE_EOP;
		} else {
			return TX_NO_PACKET;
		}
	}
}
static inline fpga_result _do_dma_tx(m2s_dma_handle_t *dma_h, uint64_t dst,
				     uint64_t src, int count, int is_last_desc,
				     fpga_dma_transfer_type_t type,
				     bool intr_en, fpga_dma_tx_ctrl_t tx_ctrl)
{
	UNUSED(type);
	msgdma_ext_desc_t desc = {0};
	fpga_result res = FPGA_OK;

	// src, dst must be 64-byte aligned
	if (dst % FPGA_DMA_ALIGN_BYTES != 0
	    || src % FPGA_DMA_ALIGN_BYTES != 0) {
		return FPGA_INVALID_PARAM;
	}

	// these fields are fixed for all DMA transfers
	desc.seq_num = 0;
	desc.wr_stride = 1;
	desc.rd_stride = 1;

	desc.control.go = 1;
	if (intr_en)
		desc.control.transfer_irq_en = 1;
	else
		desc.control.transfer_irq_en = 0;

	// Enable "earlyreaddone" in the control field of the descriptor except
	// the last. Setting early done causes the read logic to move to the
	// next descriptor before the previous descriptor completes. This
	// elminates a few hundred clock cycles of waiting between transfers.
	if (!is_last_desc)
		desc.control.early_done_en = 1;
	else
		desc.control.early_done_en = 0;

	if (tx_ctrl == GENERATE_SOP) {
		desc.control.generate_sop = 1;
		desc.control.generate_eop = 0;
	} else if (tx_ctrl == GENERATE_SOP_AND_EOP) {
		desc.control.generate_sop = 1;
		desc.control.generate_eop = 1;
	} else if (tx_ctrl == GENERATE_EOP) {
		desc.control.generate_sop = 0;
		desc.control.generate_eop = 1;
	} else {
		desc.control.generate_sop = 0;
		desc.control.generate_eop = 0;
	}

	desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
	desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
	desc.len = count;
	desc.rd_address_ext = (src >> 32) & FPGA_DMA_MASK_32_BIT;
	desc.wr_address_ext = (dst >> 32) & FPGA_DMA_MASK_32_BIT;

	res = _send_descriptor(&dma_h->header, &desc);
	ON_ERR_GOTO(res, out, "_send_descriptor");

out:
	return res;
}

void *m2sTransactionWorker(void *dma_handle)
{
	fpga_result res = FPGA_OK;
	m2s_dma_handle_t *dma_h = (m2s_dma_handle_t *)dma_handle;
	uint64_t count;
	uint64_t i;

	// At this point, we're alive.  Let the main thread know
	sem_post(&dma_h->header.dma_h->m2s_thread_sem);

	while (1) {
		bool intr_en = false;
		fpga_dma_transfer_t m2s_transfer;
		res = fpgaDMADequeue(&dma_h->header.transferRequestq,
				     &m2s_transfer);
		if (res == FPGA_NO_ACCESS) {
			// FPGA_DMA_ST_ERR("M2S thread termination");
			break;
		}
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMADequeue failed");
			return NULL;
		}
		debug_print(
			"HOST to FPGA --- src_addr = %08lx, dst_addr = %08lx\n",
			m2s_transfer.src, m2s_transfer.dst);

		if (!m2s_transfer.small_buffer) {
			m2s_transfer.num_buffers =
				((m2s_transfer.len + FPGA_DMA_BUF_SIZE - 1)
				 / FPGA_DMA_BUF_SIZE);
			m2s_transfer.num_buffers =
				min(m2s_transfer.num_buffers,
				    (uint32_t)FPGA_DMA_MAX_BUF);
			m2s_transfer.buffers = (buffer_pool_item **)calloc(
				m2s_transfer.num_buffers,
				sizeof(buffer_pool_item *));
			uint32_t i;
			for (i = 0; i < m2s_transfer.num_buffers; i++) {
				m2s_transfer.buffers[i] =
					getFreeBuffer(&dma_h->header);
			}
		}

		count = m2s_transfer.len;

		buffer_pool_item **buffers;
		// False means data is already in the buffer
		uint32_t do_memcpy = 1;
		int half_num_buffers = m2s_transfer.num_buffers / 2;
		int num_buffers = m2s_transfer.num_buffers;

		if (m2s_transfer.small_buffer) {
			buffers = &m2s_transfer.small_buffer;
			assert(m2s_transfer.num_buffers == 1);
			do_memcpy = 0;
			half_num_buffers = 1;
		} else {
			buffers = m2s_transfer.buffers;
		}

		// All same sized buffers
		uint64_t buffer_size = buffers[0]->size;

		uint64_t dma_chunks =
			(uint64_t)ceil((double)count / (double)buffer_size);
		uint64_t bytes_to_transfer = 0;
		fpga_dma_tx_ctrl_t tx_desc_ctrl;

		for (i = 0; i < dma_chunks; i++) {
			uint64_t *dma_buf_ptr =
				buffers[i % num_buffers]->dma_buf_ptr;
			uint64_t dma_buf_iova =
				buffers[i % num_buffers]->dma_buf_iova;
			// Enable interrupt for every 4th descriptor and very
			// last descriptor
			intr_en = (((i % (uint64_t)half_num_buffers)
				    == ((uint64_t)half_num_buffers - 1))
				   || i == (dma_chunks - 1));
			bytes_to_transfer = (i == (dma_chunks - 1)
					     && (count % buffer_size != 0))
						    ? count % buffer_size
						    : buffer_size;
			/*skip interrupt polling for very first interrupt*/
			if (intr_en && (i > ((uint64_t)half_num_buffers - 1))) {
				poll_interrupt(&dma_h->header M2S_TW);
			}
			if (do_memcpy) {
				local_memcpy(dma_buf_ptr,
					     (void *)(m2s_transfer.src
						      + i * buffer_size),
					     bytes_to_transfer);
			}
			tx_desc_ctrl =
				getTxCtrl(i, dma_chunks, m2s_transfer.tx_ctrl);
			res = _do_dma_tx(
				dma_h, 0, dma_buf_iova | 0x1000000000000,
				bytes_to_transfer, 1,
				m2s_transfer.transfer_type, intr_en /*intr_en*/,
				tx_desc_ctrl /*tx_ctrl*/);
			ON_ERR_GOTO(res, out,
				    "HOST_TO_FPGA_ST Transfer failed");
		}

		if (intr_en) {
			poll_interrupt(&dma_h->header M2S_TW2);
		}

		res = fpgaDMAEnqueue(&dma_h->header.dma_h->transferCompleteq,
				     &m2s_transfer);
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMAEnqueue failed");
			return NULL;
		}
	}
out:
	return dma_h;
}
