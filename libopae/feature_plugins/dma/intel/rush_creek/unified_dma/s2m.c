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

#define FORCE_DESC_FLUSH 1

// Internal Functions

#define UNUSED(x) (void)(x)

static inline fpga_result _pop_response_fifo(s2m_dma_handle_t *dma_h,
					     int *fill_level,
					     uint32_t *tf_count, int *eop)
{
	fpga_result res = FPGA_OK;
	msgdma_rsp_level_t rsp_level = {0};
	uint16_t fill = 0;
	uint32_t rsp_bytes = 0;
	msgdma_rsp_status_t rsp_status = {0};
	// fill level is the number of responses in response FIFO
	*fill_level = 0;
	*tf_count = 0;
	*eop = 0;
	uint8_t error = 0;

	// Read S2M Response fill level Dispatcher register to find the no. of
	// responses in the response FIFO
	res = MMIORead32Blk(&dma_h->header, CSR_RSP_FILL_LEVEL(&dma_h->header),
			    (uint64_t)&rsp_level.reg, sizeof(rsp_level.reg));
	ON_ERR_GOTO(res, out, "MMIORead32Blk");
	fill = rsp_level.rsp.rsp_fill_level;

	// Pop the responses to find no. of bytes trasnfered, status of transfer
	// and to avoid deadlock of DMA
	while (fill > 0 && *eop != 1) {
		res = MMIORead32Blk(&dma_h->header,
				    RSP_BYTES_TRANSFERRED(dma_h),
				    (uint64_t)&rsp_bytes, sizeof(rsp_bytes));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
		res = MMIORead32Blk(&dma_h->header, RSP_STATUS(dma_h),
				    (uint64_t)&rsp_status.reg,
				    sizeof(rsp_status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
		*tf_count += rsp_bytes;
		error = rsp_status.rsp.error;
		fill--;
		if (rsp_status.rsp.eop_arrived == 1)
			*eop = 1;
		*fill_level += 1;
		UNUSED(error);
		debug_print(
			"fill level = %x, *eop = %d, tf_count = %x, error = %x\n",
			*fill_level, *eop, *tf_count, error);
	}
out:
	return res;
}

static inline fpga_result _do_dma_rx(s2m_dma_handle_t *dma_h, uint64_t dst,
				     uint64_t src, int count, int is_last_desc,
				     fpga_dma_transfer_type_t type,
				     bool intr_en, fpga_dma_rx_ctrl_t rx_ctrl)
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
	if (intr_en) {
		desc.control.transfer_irq_en = 1;
		desc.control.wait_for_wr_rsp = 1;
	} else {
		desc.control.transfer_irq_en = 0;
		desc.control.wait_for_wr_rsp = 0;
	}

	// Enable "earlyreaddone" in the control field of the descriptor except
	// the last. Setting early done causes the read logic to move to the
	// next descriptor before the previous descriptor completes. This
	// elminates a few hundred clock cycles of waiting between transfers.
	if (!is_last_desc)
		desc.control.early_done_en = 1;
	else
		desc.control.early_done_en = 0;

	if (rx_ctrl == END_ON_EOP) {
		desc.control.end_on_eop = 1;
		desc.control.eop_rvcd_irq_en = 1;
		desc.control.wait_for_wr_rsp = 1;
	} else {
		desc.control.end_on_eop = 0;
		desc.control.eop_rvcd_irq_en = 0;
		desc.control.wait_for_wr_rsp = 1;
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

static inline fpga_result s2m_pending_desc_flush(s2m_dma_handle_t *dma_h)
{
	fpga_result res = FPGA_OK;
	msgdma_ctrl_t control = {0};
	// Flush Pending Descriptors and stop S2M DMA
	control.ct.flush_descriptors = 1;
	control.ct.stop_dispatcher = 1;
	res = MMIOWrite32Blk(&dma_h->header, CSR_CONTROL(&dma_h->header),
			     (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
	control.ct.flush_descriptors = 0;
	// Poll to check if S2M DMA was stopped
	msgdma_status_t status = {0};
	do {
		res = MMIORead32Blk(&dma_h->header, CSR_STATUS(&dma_h->header),
				    (uint64_t)&status.reg, sizeof(status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
	} while (!status.st.stopped);
	// Flush Write master; This is to flush any descriptors that sneaked in
	// after the initial descriptor flush
	control.ct.flush_descriptors = 0;
	control.ct.stop_dispatcher = 0;
	control.ct.flush_wr_master = 1;
	res = MMIOWrite32Blk(&dma_h->header, CSR_CONTROL(&dma_h->header),
			     (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
	// Re-enable global interrupts
	control.ct.global_intr_en_mask = 1;
	res = MMIOWrite32Blk(&dma_h->header, CSR_CONTROL(&dma_h->header),
			     (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
out:
	return res;
}

// Streaming to Memory Dispatcher Thread
void *s2mTransactionWorker(void *dma_handle)
{
	fpga_result res = FPGA_OK;
	s2m_dma_handle_t *dma_h = (s2m_dma_handle_t *)dma_handle;
	uint64_t count;
	int j;

	// At this point, we're alive.  Let the main thread know
	sem_post(&dma_h->header.dma_h->s2m_thread_sem);

	while (1) {
		// Head moves forward when descriptors are pushed into the
		// dispatch queue
		uint64_t head = 0;
		// Tail moves forward when data gets copied to application
		// buffer
		uint64_t tail = 0;

		fpga_dma_transfer_t s2m_transfer;
		res = fpgaDMADequeue(&dma_h->header.transferRequestq,
				     &s2m_transfer);
		if (res == FPGA_NO_ACCESS) {
			debug_print("S2M thread termination");
			break;
		}
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMADequeue failed");
			return NULL;
		}
		debug_print(
			"FPGA to HOST --- src_addr = %08lx, dst_addr = %08lx\n",
			s2m_transfer.src, s2m_transfer.dst);
		count = s2m_transfer.len;
		uint64_t dma_chunks;
		int eop_arrived = 0;

		// FIXME: Allocate a fixed number of buffers for s2m
		// These will be released back into the pool when the transfer
		// is complete
		assert(s2m_transfer.small_buffer == NULL);
		s2m_transfer.num_buffers = FPGA_DMA_MAX_BUF;
		s2m_transfer.buffers = (buffer_pool_item **)calloc(
			s2m_transfer.num_buffers, sizeof(buffer_pool_item *));
		uint32_t i;
		uint64_t *dma_buf_ptr[FPGA_DMA_MAX_BUF];
		uint64_t dma_buf_iova[FPGA_DMA_MAX_BUF];
		for (i = 0; i < s2m_transfer.num_buffers; i++) {
			s2m_transfer.buffers[i] = getFreeBuffer(&dma_h->header);
			dma_buf_ptr[i] = s2m_transfer.buffers[i]->dma_buf_ptr;
			dma_buf_iova[i] = s2m_transfer.buffers[i]->dma_buf_iova;
		}

		//	Control streaming valve; Added to S2M write master so
		// that when EOP arrives no more streaming data will be allowed
		// into the DMA giving the driver time to flush out the previous
		// descriptors
		msgdma_st_valve_ctrl_t ctrl = {0};
		// Set streaming valve to enable data flow
		ctrl.ct.en_data_flow = 1;

		dma_chunks = count / FPGA_DMA_BUF_SIZE;
		// calculate unaligned leftover bytes to be transferred
		count -= (dma_chunks * FPGA_DMA_BUF_SIZE);
		if (s2m_transfer.rx_ctrl == END_ON_EOP) {
			// Set to indicate transfer type. Streaming valve will
			// stop accepting data after EOP has arrived.
			ctrl.ct.en_non_det_tf = 1;
		} else {
			// Set to indicate transfer type.
			ctrl.ct.en_det_tf = 1;
			// Flush descriptors only while switching from
			// Non-deterministic to deterministic tf
			if (dma_h->unused_desc_count) {
				s2m_pending_desc_flush(dma_h);
				dma_h->unused_desc_count = 0;
				dma_h->next_avail_desc_idx = 0;
			}
		}
		res = MMIOWrite32Blk(&dma_h->header, ST_VALVE_CONTROL(dma_h),
				     (uint64_t)&ctrl.reg, sizeof(ctrl.reg));
		ON_ERR_GOTO(res, out, "MMIOWrite32Blk");

		int issued_intr = 0;
		int fill_level = 0;
		uint32_t tf_count = 0;
		debug_print(
			"next_avail_desc_idx = %08lx , unused_desc_count = %08lx \n",
			dma_h->next_avail_desc_idx, dma_h->unused_desc_count);

		// At this point,the descriptor queue is either empty
		// or has one or more unused descriptors left from prior
		// transfer(s)
		do {
			// The latter case
			_pop_response_fifo(dma_h, &fill_level, &tf_count,
					   &eop_arrived);
			s2m_transfer.rx_bytes += tf_count;
			while (fill_level > 0) {
				// If the queue has unused descriptors, use them
				// for our transfer
				local_memcpy(
					(void *)(s2m_transfer.dst
						 + head * FPGA_DMA_BUF_SIZE),
					dma_buf_ptr[dma_h->next_avail_desc_idx],
					min(tf_count, FPGA_DMA_BUF_SIZE));
				tf_count -= min(tf_count, FPGA_DMA_BUF_SIZE);
				dma_h->next_avail_desc_idx =
					(dma_h->next_avail_desc_idx + 1)
					% FPGA_DMA_MAX_BUF;
				dma_h->unused_desc_count--;
				head++;
				fill_level--;
			}

			// case a: We haven't used up all descriptors in the
			// queue, but EOP arrived early
			if (eop_arrived)
				break;

			// case b: We haven't used up all descriptors in the
			// queue, but current transfer ended early
			if (head == dma_chunks)
				break;

			// case c: We have used up all descriptors in the queue
			if (dma_h->unused_desc_count <= 0) {
				dma_h->next_avail_desc_idx = 0;
				break;
			}

		} while (1);
		if (eop_arrived)
			goto out_transf_complete;

		tail = head;

		while (head < dma_chunks) {
			// Total transfers in flight = head-tail+1
			int cur_num_pending = head - tail + 1;

			if (cur_num_pending == (FPGA_DMA_MAX_BUF / 2)) {
				res = _do_dma_rx(
					dma_h,
					dma_buf_iova[head % (FPGA_DMA_MAX_BUF)]
						| 0x1000000000000,
					0, FPGA_DMA_BUF_SIZE, 1,
					s2m_transfer.transfer_type,
					true /*intr_en*/,
					s2m_transfer.rx_ctrl /*rx_ctrl*/);
				ON_ERR_GOTO(
					res, out,
					"FPGA_ST_TO_HOST_MM Transfer failed");
				issued_intr = 1;
			} else if (cur_num_pending > (FPGA_DMA_MAX_BUF - 1)
				   || head == (dma_chunks - 1)) {
				/*last descriptor*/
				if (issued_intr) {
					poll_interrupt(&dma_h->header S2M_TW);
					_pop_response_fifo(dma_h, &fill_level,
							   &tf_count,
							   &eop_arrived);
					s2m_transfer.rx_bytes += tf_count;
					for (j = 0; j < fill_level; j++) {
						local_memcpy(
							(void *)(s2m_transfer
									 .dst
								 + tail * FPGA_DMA_BUF_SIZE),
							dma_buf_ptr
								[tail
								 % (FPGA_DMA_MAX_BUF)],
							min(tf_count,
							    FPGA_DMA_BUF_SIZE));
						tf_count -=
							min(tf_count,
							    FPGA_DMA_BUF_SIZE);
						// Increment tail when we memcpy
						// data back to user-buffer
						tail++;
					}
					issued_intr = 0;
					if (eop_arrived) {
						goto out_transf_complete;
					}
				}
				res = _do_dma_rx(
					dma_h,
					dma_buf_iova[head % (FPGA_DMA_MAX_BUF)]
						| 0x1000000000000,
					0, FPGA_DMA_BUF_SIZE, 1,
					s2m_transfer.transfer_type,
					true /*intr_en*/,
					s2m_transfer.rx_ctrl /*rx_ctrl*/);
				ON_ERR_GOTO(
					res, out,
					"FPGA_ST_TO_HOST_MM Transfer failed");
				issued_intr = 1;
			} else {
				res = _do_dma_rx(
					dma_h,
					dma_buf_iova[head % (FPGA_DMA_MAX_BUF)]
						| 0x1000000000000,
					0, FPGA_DMA_BUF_SIZE, 1,
					s2m_transfer.transfer_type,
					false /*intr_en*/,
					s2m_transfer.rx_ctrl /*rx_ctrl*/);
				ON_ERR_GOTO(
					res, out,
					"FPGA_ST_TO_HOST_MM Transfer failed");
			}

			// Increment head when the descriptor is issued into the
			// dispatcher queue
			head++;
		}

		if (issued_intr) {
			poll_interrupt(&dma_h->header S2M_TW2);
			do {
				_pop_response_fifo(dma_h, &fill_level,
						   &tf_count, &eop_arrived);
				s2m_transfer.rx_bytes += tf_count;
				// clear out final dma local_memcpy operations
				while (fill_level > 0) {
					// constant size transfer; no length
					// check required
					local_memcpy(
						(void *)(s2m_transfer.dst
							 + tail * FPGA_DMA_BUF_SIZE),
						dma_buf_ptr
							[tail
							 % (FPGA_DMA_MAX_BUF)],
						min(tf_count,
						    FPGA_DMA_BUF_SIZE));
					tail++;
					fill_level -= 1;
					tf_count -= min(tf_count,
							FPGA_DMA_BUF_SIZE);
				}
			} while (tail < dma_chunks && eop_arrived == 0);
			if (eop_arrived)
				goto out_transf_complete;
		}
		if (count > 0) {
			res = _do_dma_rx(
				dma_h, dma_buf_iova[0] | 0x1000000000000,
				0, count, 1, s2m_transfer.transfer_type,
				true /*intr_en*/,
				s2m_transfer.rx_ctrl /*rx_ctrl*/);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_ST Transfer failed");
			poll_interrupt(&dma_h->header S2M_TW3);
			do {
				_pop_response_fifo(dma_h, &fill_level,
						   &tf_count, &eop_arrived);
				s2m_transfer.rx_bytes += tf_count;
				if (fill_level > 0) {
					local_memcpy(
						(void *)(s2m_transfer.dst
							 + dma_chunks
								   * FPGA_DMA_BUF_SIZE),
						dma_buf_ptr[0],
						tf_count);
					count -= tf_count;
				}
			} while (count != 0 && eop_arrived == 0);
			if (eop_arrived)
				goto out_transf_complete;
		}
	out_transf_complete:
		if (eop_arrived) {
#ifdef FORCE_DESC_FLUSH
			s2m_pending_desc_flush(dma_h);
			dma_h->unused_desc_count = 0;
			dma_h->next_avail_desc_idx = 0;
#else
			dma_h->next_avail_desc_idx = tail % FPGA_DMA_MAX_BUF;
			dma_h->unused_desc_count = head - tail;
#endif
			s2m_transfer.eop_status = 1;
		}

		res = fpgaDMAEnqueue(&dma_h->header.dma_h->transferCompleteq,
				     &s2m_transfer);
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMAEnqueue failed");
			return NULL;
		}
	}
out:
	return dma_h;
}
