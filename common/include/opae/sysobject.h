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
 * @file opae/sysobject.h
 * @brief Functions to read/write from system objects.
 * On Linux systems with the OPAE kernel driver, this is used to access sysfs
 * nodes created by the driver.
 */
#ifndef __FPGA_SYSOBJECT_H__
#define __FPGA_SYSOBJECT_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an `fpga_object` data structures. An `fpga_object`
 * is a handle to an FPGA resource which can be an attribute, register or
 * a container. This object is read-only.
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[in] name A key identifying an object belonging to a resource.
 * @param[out] object Pointer to memory to store the object in
 * @param[in] flags Control behavior of object identification and creation.
 * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
 * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
 * created for objects one level down from the object identified by name.
 * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all objects
 * below the current object identified by name.
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_NOT_FOUND if an object cannot be found with the
 * given key. FPGA_NOT_SUPPORTED if this function is not supported by the
 * current implementation of this API.
 *
 * @note Names that begin with '.' or '/' or contain '..' are not allowed and
 * result in FPGA_INVALID_PARAM being returned
 *
 */
fpga_result fpgaTokenGetObject(fpga_token token, const char *name,
			       fpga_object *object, int flags);

/**
 * @brief Create an `fpga_object` data structure from a handle.
 * An `fpga_object` is a handle to an FPGA resource which can be an attribute,
 * register, or container.  This object has read/write access..
 *
 * @param[in] handle Handle identifying a resource (accelerator or device)
 * @param[in] name A key identifying an object belonging to a resource.
 * @param[out] object Pointer to memory to store the object in
 * @param[in] flags Control behavior of object identification and creation
 * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
 * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
 * created for objects one level down from the object identified by name.
 * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all objects
 * below the current object identified by name.
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_NOT_FOUND if an object cannot be found with the
 * given key. FPGA_NOT_SUPPORTED if this function is not supported by the
 * current implementation of this API.
 *
 * @note Names that begin with '.' or '/' or contain '..' are not allowed and
 * result in FPGA_INVALID_PARAM being returned
 *
 */
fpga_result fpgaHandleGetObject(fpga_handle handle, const char *name,
				fpga_object *object, int flags);

/**
 * @brief Create an `fpga_object` data structure from a parent object.  An
 * `fpga_object` is a handle to an FPGA resource which can be an attribute,
 * register, or container.  If the parent object was created with a handle,
 * then the new object will inherit the handle allowing it to have read-write
 * access to the object data.
 *
 * @param[in] parent A parent container `fpga_object`.
 * @param[in] name A key identifying a sub-object of the parent container.
 * @param[out] object Pointer to memory to store the object in.
 * @param[in] flags Control behavior of object identification and creation.
 * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
 * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
 * created for objects one level down from the object identified by name.
 * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all objects
 * below the current object identified by name.
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid - this includes a parent object that is not a
 * container object. FPGA_NOT_FOUND if an object cannot be found with the given
 * key. FPGA_NOT_SUPPORTED if this function is not supported by the current
 * implementation of this API.
 *
 * @note Names that begin with '.' or '/' or contain '..' are not allowed and
 * result in FPGA_INVALID_PARAM being returned
 *
 */
fpga_result fpgaObjectGetObject(fpga_object parent, const char *name,
				fpga_object *object, int flags);

/**
 * @brief Create an `fpga_object` data structure from a parent object using a
 * given index.  An `fpga_object` is a handle to an FPGA resource which can be
 * an attribute, register, or container.  If the parent object was created with
 * a handle, then the new object will inherit the handle allowing it to have
 * read-write access to the object data.
 *
 * @param[in] parent A parent container 'fpga_object'
 * @param[in] idx A positive index less than the size reported by the parent.
 * @param[out] object Pointer to memory to store the object in.
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid - this includes a parent object that is not a
 * container object. FPGA_NOT_FOUND if an object cannot be found with the given
 * key. FPGA_NOT_SUPPORTED if this function is not supported by the current
 * implementation of this API.
 */
fpga_result fpgaObjectGetObjectAt(fpga_object parent, size_t idx,
				  fpga_object *object);
/**
 * @brief Get the sysobject type (container or attribute)
 *
 * @param[in] obj An fpga_object instance
 * @param[out] type The type of object (FPGA_OBJECT_CONTAINER or
 * FPGA_OBJECT_ATTRIBUTE)
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters are null or invalid
 */
fpga_result fpgaObjectGetType(fpga_object obj, enum fpga_sysobject_type *type);

/**
 * @brief Free memory used for the fpga_object data structure
 *
 * @note fpgaDestroyObject() requires the address of an fpga_object
 * as created by fpgaTokenGetObject(), fpgaHandleGetObject(),
 * or fpgaObjectGetObject(). Passing any other value results in
 * undefind behavior.
 *
 * @param obj Pointer to the fpga_object instance to destroy
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if the object is NULL,
 * FPGA_EXCEPTION if an internal error is encountered.
 */
fpga_result fpgaDestroyObject(fpga_object *obj);

/**
 * @brief Retrieve the size of the object.
 *
 * @param[in] obj An fpga_object instance.
 * @param[out] value Pointer to variable to store size in.
 * @param[in] flags Flags that control how the object is read
 * If FPGA_OBJECT_SYNC is used then object will update its buffered copy before
 * retrieving the size.
 *
 * @return FPGA_OK on success. FPGA_INVALID_PARAM if any of supplied paramters
 * is invalid. FPGA_EXCEPTION if error occurred.
 */
fpga_result fpgaObjectGetSize(fpga_object obj, uint32_t *value, int flags);

/**
 * @brief Read bytes from an FPGA object
 *
 * @param[in] obj An fpga_object instance.
 * @param[out] buffer Pointer to a buffer to read bytes into.
 * @param[in] offset Byte offset relative to objects internal buffer where to
 * begin reading bytes from.
 * @param[in] len The length, in bytes, to read from the object.
 * @param[in] flags Flags that control how object is read
 * If FPGA_OBJECT_SYNC is used then object will update its buffered copy before
 * retrieving the data.
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid
 */
fpga_result fpgaObjectRead(fpga_object obj, uint8_t *buffer, size_t offset,
			   size_t len, int flags);

/**
 * @brief Read a 64-bit value from an FPGA object.
 * The value is assumed to be in string format and will be parsed. See flags
 * below for changing that behavior.
 *
 * @param[in] obj An fpga_object instance
 * @param[out] value Pointer to a 64-bit variable to store the value in
 * @param[in] flags Flags that control how the object is read
 * If FPGA_OBJECT_SYNC is used then object will update its buffered copy before
 * retrieving the data. If FPGA_OBJECT_RAW is used, then the data will be read
 * as raw bytes into the uint64_t pointer variable.
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid
 */
fpga_result fpgaObjectRead64(fpga_object obj, uint64_t *value, int flags);

/**
 * @brief Write 64-bit value to an FPGA object.
 * The value will be converted to string before writing. See flags below for
 * changing that behavior.
 *
 * @param[in] obj An fpga_object instance.
 * @param[in] value The value to write to the object
 * @param[in] flags Flags that control how the object is written
 * If FPGA_OBJECT_RAW is used, then the value will be written as raw bytes.
 *
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid
 *
 * @note The object must have been created using a handle to a resource.
 */
fpga_result fpgaObjectWrite64(fpga_object obj, uint64_t value, int flags);

/**
 * @brief Get FPGA resource accelerator or device sysfs path
 *
 * @param[in] token Token identifying a resource (accelerator or device)
 * @param[out] sysfs_path resource sysfs path
 * @return FPGA_OK on success, FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid
 *
 * @note The sysfs path of fpga token resource.
 */
fpga_result fpgaTokenSysfsPath(fpga_token token, char *sysfs_path);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* !__FPGA_SYSOBJECT_H__ */
