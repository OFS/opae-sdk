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
 * @file sysobject.h
 * @brief Functions to read/write from system objects.
 * On Linux, this is used to access sysfs nodes created by the kernel driver
 */
#ifndef __FPGA_SYSOBJECT_H__
#define __FPGA_SYSOBJECT_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an `fpga_object` data structures. An `fpga_object`
 * is a handle to an FPGA resource which can be an attribute or register.
 * This object is read-only.
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[in] name A key identifying an object belonging to a resource.
 * @param[out] object Pointer to memory to store the object in
 * @param[in] flags Control behavior of object creation
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameter is invalid. FPGA_NOT_SUPPORTED if this function is not supported
 * by the current implementation of this API.
 *
 */
fpga_result fpgaGetTokenObject(fpga_token token, const char *name,
			       fpga_object *object, int flags);


/**
 * @brief Free memory used for the fpga_object data structure
 *
 * @param obj Pointer to the fpga_object instance to destroy
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if the object is NULL,
 * FPGA_EXCEPTION if an internal error is encountered.
 */
fpga_result fpgaDestroyObject(fpga_object *obj);

/**
 * @brief Read bytes from an FPGA object
 *
 * @param [in]obj An fpga_object instance.
 * @param [out]buffer Pointer to a buffer to read bytes into.
 * @param [in]offset Byte offset relative to objects internal buffer where to
 * begin reading bytes from.
 * @param [in]len The length, in bytes, to read from the object.
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid
 */
fpga_result fpgaObjectRead(fpga_object obj, uint8_t *buffer, size_t offset,
			   size_t len, int flags);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* !__FPGA_SYSOBJECT_H__ */
