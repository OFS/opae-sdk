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
#define USE_ASE
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
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

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
fpga_result MMIOWrite64Blk(handle_common *dma_h, uint64_t device,
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
		res = fpgaWriteMMIO64(dma_h->fpga_h,
				      dma_h->chan_desc->mmio_num,
				      device,
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
fpga_result MMIOWrite32Blk(handle_common *dma_h, uint64_t device,
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
		res = fpgaWriteMMIO32(dma_h->fpga_h,
				      dma_h->chan_desc->mmio_num, device,
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
fpga_result MMIORead64Blk(handle_common *dma_h, uint64_t device,
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
		res = fpgaReadMMIO64(dma_h->fpga_h,
				     dma_h->chan_desc->mmio_num,
				     device,
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
fpga_result MMIORead32Blk(handle_common *dma_h, uint64_t device,
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
		res = fpgaReadMMIO32(dma_h->fpga_h,
				     dma_h->chan_desc->mmio_num,
				     device,
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
inline void _switch_to_ase_page(m2m_dma_handle_t *dma_h, uint64_t addr)
{
	uint64_t requested_page = addr & ~DMA_ADDR_SPAN_EXT_WINDOW_MASK;

	if (requested_page != dma_h->cur_ase_page) {
		MMIOWrite64Blk(&dma_h->header, ASE_CNTL_BASE(dma_h),
			       (uint64_t)&requested_page,
			       sizeof(requested_page));
		dma_h->cur_ase_page = requested_page;
	}
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
fpga_result _read_memory_mmio_unaligned(m2m_dma_handle_t *dma_h,
					uint64_t dev_addr, uint64_t host_addr,
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
	res = MMIORead64Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
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
fpga_result _write_memory_mmio_unaligned(m2m_dma_handle_t *dma_h,
					 uint64_t dev_addr, uint64_t host_addr,
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
	res = MMIORead64Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
			    (uint64_t)&read_tmp, sizeof(read_tmp));
	if (res != FPGA_OK)
		return res;

	// overlay our data
	local_memcpy(((char *)(&read_tmp)) + shift, (void *)host_addr, count);

	// write back to device
	res = MMIOWrite64Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + dev_aligned_addr,
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
fpga_result _write_memory_mmio(m2m_dma_handle_t *dma_h, uint64_t *dst_ptr,
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
		res = MMIOWrite32Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
		res = MMIOWrite64Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
		res = MMIOWrite32Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
fpga_result _ase_host_to_fpga(m2m_dma_handle_t *dma_h, uint64_t *dst_ptr,
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
fpga_result _read_memory_mmio(m2m_dma_handle_t *dma_h, uint64_t *src_ptr,
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
		res = MMIORead32Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
		res = MMIORead64Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
		res = MMIORead32Blk(&dma_h->header, ASE_DATA_BASE(dma_h) + offset,
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
fpga_result _ase_fpga_to_host(m2m_dma_handle_t *dma_h, uint64_t *src_ptr,
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
