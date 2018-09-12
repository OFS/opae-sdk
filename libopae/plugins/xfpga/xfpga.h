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

#ifndef __XFPGA_XFPGA_H__
#define __XFPGA_XFPGA_H__

#include <stdint.h>

#include <opae/types.h>
#ifdef __cplusplus
extern "C" {
#endif
fpga_result xfpga_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result xfpga_fpgaClose(fpga_handle handle);
fpga_result xfpga_fpgaReset(fpga_handle handle);
fpga_result xfpga_fpgaGetPropertiesFromHandle(fpga_handle handle,
					      fpga_properties *prop);
fpga_result xfpga_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result xfpga_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
fpga_result xfpga_fpgaClearProperties(fpga_properties prop);
fpga_result xfpga_fpgaCloneProperties(fpga_properties src,
				      fpga_properties *dst);
fpga_result xfpga_fpgaDestroyProperties(fpga_properties *prop);
fpga_result xfpga_fpgaPropertiesGetParent(const fpga_properties prop,
					  fpga_token *parent);
fpga_result xfpga_fpgaPropertiesSetParent(fpga_properties prop,
					  fpga_token parent);
fpga_result xfpga_fpgaPropertiesGetObjectType(const fpga_properties prop,
					      fpga_objtype *objtype);
fpga_result xfpga_fpgaPropertiesSetObjectType(fpga_properties prop,
					      fpga_objtype objtype);
fpga_result xfpga_fpgaPropertiesGetSegment(const fpga_properties prop,
					   uint16_t *segment);
fpga_result xfpga_fpgaPropertiesSetSegment(fpga_properties prop,
					   uint16_t segment);
fpga_result xfpga_fpgaPropertiesGetBus(const fpga_properties prop,
				       uint8_t *bus);
fpga_result xfpga_fpgaPropertiesSetBus(fpga_properties prop, uint8_t bus);
fpga_result xfpga_fpgaPropertiesGetDevice(const fpga_properties prop,
					  uint8_t *device);
fpga_result xfpga_fpgaPropertiesSetDevice(fpga_properties prop, uint8_t device);
fpga_result xfpga_fpgaPropertiesGetFunction(const fpga_properties prop,
					    uint8_t *function);
fpga_result xfpga_fpgaPropertiesSetFunction(fpga_properties prop,
					    uint8_t function);
fpga_result xfpga_fpgaPropertiesGetSocketID(const fpga_properties prop,
					    uint8_t *socket_id);
fpga_result xfpga_fpgaPropertiesSetSocketID(fpga_properties prop,
					    uint8_t socket_id);
fpga_result xfpga_fpgaPropertiesGetDeviceID(const fpga_properties prop,
					    uint16_t *device_id);
fpga_result xfpga_fpgaPropertiesSetDeviceID(fpga_properties prop,
					    uint16_t device_id);
fpga_result xfpga_fpgaPropertiesGetNumSlots(const fpga_properties prop,
					    uint32_t *num_slots);
fpga_result xfpga_fpgaPropertiesSetNumSlots(fpga_properties prop,
					    uint32_t num_slots);
fpga_result xfpga_fpgaPropertiesGetBBSID(const fpga_properties prop,
					 uint64_t *bbs_id);
fpga_result xfpga_fpgaPropertiesSetBBSID(fpga_properties prop, uint64_t bbs_id);
fpga_result xfpga_fpgaPropertiesGetBBSVersion(const fpga_properties prop,
					      fpga_version *bbs_version);
fpga_result xfpga_fpgaPropertiesSetBBSVersion(fpga_properties prop,
					      fpga_version bbs_version);
fpga_result xfpga_fpgaPropertiesGetVendorID(const fpga_properties prop,
					    uint16_t *vendor_id);
fpga_result xfpga_fpgaPropertiesSetVendorID(fpga_properties prop,
					    uint16_t vendor_id);
fpga_result xfpga_fpgaPropertiesGetModel(const fpga_properties prop,
					 char *model);
fpga_result xfpga_fpgaPropertiesSetModel(fpga_properties prop, char *model);
fpga_result xfpga_fpgaPropertiesGetLocalMemorySize(const fpga_properties prop,
						   uint64_t *local_memory_size);
fpga_result xfpga_fpgaPropertiesSetLocalMemorySize(fpga_properties prop,
						   uint64_t local_memory_size);
fpga_result xfpga_fpgaPropertiesGetCapabilities(const fpga_properties prop,
						uint64_t *capabilities);
fpga_result xfpga_fpgaPropertiesSetCapabilities(fpga_properties prop,
						uint64_t capabilities);
fpga_result xfpga_fpgaPropertiesGetGUID(const fpga_properties prop,
					fpga_guid *guid);
fpga_result xfpga_fpgaPropertiesSetGUID(fpga_properties prop, fpga_guid guid);
fpga_result xfpga_fpgaPropertiesGetNumMMIO(const fpga_properties prop,
					   uint32_t *mmio_spaces);
fpga_result xfpga_fpgaPropertiesSetNumMMIO(fpga_properties prop,
					   uint32_t mmio_spaces);
fpga_result xfpga_fpgaPropertiesGetNumInterrupts(const fpga_properties prop,
						 uint32_t *num_interrupts);
fpga_result xfpga_fpgaPropertiesSetNumInterrupts(fpga_properties prop,
						 uint32_t num_interrupts);
fpga_result
xfpga_fpgaPropertiesGetAcceleratorState(const fpga_properties prop,
					fpga_accelerator_state *state);
fpga_result
xfpga_fpgaPropertiesSetAcceleratorState(fpga_properties prop,
					fpga_accelerator_state state);
fpga_result xfpga_fpgaPropertiesGetObjectID(const fpga_properties prop,
					    uint64_t *object_id);
fpga_result xfpga_fpgaPropertiesSetObjectID(fpga_properties prop,
					    uint64_t object_id);
fpga_result xfpga_fpgaPropertiesGetNumErrors(const fpga_properties prop,
					     uint32_t *num_errors);
fpga_result xfpga_fpgaPropertiesSetNumErrors(const fpga_properties prop,
					     uint32_t num_errors);
fpga_result xfpga_fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint64_t value);
fpga_result xfpga_fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint64_t *value);
fpga_result xfpga_fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint32_t value);
fpga_result xfpga_fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint32_t *value);
fpga_result xfpga_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
			      uint64_t **mmio_ptr);
fpga_result xfpga_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num);
fpga_result xfpga_fpgaEnumerate(const fpga_properties *filters,
				uint32_t num_filters, fpga_token *tokens,
				uint32_t max_tokens, uint32_t *num_matches);
fpga_result xfpga_fpgaCloneToken(fpga_token src, fpga_token *dst);
fpga_result xfpga_fpgaDestroyToken(fpga_token *token);
fpga_result xfpga_fpgaGetNumUmsg(fpga_handle handle, uint64_t *value);
fpga_result xfpga_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value);
fpga_result xfpga_fpgaTriggerUmsg(fpga_handle handle, uint64_t value);
fpga_result xfpga_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr);
fpga_result xfpga_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
				    void **buf_addr, uint64_t *wsid, int flags);
fpga_result xfpga_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid);
fpga_result xfpga_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
				   uint64_t *ioaddr);
fpga_result xfpga_fpgaGetOPAECVersion(fpga_version *version);
fpga_result xfpga_fpgaGetOPAECVersionString(char *version_str, size_t len);
fpga_result xfpga_fpgaGetOPAECBuildString(char *build_str, size_t len);
fpga_result xfpga_fpgaReadError(fpga_token token, uint32_t error_num,
				uint64_t *value);
fpga_result xfpga_fpgaClearError(fpga_token token, uint32_t error_num);
fpga_result xfpga_fpgaClearAllErrors(fpga_token token);
fpga_result xfpga_fpgaGetErrorInfo(fpga_token token, uint32_t error_num,
				   struct fpga_error_info *error_info);
fpga_result xfpga_fpgaCreateEventHandle(fpga_event_handle *event_handle);
fpga_result xfpga_fpgaDestroyEventHandle(fpga_event_handle *event_handle);
fpga_result xfpga_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						 int *fd);
fpga_result xfpga_fpgaRegisterEvent(fpga_handle handle,
				    fpga_event_type event_type,
				    fpga_event_handle event_handle,
				    uint32_t flags);
fpga_result xfpga_fpgaUnregisterEvent(fpga_handle handle,
				      fpga_event_type event_type,
				      fpga_event_handle event_handle);
fpga_result xfpga_fpgaAssignPortToInterface(fpga_handle fpga,
					    uint32_t interface_num,
					    uint32_t slot_num, int flags);
fpga_result xfpga_fpgaAssignToInterface(fpga_handle fpga,
					fpga_token accelerator,
					uint32_t host_interface, int flags);
fpga_result xfpga_fpgaReleaseFromInterface(fpga_handle fpga,
					   fpga_token accelerator);
fpga_result xfpga_fpgaReconfigureSlot(fpga_handle fpga, uint32_t slot,
				      const uint8_t *bitstream,
				      size_t bitstream_len, int flags);
fpga_result xfpga_fpgaGetTokenObject(fpga_token token, const char *name,
				     fpga_object *object, int flags);
fpga_result xfpga_fpgaDestroyObject(fpga_object *obj);
fpga_result xfpga_fpgaObjectRead(fpga_object obj, uint8_t *buffer,
				 size_t offset, size_t len, int flags);

#ifdef __cplusplus
}
#endif
#endif // __XFPGA_XFPGA_H__
