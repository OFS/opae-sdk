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
 * @file buffer.h
 * @brief Functions for allocating and sharing system memory with an FPGA
 * accelerator
 *
 * To share memory between a software application and an FPGA accelerator,
 * these functions set up system components (e.g. an IOMMU) to allow
 * accelerator access to a provided memory region.
 *
 * There are a number of restrictions on what memory can be shared, depending
 * on platform capabilities. Usually, FPGA accelerators to not have access to
 * virtual address mappings of the CPU, so they can only access physical
 * addresses. To support this, the OPAE C library on Linux uses hugepages to
 * allocate large, contiguous pages of physical memory that can be shared with
 * an accelerator. It also supports sharing memory that has already been
 * allocated by an application, as long as that memory satisfies the
 * requirements of being physically contigous and page-aligned.
 */

#ifndef __FPGA_BUFFER_H__
#define __FPGA_BUFFER_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepare a shared memory buffer
 *
 * Prepares a memory buffer for shared access between an accelerator and the calling
 * process. This may either include allocation of physical memory, or
 * preparation of already allocated memory for sharing. The latter case is
 * indicated by supplying the FPGA_BUF_PREALLOCATED flag.
 *
 * This function will ask the driver to pin the indicated memory (make it
 * non-swappable), and program the IOMMU to allow access from the accelerator. If the
 * buffer was not pre-allocated (flag FPGA_BUF_PREALLOCATED), the function
 * will also allocate physical memory of the requested size and map the
 * memory into the caller's process' virtual address space. It returns in
 * 'wsid' an fpga_buffer object that can be used to program address registers
 * in the accelerator for shared access to the memory.
 *
 * When using FPGA_BUF_PREALLOCATED, the input len must be a non-zero multiple
 * of the page size, else the function returns FPGA_INVALID_PARAM. When not
 * using FPGA_BUF_PREALLOCATED, the input len is rounded up to the nearest
 * multiple of page size.
 *
 * @param[in]  handle     Handle to previously opened accelerator resource
 * @param[in]  len        Length of the buffer to allocate/prepare in bytes
 * @param[inout] buf_addr Virtual address of buffer. Contents may be NULL (OS
 *                        will choose mapping) or non-NULL (OS will take
 *                        contents as a hint for the virtual address).
 * @param[out] wsid       Handle to the allocated/prepared buffer to be used
 *                        with other functions
 * @param[in]  flags      Flags. FPGA_BUF_PREALLOCATED indicates that memory
 *                        pointed at in '*buf_addr' is already allocated an
 *                        mapped into virtual memory. FPGA_BUF_READ_ONLY
 *                        pins pages with only read access from the FPGA.
 * @returns FPGA_OK on success. FPGA_NO_MEMORY if the requested memory could
 * not be allocated. FPGA_INVALID_PARAM if invalid parameters were provided, or
 * if the parameter combination is not valid. FPGA_EXCEPTION if an internal
 * exception occurred while trying to access the handle.
 *
 * @note As a special case, when FPGA_BUF_PREALLOCATED is present in flags,
 * if len == 0 and buf_addr == NULL, then the function returns FPGA_OK if
 * pre-allocated buffers are supported. In this case, a return value other
 * than FPGA_OK indicates that pre-allocated buffers are not supported.
 */
fpga_result fpgaPrepareBuffer(fpga_handle handle,
			      uint64_t len,
			      void **buf_addr, uint64_t *wsid, int flags);

/**
 * Release a shared memory buffer
 *
 * Releases a previously prepared shared buffer. If the buffer was allocated
 * using fpgaPrepareBuffer (FPGA_BUF_PREALLOCATED was not specified), this call
 * will deallocate/free that memory. Otherwise, it will only be returned to
 * it's previous state (pinned/unpinned, cached/non-cached).
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  wsid     Handle to the allocated/prepared buffer
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if invalid parameters were
 * provided, or if the parameter combination is not valid. FPGA_EXCEPTION if an
 * internal exception occurred while trying to access the handle.
 */
fpga_result fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid);

/**
 * Retrieve base IO address for buffer
 *
 * This function is used to acquire the physical base address (on some platforms
 * called IO Virtual Address or IOVA) for a shared buffer identified by wsid.
 *
 * @note This function will disappear once the APIs for secure sharing of
 * buffer addresses is implemented.
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  wsid     Buffer handle / workspace ID referring to the buffer for
 *                      which the IO address is requested
 * @param[out] ioaddr   Pointer to memory where the IO address will be returned
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if invalid parameters were
 * provided, or if the parameter combination is not valid. FPGA_EXCEPTION if an
 * internal exception occurred while trying to access the handle.
 * FPGA_NOT_FOUND if `wsid` does not refer to a previously shared buffer.
 */
fpga_result fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
			     uint64_t *ioaddr);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_BUFFER_H__
