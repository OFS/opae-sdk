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

// Internal Functions

/**
 * _do_dma
 *
 * @brief                    Performs a DMA transaction with the FPGA
 * @param[in] dma_h          Handle to the FPGA DMA object
 * @param[in] dst            Pointer to a host or FPGA buffer to send or
 * retrieve
 * @param[in] src            Pointer to a host or FPGA buffer to send or
 * retrieve
 * @param[in] count          Number of bytes
 * @param[in] is_last_desc   True if this is the last buffer of a batch
 * @param[in] type           Direction of transfer
 * @param[in] intr_en        True means to ask for an interrupt from the FPGA
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result _do_dma(m2m_dma_handle_t *dma_h, uint64_t dst, uint64_t src,
			   int count, int is_last_desc,
			   fpga_dma_transfer_type_t type, bool intr_en)
{
	msgdma_ext_desc_t desc = {0};
	fpga_result res = FPGA_OK;
	int alignment_offset = 0;
	int segment_size = 0;

	// src, dst and count must be 64-byte aligned
	if (dst % FPGA_DMA_ALIGN_BYTES != 0 || src % FPGA_DMA_ALIGN_BYTES != 0
	    || count % FPGA_DMA_ALIGN_BYTES != 0) {
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

	if (type == FPGA_TO_FPGA_MM) {
		desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
		desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
		desc.len = count;
		desc.wr_burst_count = 4;
		desc.rd_burst_count = 4;
		desc.rd_address_ext = (src >> 32) & FPGA_DMA_MASK_32_BIT;
		desc.wr_address_ext = (dst >> 32) & FPGA_DMA_MASK_32_BIT;

		res = _send_descriptor(&dma_h->header, &desc);
		ON_ERR_GOTO(res, out, "_send_descriptor");
	}
	// either FPGA to Host or Host to FPGA transfer so we need to make sure
	// the DMA transaction is aligned to the burst size (CCIP restriction)
	else {
		// need to determine if the CCIP (host) address is aligned to
		// 4CL (256B).  When 0 the CCIP address is aligned.
		alignment_offset = (type == HOST_TO_FPGA_MM)
					   ? (src % (4 * FPGA_DMA_ALIGN_BYTES))
					   : (dst % (4 * FPGA_DMA_ALIGN_BYTES));

		// not aligned to 4CL so performing a short transfer to get
		// aligned
		if (alignment_offset != 0) {
			desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
			desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
			desc.wr_burst_count = 1;
			desc.rd_burst_count = 1;
			desc.rd_address_ext =
				(src >> 32) & FPGA_DMA_MASK_32_BIT;
			desc.wr_address_ext =
				(dst >> 32) & FPGA_DMA_MASK_32_BIT;

			// count isn't large enough to hit next 4CL boundary
			if (((4 * FPGA_DMA_ALIGN_BYTES) - alignment_offset)
			    >= count) {
				segment_size = count;
				count = 0; // only had to transfer count amount
					   // of data to reach the end of the
					   // provided buffer
			} else {
				segment_size = (4 * FPGA_DMA_ALIGN_BYTES)
					       - alignment_offset;
				src += segment_size;
				dst += segment_size;
				count -= segment_size; // subtract the segment
						       // size from count since
						       // the transfer below
						       // will bring us into 4CL
						       // alignment
				desc.control.transfer_irq_en = 0;
			}

			// will post short transfer to align to a 4CL (256 byte)
			// boundary
			desc.len = segment_size;

			res = _send_descriptor(&dma_h->header, &desc);
			ON_ERR_GOTO(res, out, "_send_descriptor");
		}
		// at this point we are 4CL (256 byte) aligned
		// if there is at least 4CL (256 bytes) of data to transfer,
		// post bursts of 4
		if (count >= (4 * FPGA_DMA_ALIGN_BYTES)) {
			desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
			desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
			desc.wr_burst_count = 4;
			desc.rd_burst_count = 4;
			desc.rd_address_ext =
				(src >> 32) & FPGA_DMA_MASK_32_BIT;
			desc.wr_address_ext =
				(dst >> 32) & FPGA_DMA_MASK_32_BIT;

			// buffer ends on 4CL boundary
			if ((count % (4 * FPGA_DMA_ALIGN_BYTES)) == 0) {
				segment_size = count;
				count = 0; // transfer below will move the
					   // remainder of the buffer
			}
			// buffers do not end on 4CL boundary so transfer only
			// up to the last 4CL boundary leaving a segment at the
			// end to finish later
			else {
				// round count down to the nearest 4CL multiple
				segment_size =
					count
					- (count % (4 * FPGA_DMA_ALIGN_BYTES));
				src += segment_size;
				dst += segment_size;
				count -= segment_size;
				desc.control.transfer_irq_en = 0;
			}

			desc.len = segment_size;

			res = _send_descriptor(&dma_h->header, &desc);
			ON_ERR_GOTO(res, out, "_send_descriptor");
		}
		// at this point we have posted all the bursts of length 4 we
		// can but there might be 64, 128, or 192 bytes of data to
		// transfer still if buffer did not end on 4CL (256 byte)
		// boundary post short transfer to handle the remainder
		if (count > 0) {
			desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
			desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
			desc.len = count;
			desc.wr_burst_count = 1;
			desc.rd_burst_count = 1;
			desc.rd_address_ext =
				(src >> 32) & FPGA_DMA_MASK_32_BIT;
			desc.wr_address_ext =
				(dst >> 32) & FPGA_DMA_MASK_32_BIT;
			if (intr_en)
				desc.control.transfer_irq_en = 1;
			// will post short transfer to move the remainder of the
			// buffer
			res = _send_descriptor(&dma_h->header, &desc);
			ON_ERR_GOTO(res, out, "_send_descriptor");
		}

	} // end of FPGA --> Host or Host --> FPGA transfer

out:
	return res;
}

static fpga_result _issue_magic(m2m_dma_handle_t *dma_h)
{
	fpga_result res = FPGA_OK;
	*(dma_h->magic_buf) = 0x0ULL;

	res = _do_dma(dma_h, dma_h->magic_iova | FPGA_DMA_WF_HOST_MASK,
		      FPGA_DMA_WF_ROM_MAGIC_NO_MASK, 64, 1, FPGA_TO_HOST_MM,
		      true /*intr_en */);
	return res;
}

static void _wait_magic(m2m_dma_handle_t *dma_h WHENCE)
{
	poll_interrupt(&dma_h->header WHENCE_VAR);
	while (*(dma_h->magic_buf) != FPGA_DMA_WF_MAGIC_NO)
		;
	*(dma_h->magic_buf) = 0x0ULL;
}

static fpga_result transferHostToFpga(m2m_dma_handle_t *dma_h,
				      fpga_dma_transfer_t *m2m_transfer,
				      fpga_dma_transfer_type_t type)
{
	fpga_result res = FPGA_OK;
	uint64_t dst = m2m_transfer->dst;
	uint64_t src = m2m_transfer->src;
	size_t count = m2m_transfer->len;
	uint64_t i = 0;
	uint64_t count_left = count;
	uint64_t aligned_addr = 0;
	uint64_t align_bytes = 0;
	int issued_intr = 0;
	debug_print("Host To Fpga ----------- src = %08lx, dst = %08lx \n", src,
		    dst);
	if (!IS_DMA_ALIGNED(dst)) {
		if (count_left < FPGA_DMA_ALIGN_BYTES) {
			res = _ase_host_to_fpga(dma_h, &dst, &src, count_left);
			ON_ERR_GOTO(res, out,
				    "HOST_TO_FPGA_MM Transfer failed\n");
			fflush(stdout);
			return res;
		} else {
			aligned_addr = ((dst / FPGA_DMA_ALIGN_BYTES) + 1)
				       * FPGA_DMA_ALIGN_BYTES;
			align_bytes = aligned_addr - dst;
			res = _ase_host_to_fpga(dma_h, &dst, &src, align_bytes);
			ON_ERR_GOTO(res, out,
				    "HOST_TO_FPGA_MM Transfer failed\n");
			count_left = count_left - align_bytes;
			fflush(stdout);
		}
	}

	buffer_pool_item **buffers;
	// False means data is already in the buffer
	uint32_t do_memcpy = 1;
	int half_num_buffers = m2m_transfer->num_buffers / 2;
	int num_buffers = m2m_transfer->num_buffers;
	//	buffer_pool_item sbp;

	if (m2m_transfer->small_buffer) {
		uint64_t offset;
		buffer_pool_item *p_sbp = m2m_transfer->small_buffer;
		assert(m2m_transfer->num_buffers == 1);
		offset = src - (uint64_t)p_sbp->dma_buf_ptr;
		assert(((uint64_t)p_sbp->dma_buf_ptr + offset
			+ m2m_transfer->len)
		       <= ((uint64_t)p_sbp->dma_buf_ptr + p_sbp->size));

		res = _do_dma(dma_h, dst,
			      (p_sbp->dma_buf_iova + offset)
				      | FPGA_DMA_HOST_MASK,
			      m2m_transfer->len, 1, type, true);

		poll_interrupt(&dma_h->header XFER_H2F2);

		return res;
	} else {
		buffers = m2m_transfer->buffers;
	}

	// All same sized buffers
	uint64_t buffer_size = buffers[0]->size;

	if (count_left) {
		uint64_t dma_chunks = count_left / buffer_size;
		count_left -= (dma_chunks * buffer_size);
		debug_print("DMA TX : dma chunks = %" PRIu64
			    ", count_left = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);

		uint64_t dma_last_chunk = (count_left / FPGA_DMA_ALIGN_BYTES)
					  * FPGA_DMA_ALIGN_BYTES;
		count_left -= dma_last_chunk;
		if (dma_last_chunk) {
			dma_chunks++;
		} else {
			dma_last_chunk = buffer_size;
		}

		bool ping_pong = true;
		if (dma_chunks <= (uint64_t)num_buffers) {
			ping_pong = false;
		}

		for (i = 0; i < dma_chunks; i++) {
			uint64_t *dma_buf_ptr =
				buffers[i % num_buffers]->dma_buf_ptr;
			uint64_t dma_buf_iova =
				buffers[i % num_buffers]->dma_buf_iova
				| FPGA_DMA_HOST_MASK;
			if (do_memcpy) {
				local_memcpy(dma_buf_ptr,
					     (void *)(src + i * buffer_size),
					     (i == (dma_chunks - 1))
						     ? dma_last_chunk
						     : buffer_size);
			}
			if ((ping_pong
			     && ((i % (uint64_t)half_num_buffers
				  == ((uint64_t)half_num_buffers - 1))))
			    || (i == (dma_chunks - 1)) /*last descriptor */) {
				uint64_t siz_last = (i == (dma_chunks - 1))
							    ? dma_last_chunk
							    : buffer_size;
				if (i == ((uint64_t)half_num_buffers - 1)) {
					res = _do_dma(dma_h,
						      (dst + i * buffer_size),
						      dma_buf_iova, siz_last, 1,
						      type, true);
				} else {
					if (issued_intr)
						poll_interrupt(
							&dma_h->header
								 XFER_H2F);
					res = _do_dma(dma_h,
						      (dst + i * buffer_size),
						      dma_buf_iova, siz_last, 1,
						      type, true /*intr_en */);
				}
				issued_intr = 1;
			} else {
				res = _do_dma(dma_h, (dst + i * buffer_size),
					      dma_buf_iova, buffer_size, 0,
					      type, false /*intr_en */);
			}
		}
		if (issued_intr) {
			poll_interrupt(&dma_h->header XFER_H2F2);
			issued_intr = 0;
		}
		if (count_left) {
			uint64_t dma_tx_bytes =
				(count_left / FPGA_DMA_ALIGN_BYTES)
				* FPGA_DMA_ALIGN_BYTES;
			assert(dma_tx_bytes == 0);
			if (count_left) {
				dst = dst + dma_chunks * buffer_size
				      + dma_tx_bytes
				      - (buffer_size - dma_last_chunk);
				src = src + dma_chunks * buffer_size
				      + dma_tx_bytes
				      - (buffer_size - dma_last_chunk);
				res = _ase_host_to_fpga(dma_h, &dst, &src,
							count_left);
				ON_ERR_GOTO(
					res, out,
					"HOST_TO_FPGA_MM Transfer failed\n");
				fflush(stdout);
			}
		}
	}
out:
	return res;
}

static fpga_result transferFpgaToHost(m2m_dma_handle_t *dma_h,
				      fpga_dma_transfer_t *m2m_transfer,
				      fpga_dma_transfer_type_t type)
{
	fpga_result res = FPGA_OK;
	uint64_t dst = m2m_transfer->dst;
	uint64_t src = m2m_transfer->src;
	size_t count = m2m_transfer->len;
	uint64_t i = 0;
	uint64_t j = 0;
	uint64_t count_left = count;
	uint64_t aligned_addr = 0;
	uint64_t align_bytes = 0;
	int wf_issued = 0;

	debug_print("FPGA To Host ----------- src = %08lx, dst = %08lx \n", src,
		    dst);
	if (!IS_DMA_ALIGNED(src)) {
		if (count_left < FPGA_DMA_ALIGN_BYTES) {
			res = _ase_fpga_to_host(dma_h, &src, &dst, count_left);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_MM Transfer failed");
			fflush(stdout);
			return res;
		} else {
			aligned_addr = ((src / FPGA_DMA_ALIGN_BYTES) + 1)
				       * FPGA_DMA_ALIGN_BYTES;
			align_bytes = aligned_addr - src;
			res = _ase_fpga_to_host(dma_h, &src, &dst, align_bytes);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_MM Transfer failed");
			count_left = count_left - align_bytes;
			fflush(stdout);
		}
	}

	buffer_pool_item **buffers;
	// False means data transferred directly to the buffer
	uint32_t do_memcpy = 1;
	int half_num_buffers = m2m_transfer->num_buffers / 2;
	int num_buffers = m2m_transfer->num_buffers;

	if (m2m_transfer->small_buffer) {
		uint64_t offset;
		buffer_pool_item *p_sbp = m2m_transfer->small_buffer;
		assert(m2m_transfer->num_buffers == 1);

		offset = dst - (uint64_t)p_sbp->dma_buf_ptr;
		assert(((uint64_t)p_sbp->dma_buf_ptr + offset
			+ m2m_transfer->len)
		       <= ((uint64_t)p_sbp->dma_buf_ptr + p_sbp->size));

		res = _do_dma(dma_h,
			      (p_sbp->dma_buf_iova + offset)
				      | FPGA_DMA_HOST_MASK,
			      src, m2m_transfer->len, 1, type, true);

		res = _issue_magic(dma_h);
		ON_ERR_GOTO(res, out, "Magic number issue failed");
		_wait_magic(dma_h WM_F2H2);

		return res;
	} else {
		buffers = m2m_transfer->buffers;
	}

	// All same sized buffers
	uint64_t buffer_size = buffers[0]->size;

	if (count_left) {
		uint64_t dma_chunks = count_left / buffer_size;
		count_left -= (dma_chunks * buffer_size);
		debug_print("DMA TX : dma chunks = %" PRIu64
			    ", count_left = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);
		// assert(num_buffers >= 8);
		uint64_t pending_buf = 0;

		uint64_t dma_last_chunk = (count_left / FPGA_DMA_ALIGN_BYTES)
					  * FPGA_DMA_ALIGN_BYTES;
		count_left -= dma_last_chunk;
		if (dma_last_chunk) {
			dma_chunks++;
		} else {
			dma_last_chunk = buffer_size;
		}

		bool ping_pong = true;
		if (dma_chunks <= (uint64_t)num_buffers) {
			ping_pong = false;
		}

		for (i = 0; i < dma_chunks; i++) {
			uint64_t dma_buf_iova =
				buffers[i % num_buffers]->dma_buf_iova
				| FPGA_DMA_HOST_MASK;

			uint64_t siz_last = (i == (dma_chunks - 1))
						    ? dma_last_chunk
						    : buffer_size;
			res = _do_dma(dma_h, dma_buf_iova,
				      (src + i * buffer_size), siz_last, 1,
				      type, false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_MM Transfer failed");

			const int num_pending = i - pending_buf + 1;
			if (ping_pong && (num_pending == half_num_buffers)) {
				// Enters this loop only once,after first batch
				// of descriptors.
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				wf_issued = 1;
			}
			if ((ping_pong && (num_pending > (num_buffers - 1)))
			    || (i == (dma_chunks - 1)) /*last descriptor */) {
				if (wf_issued) {
					_wait_magic(dma_h WM_F2H);
					for (j = 0;
					     j < (uint64_t)half_num_buffers;
					     j++) {
						// constant size transfer; no
						// length check required
						if (do_memcpy) {
							local_memcpy(
								(void *)(dst
									 + pending_buf
										   * buffer_size),
								buffers[pending_buf
									% num_buffers]
									->dma_buf_ptr,
								buffer_size);
						}
						pending_buf++;
					}
					wf_issued = 0;
				}
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				wf_issued = 1;
			}
		}

		if (wf_issued)
			_wait_magic(dma_h WM_F2H2);

		// clear out final dma memcpy operations
		while (pending_buf < dma_chunks) {
			uint64_t siz_last = (pending_buf == (dma_chunks - 1))
						    ? dma_last_chunk
						    : buffer_size;

			if (do_memcpy) {
				local_memcpy(
					(void *)(dst
						 + pending_buf * buffer_size),
					buffers[pending_buf % (num_buffers)]
						->dma_buf_ptr,
					siz_last);
			}
			pending_buf++;
		}

		if (count_left > 0) {
			uint64_t dma_tx_bytes =
				(count_left / FPGA_DMA_ALIGN_BYTES)
				* FPGA_DMA_ALIGN_BYTES;
			assert(dma_tx_bytes == 0);

			if (count_left) {
				dst = dst + dma_chunks * buffer_size
				      + dma_tx_bytes
				      - (buffer_size - dma_last_chunk);
				src = src + dma_chunks * buffer_size
				      + dma_tx_bytes
				      - (buffer_size - dma_last_chunk);
				res = _ase_fpga_to_host(dma_h, &src, &dst,
							count_left);
				ON_ERR_GOTO(res, out,
					    "FPGA_TO_HOST_MM Transfer failed");
				fflush(stdout);
			}
		}
	}
out:
	return res;
}

static fpga_result transferFpgaToFpga(m2m_dma_handle_t *dma_h,
				      fpga_dma_transfer_t *m2m_transfer,
				      fpga_dma_transfer_type_t type)
{
	fpga_result res = FPGA_OK;
	uint64_t dst = m2m_transfer->dst;
	uint64_t src = m2m_transfer->src;
	size_t count = m2m_transfer->len;
	uint64_t i = 0;
	uint64_t count_left = count;
	uint64_t *tmp_buf = NULL;
	if (IS_DMA_ALIGNED(dst) && IS_DMA_ALIGNED(src)
	    && IS_DMA_ALIGNED(count_left)) {
		uint64_t dma_chunks = count_left / FPGA_DMA_BUF_SIZE;
		count_left -= (dma_chunks * FPGA_DMA_BUF_SIZE);
		debug_print("!!!FPGA to FPGA!!! TX :dma chunks = %" PRIu64
			    ", count = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);

		for (i = 0; i < dma_chunks; i++) {
			res = _do_dma(dma_h, (dst + i * FPGA_DMA_BUF_SIZE),
				      (src + i * FPGA_DMA_BUF_SIZE),
				      FPGA_DMA_BUF_SIZE, 0, type,
				      false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_FPGA_MM Transfer failed");
			if ((i + 1) % FPGA_DMA_MAX_BUF == 0
			    || i == (dma_chunks - 1) /*last descriptor */) {
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				_wait_magic(dma_h WM_F2F);
			}
		}
		if (count_left > 0) {
			debug_print(
				"Count_left = %08lx  was transfered using DMA\n",
				count_left);
			res = _do_dma(dma_h,
				      (dst + dma_chunks * FPGA_DMA_BUF_SIZE),
				      (src + dma_chunks * FPGA_DMA_BUF_SIZE),
				      count_left, 1, type, false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_FPGA_MM Transfer failed");
			res = _issue_magic(dma_h);
			ON_ERR_GOTO(res, out, "Magic number issue failed");
			_wait_magic(dma_h WM_F2F2);
		}
	} else {
		if ((src < dst) && (src + count_left >= dst)) {
			debug_print(
				"Overlapping addresses, Provide correct dst address\n");
			return FPGA_NOT_SUPPORTED;
		}
		uint32_t tx_chunks = count_left / FPGA_DMA_BUF_ALIGN_SIZE;
		count_left -= (tx_chunks * FPGA_DMA_BUF_ALIGN_SIZE);
		debug_print(
			"!!!FPGA to FPGA TX!!! : tx chunks = %d, count = %08lx, dst = %08lx, src = %08lx \n",
			tx_chunks, count_left, dst, src);
		tmp_buf = (uint64_t *)malloc(FPGA_DMA_BUF_ALIGN_SIZE);
		fpga_dma_transfer_t tmp_transfer;
		local_memcpy(&tmp_transfer, m2m_transfer, sizeof(tmp_transfer));
		for (i = 0; i < tx_chunks; i++) {
			tmp_transfer.dst = (uint64_t)tmp_buf;
			tmp_transfer.src = (src + i * FPGA_DMA_BUF_ALIGN_SIZE);
			tmp_transfer.len = FPGA_DMA_BUF_ALIGN_SIZE;
			res = transferFpgaToHost(dma_h, &tmp_transfer,
						 FPGA_TO_HOST_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
			tmp_transfer.dst = (dst + i * FPGA_DMA_BUF_ALIGN_SIZE);
			tmp_transfer.src = (uint64_t)tmp_buf;
			tmp_transfer.len = FPGA_DMA_BUF_ALIGN_SIZE;
			res = transferHostToFpga(dma_h, &tmp_transfer,
						 HOST_TO_FPGA_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
		}
		if (count_left > 0) {
			tmp_transfer.dst = (uint64_t)tmp_buf;
			tmp_transfer.src =
				(src + tx_chunks * FPGA_DMA_BUF_ALIGN_SIZE);
			tmp_transfer.len = count_left;
			res = transferFpgaToHost(dma_h, &tmp_transfer,
						 FPGA_TO_HOST_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
			tmp_transfer.dst =
				(dst + tx_chunks * FPGA_DMA_BUF_ALIGN_SIZE);
			tmp_transfer.src = (uint64_t)tmp_buf;
			tmp_transfer.len = count_left;
			res = transferHostToFpga(dma_h, &tmp_transfer,
						 HOST_TO_FPGA_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
		}
		free(tmp_buf);
	}
out:
	return res;
out_spl:
	free(tmp_buf);
	return res;
}

#ifdef CHECK_DELAYS_MM
// return elapsed time
static inline double getTime(struct timespec start, struct timespec end)
{
	uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec
			- start.tv_nsec;
	return (double)diff / (double)1000000000L;
}
#endif

static fpga_result fpgaDMATransferSync(m2m_dma_handle_t *dma_h,
				       fpga_dma_transfer_t *m2m_transfer)
{

	fpga_result res = FPGA_OK;
#ifdef CHECK_DELAYS_MM
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
#endif

	if (m2m_transfer->transfer_type == HOST_TO_FPGA_MM) {
		res = transferHostToFpga(dma_h, m2m_transfer, HOST_TO_FPGA_MM);
	} else if (m2m_transfer->transfer_type == FPGA_TO_HOST_MM) {
		res = transferFpgaToHost(dma_h, m2m_transfer, FPGA_TO_HOST_MM);
	} else if (m2m_transfer->transfer_type == FPGA_TO_FPGA_MM) {
		res = transferFpgaToFpga(dma_h, m2m_transfer, FPGA_TO_FPGA_MM);
	} else {
		return FPGA_NOT_SUPPORTED;
	}

#ifdef CHECK_DELAYS_MM
	clock_gettime(CLOCK_MONOTONIC, &end);
	double tot_time = getTime(start, end);
	int i;
	counts *c;
	char *w[] = {
		"wait_magic_f2f_2", "xfer_h2f",		"xfer_h2f_2",
		"xfer_h2f_3",       "m2s_worker",       "m2s_worker2",
		"s2m_worker",       "s2m_worker_2",     "s2m_worker_3",
		"wait_magic_f2h",   "wait_magic_f2h_2", "wait_magic_f2h_3",
		"wait_magic_f2f",   "poll_wait_magic",
	};
	printf("*********\n");
	for (i = 0; i < NUM_DELAYZ; i++) {
		c = &delayz[i];
		if (c->call_count != 0) {
			printf("%s: call count = %lld ", w[i],
			       (long long)c->call_count);
			printf("Polling - wait count = %lld busy count = "
			       "%lld full count = %lld ",
			       (long long)c->poll_wait_count,
			       (long long)c->poll_busy_count,
			       (long long)buf_full_count);
			printf("bandwidth = %lf ",
			       (double)count
				       / ((double)tot_time * 1000 * 1000));
			printf("wait / busy = %lf\n",
			       (double)c->poll_wait_count
				       / (double)c->poll_busy_count);
			c->call_count = 0;
		}
		c->poll_wait_count = 0;
		c->poll_busy_count = 0;
		c->call_count = 0;
	}
	printf("*********\n");
	buf_full_count = 0;
	fflush(stdout);
	fflush(stderr);
#endif

	return res;
}

void *m2mTransactionWorker(void *dma_handle)
{
	fpga_result res = FPGA_OK;
	m2m_dma_handle_t *dma_h = (m2m_dma_handle_t *)dma_handle;

	// At this point, we're alive.  Let the main thread know
	sem_post(&dma_h->header.dma_h->m2m_thread_sem);

	while (1) {
		fpga_dma_transfer_t m2m_transfer;
		res = fpgaDMADequeue(&dma_h->header.transferRequestq,
				     &m2m_transfer);
		if (res == FPGA_NO_ACCESS) {
			debug_print("M2M thread termination");
			break;
		}
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMADequeue failed");
			return NULL;
		}
		debug_print("MM --- src_addr = %08lx, dst_addr = %08lx\n",
			    m2m_transfer.src, m2m_transfer.dst);

		if (!m2m_transfer.small_buffer) {
			m2m_transfer.num_buffers =
				((m2m_transfer.len + FPGA_DMA_BUF_SIZE - 1)
				 / FPGA_DMA_BUF_SIZE);
			m2m_transfer.num_buffers =
				min(m2m_transfer.num_buffers,
				    (uint32_t)FPGA_DMA_MAX_BUF);
			m2m_transfer.buffers = (buffer_pool_item **)calloc(
				m2m_transfer.num_buffers,
				sizeof(buffer_pool_item *));
			uint32_t i;
			for (i = 0; i < m2m_transfer.num_buffers; i++) {
				m2m_transfer.buffers[i] =
					getFreeBuffer(&dma_h->header);
			}
		}

		res = fpgaDMATransferSync(dma_h, &m2m_transfer);

		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMATransferSync failed");
			return NULL;
		}

		res = fpgaDMAEnqueue(&dma_h->header.dma_h->transferCompleteq,
				     &m2m_transfer);
		if (res != FPGA_OK) {
			FPGA_DMA_ST_ERR("fpgaDMAEnqueue failed");
			return NULL;
		}
	}

	return dma_handle;
}
