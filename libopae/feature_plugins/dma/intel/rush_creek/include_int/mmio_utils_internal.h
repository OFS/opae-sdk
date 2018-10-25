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
 * \mmio_utils_internal.h
 * \brief FPGA DMA BBB Internal Header
 */

#ifndef __DMA_FPGA_MMIO_UTILS_INT_H__
#define __DMA_FPGA_MMIO_UTILS_INT_H__

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
			   uint64_t host, uint64_t bytes);

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
			   uint64_t host, uint64_t bytes);

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
			  uint64_t host, uint64_t bytes);

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
			  uint64_t host, uint64_t bytes);

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
inline void _switch_to_ase_page(m2m_dma_handle_t *dma_h, uint64_t addr);

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
					uint64_t count);

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
					 uint64_t count);

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
			       uint64_t *src_ptr, uint64_t *count);

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
			      uint64_t *src_ptr, uint64_t count);

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
			      uint64_t *dst_ptr, uint64_t *count);

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
			      uint64_t *dst_ptr, uint64_t count);

#endif // __DMA_FPGA_MMIO_UTILS_INT_H__
