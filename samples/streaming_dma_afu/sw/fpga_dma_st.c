// Copyright(c) 2018, Intel Corporation
//
// Redistribution  and	use  in source	and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//	 this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//	 this list of conditions and the following disclaimer in the documentation
//	 and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//	 may be used to  endorse or promote  products derived  from this  software
//	 without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMEdesc.  IN NO EVENT	SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR	ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,	EXEMPLARY,	OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,	BUT  NOT LIMITED  TO,  PROCUREMENT	OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,	DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,	WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,	EVEN IF ADVISED OF THE
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
#include "fpga_dma_st_internal.h"
#include "fpga_dma.h"

static int err_cnt = 0;
/*
 * macro for checking return codes
 */

#define ON_ERR_GOTO(res, label, desc)\
do {\
	if ((res) != FPGA_OK) {\
		err_cnt++;\
		fprintf(stderr, "Error %s: %s\n", (desc), fpgaErrStr(res));\
		goto label;\
	}\
} while (0)

#define ON_ERR_RETURN(res, desc)\
do {\
	if ((res) != FPGA_OK) {\
		error_print("Error %s: %s\n", (desc), fpgaErrStr(res));\
		return(res);\
	}\
} while (0)

// Internal Functions
// End of feature list
static bool _fpga_dma_feature_eol(uint64_t dfh) {
	return ((dfh >> AFU_DFH_EOL_OFFSET) & 1) == 1;
}

// Feature type is BBB
static bool _fpga_dma_feature_is_bbb(uint64_t dfh) {
	// BBB is type 2
	return ((dfh >> AFU_DFH_TYPE_OFFSET) & 0xf) == FPGA_DMA_BBB;
}

// Offset to the next feature header
static uint64_t _fpga_dma_feature_next(uint64_t dfh) {
	return (dfh >> AFU_DFH_NEXT_OFFSET) & 0xffffff;
}

static void queueInit(qinfo_t *q) {
	q->read_index = q->write_index = -1;
	sem_init(&q->entries, 0, 0);
	pthread_mutex_init(&q->qmutex, NULL);
}

static void queueDestroy(qinfo_t *q) {
	q->read_index = q->write_index = -1;
	sem_destroy(&q->entries);
	pthread_mutex_destroy(&q->qmutex);
}

static bool enqueue(qinfo_t *q, fpga_dma_transfer_t tf) {
	int value=0;
	pthread_mutex_lock(&q->qmutex);
	sem_getvalue(&q->entries, &value);
	if(value == FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS) {
		pthread_mutex_unlock(&q->qmutex);
		return false;
	}
	// Increment tail index
	q->write_index = (q->write_index+1)%FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS;
	// Add the item to the Queue
	q->queue[q->write_index] = tf;
	sem_post(&q->entries);
	pthread_mutex_unlock(&q->qmutex);
	return true;
}

static void dequeue(qinfo_t *q, fpga_dma_transfer_t *tf) {
	// Wait till have an entry
	sem_wait(&q->entries);

	pthread_mutex_lock(&q->qmutex);
	q->read_index = (q->read_index+1)%FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS;
	*tf = q->queue[q->read_index];
	pthread_mutex_unlock(&q->qmutex);
}

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
	if (n >= MIN_SSE2_SIZE) // Arbitrary crossover performance point
	{
		debug_print("copying 0x%lx bytes with SSE2\n", (uint64_t) ALIGN_TO_CL(n));
		aligned_block_copy_sse2((int64_t * __restrict) dst, (int64_t * __restrict) src, ALIGN_TO_CL(n));
		ldst = (void *)((uint64_t) dst + ALIGN_TO_CL(n));
		lsrc = (void *)((uint64_t) src + ALIGN_TO_CL(n));
		n -= ALIGN_TO_CL(n);
	}

	if (n)
	{
		register unsigned long int dummy;
		debug_print("copying 0x%lx bytes with REP MOVSB\n", n);
		__asm__ __volatile__("rep movsb\n":"=&D"(ldst), "=&S"(lsrc), "=&c"(dummy):"0"(ldst), "1"(lsrc), "2"(n):"memory");
	}

	return dst;
#endif
}

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
static fpga_result MMIOWrite64Blk(fpga_dma_handle_t dma_h, uint64_t device,
				uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_QWORD(device));
	assert(IS_ALIGNED_QWORD(bytes));

	uint64_t *haddr = (uint64_t *) host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	volatile uint64_t *dev_addr = HOST_MMIO_64_ADDR(dma_h, device);
#endif

	//debug_print("copying %lld bytes from 0x%p to 0x%p\n",(long long int)bytes, haddr, (void *)device);
	for (i = 0; i < bytes / sizeof(uint64_t); i++) {
#if !defined(USE_PTR_MMIO_ACCESS) || defined(USE_ASE)
		res = fpgaWriteMMIO64(dma_h->fpga_h, dma_h->mmio_num, device, *haddr);
		ON_ERR_RETURN(res, "fpgaWriteMMIO64");
		haddr++;
		device += sizeof(uint64_t);
#elif defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
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
static fpga_result MMIOWrite32Blk(fpga_dma_handle_t dma_h, uint64_t device,
				uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_DWORD(device));
	assert(IS_ALIGNED_DWORD(bytes));

	uint32_t *haddr = (uint32_t *) host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	volatile uint32_t *dev_addr = HOST_MMIO_32_ADDR(dma_h, device);
#endif

	//debug_print("copying %lld bytes from 0x%p to 0x%p\n", (long long int)bytes, haddr, (void *)device);
	for (i = 0; i < bytes / sizeof(uint32_t); i++) {
#if !defined(USE_PTR_MMIO_ACCESS) || defined(USE_ASE)
		res = fpgaWriteMMIO32(dma_h->fpga_h, dma_h->mmio_num, device, *haddr);
		ON_ERR_RETURN(res, "fpgaWriteMMIO32");
		haddr++;
		device += sizeof(uint32_t);
#elif defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
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
static fpga_result MMIORead64Blk(fpga_dma_handle_t dma_h, uint64_t device,
					uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_QWORD(device));
	assert(IS_ALIGNED_QWORD(bytes));

	uint64_t *haddr = (uint64_t *) host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	volatile uint64_t *dev_addr = HOST_MMIO_64_ADDR(dma_h, device);
#endif

	//debug_print("copying %lld bytes from 0x%p to 0x%p\n",(long long int)bytes, (void *)device, haddr);
	for (i = 0; i < bytes / sizeof(uint64_t); i++) {
#if !defined(USE_PTR_MMIO_ACCESS) || defined(USE_ASE)
		res = fpgaReadMMIO64(dma_h->fpga_h, dma_h->mmio_num, device, haddr);
		ON_ERR_RETURN(res, "fpgaReadMMIO64");
		haddr++;
		device += sizeof(uint64_t);
#elif defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
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
static fpga_result MMIORead32Blk(fpga_dma_handle_t dma_h, uint64_t device,
				uint64_t host, uint64_t bytes)
{
	assert(IS_ALIGNED_DWORD(device));
	assert(IS_ALIGNED_DWORD(bytes));

	uint32_t *haddr = (uint32_t *) host;
	uint64_t i;
	fpga_result res = FPGA_OK;

#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	volatile uint32_t *dev_addr = HOST_MMIO_32_ADDR(dma_h, device);
#endif

	//debug_print("copying %lld bytes from 0x%p to 0x%p\n",(long long int)bytes, (void *)device, haddr);
	for (i = 0; i < bytes / sizeof(uint32_t); i++) {
#if !defined(USE_PTR_MMIO_ACCESS) || defined(USE_ASE)
		res = fpgaReadMMIO32(dma_h->fpga_h, dma_h->mmio_num, device, haddr);
		ON_ERR_RETURN(res, "fpgaReadMMIO32");
		haddr++;
		device += sizeof(uint32_t);
#elif defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
		*haddr++ = *dev_addr++;
#endif
	}
return res;
}

static fpga_result _pop_response_fifo(fpga_dma_handle_t dma_h, int *fill_level, uint32_t *tf_count, int *eop) {
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

	// Read S2M Response fill level Dispatcher register to find the no. of responses in the response FIFO
	res =	MMIORead32Blk(dma_h, CSR_RSP_FILL_LEVEL(dma_h), (uint64_t)&rsp_level.reg, sizeof(rsp_level.reg));
	ON_ERR_GOTO(res, out, "MMIORead32Blk");
	fill = rsp_level.rsp.rsp_fill_level;

	// Pop the responses to find no. of bytes trasnfered, status of transfer and to avoid deadlock of DMA
	while (fill > 0 && *eop != 1) {
		res = MMIORead32Blk(dma_h, RSP_BYTES_TRANSFERRED(dma_h), (uint64_t)&rsp_bytes, sizeof(rsp_bytes));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
		res = MMIORead32Blk(dma_h, RSP_STATUS(dma_h), (uint64_t)&rsp_status.reg, sizeof(rsp_status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
		*tf_count += rsp_bytes;
		error = rsp_status.rsp.error;
		fill--;
		if(rsp_status.rsp.eop_arrived == 1)
			*eop = 1;
		*fill_level += 1;
		debug_print("fill level = %x, *eop = %d, tf_count = %x, error = %x\n", *fill_level, *eop, *tf_count, error);
	}
out:
	return res;
}

static fpga_result _send_descriptor(fpga_dma_handle_t dma_h, msgdma_ext_desc_t *desc) {
	fpga_result res = FPGA_OK;
	msgdma_status_t status = {0};

	if(dma_h->ch_type == RX_ST) {
	debug_print("desc.rd_address = %x\n",desc->rd_address);
	debug_print("desc.wr_address = %x\n",desc->wr_address);
	debug_print("desc.len = %x\n",desc->len);
	debug_print("desc.wr_burst_count = %x\n",desc->wr_burst_count);
	debug_print("desc.rd_burst_count = %x\n",desc->rd_burst_count);
	debug_print("desc.wr_stride %x\n",desc->wr_stride);
	debug_print("desc.rd_stride %x\n",desc->rd_stride);
	debug_print("desc.rd_address_ext %x\n",desc->rd_address_ext);
	debug_print("desc.wr_address_ext %x\n",desc->wr_address_ext);

	debug_print("SGDMA_CSR_BASE = %lx SGDMA_DESC_BASE=%lx\n",dma_h->dma_csr_base, dma_h->dma_desc_base);
	}

	do {
		res = MMIORead32Blk(dma_h, CSR_STATUS(dma_h), (uint64_t)&status.reg, sizeof(status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
	} while (status.st.desc_buf_full);

	res = MMIOWrite64Blk(dma_h, dma_h->dma_desc_base, (uint64_t)desc, sizeof(*desc));
	ON_ERR_GOTO(res, out, "MMIOWrite64Blk");

out:
	return res;
}

static fpga_result _do_dma_tx(fpga_dma_handle_t dma_h, uint64_t dst, uint64_t src, int count, int is_last_desc, fpga_dma_transfer_type_t type, bool intr_en, fpga_dma_tx_ctrl_t tx_ctrl) {
	msgdma_ext_desc_t desc = {0};
	fpga_result res = FPGA_OK;

	// src, dst must be 64-byte aligned
	if(dst%FPGA_DMA_ALIGN_BYTES !=0 ||
		src%FPGA_DMA_ALIGN_BYTES !=0) {
		return FPGA_INVALID_PARAM;
	}

	// these fields are fixed for all DMA transfers
	desc.seq_num = 0;
	desc.wr_stride = 1;
	desc.rd_stride = 1;

	desc.control.go = 1;
	if(intr_en)
		desc.control.transfer_irq_en = 1;
	else
		desc.control.transfer_irq_en = 0;

	// Enable "earlyreaddone" in the control field of the descriptor except the last.
	// Setting early done causes the read logic to move to the next descriptor
	// before the previous descriptor completes.
	// This elminates a few hundred clock cycles of waiting between transfers.
	if(!is_last_desc)
		desc.control.early_done_en = 1;
	else
		desc.control.early_done_en = 0;

	if(tx_ctrl == GENERATE_EOP)
		desc.control.generate_eop = 1;
	else
		desc.control.generate_eop = 0;
	
	desc.rd_address = src & FPGA_DMA_MASK_32_BIT;
	desc.wr_address = dst & FPGA_DMA_MASK_32_BIT;
	desc.len = count;
	desc.rd_address_ext = (src >> 32) & FPGA_DMA_MASK_32_BIT;
	desc.wr_address_ext = (dst >> 32) & FPGA_DMA_MASK_32_BIT;

	res = _send_descriptor(dma_h, &desc);
	ON_ERR_GOTO(res, out, "_send_descriptor");

out:
	return res;
}

static fpga_result _do_dma_rx(fpga_dma_handle_t dma_h, uint64_t dst, uint64_t src, int count, int is_last_desc, fpga_dma_transfer_type_t type, bool intr_en, fpga_dma_rx_ctrl_t rx_ctrl) {
	msgdma_ext_desc_t desc = {0};
	fpga_result res = FPGA_OK;

	// src, dst must be 64-byte aligned
	if(dst%FPGA_DMA_ALIGN_BYTES !=0 ||
		src%FPGA_DMA_ALIGN_BYTES !=0) {
		return FPGA_INVALID_PARAM;
	}

	// these fields are fixed for all DMA transfers
	desc.seq_num = 0;
	desc.wr_stride = 1;
	desc.rd_stride = 1;

	desc.control.go = 1;
	if(intr_en) {
		desc.control.transfer_irq_en = 1;
		desc.control.wait_for_wr_rsp = 1;
	} else {
		desc.control.transfer_irq_en = 0;
		desc.control.wait_for_wr_rsp = 0;
	}

	// Enable "earlyreaddone" in the control field of the descriptor except the last.
	// Setting early done causes the read logic to move to the next descriptor
	// before the previous descriptor completes.
	// This elminates a few hundred clock cycles of waiting between transfers.
	if(!is_last_desc)
		desc.control.early_done_en = 1;
	else
		desc.control.early_done_en = 0;

	if(rx_ctrl == END_ON_EOP) {
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

	res = _send_descriptor(dma_h, &desc);
	ON_ERR_GOTO(res, out, "_send_descriptor");

out:
	return res;
}


static fpga_result clear_interrupt(fpga_dma_handle_t dma_h) {
	//clear interrupt by writing 1 to IRQ bit in status register
	msgdma_status_t status = {0};
	status.st.irq = 1;

	return MMIOWrite32Blk(dma_h, CSR_STATUS(dma_h), (uint64_t)&status.reg, sizeof(status.reg));
}

static fpga_result poll_interrupt(fpga_dma_handle_t dma_h) {
	struct pollfd pfd = {0};
	fpga_result res = FPGA_OK;

	res = fpgaGetOSObjectFromEventHandle(dma_h->eh, &pfd.fd);
	ON_ERR_GOTO(res, out, "fpgaGetOSObjectFromEventHandle failed\n");
	pfd.events = POLLIN;
	int poll_res = poll(&pfd, 1, -1);
	if(poll_res < 0) {
		fprintf( stderr, "Poll error errno = %s\n",strerror(errno));
		res = FPGA_EXCEPTION;
		goto out;
	} else if(poll_res == 0) {
		fprintf( stderr, "Poll(interrupt) timeout \n");
		res = FPGA_EXCEPTION;
	} else {
		uint64_t count = 0;
		ssize_t bytes_read = read(pfd.fd, &count, sizeof(count));
		if(bytes_read > 0) {
			debug_print( "Poll success. Return = %d, count = %d\n",poll_res, (int)count);
			res = FPGA_OK;
		} else {
			fprintf( stderr, "Error: poll failed read: %s\n", bytes_read > 0 ? strerror(errno) : "zero bytes read");
			res = FPGA_EXCEPTION;
		}
	}

out:
	clear_interrupt(dma_h);
	return res;
}

static fpga_result s2m_pending_desc_flush(fpga_dma_handle_t dma_h) {
	fpga_result res = FPGA_OK;
	msgdma_ctrl_t control = {0};
	// Flush Pending Descriptors and stop S2M DMA
	control.ct.flush_descriptors = 1;
	control.ct.stop_dispatcher = 1;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
	control.ct.flush_descriptors = 0;
	// Poll to check if S2M DMA was stopped
	msgdma_status_t status = {0};
	do {
		res = MMIORead32Blk(dma_h, CSR_STATUS(dma_h), (uint64_t)&status.reg, sizeof(status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
	} while (!status.st.stopped);
	// Flush Write master; This is to flush any descriptors that sneaked in after the initial descriptor flush
	control.ct.flush_descriptors = 0;
	control.ct.stop_dispatcher = 0;
	control.ct.flush_wr_master = 1;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
	// Re-enable global interrupts
	control.ct.global_intr_en_mask = 1;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&control.reg, sizeof(control.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");	
out:
	return res;
}


	 
static void *m2sTransactionWorker(void* dma_handle) {
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t dma_h = (fpga_dma_handle_t )dma_handle;
	uint64_t count;
	uint64_t i;

	while (1) {
		int issued_intr = 0;
		fpga_dma_transfer_t m2s_transfer;
		dequeue(&dma_h->qinfo, &m2s_transfer);
		debug_print("HOST to FPGA --- src_addr = %08lx, dst_addr = %08lx\n", m2s_transfer->src, m2s_transfer->dst);
		count = m2s_transfer->len;
		uint64_t dma_chunks = count/FPGA_DMA_BUF_SIZE;
		count -= (dma_chunks*FPGA_DMA_BUF_SIZE);
		
		for(i=0; i<dma_chunks; i++) {
			local_memcpy(dma_h->dma_buf_ptr[i%FPGA_DMA_MAX_BUF], (void*)(m2s_transfer->src+i*FPGA_DMA_BUF_SIZE), FPGA_DMA_BUF_SIZE);
			if((i%(FPGA_DMA_MAX_BUF/2) == (FPGA_DMA_MAX_BUF/2)-1) || i == (dma_chunks - 1)/*last descriptor*/) {
				if(issued_intr) {
					poll_interrupt(dma_h);
				}
				if(count == 0 && i == (dma_chunks-1) && m2s_transfer->tx_ctrl == GENERATE_EOP)
					res = _do_dma_tx(dma_h, 0, dma_h->dma_buf_iova[i%FPGA_DMA_MAX_BUF] | 0x1000000000000, FPGA_DMA_BUF_SIZE, 1, m2s_transfer->transfer_type, true/*intr_en*/, GENERATE_EOP/*tx_ctrl*/);
				else
					res = _do_dma_tx(dma_h, 0, dma_h->dma_buf_iova[i%FPGA_DMA_MAX_BUF] | 0x1000000000000, FPGA_DMA_BUF_SIZE, 1, m2s_transfer->transfer_type, true/*intr_en*/, TX_NO_PACKET/*tx_ctrl*/);
				ON_ERR_GOTO(res, out, "HOST_TO_FPGA_ST Transfer failed");
				issued_intr = 1;
			} else {
				res = _do_dma_tx(dma_h, 0, dma_h->dma_buf_iova[i%FPGA_DMA_MAX_BUF] | 0x1000000000000, FPGA_DMA_BUF_SIZE, 1, m2s_transfer->transfer_type, false/*intr_en*/, TX_NO_PACKET/*tx_ctrl*/);
				ON_ERR_GOTO(res, out, "HOST_TO_FPGA_ST Transfer failed");
			}
		}
		if(issued_intr) {
			poll_interrupt(dma_h);
			issued_intr = 0;
		}
		if(count > 0) {
			local_memcpy(dma_h->dma_buf_ptr[0], (void*)(m2s_transfer->src+dma_chunks*FPGA_DMA_BUF_SIZE), count);
			res = _do_dma_tx(dma_h, 0, dma_h->dma_buf_iova[0] | 0x1000000000000, count, 1, m2s_transfer->transfer_type, true/*intr_en*/, GENERATE_EOP/*tx_ctrl*/);
			ON_ERR_GOTO(res, out, "HOST_TO_FPGA_ST Transfer failed");
			poll_interrupt(dma_h);
		}
		// transfer_complete, if a callback was registered, invoke it
		if(m2s_transfer->cb) {
			m2s_transfer->cb(m2s_transfer->context);
		}
		
		// Mark transfer complete
		sem_post(&m2s_transfer->tf_status);
		pthread_mutex_unlock(&m2s_transfer->tf_mutex);
	}
out:
	return NULL;
}

// Streaming to Memory Dispatcher Thread
static void *s2mTransactionWorker(void* dma_handle) {
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t dma_h = (fpga_dma_handle_t )dma_handle;
	uint64_t count;
	int j;
	while (1) {
		// Head moves forward when descriptors are pushed into the dispatch queue
		uint64_t head = 0;
		// Tail moves forward when data gets copied to application buffer		
		uint64_t tail = 0;

		fpga_dma_transfer_t s2m_transfer;
		dequeue(&dma_h->qinfo, &s2m_transfer);
		//Initialize bytes received before every transfer
		s2m_transfer->rx_bytes = 0;
		debug_print("FPGA to HOST --- src_addr = %08lx, dst_addr = %08lx\n", s2m_transfer->src, s2m_transfer->dst);
		count = s2m_transfer->len;
		uint64_t dma_chunks;
		int eop_arrived = 0;
		
		//	Control streaming valve; Added to S2M write master so that when EOP arrives no more streaming data will be allowed into the DMA giving the driver time to flush out the previous descriptors
		msgdma_st_valve_ctrl_t ctrl = {0};
		// Set streaming valve to enable data flow
		ctrl.ct.en_data_flow = 1;
		if(s2m_transfer->rx_ctrl == END_ON_EOP) {
			dma_chunks = UINTMAX_MAX;
			count = 0;
			// Set to indicate transfer type. Streaming valve will stop accepting data after EOP has arrived.
			ctrl.ct.en_non_det_tf = 1;
		} else {
			dma_chunks = count/FPGA_DMA_BUF_SIZE;
			// calculate unaligned leftover bytes to be transferred
			count -= (dma_chunks*FPGA_DMA_BUF_SIZE);
			// Set to indicate transfer type.
			ctrl.ct.en_det_tf = 1;
			// Flush descriptors only while switching from Non-deterministic to deterministic tf
			if(dma_h->unused_desc_count) {
				s2m_pending_desc_flush(dma_h);
				dma_h->unused_desc_count = 0;
				dma_h->next_avail_desc_idx = 0;
			}
		}
		res = MMIOWrite32Blk(dma_h, ST_VALVE_CONTROL(dma_h), (uint64_t)&ctrl.reg, sizeof(ctrl.reg));
		ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
		
		int issued_intr = 0;
		int fill_level = 0;
		uint32_t tf_count = 0;
		debug_print("next_avail_desc_idx = %08lx , unused_desc_count = %08lx \n",dma_h->next_avail_desc_idx, dma_h->unused_desc_count);

		// At this point,the descriptor queue is either empty
		// or has one or more unused descriptors left from prior transfer(s)
		do {
			// The latter case
			_pop_response_fifo(dma_h, &fill_level, &tf_count, &eop_arrived);		
			s2m_transfer->rx_bytes += tf_count;	
			while(fill_level > 0) {
				// If the queue has unused descriptors, use them for our transfer
				local_memcpy((void*)(s2m_transfer->dst + head * FPGA_DMA_BUF_SIZE), dma_h->dma_buf_ptr[dma_h->next_avail_desc_idx], MIN(tf_count, FPGA_DMA_BUF_SIZE));
				tf_count -= MIN(tf_count, FPGA_DMA_BUF_SIZE);
				dma_h->next_avail_desc_idx = (dma_h->next_avail_desc_idx+1) % FPGA_DMA_MAX_BUF;
				dma_h->unused_desc_count--;
				head++;
				fill_level--;
			}

			// case a: We haven't used up all descriptors in the queue, but EOP arrived early
			if(eop_arrived)
				break;

			// case b: We haven't used up all descriptors in the queue, but current transfer ended early
			if(head == dma_chunks)
				break;

			// case c: We have used up all descriptors in the queue
			if(dma_h->unused_desc_count <= 0) {
				dma_h->next_avail_desc_idx = 0;
				break;
			}

		} while(1);
		if(eop_arrived)
			goto out_transf_complete;

		tail = head;
		
		while(head < dma_chunks) {
			// Total transfers in flight = head-tail+1
			int cur_num_pending = head-tail+1;

			if(cur_num_pending == (FPGA_DMA_MAX_BUF/2)) {
				res = _do_dma_rx(dma_h, dma_h->dma_buf_iova[head%(FPGA_DMA_MAX_BUF)] | 0x1000000000000, 0, FPGA_DMA_BUF_SIZE, 1, s2m_transfer->transfer_type, true/*intr_en*/, s2m_transfer->rx_ctrl/*rx_ctrl*/);
				ON_ERR_GOTO(res, out, "FPGA_ST_TO_HOST_MM Transfer failed");
				issued_intr = 1;
			} else if(cur_num_pending > (FPGA_DMA_MAX_BUF-1) || head == (dma_chunks - 1)/*last descriptor*/) {
				if(issued_intr) {
					poll_interrupt(dma_h);
					_pop_response_fifo(dma_h, &fill_level, &tf_count, &eop_arrived);
					s2m_transfer->rx_bytes += tf_count;
					for(j=0; j<fill_level; j++) {
						local_memcpy((void*)(s2m_transfer->dst + tail * FPGA_DMA_BUF_SIZE), dma_h->dma_buf_ptr[tail % (FPGA_DMA_MAX_BUF)], MIN(tf_count, FPGA_DMA_BUF_SIZE));
						tf_count -= MIN(tf_count, FPGA_DMA_BUF_SIZE);
						// Increment tail when we memcpy data back to user-buffer
						tail++;
					}
					issued_intr = 0;
					if(eop_arrived == 1) {
						dma_h->next_avail_desc_idx = tail % FPGA_DMA_MAX_BUF;
						dma_h->unused_desc_count = head - tail;	
						break;
					}
				}
				res = _do_dma_rx(dma_h, dma_h->dma_buf_iova[head % (FPGA_DMA_MAX_BUF)] | 0x1000000000000, 0, FPGA_DMA_BUF_SIZE, 1, s2m_transfer->transfer_type, true/*intr_en*/, s2m_transfer->rx_ctrl/*rx_ctrl*/);
				ON_ERR_GOTO(res, out, "FPGA_ST_TO_HOST_MM Transfer failed");
				issued_intr = 1;
			} else {
				res = _do_dma_rx(dma_h, dma_h->dma_buf_iova[head % (FPGA_DMA_MAX_BUF)] | 0x1000000000000, 0, FPGA_DMA_BUF_SIZE, 1, s2m_transfer->transfer_type, false/*intr_en*/, s2m_transfer->rx_ctrl/*rx_ctrl*/);
				ON_ERR_GOTO(res, out, "FPGA_ST_TO_HOST_MM Transfer failed");
			}

			// Increment head when the descriptor is issued into the dispatcher queue
			head++;
		}

		if(eop_arrived)
			goto out_transf_complete;
		if(s2m_transfer->rx_ctrl != END_ON_EOP) {
			if(issued_intr) {
				poll_interrupt(dma_h);
				do {
					_pop_response_fifo(dma_h, &fill_level, &tf_count, &eop_arrived);
					s2m_transfer->rx_bytes += tf_count;
					// clear out final dma local_memcpy operations
					while(fill_level > 0) {
						// constant size transfer; no length check required
						local_memcpy((void*)(s2m_transfer->dst + tail * FPGA_DMA_BUF_SIZE), dma_h->dma_buf_ptr[tail % (FPGA_DMA_MAX_BUF)], MIN(tf_count,FPGA_DMA_BUF_SIZE));
						tail++;
						fill_level -=1;
						tf_count -= MIN(tf_count, FPGA_DMA_BUF_SIZE);
					}
				} while(tail < dma_chunks);
			}
			if(count > 0) {
				res = _do_dma_rx(dma_h, dma_h->dma_buf_iova[0] | 0x1000000000000, 0, count, 1, s2m_transfer->transfer_type, true/*intr_en*/, s2m_transfer->rx_ctrl/*rx_ctrl*/);
				ON_ERR_GOTO(res, out, "FPGA_TO_HOST_ST Transfer failed");
				poll_interrupt(dma_h);
				do {
					_pop_response_fifo(dma_h, &fill_level, &tf_count, &eop_arrived);
					s2m_transfer->rx_bytes += tf_count;
					if(fill_level > 0) {
						local_memcpy((void*)(s2m_transfer->dst+dma_chunks*FPGA_DMA_BUF_SIZE), dma_h->dma_buf_ptr[0], tf_count);
						count -= tf_count;
					}
				} while(count != 0);
			}
		}	
	out_transf_complete:
		//transfer complete
		if(s2m_transfer->cb) {
			s2m_transfer->cb(s2m_transfer->context);
		}
		sem_post(&s2m_transfer->tf_status);
		pthread_mutex_unlock(&s2m_transfer->tf_mutex);
	}
out:
	return NULL;
}

// Public APIs
fpga_result fpgaCountDMAChannels(fpga_handle fpga, size_t *count) {
	// Discover total# DMA channels by traversing the device feature list
	// We may encounter one or more BBBs during discovery
	// Populate the count
	fpga_result res = FPGA_OK;

	if(!fpga) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		return FPGA_INVALID_PARAM;
	}

	if(!count) {
		FPGA_DMA_ST_ERR("Invalid pointer to count");
		return FPGA_INVALID_PARAM;
	}

	uint64_t offset = 0;
#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	uint64_t mmio_va;

	res = fpgaMapMMIO(fpga, 0, (uint64_t **)&mmio_va);
	ON_ERR_GOTO(res, out, "fpgaMapMMIO");
#endif
	// Discover DMA BBB channels by traversing the device feature list
	bool end_of_list = false;
	uint64_t dfh = 0;
	do {
		uint64_t feature_uuid_lo, feature_uuid_hi;
#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
		// Read the next feature header
		dfh = *((volatile uint64_t *)((uint64_t)mmio_va + (uint64_t)(offset)));

		// Read the current feature's UUID
		feature_uuid_lo = *((volatile uint64_t *)((uint64_t)mmio_va + (uint64_t)(offset + 8)));
		feature_uuid_hi = *((volatile uint64_t *)((uint64_t)mmio_va + (uint64_t)(offset + 16)));
#else
		uint32_t mmio_no = 0;
		// Read the next feature header
		res = fpgaReadMMIO64(fpga, mmio_no, offset, &dfh);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");

		// Read the current feature's UUID
		res = fpgaReadMMIO64(fpga, mmio_no, offset + 8, &feature_uuid_lo);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");

		res = fpgaReadMMIO64(fpga, mmio_no, offset + 16, &feature_uuid_hi);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");
#endif
		if (_fpga_dma_feature_is_bbb(dfh) &&
			(((feature_uuid_lo == M2S_DMA_UUID_L) && (feature_uuid_hi == M2S_DMA_UUID_H)) ||
			((feature_uuid_lo == S2M_DMA_UUID_L) && (feature_uuid_hi == S2M_DMA_UUID_H)))) {
			// Found one. Record it.
			*count = *count+1;
		}

		// End of the list?
		end_of_list = _fpga_dma_feature_eol(dfh);
		// Move to the next feature header
		offset = offset + _fpga_dma_feature_next(dfh);
	} while(!end_of_list);

out:
	return res;
}

fpga_result fpgaDMAOpen(fpga_handle fpga, uint64_t dma_channel_index, fpga_dma_handle_t *dma) {
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t dma_h;
	int channel_index = 0;
	int i = 0;

	if(!fpga) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		return FPGA_INVALID_PARAM;
	}

	if(!dma) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA handle");
		return FPGA_INVALID_PARAM;
	}

	// init the dma handle
	dma_h = (fpga_dma_handle_t)malloc(sizeof(struct fpga_dma_handle));
	if(!dma_h) {
		return FPGA_NO_MEMORY;
	}
	dma_h->fpga_h = fpga;
	for(i=0; i < FPGA_DMA_MAX_BUF; i++)
		dma_h->dma_buf_ptr[i] = NULL;
	dma_h->mmio_num = 0;
	dma_h->mmio_offset = 0;
	queueInit(&dma_h->qinfo);

	bool end_of_list = false;
	bool dma_found = false;

#if defined(USE_PTR_MMIO_ACCESS) && !defined(USE_ASE)
	res = fpgaMapMMIO(dma_h->fpga_h, 0, (uint64_t **)&dma_h->mmio_va);
	ON_ERR_GOTO(res, out, "fpgaMapMMIO");
#endif

	dfh_feature_t dfh;
	uint64_t offset = dma_h->mmio_offset;
	do {
		// Read the next feature header
		res = MMIORead64Blk(dma_h, offset, (uint64_t)&dfh, sizeof(dfh));
		ON_ERR_GOTO(res, out, "MMIORead64Blk");
		
		if (_fpga_dma_feature_is_bbb(dfh.dfh) &&
			(((dfh.feature_uuid_lo == M2S_DMA_UUID_L) && (dfh.feature_uuid_hi == M2S_DMA_UUID_H)) ||
			((dfh.feature_uuid_lo == S2M_DMA_UUID_L) && (dfh.feature_uuid_hi == S2M_DMA_UUID_H))) ) {

			// Found one. Record it.
			if(channel_index == dma_channel_index) {
				dma_h->dma_base = offset;
				if ((dfh.feature_uuid_lo == M2S_DMA_UUID_L) && (dfh.feature_uuid_hi == M2S_DMA_UUID_H))
					dma_h->ch_type=TX_ST;
				else if ((dfh.feature_uuid_lo == S2M_DMA_UUID_L) && (dfh.feature_uuid_hi == S2M_DMA_UUID_H)) {
					dma_h->ch_type=RX_ST;
					dma_h->dma_rsp_base = dma_h->dma_base+FPGA_DMA_RESPONSE;
					dma_h->dma_streaming_valve_base = dma_h->dma_base+FPGA_DMA_STREAMING_VALVE;
				}
				dma_h->dma_csr_base = dma_h->dma_base+FPGA_DMA_CSR;
				dma_h->dma_desc_base = dma_h->dma_base+FPGA_DMA_DESC;
				dma_found = true;
				dma_h->dma_channel = dma_channel_index;
				debug_print("DMA Base Addr = %08lx\n", dma_h->dma_base);
				break;
			} else {
				channel_index += 1;
			}
		}

		// End of the list?
		end_of_list = _fpga_dma_feature_eol(dfh.dfh);
		// Move to the next feature header
		offset = offset + _fpga_dma_feature_next(dfh.dfh);
	} while(!end_of_list);

	dma_h->next_avail_desc_idx = 0;
	dma_h->unused_desc_count = 0;
	if(dma_found) {
		*dma = dma_h;
		res = FPGA_OK;
	}
	else {
		*dma = NULL;
		res = FPGA_NOT_FOUND;
		ON_ERR_GOTO(res, out, "DMA not found");
	}

	// Buffer size must be page aligned for prepareBuffer
	for(i=0; i< FPGA_DMA_MAX_BUF; i++) {
		res = fpgaPrepareBuffer(dma_h->fpga_h, FPGA_DMA_BUF_SIZE, (void **)&(dma_h->dma_buf_ptr[i]), &dma_h->dma_buf_wsid[i], 0);
		ON_ERR_GOTO(res, out, "fpgaPrepareBuffer");

		res = fpgaGetIOAddress(dma_h->fpga_h, dma_h->dma_buf_wsid[i], &dma_h->dma_buf_iova[i]);
		ON_ERR_GOTO(res, rel_buf, "fpgaGetIOAddress");
	}

	if(dma_h->ch_type == TX_ST) {
		if(pthread_create(&dma_h->thread_id, NULL, m2sTransactionWorker, (void*)dma_h) != 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, rel_buf, "pthread_create");
		}
	} else if(dma_h->ch_type == RX_ST) {
		if(pthread_create(&dma_h->thread_id, NULL, s2mTransactionWorker, (void*)dma_h) != 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, rel_buf, "pthread_create");
		}
	}

	// turn on global interrupts
	msgdma_ctrl_t ctrl = {0};
	ctrl.ct.global_intr_en_mask = 1;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&ctrl.reg, sizeof(ctrl.reg));
	ON_ERR_GOTO(res, rel_buf, "MMIOWrite32Blk");

	// register interrupt event handle
	res = fpgaCreateEventHandle(&dma_h->eh);
	ON_ERR_GOTO(res, rel_buf, "fpgaCreateEventHandle");
	res = fpgaRegisterEvent(dma_h->fpga_h, FPGA_EVENT_INTERRUPT, dma_h->eh, dma_h->dma_channel/*vector id*/);
	ON_ERR_GOTO(res, destroy_eh, "fpgaRegisterEvent");
	return FPGA_OK;

destroy_eh:
	res = fpgaDestroyEventHandle(&dma_h->eh);
	ON_ERR_GOTO(res, rel_buf, "fpgaRegisterEvent");

rel_buf:
	for(i=0; i< FPGA_DMA_MAX_BUF; i++) {
		res = fpgaReleaseBuffer(dma_h->fpga_h, dma_h->dma_buf_wsid[i]);
		ON_ERR_GOTO(res, out, "fpgaReleaseBuffer");
	}
out:
	queueDestroy(&dma_h->qinfo);
	if(!dma_found)
		free(dma_h);
	return res;
}

fpga_result fpgaDMAClose(fpga_dma_handle_t dma_h) {
	fpga_result res = FPGA_OK;
	int i = 0;
	if(!dma_h) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if(!dma_h->fpga_h) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	for(i=0; i<FPGA_DMA_MAX_BUF; i++) {
		res = fpgaReleaseBuffer(dma_h->fpga_h, dma_h->dma_buf_wsid[i]);
		ON_ERR_GOTO(res, out, "fpgaReleaseBuffer failed");
	}

	fpgaUnregisterEvent(dma_h->fpga_h, FPGA_EVENT_INTERRUPT, dma_h->eh);
	fpgaDestroyEventHandle(&dma_h->eh);

	// turn off global interrupts
	msgdma_ctrl_t ctrl = {0};
	ctrl.ct.global_intr_en_mask = 0;
	res = MMIOWrite32Blk(dma_h, CSR_CONTROL(dma_h), (uint64_t)&ctrl.reg, sizeof(ctrl.reg));
	ON_ERR_GOTO(res, out, "MMIOWrite32Blk");
	queueDestroy(&dma_h->qinfo);
	if(pthread_cancel(dma_h->thread_id) != 0) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, out, "pthread_cancel");
	}

out:
	free((void*)dma_h);
	return res;
}

fpga_result fpgaGetDMAChannelType(fpga_dma_handle_t dma, fpga_dma_channel_type_t *ch_type) {
	if(!dma) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if(!ch_type) {
		FPGA_DMA_ST_ERR("Invalid pointer to channel type");
		return FPGA_INVALID_PARAM;
	}

	*ch_type = dma->ch_type;
	return FPGA_OK;
}

fpga_result fpgaDMATransferInit(fpga_dma_transfer_t *transfer) {
	fpga_result res = FPGA_OK;
	fpga_dma_transfer_t tmp; 
	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	tmp = (fpga_dma_transfer_t)malloc(sizeof(struct fpga_dma_transfer));
	if(!tmp) {
		res = FPGA_NO_MEMORY;
		return res;
	}

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
	pthread_mutex_init(&(tmp)->tf_mutex, NULL);
	sem_init(&(tmp)->tf_status, 0, TRANSFER_NOT_IN_PROGRESS);
	*transfer = tmp;
	return res;
}

fpga_result fpgaDMATransferDestroy(fpga_dma_transfer_t transfer) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	sem_destroy(&transfer->tf_status);
	pthread_mutex_destroy(&transfer->tf_mutex);
	free(transfer);
	
	return res;
}

fpga_result fpgaDMATransferSetSrc(fpga_dma_transfer_t transfer, uint64_t src) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->src = src;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetDst(fpga_dma_transfer_t transfer, uint64_t dst) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->dst = dst;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetLen(fpga_dma_transfer_t transfer, uint64_t len) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->len = len;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetTransferType(fpga_dma_transfer_t transfer, fpga_dma_transfer_type_t type) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if(type >= FPGA_MAX_TRANSFER_TYPE) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer type");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->transfer_type = type;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetTxControl(fpga_dma_transfer_t transfer, fpga_dma_tx_ctrl_t tx_ctrl) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if(tx_ctrl >= FPGA_MAX_TX_CTRL) {
		FPGA_DMA_ST_ERR("Invalid TX Ctrl");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->tx_ctrl = tx_ctrl;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetRxControl(fpga_dma_transfer_t transfer, fpga_dma_rx_ctrl_t rx_ctrl) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if(rx_ctrl >= FPGA_MAX_RX_CTRL) {
		FPGA_DMA_ST_ERR("Invalid RX Ctrl");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->rx_ctrl = rx_ctrl;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferSetTransferCallback(fpga_dma_transfer_t transfer, fpga_dma_transfer_cb cb, void *ctxt) {
	fpga_result res = FPGA_OK;

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_lock(&transfer->tf_mutex);
	transfer->cb = cb;
	transfer->context = ctxt;
	pthread_mutex_unlock(&transfer->tf_mutex);
	return res;
}

fpga_result fpgaDMATransferGetBytesTransferred(fpga_dma_transfer_t transfer, size_t *rx_bytes) {

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if(!rx_bytes) {
		FPGA_DMA_ST_ERR("Invalid pointer to transferred bytes");
		return FPGA_INVALID_PARAM;
	}
	*rx_bytes = transfer->rx_bytes;
	return FPGA_OK;
}


fpga_result fpgaDMATransfer(fpga_dma_handle_t dma, fpga_dma_transfer_t transfer) {
	fpga_result res = FPGA_OK;
	bool ret;

	if(!dma) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if(!transfer) {
		FPGA_DMA_ST_ERR("Invalid DMA transfer");
		return FPGA_INVALID_PARAM;
	}

	if(!(transfer->transfer_type == HOST_MM_TO_FPGA_ST || transfer->transfer_type == FPGA_ST_TO_HOST_MM)) {
		FPGA_DMA_ST_ERR("Transfer unsupported");
		return FPGA_NOT_SUPPORTED;
	}

	if(dma->ch_type == RX_ST && transfer->transfer_type == HOST_MM_TO_FPGA_ST) {
		FPGA_DMA_ST_ERR("Incompatible transfer on stream to memory DMA channel");
		return FPGA_INVALID_PARAM;
	}

	if(dma->ch_type == TX_ST && transfer->transfer_type == FPGA_ST_TO_HOST_MM) {
		FPGA_DMA_ST_ERR("Incompatible transfer on memory to stream DMA channel");
		return FPGA_INVALID_PARAM;
	}

	// Lock transfer in preparation for transfer
	// Mutex will be unlocked from the worker thread once transfer is complete
	pthread_mutex_lock(&transfer->tf_mutex);

	// Transfer in progress
	sem_wait(&transfer->tf_status);

	// Enqueue the transfer, transfer will be processed in worker thread
	do {
		ret = enqueue(&dma->qinfo, transfer);
	} while(ret != true);
	// Blocking transfer
	if(!transfer->cb) {
		sem_wait(&transfer->tf_status);
		sem_post(&transfer->tf_status);
	}

	return res;
}
