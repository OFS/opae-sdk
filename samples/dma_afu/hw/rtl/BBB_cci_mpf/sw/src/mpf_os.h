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
 * \file mpf_os.h
 * \brief OS-specific functions required by other MPF modules.
 */

#ifndef __FPGA_MPF_OS_H__
#define __FPGA_MPF_OS_H__

/**
 * Memory barrier.
 *
 */
void mpfOsMemoryBarrier(void);


/**
 * Anonymous mutex type.
 */
typedef void* mpf_os_mutex_handle;


/**
 * Allocate and initialize a mutex object.
 *
 * @param[out] mutex       Mutex object.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsPrepareMutex(
    mpf_os_mutex_handle* mutex
);


/**
 * Release a mutex object.
 *
 * @param[in] mutex        Mutex object.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsReleaseMutex(
    mpf_os_mutex_handle mutex
);


/**
 * Lock a mutex.
 *
 * @param[in] mutex        Pointer to mutex object.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsLockMutex(
    mpf_os_mutex_handle mutex
);


/**
 * Unlock a mutex.
 *
 * @param[in] mutex        Pointer to mutex object.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsUnlockMutex(
    mpf_os_mutex_handle mutex
);


/**
 * Map a memory buffer.
 *
 * @param[in]  num_bytes   Number of bytes to map.
 * @param[inout] page_size Physical page size requested.  On return the page_size
 *                         is set to the actual size used.  If big pages are
 *                         unavailable, smaller physical pages may be permitted.
 * @param[out] buffer      Address of the allocated buffer
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsMapMemory(
    size_t num_bytes,
    mpf_vtp_page_size* page_size,
    void** buffer
);


/**
 * Unmap a memory buffer.
 *
 * @param[in]  buffer      Buffer to release.
 * @param[in]  num_bytes   Number of bytes to map.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfOsUnmapMemory(
    void* buffer,
    size_t num_bytes
);

#endif // __FPGA_MPF_OS_H__
