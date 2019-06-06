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
 * @file mmio.h
 * @brief Functions for mapping and accessing MMIO space
 *
 * Most FPGA accelerators provide access to control registers through
 * memory-mappable address spaces, commonly referred to as "MMIO spaces". This
 * file provides functions to map, unmap, read, and write MMIO spaces.
 *
 * Note that an accelerator may have multiple MMIO spaces, denoted by the
 * `mmio_num` argument of the APIs below. The meaning and properties of each
 * MMIO space are up to the accelerator designer.
 */

#ifndef __FPGA_MMIO_H__
#define __FPGA_MMIO_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write 64 bit value to MMIO space
 *
 * This function will write to MMIO space of the target object at a specified
 * offset.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[in]  offset   Byte offset into MMIO space
 * @param[in]  value    Value to write (64 bit)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaWriteMMIO64(fpga_handle handle,
			    uint32_t mmio_num, uint64_t offset,
			    uint64_t value);

/**
 * Read 64 bit value from MMIO space
 *
 * This function will read from MMIO space of the target object at a specified
 * offset.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[in]  offset   Byte offset into MMIO space
 * @param[out] value    Pointer to memory where read value is returned (64 bit)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaReadMMIO64(fpga_handle handle,
			   uint32_t mmio_num,
			   uint64_t offset, uint64_t *value);

/**
 * Write 32 bit value to MMIO space
 *
 * This function will write to MMIO space of the target object at a specified
 * offset.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[in]  offset   Byte offset into MMIO space
 * @param[in]  value    Value to write (32 bit)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaWriteMMIO32(fpga_handle handle,
			    uint32_t mmio_num, uint64_t offset,
			    uint32_t value);

/**
 * Read 32 bit value from MMIO space
 *
 * This function will read from MMIO space of the target object at a specified
 * offset.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[in]  offset   Byte offset into MMIO space
 * @param[out] value    Pointer to memory where read value is returned (32 bit)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaReadMMIO32(fpga_handle handle,
			   uint32_t mmio_num,
			   uint64_t offset, uint32_t *value);

/**
 * Write 512 bit value to MMIO space
 *
 * 512 bit MMIO writes may not be supported on all platforms.
 *
 * This function will write to MMIO space of the target object at a specified
 * offset.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[in]  offset   Byte offset into MMIO space
 * @param[in]  value    Pointer to memory holding value to write (512 bits)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaWriteMMIO512(fpga_handle handle,
			    uint32_t mmio_num, uint64_t offset,
			    void *value);

/**
 * Map MMIO space
 *
 * This function will return a pointer to the specified MMIO space of the
 * target object in process virtual memory, if supported by the target. Some
 * MMIO spaces may be restricted to privileged processes, depending on the used
 * handle and type.
 *
 * After mapping the respective MMIO space, you can access it through direct
 * pointer operations (observing supported access sizes and alignments of the
 * target platform and accelerator).
 *
 * @note Some targets (such as the ASE simulator) do not support memory-mapping
 * of IO register spaces and will not return a pointer to an actually mapped
 * space. Instead, they will return `FPGA_NOT_SUPPORTED`. Usually, these
 * platforms still allow the application to issue MMIO operations using
 * fpgaReadMMIO32(), fpgaWriteMMIO32(), fpgeReadMMIO64(), and
 * fpgaWriteMMIO64().
 *
 * If the caller passes in NULL for mmio_ptr, no mapping will be performed, and
 * no virtual address will be returned, though the call will return `FPGA_OK`.
 * This implies that all accesses will be performed through fpgaReadMMIO32(),
 * fpgaWriteMMIO32(), fpgeReadMMIO64(), and fpgaWriteMMIO64(). This is the only
 * supported case for ASE.
 *
 * The number of available MMIO spaces can be retrieved through the num_mmio
 * property (fpgaPropertyGetNumMMIO()).
 *
 * @param[in]  handle   Handle to previously opened resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @param[out] mmio_ptr Pointer to memory where a pointer to the MMIO space
 *                      will be returned. May be NULL, in which case no pointer
 *                      is returned. Returned address may be NULL if underlying
 *                      platform does not support memory mapping for register
 *                      access.
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle. FPGA_NO_ACCESS if the process'
 * permissions are not sufficient to map the requested MMIO space.
 * FPGA_NOT_SUPPORTED if platform does not support memory mapped IO.
 */
fpga_result fpgaMapMMIO(fpga_handle handle,
			uint32_t mmio_num, uint64_t **mmio_ptr);

/**
 * Unmap MMIO space
 *
 * This function will unmap a previously mapped MMIO space of the target opject,
 * rendering any pointers to it invalid.
 *
 * @note This call is only supported by hardware targets, not by ASE
 *       simulation.
 *
 * @param[in]  handle   Handle to previously opened resource
 * @param[in]  mmio_num Number of MMIO space to access
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the handle.
 */
fpga_result fpgaUnmapMMIO(fpga_handle handle,
			  uint32_t mmio_num);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_MMIO_H__
