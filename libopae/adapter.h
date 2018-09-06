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

#ifndef __OPAE_ADAPTER_H__
#define __OPAE_ADAPTER_H__
#include <stdbool.h>

#include <opae/types.h>

typedef struct _opae_plugin {
	char *path;      // location on file system
	void *dl_handle; // handle to the loaded library instance
} opae_plugin;

typedef struct _opae_api_adapter_table {

	struct _opae_api_adapter_table *next;
	opae_plugin plugin;

	fpga_result (*fpgaOpen)(fpga_token token, fpga_handle *handle,
				int flags);

	fpga_result (*fpgaClose)(fpga_handle handle);

	fpga_result (*fpgaReset)(fpga_handle handle);

	fpga_result (*fpgaGetPropertiesFromHandle)(fpga_handle handle,
						   fpga_properties *prop);

	fpga_result (*fpgaGetProperties)(fpga_token token,
					 fpga_properties *prop);

	fpga_result (*fpgaUpdateProperties)(fpga_token token,
					    fpga_properties prop);

	fpga_result (*fpgaClearProperties)(fpga_properties prop);

	fpga_result (*fpgaCloneProperties)(fpga_properties src,
					   fpga_properties *dst);

	fpga_result (*fpgaDestroyProperties)(fpga_properties *prop);

	fpga_result (*fpgaPropertiesGetParent)(const fpga_properties prop,
					       fpga_token *parent);

	fpga_result (*fpgaPropertiesSetParent)(fpga_properties prop,
					       fpga_token parent);

	fpga_result (*fpgaPropertiesGetObjectType)(const fpga_properties prop,
						   fpga_objtype *objtype);

	fpga_result (*fpgaPropertiesSetObjectType)(fpga_properties prop,
						   fpga_objtype objtype);

	fpga_result (*fpgaPropertiesGetSegment)(const fpga_properties prop,
						uint16_t *segment);

	fpga_result (*fpgaPropertiesSetSegment)(fpga_properties prop,
						uint16_t segment);

	fpga_result (*fpgaPropertiesGetBus)(const fpga_properties prop,
					    uint8_t *bus);

	fpga_result (*fpgaPropertiesSetBus)(fpga_properties prop, uint8_t bus);

	fpga_result (*fpgaPropertiesGetDevice)(const fpga_properties prop,
					       uint8_t *device);

	fpga_result (*fpgaPropertiesSetDevice)(fpga_properties prop,
					       uint8_t device);

	fpga_result (*fpgaPropertiesGetFunction)(const fpga_properties prop,
						 uint8_t *function);

	fpga_result (*fpgaPropertiesSetFunction)(fpga_properties prop,
						 uint8_t function);

	fpga_result (*fpgaPropertiesGetSocketID)(const fpga_properties prop,
						 uint8_t *socket_id);

	fpga_result (*fpgaPropertiesSetSocketID)(fpga_properties prop,
						 uint8_t socket_id);

	fpga_result (*fpgaPropertiesGetDeviceID)(const fpga_properties prop,
						 uint16_t *device_id);

	fpga_result (*fpgaPropertiesSetDeviceID)(fpga_properties prop,
						 uint16_t device_id);

	fpga_result (*fpgaPropertiesGetNumSlots)(const fpga_properties prop,
						 uint32_t *num_slots);

	fpga_result (*fpgaPropertiesSetNumSlots)(fpga_properties prop,
						 uint32_t num_slots);

	fpga_result (*fpgaPropertiesGetBBSID)(const fpga_properties prop,
					      uint64_t *bbs_id);

	fpga_result (*fpgaPropertiesSetBBSID)(fpga_properties prop,
					      uint64_t bbs_id);

	fpga_result (*fpgaPropertiesGetBBSVersion)(const fpga_properties prop,
						   fpga_version *bbs_version);

	fpga_result (*fpgaPropertiesSetBBSVersion)(fpga_properties prop,
						   fpga_version version);

	fpga_result (*fpgaPropertiesGetVendorID)(const fpga_properties prop,
						 uint16_t *vendor_id);

	fpga_result (*fpgaPropertiesSetVendorID)(fpga_properties prop,
						 uint16_t vendor_id);

	fpga_result (*fpgaPropertiesGetModel)(const fpga_properties prop,
					      char *model);

	fpga_result (*fpgaPropertiesSetModel)(fpga_properties prop,
					      char *model);

	fpga_result (*fpgaPropertiesGetLocalMemorySize)(
		const fpga_properties prop, uint64_t *lms);

	fpga_result (*fpgaPropertiesSetLocalMemorySize)(fpga_properties prop,
							uint64_t lms);

	fpga_result (*fpgaPropertiesGetCapabilities)(const fpga_properties prop,
						     uint64_t *capabilities);

	fpga_result (*fpgaPropertiesSetCapabilities)(fpga_properties prop,
						     uint64_t capabilities);

	fpga_result (*fpgaPropertiesGetGUID)(const fpga_properties prop,
					     fpga_guid *guid);

	fpga_result (*fpgaPropertiesSetGUID)(fpga_properties prop,
					     fpga_guid guid);

	fpga_result (*fpgaPropertiesGetNumMMIO)(const fpga_properties prop,
						uint32_t *mmio_spaces);

	fpga_result (*fpgaPropertiesSetNumMMIO)(fpga_properties prop,
						uint32_t mmio_spaces);

	fpga_result (*fpgaPropertiesGetNumInterrupts)(
		const fpga_properties prop, uint32_t *num_interrupts);

	fpga_result (*fpgaPropertiesSetNumInterrupts)(fpga_properties prop,
						      uint32_t num_interrupts);

	fpga_result (*fpgaPropertiesGetAcceleratorState)(
		const fpga_properties prop, fpga_accelerator_state *state);

	fpga_result (*fpgaPropertiesSetAcceleratorState)(
		fpga_properties prop, fpga_accelerator_state state);

	fpga_result (*fpgaPropertiesGetObjectID)(const fpga_properties prop,
						 uint64_t *object_id);

	fpga_result (*fpgaPropertiesSetObjectID)(const fpga_properties prop,
						 uint64_t object_id);

	fpga_result (*fpgaPropertiesGetNumErrors)(const fpga_properties prop,
						  uint32_t *num_errors);

	fpga_result (*fpgaPropertiesSetNumErrors)(const fpga_properties prop,
						  uint32_t num_errors);

	fpga_result (*fpgaWriteMMIO64)(fpga_handle handle, uint32_t mmio_num,
				       uint64_t offset, uint64_t value);

	fpga_result (*fpgaReadMMIO64)(fpga_handle handle, uint32_t mmio_num,
				      uint64_t offset, uint64_t *value);

	fpga_result (*fpgaWriteMMIO32)(fpga_handle handle, uint32_t mmio_num,
				       uint64_t offset, uint32_t value);

	fpga_result (*fpgaReadMMIO32)(fpga_handle handle, uint32_t mmio_num,
				      uint64_t offset, uint32_t *value);

	fpga_result (*fpgaMapMMIO)(fpga_handle handle, uint32_t mmio_num,
				   uint64_t **mmio_ptr);

	fpga_result (*fpgaUnmapMMIO)(fpga_handle handle, uint32_t mmio_num);

	fpga_result (*fpgaEnumerate)(const fpga_properties *filters,
				     uint32_t num_filters, fpga_token *tokens,
				     uint32_t max_tokens,
				     uint32_t *num_matches);

	fpga_result (*fpgaCloneToken)(fpga_token src, fpga_token *dst);

	fpga_result (*fpgaDestroyToken)(fpga_token *token);

	fpga_result (*fpgaGetNumUmsg)(fpga_handle handle, uint64_t *value);

	fpga_result (*fpgaSetUmsgAttributes)(fpga_handle handle,
					     uint64_t value);

	fpga_result (*fpgaTriggerUmsg)(fpga_handle handle, uint64_t value);

	fpga_result (*fpgaGetUmsgPtr)(fpga_handle handle, uint64_t **umsg_ptr);

	fpga_result (*fpgaPrepareBuffer)(fpga_handle handle, uint64_t len,
					 void **buf_addr, uint64_t *wsid,
					 int flags);

	fpga_result (*fpgaReleaseBuffer)(fpga_handle handle, uint64_t wsid);

	fpga_result (*fpgaGetIOAddress)(fpga_handle handle, uint64_t wsid,
					uint64_t *ioaddr);
/*
	fpga_result (*fpgaGetOPAECVersion)(fpga_version *version);

	fpga_result (*fpgaGetOPAECVersionString)(char *version_str, size_t len);

	fpga_result (*fpgaGetOPAECBuildString)(char *build_str, size_t len);
*/

	fpga_result (*fpgaReadError)(fpga_token token, uint32_t error_num,
				     uint64_t *value);

	fpga_result (*fpgaClearError)(fpga_token token, uint32_t error_num);

	fpga_result (*fpgaClearAllErrors)(fpga_token token);

	fpga_result (*fpgaGetErrorInfo)(fpga_token token, uint32_t error_num,
					struct fpga_error_info *error_info);

/*
	const char *(*fpgaErrStr)(fpga_result e);
*/

	fpga_result (*fpgaCreateEventHandle)(fpga_event_handle *event_handle);

	fpga_result (*fpgaDestroyEventHandle)(fpga_event_handle *event_handle);

	fpga_result (*fpgaGetOSObjectFromEventHandle)(
		const fpga_event_handle eh, int *fd);

	fpga_result (*fpgaRegisterEvent)(fpga_handle handle,
					 fpga_event_type event_type,
					 fpga_event_handle event_handle,
					 uint32_t flags);

	fpga_result (*fpgaUnregisterEvent)(fpga_handle handle,
					   fpga_event_type event_type,
					   fpga_event_handle event_handle);

	fpga_result (*fpgaAssignPortToInterface)(fpga_handle fpga,
						 uint32_t interface_num,
						 uint32_t slot_num, int flags);

	fpga_result (*fpgaAssignToInterface)(fpga_handle fpga,
					     fpga_token accelerator,
					     uint32_t host_interface,
					     int flags);

	fpga_result (*fpgaReleaseFromInterface)(fpga_handle fpga,
						fpga_token accelerator);

	fpga_result (*fpgaReconfigureSlot)(fpga_handle fpga, uint32_t slot,
					   const uint8_t *bitstream,
					   size_t bitstream_len, int flags);

	fpga_result (*fpgaGetTokenObject)(fpga_token token, const char *name,
                                          fpga_object *object, int flags);

	fpga_result (*fpgaDestroyObject)(fpga_object *obj);

	fpga_result (*fpgaObjectRead)(fpga_object obj, uint8_t *buffer, size_t offset,
                                      size_t len, int flags);

	// configuration functions
	int (*initialize)(void);
	int (*finalize)(void);

	// first-level query
	bool (*supports_device)(const char *device_type);
	bool (*supports_host)(const char *hostname);

} opae_api_adapter_table;

#endif /* __OPAE_ADAPTER_H__ */
