// Copyright(c) 2017, Intel Corporation
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
#include "safe_string/safe_string.h"
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

#ifdef FPGA_DMA_DEBUG
static int err_cnt = 0;
#endif

#ifdef CHECK_DELAYS
double poll_wait_count = 0;
double buf_full_count = 0;
#endif

uint64_t fpga_dma_buf_size = 1023 * 1024;

// For signal handler - need to properly handle HUP
static struct sigaction old_action;
static volatile uint32_t *CsrControl;
static void sig_handler(int sig, siginfo_t *signfo, void *unused);

/**
 * local_memcpy
 *
 * @brief                memcpy using SSE2 or REP MOVSB
 * @param[in] dst        Pointer to the destination memory
 * @param[in] src        Pointer to the source memory
 * @param[in] n          Size in bytes
 * @return dst
 *
 */
void *local_memcpy(void *dst, void *src, size_t n)
{
#ifdef USE_MEMCPY
	return memcpy(dst, src, n);
#else
	void *ldst = dst;
	void *lsrc = (void *)src;
	if (IS_CL_ALIGNED(src) && IS_CL_ALIGNED(dst)) // 64-byte aligned
	{
		if (n >= MIN_SSE2_SIZE) // Arbitrary crossover performance point
		{
			debug_print("copying 0x%lx bytes with SSE2\n",
				    (uint64_t)ALIGN_TO_CL(n));
			aligned_block_copy_sse2((int64_t * __restrict) dst,
						(int64_t * __restrict) src,
						ALIGN_TO_CL(n));
			ldst = (void *)((uint64_t)dst + ALIGN_TO_CL(n));
			lsrc = (void *)((uint64_t)src + ALIGN_TO_CL(n));
			n -= ALIGN_TO_CL(n);
		}
	} else {
		if (n >= MIN_SSE2_SIZE) // Arbitrary crossover performance point
		{
			debug_print(
				"copying 0x%lx bytes (unaligned) with SSE2\n",
				(uint64_t)ALIGN_TO_CL(n));
			unaligned_block_copy_sse2((int64_t * __restrict) dst,
						  (int64_t * __restrict) src,
						  ALIGN_TO_CL(n));
			ldst = (void *)((uint64_t)dst + ALIGN_TO_CL(n));
			lsrc = (void *)((uint64_t)src + ALIGN_TO_CL(n));
			n -= ALIGN_TO_CL(n);
		}
	}

	if (n) {
		register unsigned long int dummy;
		debug_print("copying 0x%lx bytes with REP MOVSB\n", n);
		__asm__ __volatile__("rep movsb\n"
				     : "=&D"(ldst), "=&S"(lsrc), "=&c"(dummy)
				     : "0"(ldst), "1"(lsrc), "2"(n)
				     : "memory");
	}

	return dst;
#endif
}

/*
 * macro for checking return codes
 */
#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			error_print("Error %s: %s\n", (desc),                  \
				    fpgaErrStr(res));                          \
			goto label;                                            \
		}                                                              \
	} while (0)

#define ON_ERR_RETURN(res, desc)                                               \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			error_print("Error %s: %s\n", (desc),                  \
				    fpgaErrStr(res));                          \
			return (res);                                          \
		}                                                              \
	} while (0)

// Internal Functions

/**
 * MMIOWrite64Blk
 *
 * @brief                Writes a block of 64-bit values to FPGA MMIO space
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] device     FPGA address
 * @param[in] host       Host buffer address
 * @param[in] count      Size in bytes
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result MMIOWrite64Blk(fpga_dma_handle dma_h, uint64_t device,
				  uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_QWORD(device));
	assert(IS_ALIGNED_QWORD(bytes));

	uint64_t *haddr = (uint64_t *)host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#ifndef USE_ASE
	volatile uint64_t *dev_addr = HOST_MMIO_64_ADDR(dma_h, device);
#endif

	debug_print("copying %lld bytes from 0x%p to 0x%p\n",
		    (long long int)bytes, haddr, (void *)device);
	for (i = 0; i < bytes / sizeof(uint64_t); i++) {
#ifdef USE_ASE
		res = fpgaWriteMMIO64(dma_h->fpga_h, dma_h->mmio_num, device,
				      *haddr);
		ON_ERR_RETURN(res, "fpgaWriteMMIO64");
		haddr++;
		device += sizeof(uint64_t);
#else
		*dev_addr++ = *haddr++;
#endif
	}
	return res;
}

/**
 * MMIOWrite32Blk
 *
 * @brief                Writes a block of 32-bit values to FPGA MMIO space
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] device     FPGA address
 * @param[in] host       Host buffer address
 * @param[in] count      Size in bytes
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result MMIOWrite32Blk(fpga_dma_handle dma_h, uint64_t device,
				  uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_DWORD(device));
	assert(IS_ALIGNED_DWORD(bytes));

	uint32_t *haddr = (uint32_t *)host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#ifndef USE_ASE
	volatile uint32_t *dev_addr = HOST_MMIO_32_ADDR(dma_h, device);
#endif

	debug_print("copying %lld bytes from 0x%p to 0x%p\n",
		    (long long int)bytes, haddr, (void *)device);
	for (i = 0; i < bytes / sizeof(uint32_t); i++) {
#ifdef USE_ASE
		res = fpgaWriteMMIO32(dma_h->fpga_h, dma_h->mmio_num, device,
				      *haddr);
		ON_ERR_RETURN(res, "fpgaWriteMMIO32");
		haddr++;
		device += sizeof(uint32_t);
#else
		*dev_addr++ = *haddr++;
#endif
	}
	return res;
}

/**
 * MMIORead64Blk
 *
 * @brief                Reads a block of 64-bit values from FPGA MMIO space
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] device     FPGA address
 * @param[in] host       Host buffer address
 * @param[in] count      Size in bytes
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result MMIORead64Blk(fpga_dma_handle dma_h, uint64_t device,
				 uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_QWORD(device));
	assert(IS_ALIGNED_QWORD(bytes));

	uint64_t *haddr = (uint64_t *)host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#ifndef USE_ASE
	volatile uint64_t *dev_addr = HOST_MMIO_64_ADDR(dma_h, device);
#endif

	debug_print("copying %lld bytes from 0x%p to 0x%p\n",
		    (long long int)bytes, (void *)device, haddr);
	for (i = 0; i < bytes / sizeof(uint64_t); i++) {
#ifdef USE_ASE
		res = fpgaReadMMIO64(dma_h->fpga_h, dma_h->mmio_num, device,
				     haddr);
		ON_ERR_RETURN(res, "fpgaReadMMIO64");
		haddr++;
		device += sizeof(uint64_t);
#else
		*haddr++ = *dev_addr++;
#endif
	}
	return res;
}

/**
 * MMIORead32Blk
 *
 * @brief                Reads a block of 32-bit values from FPGA MMIO space
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] device     FPGA address
 * @param[in] host       Host buffer address
 * @param[in] count      Size in bytes
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result MMIORead32Blk(fpga_dma_handle dma_h, uint64_t device,
				 uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_DWORD(device));
	assert(IS_ALIGNED_DWORD(bytes));

	uint32_t *haddr = (uint32_t *)host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#ifndef USE_ASE
	volatile uint32_t *dev_addr = HOST_MMIO_32_ADDR(dma_h, device);
#endif

	debug_print("copying %lld bytes from 0x%p to 0x%p\n",
		    (long long int)bytes, (void *)device, haddr);
	for (i = 0; i < bytes / sizeof(uint32_t); i++) {
#ifdef USE_ASE
		res = fpgaReadMMIO32(dma_h->fpga_h, dma_h->mmio_num, device,
				     haddr);
		ON_ERR_RETURN(res, "fpgaReadMMIO32");
		haddr++;
		device += sizeof(uint32_t);
#else
		*haddr++ = *dev_addr++;
#endif
	}
	return res;
}

// End of feature list
static inline bool _fpga_dma_feature_eol(uint64_t dfh)
{
	return ((dfh >> AFU_DFH_EOL_OFFSET) & 1) == 1;
}

// Feature type is BBB
static inline bool _fpga_dma_feature_is_bbb(uint64_t dfh)
{
	// BBB is type 2
	return ((dfh >> AFU_DFH_TYPE_OFFSET) & 0xf) == FPGA_DMA_BBB;
}

// Offset to the next feature header
static inline uint64_t _fpga_dma_feature_next(uint64_t dfh)
{
	return (dfh >> AFU_DFH_NEXT_OFFSET) & 0xffffff;
}

/**
 * _switch_to_ase_page
 *
 * @brief                Updates the current page of ASE to the address given
 * @param[in] dma_h      Handle to the FPGA DMA object
 * @param[in] addr       Address to which the ASE page should be switched
 * @return Nothing.  Side-effect is to update the current page in the DMA
 * handle.
 *
 */
static inline void _switch_to_ase_page(fpga_dma_handle dma_h, uint64_t addr)
{
	uint64_t requested_page = addr & ~DMA_ADDR_SPAN_EXT_WINDOW_MASK;

	if (requested_page != dma_h->cur_ase_page) {
		MMIOWrite64Blk(dma_h, ASE_CNTL_BASE(dma_h),
			       (uint64_t)&requested_page,
			       sizeof(requested_page));
		dma_h->cur_ase_page = requested_page;
	}
}

/**
 * _send_descriptor
 *
 * @brief                Queues a DMA descriptor to the FPGA
 * @param[in] dma_h      Handle to the FPGA DMA object
 * @param[in] desc       Pointer to a descriptor structure to send
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result _send_descriptor(fpga_dma_handle dma_h,
				    msgdma_ext_desc_t *desc)
{
	fpga_result res = FPGA_OK;
	msgdma_status_t status = {0};

	debug_print("desc.rd_address = %x\n", desc->rd_address);
	debug_print("desc.wr_address = %x\n", desc->wr_address);
	debug_print("desc.len = %x\n", desc->len);
	debug_print("desc.wr_burst_count = %x\n", desc->wr_burst_count);
	debug_print("desc.rd_burst_count = %x\n", desc->rd_burst_count);
	debug_print("desc.wr_stride %x\n", desc->wr_stride);
	debug_print("desc.rd_stride %x\n", desc->rd_stride);
	debug_print("desc.rd_address_ext %x\n", desc->rd_address_ext);
	debug_print("desc.wr_address_ext %x\n", desc->wr_address_ext);

	debug_print("SGDMA_CSR_BASE = %lx SGDMA_DESC_BASE=%lx\n",
		    dma_h->dma_csr_base, dma_h->dma_desc_base);

#ifdef CHECK_DELAYS
	bool first = true;
#endif
	do {
		res = MMIORead32Blk(dma_h, CSR_STATUS(dma_h),
				    (uint64_t)&status.reg, sizeof(status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
#ifdef CHECK_DELAYS
		if (first && status.st.desc_buf_full) {
			buf_full_count++;
			first = false;
		}
#endif
	} while (status.st.desc_buf_full);

	res = MMIOWrite64Blk(dma_h, dma_h->dma_desc_base, (uint64_t)desc,
			     sizeof(*desc));
	ON_ERR_GOTO(res, out, "MMIOWrite64Blk");

out:
	return res;
}

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
static fpga_result _do_dma(fpga_dma_handle dma_h, uint64_t dst, uint64_t src,
			   int count, int is_last_desc,
			   fpga_dma_transfer_t type, bool intr_en)
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

		res = _send_descriptor(dma_h, &desc);
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

			res = _send_descriptor(dma_h, &desc);
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
				segment_size =
					count
					- (count
					   % (4 * FPGA_DMA_ALIGN_BYTES)); // round
									  // count
									  // down
									  // to
									  // the
									  // nearest
									  // multiple
									  // of
									  // 4CL
				src += segment_size;
				dst += segment_size;
				count -= segment_size;
				desc.control.transfer_irq_en = 0;
			}

			desc.len = segment_size;

			res = _send_descriptor(dma_h, &desc);
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
			res = _send_descriptor(dma_h, &desc);
			ON_ERR_GOTO(res, out, "_send_descriptor");
		}

	} // end of FPGA --> Host or Host --> FPGA transfer

out:
	return res;
}

// Public APIs
fpga_result fpgaDmaOpen(fpga_handle fpga, int dma_idx, fpga_dma_handle *dma_p)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle dma_h = NULL;
	int i = 0;
	if (!fpga) {
		return FPGA_INVALID_PARAM;
	}
	if (!dma_p) {
		return FPGA_INVALID_PARAM;
	}
    if (dma_idx < 0 || dma_idx > 3) {
        return FPGA_INVALID_PARAM;
    }

	// init the dma handle
	dma_h = (fpga_dma_handle)malloc(sizeof(struct _dma_handle_t));
	if (!dma_h) {
		return FPGA_NO_MEMORY;
	}
	dma_h->fpga_h = fpga;
	for (i = 0; i < FPGA_DMA_MAX_BUF; i++)
		dma_h->dma_buf_ptr[i] = NULL;
	dma_h->mmio_num = 0;
	dma_h->mmio_offset = 0;
	dma_h->cur_ase_page = 0xffffffffffffffffUll;

	// Discover DMA BBB by traversing the device feature list
	bool end_of_list = false;
	bool dma_found = false;

#ifndef USE_ASE
	res = fpgaMapMMIO(dma_h->fpga_h, 0, (uint64_t **)&dma_h->mmio_va);
	ON_ERR_GOTO(res, out, "fpgaMapMMIO");
#endif

	uint64_t offset = dma_h->mmio_offset;
	dfh_feature_t dfh = {0, 0, 0};
	do {
		// Read the next feature header
		res = MMIORead64Blk(dma_h, offset, (uint64_t)&dfh, sizeof(dfh));
		ON_ERR_GOTO(res, out, "MMIORead64Blk");

		if (_fpga_dma_feature_is_bbb(dfh.dfh)
		    && (dfh.feature_uuid_lo == FPGA_DMA_UUID_L)
		    && (dfh.feature_uuid_hi == FPGA_DMA_UUID_H)) {
			if (dma_idx-- == 0) {
				// Found the specific one. Record it.
				dma_h->dma_base = offset;
				dma_h->dma_csr_base = dma_h->dma_base + FPGA_DMA_CSR;
				dma_h->dma_desc_base = dma_h->dma_base + FPGA_DMA_DESC;
				dma_h->dma_ase_cntl_base =
					dma_h->dma_base + FPGA_DMA_ADDR_SPAN_EXT_CNTL;
				dma_h->dma_ase_data_base =
					dma_h->dma_base + FPGA_DMA_ADDR_SPAN_EXT_DATA;
				dma_found = true;
				break;
			}
		}
		// End of the list?
		end_of_list = _fpga_dma_feature_eol(dfh.dfh);

		// Move to the next feature header
		if (_fpga_dma_feature_next(dfh.dfh) == 0xffff) {
			dma_found = false;
			break;
		}
		offset = offset + _fpga_dma_feature_next(dfh.dfh);
	} while (!end_of_list);

	if (dma_found) {
		*dma_p = dma_h;
		res = FPGA_OK;
	} else {
		*dma_p = NULL;
		res = FPGA_NOT_FOUND;
		goto out;
	}

	// Buffer size must be page aligned for prepareBuffer
	for (i = 0; i < FPGA_DMA_MAX_BUF; i++) {
		res = fpgaPrepareBuffer(dma_h->fpga_h, fpga_dma_buf_size,
					(void **)&(dma_h->dma_buf_ptr[i]),
					&dma_h->dma_buf_wsid[i], 0);
		ON_ERR_GOTO(res, out, "fpgaPrepareBuffer");

		res = fpgaGetIOAddress(dma_h->fpga_h, dma_h->dma_buf_wsid[i],
				       &dma_h->dma_buf_iova[i]);
		ON_ERR_GOTO(res, rel_buf, "fpgaGetIOAddress");
	}

	// Allocate magic number buffer
	res = fpgaPrepareBuffer(dma_h->fpga_h, FPGA_DMA_ALIGN_BYTES,
				(void **)&(dma_h->magic_buf),
				&dma_h->magic_wsid, 0);
	ON_ERR_GOTO(res, out, "fpgaPrepareBuffer");

	res = fpgaGetIOAddress(dma_h->fpga_h, dma_h->magic_wsid,
			       &dma_h->magic_iova);
	ON_ERR_GOTO(res, rel_buf, "fpgaGetIOAddress");
	memset_s((void *)dma_h->magic_buf, FPGA_DMA_ALIGN_BYTES, 0);

	// turn on global interrupts
	msgdma_ctrl_t ctrl = {0};
	ctrl.ct.global_intr_en_mask = 1;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&ctrl.reg,
			     sizeof(ctrl.reg));
	ON_ERR_GOTO(res, rel_buf, "MMIOWrite32Blk");

	// register interrupt event handle
	res = fpgaCreateEventHandle(&dma_h->eh);
	ON_ERR_GOTO(res, rel_buf, "fpgaCreateEventHandle");

	res = fpgaRegisterEvent(dma_h->fpga_h, FPGA_EVENT_INTERRUPT, dma_h->eh,
				0 /*vector id */);
	ON_ERR_GOTO(res, destroy_eh, "fpgaRegisterEvent");

	struct sigaction sa;
	int sigres;

	memset_s(&sa, sizeof(sa), 0);
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = sig_handler;

	sigres = sigaction(SIGHUP, &sa, &old_action);
	if (sigres < 0) {
		ON_ERR_GOTO(sigres < 0, destroy_eh,
			    "Error: failed to unregister signal handler.\n");
	}
	CsrControl = HOST_MMIO_32_ADDR(dma_h, CSR_CONTROL(dma_h));

	return FPGA_OK;

destroy_eh:
	res = fpgaDestroyEventHandle(&dma_h->eh);
	ON_ERR_GOTO(res, rel_buf, "fpgaDestroyEventHandle");

rel_buf:
	for (i = 0; i < FPGA_DMA_MAX_BUF; i++) {
		res = fpgaReleaseBuffer(dma_h->fpga_h, dma_h->dma_buf_wsid[i]);
		ON_ERR_GOTO(res, out, "fpgaReleaseBuffer");
	}
out:
	if (!dma_found)
		free(dma_h);
	return res;
}

/**
 * _read_memory_mmio_unaligned
 *
 * @brief                Performs a unaligned read(address not 4/8/64 byte
 * aligned) from FPGA address(device address).
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] dev_addr   FPGA address
 * @param[in] host_addr  Host buffer address
 * @param[in] count      Size in bytes, always less than 8bytes.
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result _read_memory_mmio_unaligned(fpga_dma_handle dma_h,
					       uint64_t dev_addr,
					       uint64_t host_addr,
					       uint64_t count)
{
	fpga_result res = FPGA_OK;

	assert(count < QWORD_BYTES);

	if (0 == count)
		return res;

	uint64_t shift = dev_addr % QWORD_BYTES;
	debug_print("shift = %08lx , count = %08lx \n", shift, count);

	_switch_to_ase_page(dma_h, dev_addr);
	uint64_t dev_aligned_addr =
		(dev_addr - shift) & DMA_ADDR_SPAN_EXT_WINDOW_MASK;

	// read data from device memory
	uint64_t read_tmp = 0;
	res = MMIORead64Blk(dma_h, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
			    (uint64_t)&read_tmp, sizeof(read_tmp));
	if (res != FPGA_OK)
		return res;

	// overlay our data
	local_memcpy((void *)host_addr, ((char *)(&read_tmp)) + shift, count);

	return res;
}

/**
 * _write_memory_mmio_unaligned
 *
 * @brief                Performs an unaligned write(address not 4/8/64 byte
 * aligned) to FPGA address(device address).
 * @param[in] dma        Handle to the FPGA DMA object
 * @param[in] dev_addr   FPGA address
 * @param[in] host_addr  Host buffer address
 * @param[in] count      Size in bytes, always less than 8bytes.
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
static fpga_result _write_memory_mmio_unaligned(fpga_dma_handle dma_h,
						uint64_t dev_addr,
						uint64_t host_addr,
						uint64_t count)
{
	fpga_result res = FPGA_OK;

	assert(count < QWORD_BYTES);

	if (0 == count)
		return res;

	uint64_t shift = dev_addr % QWORD_BYTES;
	debug_print("shift = %08lx , count = %08lx \n", shift, count);

	_switch_to_ase_page(dma_h, dev_addr);
	uint64_t dev_aligned_addr = (dev_addr - (dev_addr % QWORD_BYTES))
				    & DMA_ADDR_SPAN_EXT_WINDOW_MASK;

	// read data from device memory
	uint64_t read_tmp = 0;
	res = MMIORead64Blk(dma_h, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
			    (uint64_t)&read_tmp, sizeof(read_tmp));
	if (res != FPGA_OK)
		return res;

	// overlay our data
	local_memcpy(((char *)(&read_tmp)) + shift, (void *)host_addr, count);

	// write back to device
	res = MMIOWrite64Blk(dma_h, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
			     (uint64_t)&read_tmp, sizeof(read_tmp));
	if (res != FPGA_OK)
		return res;

	return res;
}

/**
 * _write_memory_mmio
 *
 * @brief                   Writes to a DWORD/QWORD aligned memory address(FPGA
 * address).
 * @param[in] dma           Handle to the FPGA DMA object
 * @param[in/out] dst_ptr   Pointer to the FPGA address
 * @param[in/out] src_ptr   Pointer to the Host buffer address
 * @param[in/out] count     Pointer to the Size in bytes
 * @return fpga_result      FPGA_OK on success, return code otherwise.  Updates
 * src, dst, and count
 *
 */
static fpga_result _write_memory_mmio(fpga_dma_handle dma_h, uint64_t *dst_ptr,
				      uint64_t *src_ptr, uint64_t *count)
{
	fpga_result res = FPGA_OK;

	if (*count < DWORD_BYTES)
		return res;

	assert(*count >= DWORD_BYTES);
	assert(IS_ALIGNED_DWORD(*dst_ptr));
	if (!IS_ALIGNED_DWORD(*dst_ptr)) // If QWORD aligned, this will be true
		return FPGA_EXCEPTION;

	uint64_t src = *src_ptr;
	uint64_t dst = *dst_ptr;
	uint64_t align_bytes = *count;
	uint64_t offset = 0;

	if (!IS_ALIGNED_QWORD(dst)) {
		// Write out a single DWORD to get QWORD aligned
		_switch_to_ase_page(dma_h, dst);
		offset = dst & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIOWrite32Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				     (uint64_t)src, DWORD_BYTES);
		ON_ERR_RETURN(res, "MMIOWrite32Blk");
		src += DWORD_BYTES;
		dst += DWORD_BYTES;
		align_bytes -= DWORD_BYTES;
	}

	if (0 == align_bytes)
		return res;

	assert(IS_ALIGNED_QWORD(dst));

	// Write out blocks of 64-bit values
	while (align_bytes >= QWORD_BYTES) {
		uint64_t left_in_page = DMA_ADDR_SPAN_EXT_WINDOW;
		left_in_page -= dst & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		uint64_t size_to_copy =
			min(left_in_page, (align_bytes & ~(QWORD_BYTES - 1)));
		if (size_to_copy < QWORD_BYTES)
			break;
		_switch_to_ase_page(dma_h, dst);
		offset = dst & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIOWrite64Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				     (uint64_t)src, size_to_copy);
		ON_ERR_RETURN(res, "MMIOWrite64Blk");
		src += size_to_copy;
		dst += size_to_copy;
		align_bytes -= size_to_copy;
	}

	if (align_bytes >= DWORD_BYTES) {
		// Write out remaining DWORD
		_switch_to_ase_page(dma_h, dst);
		offset = dst & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIOWrite32Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				     (uint64_t)src, DWORD_BYTES);
		ON_ERR_RETURN(res, "MMIOWrite32Blk");
		src += DWORD_BYTES;
		dst += DWORD_BYTES;
		align_bytes -= DWORD_BYTES;
	}

	assert(align_bytes < DWORD_BYTES);

	*src_ptr = src;
	*dst_ptr = dst;
	*count = align_bytes;
	return res;
}

/**
 * _ase_host_to_fpga
 *
 * @brief                   Tx "count" bytes from HOST to FPGA using Address
 * span expander(ASE)- will internally make calls to handle unaligned and
 * aligned MMIO writes.
 * @param[in] dma           Handle to the FPGA DMA object
 * @param[in/out] dst_ptr   Pointer to the FPGA address
 * @param[in/out] src_ptr   Pointer to the Host buffer address
 * @param[in] count         Size in bytes
 * @return fpga_result      FPGA_OK on success, return code otherwise.  Updates
 * src and dst
 *
 */
static fpga_result _ase_host_to_fpga(fpga_dma_handle dma_h, uint64_t *dst_ptr,
				     uint64_t *src_ptr, uint64_t count)
{
	fpga_result res = FPGA_OK;
	uint64_t dst = *dst_ptr;
	uint64_t src = *src_ptr;
	uint64_t count_left = count;
	uint64_t unaligned_size = 0;

	debug_print("dst_ptr = %08lx , count = %08lx, src = %08lx \n", *dst_ptr,
		    count, *src_ptr);

	// Aligns address to 8 byte using dst masking method
	if (!IS_ALIGNED_DWORD(dst) && !IS_ALIGNED_QWORD(dst)) {
		unaligned_size = QWORD_BYTES - (dst % QWORD_BYTES);
		if (unaligned_size > count_left)
			unaligned_size = count_left;
		res = _write_memory_mmio_unaligned(dma_h, dst, src,
						   unaligned_size);
		if (res != FPGA_OK)
			return res;
		count_left -= unaligned_size;
		src += unaligned_size;
		dst += unaligned_size;
	}
	// Handles 8/4 byte MMIO transfer
	res = _write_memory_mmio(dma_h, &dst, &src, &count_left);
	if (res != FPGA_OK)
		return res;

	// Left over unaligned count bytes are transfered using dst masking
	// method
	unaligned_size = QWORD_BYTES - (dst % QWORD_BYTES);
	if (unaligned_size > count_left)
		unaligned_size = count_left;

	res = _write_memory_mmio_unaligned(dma_h, dst, src, unaligned_size);
	if (res != FPGA_OK)
		return res;

	count_left -= unaligned_size;

	*dst_ptr = dst + unaligned_size;
	*src_ptr = src + unaligned_size;

	return FPGA_OK;
}

/**
 * _read_memory_mmio
 *
 * @brief                   Reads a DWORD/QWORD aligned memory address(FPGA
 * address).
 * @param[in] dma           Handle to the FPGA DMA object
 * @param[in/out] dst_ptr   Pointer to the Host Buffer Address
 * @param[in/out] src_ptr   Pointer to the FPGA address
 * @param[in/out] count     Pointer to the size in bytes
 * @return fpga_result      FPGA_OK on success, return code otherwise.  Updates
 * src, dst, and count
 *
 */
static fpga_result _read_memory_mmio(fpga_dma_handle dma_h, uint64_t *src_ptr,
				     uint64_t *dst_ptr, uint64_t *count)
{
	fpga_result res = FPGA_OK;

	if (*count < DWORD_BYTES)
		return res;

	assert(*count >= DWORD_BYTES);
	assert(IS_ALIGNED_DWORD(*src_ptr));
	if (!IS_ALIGNED_DWORD(*src_ptr)) // If QWORD aligned, this will be true
		return FPGA_EXCEPTION;

	uint64_t src = *src_ptr;
	uint64_t dst = *dst_ptr;
	uint64_t align_bytes = *count;
	uint64_t offset = 0;

	if (!IS_ALIGNED_QWORD(src)) {
		// Read a single DWORD to get QWORD aligned
		_switch_to_ase_page(dma_h, src);
		offset = src & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIORead32Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				    (uint64_t)dst, DWORD_BYTES);
		ON_ERR_RETURN(res, "MMIORead32Blk");
		src += DWORD_BYTES;
		dst += DWORD_BYTES;
		align_bytes -= DWORD_BYTES;
	}

	if (0 == align_bytes)
		return res;

	assert(IS_ALIGNED_QWORD(src));

	// Read blocks of 64-bit values
	while (align_bytes >= QWORD_BYTES) {
		uint64_t left_in_page = DMA_ADDR_SPAN_EXT_WINDOW;
		left_in_page -= src & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		uint64_t size_to_copy =
			min(left_in_page, (align_bytes & ~(QWORD_BYTES - 1)));
		if (size_to_copy < QWORD_BYTES)
			break;
		_switch_to_ase_page(dma_h, src);
		offset = src & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIORead64Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				    (uint64_t)dst, size_to_copy);
		ON_ERR_RETURN(res, "MMIORead64Blk");
		src += size_to_copy;
		dst += size_to_copy;
		align_bytes -= size_to_copy;
	}

	if (align_bytes >= DWORD_BYTES) {
		// Read remaining DWORD
		_switch_to_ase_page(dma_h, src);
		offset = src & DMA_ADDR_SPAN_EXT_WINDOW_MASK;
		res = MMIORead32Blk(dma_h, ASE_DATA_BASE(dma_h) + offset,
				    (uint64_t)dst, DWORD_BYTES);
		ON_ERR_RETURN(res, "MMIORead32Blk");
		src += DWORD_BYTES;
		dst += DWORD_BYTES;
		align_bytes -= DWORD_BYTES;
	}

	assert(align_bytes < DWORD_BYTES);

	*src_ptr = src;
	*dst_ptr = dst;
	*count = align_bytes;
	return res;
}

/**
 * _ase_fpga_to_host
 *
 * @brief                   Tx "count" bytes from FPGA to HOST using Address
 * span expander(ASE)- will internally make calls to handle unaligned and
 * aligned MMIO writes.
 * @param[in] dma           Handle to the FPGA DMA object
 * @param[in/out] dst_ptr   Pointer to the Host Buffer Address
 * @param[in/out] src_ptr   Pointer to the FPGA address
 * @param[in/out] count     Size in bytes
 * @return fpga_result      FPGA_OK on success, return code otherwise.  Updates
 * src and dst
 *
 */
static fpga_result _ase_fpga_to_host(fpga_dma_handle dma_h, uint64_t *src_ptr,
				     uint64_t *dst_ptr, uint64_t count)
{
	fpga_result res = FPGA_OK;
	uint64_t src = *src_ptr;
	uint64_t dst = *dst_ptr;
	uint64_t count_left = count;
	uint64_t unaligned_size = 0;

	debug_print("dst_ptr = %08lx , count = %08lx, src = %08lx \n", *dst_ptr,
		    count, *src_ptr);

	// Aligns address to 8 byte using src masking method
	if (!IS_ALIGNED_DWORD(src) && !IS_ALIGNED_QWORD(src)) {
		unaligned_size = QWORD_BYTES - (src % QWORD_BYTES);
		if (unaligned_size > count_left)
			unaligned_size = count_left;
		res = _read_memory_mmio_unaligned(dma_h, src, dst,
						  unaligned_size);
		if (res != FPGA_OK)
			return res;
		count_left -= unaligned_size;
		dst += unaligned_size;
		src += unaligned_size;
	}
	// Handles 8/4 byte MMIO transfer
	res = _read_memory_mmio(dma_h, &src, &dst, &count_left);
	if (res != FPGA_OK)
		return res;

	// Left over unaligned count bytes are transfered using src masking
	// method
	unaligned_size = QWORD_BYTES - (src % QWORD_BYTES);
	if (unaligned_size > count_left)
		unaligned_size = count_left;

	res = _read_memory_mmio_unaligned(dma_h, src, dst, unaligned_size);
	if (res != FPGA_OK)
		return res;

	count_left -= unaligned_size;

	*dst_ptr = dst + unaligned_size;
	*src_ptr = src + unaligned_size;

	return FPGA_OK;
}

static fpga_result clear_interrupt(fpga_dma_handle dma_h)
{
	// clear interrupt by writing 1 to IRQ bit in status register
	msgdma_status_t status = {0};
	status.st.irq = 1;

	return MMIOWrite32Blk(dma_h, CSR_STATUS(dma_h), (uint64_t)&status.reg,
			      sizeof(status.reg));
}

static fpga_result poll_interrupt(fpga_dma_handle dma_h)
{
	struct pollfd pfd = {0};
	fpga_result res = FPGA_OK;
	int poll_res;

	res = fpgaGetOSObjectFromEventHandle(dma_h->eh, &pfd.fd);
	ON_ERR_GOTO(res, out, "fpgaGetOSObjectFromEventHandle failed\n");

	pfd.events = POLLIN;

#ifdef CHECK_DELAYS
	if (0 == poll(&pfd, 1, 0))
		poll_wait_count++;
#endif
	poll_res = poll(&pfd, 1, FPGA_DMA_TIMEOUT_MSEC);
	if (poll_res < 0) {
		fprintf(stderr, "Poll error errno = %s\n", strerror(errno));
		res = FPGA_EXCEPTION;
		goto out;
	} else if (poll_res == 0) {
		fprintf(stderr, "Poll(interrupt) timeout \n");
		res = FPGA_EXCEPTION;
	} else {
		uint64_t count = 0;
		ssize_t bytes_read = read(pfd.fd, &count, sizeof(count));
		if (bytes_read > 0) {
			debug_print("Poll success. Return = %d, count = %d\n",
				    poll_res, (int)count);
			res = FPGA_OK;
		} else {
			fprintf(stderr, "Error: poll failed read: %s\n",
				bytes_read > 0 ? strerror(errno)
					       : "zero bytes read");
			res = FPGA_EXCEPTION;
		}
	}

out:
	clear_interrupt(dma_h);
	return res;
}

static fpga_result _issue_magic(fpga_dma_handle dma_h)
{
	fpga_result res = FPGA_OK;
	*(dma_h->magic_buf) = 0x0ULL;

	res = _do_dma(dma_h, dma_h->magic_iova | FPGA_DMA_WF_HOST_MASK,
		      FPGA_DMA_WF_ROM_MAGIC_NO_MASK, 64, 1, FPGA_TO_HOST_MM,
		      true /*intr_en */);
	return res;
}

static void _wait_magic(fpga_dma_handle dma_h)
{
	poll_interrupt(dma_h);
	while (*(dma_h->magic_buf) != FPGA_DMA_WF_MAGIC_NO)
		;
	*(dma_h->magic_buf) = 0x0ULL;
}

fpga_result transferHostToFpga(fpga_dma_handle dma_h, uint64_t dst,
			       uint64_t src, size_t count,
			       fpga_dma_transfer_t type)
{
	fpga_result res = FPGA_OK;
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
			return res;
		} else {
			aligned_addr = ((dst / FPGA_DMA_ALIGN_BYTES) + 1)
				       * FPGA_DMA_ALIGN_BYTES;
			align_bytes = aligned_addr - dst;
			res = _ase_host_to_fpga(dma_h, &dst, &src, align_bytes);
			ON_ERR_GOTO(res, out,
				    "HOST_TO_FPGA_MM Transfer failed\n");
			count_left = count_left - align_bytes;
		}
	}
	if (count_left) {
		uint64_t dma_chunks = count_left / fpga_dma_buf_size;
		count_left -= (dma_chunks * fpga_dma_buf_size);
		debug_print("DMA TX : dma chuncks = %" PRIu64
			    ", count_left = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);

		for (i = 0; i < dma_chunks; i++) {
			// constant size transfer, no length check required for
			// memcpy
			local_memcpy(dma_h->dma_buf_ptr[i % FPGA_DMA_MAX_BUF],
				     (void *)(src + i * fpga_dma_buf_size),
				     fpga_dma_buf_size);
			if ((i % (FPGA_DMA_MAX_BUF / 2)
			     == (FPGA_DMA_MAX_BUF / 2) - 1)
			    || i == (dma_chunks - 1) /*last descriptor */) {
				if (i == (FPGA_DMA_MAX_BUF / 2) - 1) {
					res = _do_dma(
						dma_h,
						(dst + i * fpga_dma_buf_size),
						dma_h->dma_buf_iova
								[i
								 % FPGA_DMA_MAX_BUF]
							| FPGA_DMA_HOST_MASK,
						fpga_dma_buf_size, 0, type,
						true);
				} else {
					if (issued_intr)
						poll_interrupt(dma_h);
					res = _do_dma(
						dma_h,
						(dst + i * fpga_dma_buf_size),
						dma_h->dma_buf_iova
								[i
								 % FPGA_DMA_MAX_BUF]
							| FPGA_DMA_HOST_MASK,
						fpga_dma_buf_size, 0, type,
						true /*intr_en */);
				}
				issued_intr = 1;
			} else {
				res = _do_dma(
					dma_h, (dst + i * fpga_dma_buf_size),
					dma_h->dma_buf_iova[i
							    % FPGA_DMA_MAX_BUF]
						| FPGA_DMA_HOST_MASK,
					fpga_dma_buf_size, 0, type,
					false /*intr_en */);
			}
		}
		if (issued_intr) {
			poll_interrupt(dma_h);
			issued_intr = 0;
		}
		if (count_left) {
			uint64_t dma_tx_bytes =
				(count_left / FPGA_DMA_ALIGN_BYTES)
				* FPGA_DMA_ALIGN_BYTES;
			if (dma_tx_bytes != 0) {
				debug_print(
					"dma_tx_bytes = %08lx  was transfered using DMA\n",
					dma_tx_bytes);
				if (dma_tx_bytes > fpga_dma_buf_size) {
					res = FPGA_NO_MEMORY;
					ON_ERR_GOTO(res, out,
						    "Illegal transfer size\n");
				}

				local_memcpy(
					dma_h->dma_buf_ptr[0],
					(void *)(src
						 + dma_chunks
							   * fpga_dma_buf_size),
					dma_tx_bytes);
				res = _do_dma(
					dma_h,
					(dst + dma_chunks * fpga_dma_buf_size),
					dma_h->dma_buf_iova[0]
						| FPGA_DMA_HOST_MASK,
					dma_tx_bytes, 1, type,
					true /*intr_en */);
				ON_ERR_GOTO(
					res, out,
					"HOST_TO_FPGA_MM Transfer failed\n");
				poll_interrupt(dma_h);
			}
			count_left -= dma_tx_bytes;
			if (count_left) {
				dst = dst + dma_chunks * fpga_dma_buf_size
				      + dma_tx_bytes;
				src = src + dma_chunks * fpga_dma_buf_size
				      + dma_tx_bytes;
				res = _ase_host_to_fpga(dma_h, &dst, &src,
							count_left);
				ON_ERR_GOTO(
					res, out,
					"HOST_TO_FPGA_MM Transfer failed\n");
			}
		}
	}
out:
	return res;
}

fpga_result transferFpgaToHost(fpga_dma_handle dma_h, uint64_t dst,
			       uint64_t src, size_t count,
			       fpga_dma_transfer_t type)
{
	fpga_result res = FPGA_OK;
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
			return res;
		} else {
			aligned_addr = ((src / FPGA_DMA_ALIGN_BYTES) + 1)
				       * FPGA_DMA_ALIGN_BYTES;
			align_bytes = aligned_addr - src;
			res = _ase_fpga_to_host(dma_h, &src, &dst, align_bytes);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_MM Transfer failed");
			count_left = count_left - align_bytes;
		}
	}
	if (count_left) {
		uint64_t dma_chunks = count_left / fpga_dma_buf_size;
		count_left -= (dma_chunks * fpga_dma_buf_size);
		debug_print("DMA TX : dma chunks = %" PRIu64
			    ", count_left = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);
		assert(FPGA_DMA_MAX_BUF >= 8);
		uint64_t pending_buf = 0;
		for (i = 0; i < dma_chunks; i++) {
			res = _do_dma(
				dma_h,
				dma_h->dma_buf_iova[i % (FPGA_DMA_MAX_BUF)]
					| FPGA_DMA_HOST_MASK,
				(src + i * fpga_dma_buf_size),
				fpga_dma_buf_size, 1, type, false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_HOST_MM Transfer failed");

			const int num_pending = i - pending_buf + 1;
			if (num_pending
			    == (FPGA_DMA_MAX_BUF
				/ 2)) { // Enters this loop only once,after
					// first batch of descriptors.
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				wf_issued = 1;
			}
			if (num_pending > (FPGA_DMA_MAX_BUF - 1)
			    || i == (dma_chunks - 1) /*last descriptor */) {
				if (wf_issued) {
					_wait_magic(dma_h);
					for (j = 0; j < (FPGA_DMA_MAX_BUF / 2);
					     j++) {
						// constant size transfer; no
						// length check required
						local_memcpy(
							(void *)(dst
								 + pending_buf
									   * fpga_dma_buf_size),
							dma_h->dma_buf_ptr
								[pending_buf
								 % (FPGA_DMA_MAX_BUF)],
							fpga_dma_buf_size);
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
			_wait_magic(dma_h);

		// clear out final dma memcpy operations
		while (pending_buf < dma_chunks) {
			// constant size transfer; no length check required
			local_memcpy(
				(void *)(dst + pending_buf * fpga_dma_buf_size),
				dma_h->dma_buf_ptr[pending_buf
						   % (FPGA_DMA_MAX_BUF)],
				fpga_dma_buf_size);
			pending_buf++;
		}
		if (count_left > 0) {
			uint64_t dma_tx_bytes =
				(count_left / FPGA_DMA_ALIGN_BYTES)
				* FPGA_DMA_ALIGN_BYTES;
			if (dma_tx_bytes != 0) {
				debug_print(
					"dma_tx_bytes = %08lx  was transfered using DMA\n",
					dma_tx_bytes);
				res = _do_dma(
					dma_h,
					dma_h->dma_buf_iova[0]
						| FPGA_DMA_HOST_MASK,
					(src + dma_chunks * fpga_dma_buf_size),
					dma_tx_bytes, 1, type,
					false /*intr_en */);
				ON_ERR_GOTO(res, out,
					    "FPGA_TO_HOST_MM Transfer failed");
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				_wait_magic(dma_h);
				if (dma_tx_bytes > fpga_dma_buf_size) {
					res = FPGA_NO_MEMORY;
					ON_ERR_GOTO(res, out,
						    "Illegal transfer size\n");
				}
				local_memcpy(
					(void *)(dst
						 + dma_chunks
							   * fpga_dma_buf_size),
					dma_h->dma_buf_ptr[0], dma_tx_bytes);
			}
			count_left -= dma_tx_bytes;
			if (count_left) {
				dst = dst + dma_chunks * fpga_dma_buf_size
				      + dma_tx_bytes;
				src = src + dma_chunks * fpga_dma_buf_size
				      + dma_tx_bytes;
				res = _ase_fpga_to_host(dma_h, &src, &dst,
							count_left);
				ON_ERR_GOTO(res, out,
					    "FPGA_TO_HOST_MM Transfer failed");
			}
		}
	}
out:
	return res;
}

fpga_result transferFpgaToFpga(fpga_dma_handle dma_h, uint64_t dst,
			       uint64_t src, size_t count,
			       fpga_dma_transfer_t type)
{
	fpga_result res = FPGA_OK;
	uint64_t i = 0;
	uint64_t count_left = count;
	uint64_t *tmp_buf = NULL;
	if (IS_DMA_ALIGNED(dst) && IS_DMA_ALIGNED(src)
	    && IS_DMA_ALIGNED(count_left)) {
		uint64_t dma_chunks = count_left / fpga_dma_buf_size;
		count_left -= (dma_chunks * fpga_dma_buf_size);
		debug_print("!!!FPGA to FPGA!!! TX :dma chunks = %" PRIu64
			    ", count = %08lx, dst = %08lx, src = %08lx \n",
			    dma_chunks, count_left, dst, src);

		for (i = 0; i < dma_chunks; i++) {
			res = _do_dma(dma_h, (dst + i * fpga_dma_buf_size),
				      (src + i * fpga_dma_buf_size),
				      fpga_dma_buf_size, 0, type,
				      false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_FPGA_MM Transfer failed");
			if ((i + 1) % FPGA_DMA_MAX_BUF == 0
			    || i == (dma_chunks - 1) /*last descriptor */) {
				res = _issue_magic(dma_h);
				ON_ERR_GOTO(res, out,
					    "Magic number issue failed");
				_wait_magic(dma_h);
			}
		}
		if (count_left > 0) {
			debug_print(
				"Count_left = %08lx  was transfered using DMA\n",
				count_left);
			res = _do_dma(dma_h,
				      (dst + dma_chunks * fpga_dma_buf_size),
				      (src + dma_chunks * fpga_dma_buf_size),
				      count_left, 1, type, false /*intr_en */);
			ON_ERR_GOTO(res, out,
				    "FPGA_TO_FPGA_MM Transfer failed");
			res = _issue_magic(dma_h);
			ON_ERR_GOTO(res, out, "Magic number issue failed");
			_wait_magic(dma_h);
		}
	} else {
		if ((src < dst) && (src + count_left >= dst)) {
			debug_print(
				"Overlapping addresses, Provide correct dst address\n");
			return FPGA_NOT_SUPPORTED;
		}
		uint32_t tx_chunks = count_left / fpga_dma_buf_size;
		count_left -= (tx_chunks * fpga_dma_buf_size);
		debug_print(
			"!!!FPGA to FPGA TX!!! : tx chunks = %d, count = %08lx, dst = %08lx, src = %08lx \n",
			tx_chunks, count_left, dst, src);
		tmp_buf = (uint64_t *)malloc(fpga_dma_buf_size);
		for (i = 0; i < tx_chunks; i++) {
			res = transferFpgaToHost(
				dma_h, (uint64_t)tmp_buf,
				(src + i * fpga_dma_buf_size),
				fpga_dma_buf_size, FPGA_TO_HOST_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
			res = transferHostToFpga(
				dma_h, (dst + i * fpga_dma_buf_size),
				(uint64_t)tmp_buf, fpga_dma_buf_size,
				HOST_TO_FPGA_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
		}
		if (count_left > 0) {
			res = transferFpgaToHost(
				dma_h, (uint64_t)tmp_buf,
				(src + tx_chunks * fpga_dma_buf_size),
				count_left, FPGA_TO_HOST_MM);
			ON_ERR_GOTO(res, out_spl,
				    "FPGA_TO_FPGA_MM Transfer failed");
			res = transferHostToFpga(
				dma_h,
				(dst + tx_chunks * fpga_dma_buf_size),
				(uint64_t)tmp_buf, count_left, HOST_TO_FPGA_MM);
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

fpga_result fpgaDmaTransferSync(fpga_dma_handle dma_h, uint64_t dst,
				uint64_t src, size_t count,
				fpga_dma_transfer_t type)
{

	fpga_result res = FPGA_OK;

	if (!dma_h)
		return FPGA_INVALID_PARAM;

	if (type >= FPGA_MAX_TRANSFER_TYPE)
		return FPGA_INVALID_PARAM;

	if (!(type == HOST_TO_FPGA_MM || type == FPGA_TO_HOST_MM
	      || type == FPGA_TO_FPGA_MM))
		return FPGA_NOT_SUPPORTED;

	if (!dma_h->fpga_h)
		return FPGA_INVALID_PARAM;

	if (type == HOST_TO_FPGA_MM) {
		res = transferHostToFpga(dma_h, dst, src, count,
					 HOST_TO_FPGA_MM);
	} else if (type == FPGA_TO_HOST_MM) {
		res = transferFpgaToHost(dma_h, dst, src, count,
					 FPGA_TO_HOST_MM);
	} else if (type == FPGA_TO_FPGA_MM) {
		res = transferFpgaToFpga(dma_h, dst, src, count,
					 FPGA_TO_FPGA_MM);
	} else {
		return FPGA_NOT_SUPPORTED;
	}

	return res;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#define UNUSED(...) (void)(__VA_ARGS__)

fpga_result fpgaDmaTransferAsync(fpga_dma_handle dma, uint64_t dst,
				 uint64_t src, size_t count,
				 fpga_dma_transfer_t type,
				 fpga_dma_transfer_cb cb, void *context)
{
	// TODO
	UNUSED(dma, dst, src, count, type, cb, context);
	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaDmaClose(fpga_dma_handle dma_h)
{
	fpga_result res = FPGA_OK;
	int i = 0;
	int sigres;
	if (!dma_h) {
		return FPGA_INVALID_PARAM;
	}

	if (!dma_h->fpga_h) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	if (CsrControl) {
		sigres = sigaction(SIGHUP, &old_action, NULL);
		if (sigres < 0) {
			error_print(
				"Error: failed to unregister signal handler.\n");
		}
		CsrControl = NULL;
	}

	for (i = 0; i < FPGA_DMA_MAX_BUF; i++) {
		res = fpgaReleaseBuffer(dma_h->fpga_h, dma_h->dma_buf_wsid[i]);
		ON_ERR_GOTO(res, out, "fpgaReleaseBuffer failed");
	}

	res = fpgaReleaseBuffer(dma_h->fpga_h, dma_h->magic_wsid);
	ON_ERR_GOTO(res, out, "fpgaReleaseBuffer");

	fpgaUnregisterEvent(dma_h->fpga_h, FPGA_EVENT_INTERRUPT, dma_h->eh);
	fpgaDestroyEventHandle(&dma_h->eh);

	// turn off global interrupts
	msgdma_ctrl_t ctrl = {0};
	ctrl.ct.global_intr_en_mask = 0;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&ctrl.reg,
			     sizeof(ctrl.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");

out:
	// Ensure double close will fail
	dma_h->fpga_h = 0;
	free((void *)dma_h);
	return res;
}

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	(void)(info);
	(void)(unused);

	if (CsrControl == NULL) {
		return;
	}

	switch (sig) {
	case SIGHUP: {
		// Driver removed - shut down!
		*CsrControl = DMA_SHUTDOWN_CTL_VAL;
		ON_ERR_GOTO(FPGA_NO_DRIVER, out, "Got SIGHUP. Exiting.\n");
	out:
		*CsrControl = DMA_SHUTDOWN_CTL_VAL;
		usleep(1000);
		exit(-1);
	} break;
	default:
		break;
	}
}
