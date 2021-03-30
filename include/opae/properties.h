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
 * @file opae/properties.h
 * @brief Functions for examining and manipulating `fpga_properties` objects
 *
 * In OPAE, `fpga_properties` objects are used both for obtaining information
 * about resources and for selectively enumerating resources based on their
 * properties. This file provides accessor functions (get/set) to allow reading
 * and writing individual items of an `fpga_properties` object. Generally, not
 * all object types supported by OPAE carry all properties. If you call a
 * property accessor method on a `fpga_properties` object that does not support
 * this particular property, it will return FPGA_INVALID_PARAM.
 *
 * # Accessor Return Values
 * In addition to the return values specified in the documentation below, all
 * accessor functions return FPGA_OK on success, FPGA_INVALID_PARAM if you pass
 * NULL or invalid parameters (i.e. non-initialized properties objects),
 * FPGA_EXCEPTION if an internal exception occurred trying to access the
 * properties object, FPGA_NOT_FOUND if the requested property is not part of
 * the supplied properties object.
 */

#ifndef __FPGA_PROPERTIES_H__
#define __FPGA_PROPERTIES_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a fpga_properties object
 *
 * Initializes the memory pointed at by `prop` to represent a properties
 * object, and populates it with the properties of the resource referred to by
 * `handle`. Individual properties can then be queried using fpgaPropertiesGet*()
 * accessor functions.
 *
 * @note fpgaGetPropertiesFromHandle() will allocate memory for the created properties
 * object returned in `prop`. It is the responsibility of the caller
 * to free this memory after use by calling fpgaDestroyProperties().
 *
 * @param[in]  handle     Open handle to get properties for.
 * @param[out] prop       Pointer to a variable of type fpga_properties
 * @returns FPGA_OK on success. FPGA_NO_MEMORY if no memory could be allocated
 * to create the `fpga_properties` object. FPGA_EXCEPTION if an exception
 * happend while initializing the `fpga_properties` object.
 */
fpga_result fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop);

/**
 * Create a fpga_properties object
 *
 * Initializes the memory pointed at by `prop` to represent a properties
 * object, and populates it with the properties of the resource referred to by
 * `token`. Individual properties can then be queried using fpgaPropertiesGet*()
 * accessor functions.
 *
 * If `token` is NULL, an "empty" properties object is created to be used as a
 * filter for fpgaEnumerate(). All individual fields are set to `don`t care`,
 * which implies that the fpga_properties object would match all FPGA resources
 * if used for an fpgaEnumerate() query. The matching criteria can be further
 * refined by using fpgaSet* functions on the properties object, or the
 * object can be populated with the actual properties of a resource by using
 * fpgaUpdateProperties().
 *
 * @note fpgaGetProperties() will allocate memory for the created properties
 * object returned in `prop`. It is the responsibility of the caller
 * to free this memory after use by calling fpgaDestroyProperties().
 *
 * @param[in]  token      Token to get properties for. Can be NULL, which will
 *                        create an empty properties object to be used as a
 *                        filter for fpgaEnumerate().
 * @param[out] prop       Pointer to a variable of type fpga_properties
 * @returns FPGA_OK on success. FPGA_NO_MEMORY if no memory could be allocated
 * to create the `fpga_properties` object. FPGA_EXCEPTION if an exception
 * happend while initializing the `fpga_properties` object.
 */
fpga_result fpgaGetProperties(fpga_token token, fpga_properties *prop);

/**
 * Update a fpga_properties object
 *
 * Populates the properties object 'prop' with properties of the resource
 * referred to by 'token'. Unlike fpgaGetProperties(), this call will not create
 * a new properties object or allocate memory for it, but use a previously
 * created properties object.
 *
 * @param[in]  token      Token to retrieve properties for
 * @param[in]  prop       fpga_properties object to update
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if `token` or `prop` are not
 * valid objects. FPGA_NOT_FOUND if the resource referred to by `token` was
 * not found. FPGA_NO_DRIVER if not driver is loaded. FPGA_EXCEPTION if an
 * internal exception occured when trying to update `prop`.
 */
fpga_result fpgaUpdateProperties(fpga_token token, fpga_properties prop);

/**
 * Clear a fpga_properties object
 *
 * Sets all fields of the properties object pointed at by 'prop' to 'don't
 * care', which implies that the fpga_properties object would match all FPGA
 * resources if used for an fpgaEnumerate() query. The matching criteria can be
 * further refined by using fpgaSet* functions on the properties object.
 *
 * Instead of creating a new fpga_properties object every time, this function
 * can be used to re-use fpga_properties objects from previous queries.
 *
 * @param[in]  prop       fpga_properties object to clear
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if `prop` is not a valid
 * object. FPGA_EXCEPTION if an * internal exception occured when trying to
 * access `prop`.
 */
fpga_result fpgaClearProperties(fpga_properties prop);

/**
 * Clone a fpga_properties object
 *
 * Creates a copy of an fpga_properties object.
 *
 * @note This call creates a new properties object and allocates memory for it.
 * Both the 'src' and the newly created 'dst' objects will eventually need to be
 * destroyed using fpgaDestroyProperties().
 *
 * @param[in]  src        fpga_properties object to copy
 * @param[out] dst        New fpga_properties object cloned from 'src'
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if `src` is not a valid
 * object, or if `dst` is NULL. FPGA_NO_MEMORY if there was not enough memory
 * to allocate an `fpga_properties` object for `dst`. FPGA_EXCEPTION if an
 * internal exception occurred either accessing `src` or updating `dst`.
 */
fpga_result fpgaCloneProperties(fpga_properties src, fpga_properties *dst);

/**
 * Destroy a fpga_properties object
 *
 * Destroys an existing fpga_properties object that the caller has previously
 * created using fpgaGetProperties() or fpgaCloneProperties().
 *
 * @note fpgaDestroyProperties() requires the address of an fpga_properties
 * object, similar to fpgaGetPropertiesFromHandle(), fpgaGetProperties(),
 * and fpgaCloneProperties(). Passing any other value results in undefined
 * behavior.
 *
 * @param[inout]  prop    Pointer to the fpga_properties object to destroy
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM is `prop` is not a valid
 * object. FPGA_EXCEPTION if an internal exception occurrred while trying to
 * access `prop`.
 */
fpga_result fpgaDestroyProperties(fpga_properties *prop);

/**
 * Get the token of the parent object
 *
 * Returns the token of the parent of the queried resource in '*parent'.
 *
 * @param[in]  prop   Properties object to query
 * @param[out] parent Pointer to a token variable of the resource 'prop' is
 *                    associated with
 * @returns FPGA_NOT_FOUND if resource does not have a
 * parent (e.g. an FPGA_DEVICE resource does not have parents). Also see
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetParent(const fpga_properties prop,
				    fpga_token *parent);

/**
 * Set the token of the parent object
 *
 * @param[in]  prop   Properties object to modify
 * @param[out] parent Pointer to a token variable of the resource 'prop' is
 *                    associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetParent(fpga_properties prop,
				    fpga_token parent);
/**
 * Get the object type of a resource
 *
 * Returns the object type of the queried resource.
 *
 * @param[in]  prop    Properties object to query
 * @param[out] objtype Pointer to an object type variable of the resource
 *                     'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetObjectType(const fpga_properties prop,
					fpga_objtype *objtype);

/**
 * Set the object type of a resource
 *
 * Sets the object type of the resource. * Currently supported object types are
 * FPGA_DEVICE and FPGA_ACCELERATOR.
 *
 * @param[in]  prop    Properties object to modify
 * @param[out] objtype Object type of the resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetObjectType(fpga_properties prop,
					fpga_objtype objtype);
/**
 * Get the PCI segment number of a resource
 *
 * Returns the segment number of the queried resource.
 *
 * @param[in]  prop    Properties object to query
 * @param[out] segment Pointer to a PCI segment variable of the resource 'prop'
 *                     is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetSegment(const fpga_properties prop, uint16_t *segment);

/**
 * Set the PCI segment number of a resource
 *
 * @param[in]  prop    Properties object to modify
 * @param[in]  segment PCI segment number of the resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetSegment(fpga_properties prop, uint16_t segment);

/**
 * Get the PCI bus number of a resource
 *
 * Returns the bus number the queried resource.
 *
 * @param[in]  prop    Properties object to query
 * @param[out] bus     Pointer to a PCI bus variable of the resource 'prop'
 *                     is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetBus(const fpga_properties prop, uint8_t *bus);

/**
 * Set the PCI bus number of a resource
 *
 * @param[in]  prop    Properties object to modify
 * @param[in]  bus     PCI bus number of the resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetBus(fpga_properties prop, uint8_t bus);

/**
 * Get the PCI device number of a resource
 *
 * Returns the device number the queried resource.
 *
 * @param[in]  prop    Properties object to query
 * @param[out] device  Pointer to a PCI device variable of the resource 'prop'
 *                     is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetDevice(const fpga_properties prop,
				    uint8_t *device);

/**
 * Set the PCI device number of a resource
 *
 * Enforces the limitation on the number of devices as specified in the
 * PCI spec.
 *
 * @param[in]  prop    Properties object to modify
 * @param[in]  device  PCI device number of the resource 'prop' is associated
 *                     with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetDevice(fpga_properties prop,
				    uint8_t device);

/**
 * Get the PCI function number of a resource
 *
 * Returns the function number the queried resource.
 *
 * @param[in]  prop     Properties object to query
 * @param[out] function Pointer to PCI function variable of the
 *                      resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetFunction(const fpga_properties prop,
				      uint8_t *function);

/**
 * Set the PCI function number of a resource
 *
 * Enforces the limitation on the number of functions as specified in the
 * PCI spec.
 *
 * @param[in]  prop     Properties object to modify
 * @param[in]  function PCI function number of the resource 'prop' is
 *                      associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetFunction(fpga_properties prop,
				      uint8_t function);

/**
 * Get the socket id of a resource
 *
 * Returns the socket id of the queried resource.
 *
 * @param[in]  prop      Properties object to query
 * @param[out] socket_id Pointer to a socket id variable of the
 *                       resource 'prop'
 *                       is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 * See also "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetSocketID(const fpga_properties prop,
				      uint8_t *socket_id);

/**
 * Set the socket id of the resource
 *
 * @param[in]  prop      Properties object to modify
 * @param[in]  socket_id Socket id of the resource 'prop' is
 *                       associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetSocketID(fpga_properties prop,
				      uint8_t socket_id);

/**
 * Get the device id of the resource
 *
 * @param[in]  prop      Properties object to query
 * @param[out] device_id Pointer to a device id variable of the
 *                       resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetDeviceID(const fpga_properties prop,
				      uint16_t *device_id);

/**
 * Set the device id of the resource
 *
 * @param[in]  prop      Properties object to modify
 * @param[in]  device_id Device id of the resource 'prop' is associated with
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetDeviceID(fpga_properties prop,
				      uint16_t device_id);

/**
 * Get the number of slots of an FPGA resource property
 *
 * Returns the number of slots present in an FPGA.
 *
 * @param[in]  prop       Properties object to query - must be of type FPGA_DEVICE
 * @param[out] num_slots  Pointer to number of slots variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetNumSlots(const fpga_properties prop,
				      uint32_t *num_slots);

/**
 * Set the number of slots of an FPGA resource property
 *
 * @param[in]  prop       Properties object to modify - must be of type
 *                        FPGA_DEVICE
 * @param[in] num_slots   Number of slots of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetNumSlots(fpga_properties prop,
				      uint32_t num_slots);

/**
 * Get the BBS ID of an FPGA resource property
 *
 * Returns the blue bitstream id of an FPGA.
 *
 * @param[in]  prop       Properties object to query - must be of type FPGA_DEVICE
 * @param[out] bbs_id     Pointer to a bbs id variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetBBSID(const fpga_properties prop,
				   uint64_t *bbs_id);


/**
 * Set the BBS ID of an FPGA resource property
 *
 * @param[in]  prop       Properties object to modify - must be of type
 *                        FPGA_DEVICE
 * @param[in]  bbs_id     Blue bitstream id of the FPGA resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetBBSID(fpga_properties prop,
				   uint64_t bbs_id);


/**
 * Get the BBS Version of an FPGA resource property
 *
 * Returns the blue bitstream version of an FPGA.
 *
 * @param[in]  prop        Properties object to query - must be of type
 *                         FPGA_DEVICE
 * @param[out] bbs_version Pointer to a bbs version variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetBBSVersion(const fpga_properties prop,
					fpga_version *bbs_version);

/**
 * Set the BBS Version of an FPGA resource property
 *
 * @param[in]  prop        Properties object to modify - must be of type
 *                         FPGA_DEVICE
 * @param[in]  version     Blue bitstream version of the FPGA resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetBBSVersion(fpga_properties prop,
					fpga_version version);


/**
 * Get the vendor id of an FPGA resource property
 *
 * Returns the vendor id of an FPGA.
 *
 * @param[in]  prop      Properties object to query - must be of type FPGA_DEVICE
 * @param[out] vendor_id Pointer to a vendor id variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesGetVendorID(const fpga_properties prop,
				      uint16_t *vendor_id);


/**
 * Set the vendor id of an FPGA resource property
 *
 * @param[in]  prop      Properties object to modify - must be of type FPGA_DEVICE
 * @param[in]  vendor_id Vendor id of the FPGA resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesSetVendorID(fpga_properties prop,
				      uint16_t vendor_id);

/**
 * Get the model of an FPGA resource property
 *
 * Returns the model of an FPGA.
 *
 * @param[in]  prop  Properties object to query - must be of type FPGA_DEVICE
 * @param[in]  model Model of the FPGA resource (string of minimum
 *                   FPGA_MODEL_LENGTH length
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesGetModel(const fpga_properties prop,
				   char *model);


/**
 * Set the model of an FPGA resource property
 *
 * @param[in]  prop  Properties object to modify - must be of type FPGA_DEVICE
 * @param[in]  model Model of the FPGA resource (string of maximum
 *                   FPGA_MODEL_LENGTH length
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesSetModel(fpga_properties prop,
				   char *model);


/**
 * Get the local memory size of an FPGA resource property
 *
 * Returns the local memory size of an FPGA.
 *
 * @param[in]  prop  Properties object to query - must be of type FPGA_DEVICE
 * @param[out] lms   Pointer to a memory size variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesGetLocalMemorySize(const fpga_properties prop,
					     uint64_t *lms);


/**
 * Set the local memory size of an FPGA resource property
 *
 * @param[in]  prop  Properties object to modify - must be of type FPGA_DEVICE
 * @param[in]  lms   Local memory size of the FPGA resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesSetLocalMemorySize(fpga_properties prop,
					     uint64_t lms);

/**
 * Get the capabilities FPGA resource property
 *
 * Returns the capabilities of an FPGA.
 * Capabilities is a bitfield value
 *
 * @param[in]  prop         Properties object to query - must be of type
 *                          FPGA_DEVICE
 * @param[out] capabilities Pointer to a capabilities variable of the FPGA
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesGetCapabilities(const fpga_properties prop,
					  uint64_t *capabilities);


/**
 * Set the capabilities of an FPGA resource property
 *
 * Capabilities is a bitfield value
 *
 * @param[in]  prop         Properties object to modify - must be of type
 *                          FPGA_DEVICE
 * @param[in]  capabilities Capabilities of the FPGA resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_DEVICE. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 *
 * @note This API is not currently supported.
 */
fpga_result fpgaPropertiesSetCapabilities(fpga_properties prop,
					  uint64_t capabilities);

/**
 * Get the GUID of a resource
 *
 * Returns the GUID of an FPGA or accelerator object.
 *
 * For an accelerator, the GUID uniquely identifies a specific accelerator context type,
 * i.e. different accelerators will have different GUIDs. For an FPGA, the GUID
 * is used to identify a certain instance of an FPGA, e.g. to determine whether
 * a given bitstream would be compatible.
 *
 * @param[in]  prop       Properties object to query
 * @param[out] guid       Pointer to a GUID of the slot variable
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetGUID(const fpga_properties prop,
				  fpga_guid *guid);

/**
 * Set the GUID of a resource
 *
 * Sets the GUID of an FPGA or accelerator object.
 *
 * For an accelerator, the GUID uniquely identifies a specific accelerator context type,
 * i.e. different accelerators will have different GUIDs. For an FPGA, the GUID
 * is used to identify a certain instance of an FPGA, e.g. to determine whether
 * a given bitstream would be compatible.
 *
 * @param[in]  prop       Properties object to modify
 * @param[out] guid       Pointer to a GUID of the slot variable
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetGUID(fpga_properties prop, fpga_guid guid);

/**
 * Get the number of mmio spaces
 *
 * Returns the number of mmio spaces of an AFU properties structure.
 *
 * @param[in]  prop        Properties object to query - must be of type FPGA_ACCELERATOR
 * @param[out] mmio_spaces Pointer to a variable for number of mmio spaces
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetNumMMIO(const fpga_properties prop,
				     uint32_t *mmio_spaces);

/**
 * Set the number of mmio spaces
 *
 * Sets the number of mmio spaces of an AFU properties structure.
 *
 * @param[in] prop        Properties object to modify - must be of type FPGA_ACCELERATOR
 * @param[in] mmio_spaces Number of MMIO spaces of the accelerator
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetNumMMIO(fpga_properties prop,
				     uint32_t mmio_spaces);

/**
 * Get the number of interrupts
 *
 * Returns the number of interrupts of an accelerator properties structure.
 *
 * @param[in]  prop            Properties object to query - must be of type
 *                             FPGA_ACCELERATOR
 * @param[out] num_interrupts  Pointer to a variable for number of interrupts
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetNumInterrupts(const fpga_properties prop,
					   uint32_t *num_interrupts);

/**
 * Set the number of interrupts
 *
 * Sets the number of interrupts of an accelerator properties structure.
 *
 * @param[in] prop            Properties object to modify - must be of type
 *                            FPGA_ACCELERATOR
 * @param[in] num_interrupts  Number of interrupts of the accelerator
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetNumInterrupts(fpga_properties prop,
					   uint32_t num_interrupts);

/**
 * Get the state of a accelerator resource property
 *
 * Returns the accelerator state of a accelerator.
 *
 * @param[in]  prop   Properties object to query - must be of type FPGA_ACCELERATOR
 * @param[out] state Pointer to a accelerator state variable of the accelerator
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetAcceleratorState(const fpga_properties prop,
					      fpga_accelerator_state *state);


/**
 * Set the state of an accelerator resource property
 *
 * @param[in] prop    Properties object to modify - must be of type FPGA_ACCELERATOR
 * @param[in] state  accelerator state of the accelerator resource
 * @returns FPGA_INVALID_PARAM if object type is not FPGA_ACCELERATOR. See also
 * "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetAcceleratorState(fpga_properties prop,
					      fpga_accelerator_state state);

/**
 * Get the object ID of a resource
 *
 * Returns the object ID of a resource. The object ID is a 64 bit identifier
 * that is unique within a single node or system. It represents a similar
 * concept as the token, but can be used across processes (e.g. passed on the
 * command line).
 *
 * @param[in]  prop       Properties object to query
 * @param[out] object_id  Pointer to a 64bit memory location to store the object
 *                        ID in
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetObjectID(const fpga_properties prop,
					    uint64_t *object_id);


/**
 * Set the object ID of a resource
 *
 * Sets the object ID of a resource. The object ID is a 64 bit identifier
 * that is unique within a single node or system. It represents a similar
 * concept as the token, but can be used across processes (e.g. passed on the
 * command line).
 *
 * @param[in]  prop       Properties object to query
 * @param[in]  object_id  A 64bit value to use as the object ID
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetObjectID(const fpga_properties prop,
					    uint64_t object_id);


/**
 * Get the number of errors that can be reported by a resource
 *
 * Returns the number of error registers understood by a resource.
 *
 * @param[in]  prop       Properties object to query
 * @param[out] num_errors Pointer to a 32 bit memory location to store the
 *                        number of supported errors in
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesGetNumErrors(const fpga_properties prop,
				       uint32_t *num_errors);


/**
 * Set the number of error registers
 *
 * Set the number of error registers understood by a resource to enumerate.
 *
 * @param[in]  prop       Properties object to query
 * @param[in]  num_errors Number of errors
 * @returns See "Accessor Return Values" in [properties.h](#properties-h).
 */
fpga_result fpgaPropertiesSetNumErrors(const fpga_properties prop,
				       uint32_t num_errors);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_PROPERTIES_H__

