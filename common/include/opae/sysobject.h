// Copyright(c) 2017-2018, Intel Corporation
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
 * @file sys.h
 * @brief Functions to read/write from system objects.
 * On Linux, this is used to access sysfs nodes created by the kernel driver
 */
#ifndef __SYS_H__
#define __SYS_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read a 32-bit value from an FPGA resource object as identified by a
 * string key. On Linux, this is equivalent to reading from a sysfs node with
 * 'key' as the path relative to the sysfs directory of the resource identified
 * by the token.
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[out] value Pointer to 32-bit variable to store the object's value
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 */
fpga_result fpgaReadObject32(fpga_token token, const char *key,
			     uint32_t *value);

/**
 * @brief Read a 64-bit value from an FPGA resource object as identified by a
 * string key. On Linux, this is equivalent to reading from a sysfs node with
 * 'key' as the path relative to the sysfs directory of the resource identified
 * by the token.
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[out] value Pointer to 64-bit variable to store the object's value
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 */
fpga_result fpgaReadObject64(fpga_token token, const char *key,
			     uint64_t *value);

/**
 * @brief Read a sequence of bytes from ann FPGA resource object as identified
 * by a string key. On Linux, this is equivalent to reading from a sysfs node
 * with 'key' as the path relative to the sysfs directory of the resource
 * identified by the token.
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[out] buffer Pointer to memory to store the object's bytes - a null
 * pointer indicates a request for the total number of bytes required
 * @param[in] offset Start of byte sequence to read
 * @param[out] len The number of bytes to read. This is updated with the
 * actual number of bytes read
 *
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 *
 * @note A null output buffer can be used to get the total number of bytes the
 * caller should allocate relative to the offset. A null buffer and an offset
 * of 0 will get the total number of bytes used by the object.
 */
fpga_result fpgaReadObjectBytes(fpga_token token, const char *key,
				uint8_t *buffer, size_t offset, size_t *len);

/**
 * @brief Write a 32-bit value to an FPGA resource object as identified by a
 * string key. On Linux, this is equivalent to writing to a sysfs node with
 * 'key' as the path relative to the sysfs directory of the resource identified
 * by the handle.
 *
 * @param[in] handle Handle to an open resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[in] value The 32-bit value to write to the object
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 *
 * @note Write operations require a handle to an open resource to avoid
 * possible race conditions
 */
fpga_result fpgaWriteObject32(fpga_handle handle, const char *key,
			      uint32_t value);

/**
 * @brief Write a 64-bit value from an FPGA resource object as identified by a
 * string key. On Linux, this is equivalent to writing to a sysfs node with
 * 'key' as the path relative to the sysfs directory of the resource identified
 * by the handle.
 *
 * @param[in] handle Handle to an open resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[in] value The 64-bit value to write to the object
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 *
 * @note Write operations require a handle to an open resource to avoid
 * possible race conditions
 */
fpga_result fpgaWriteObject64(fpga_handle handle, const char *key,
			      uint64_t value);

/**
 * @brief Write a sequence of bytes to an FPGA resource object as identified
 * by a string key. On Linux, this is equivalent to writing to a sysfs node
 * with 'key' as the path relative to the sysfs directory of the resource
 * identified by the handle.
 *
 * @param[in] handle Handle to an open resource (accelerator or device)
 * @param[in] key A key identifying the object with respect to the resource
 * @param[in] buffer Pointer to memory to write to the the object
 * @param[in] offset Where to start writing bytes in the object
 * @param[in] len The number of bytes to read. This is updated with the
 * actual number of bytes read
 *
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 *
 * @note Write operations require a handle to an open resource to avoid
 * possible race conditions
 */
fpga_result fpgaWriteObjectBytes(fpga_handle handle, const char *key,
				 uint8_t *buffer, size_t offset, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* !__SYS_H__ */
