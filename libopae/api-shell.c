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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "types_int.h"

fpga_result fpgaOpen(fpga_token token,
		     fpga_handle *handle,
		     int flags)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaClose(fpga_handle handle)
{
	opae_wrapped_handle *wh = opae_validate_wrapped_handle(handle);

	if (!wh)
		return FPGA_INVALID_PARAM;

	if (wh->adapter_table->fpgaClose)
		return wh->adapter_table->fpgaClose(wh->opae_handle);

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReset(fpga_handle handle)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetPropertiesFromHandle(fpga_handle handle,
					fpga_properties *prop)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetProperties(fpga_token token,
			      fpga_properties *prop)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaUpdateProperties(fpga_token token,
				 fpga_properties prop)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaClearProperties(fpga_properties prop)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaCloneProperties(fpga_properties src,
				fpga_properties *dst)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaDestroyProperties(fpga_properties *prop)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetParent(const fpga_properties prop,
				    fpga_token *parent)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetParent(fpga_properties prop,
				    fpga_token parent)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetObjectType(const fpga_properties prop,
					fpga_objtype *objtype)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetObjectType(fpga_properties prop,
					fpga_objtype objtype)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetSegment(const fpga_properties prop,
				     uint16_t *segment)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetSegment(fpga_properties prop,
				     uint16_t segment)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetBus(const fpga_properties prop,
				 uint8_t *bus)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetBus(fpga_properties prop,
				 uint8_t bus)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetDevice(const fpga_properties prop,
				    uint8_t *device)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetDevice(fpga_properties prop,
				    uint8_t device)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetFunction(const fpga_properties prop,
				      uint8_t *function)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetFunction(fpga_properties prop,
				      uint8_t function)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetSocketID(const fpga_properties prop,
				      uint8_t *socket_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetSocketID(fpga_properties prop,
				      uint8_t socket_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetDeviceID(const fpga_properties prop,
				      uint16_t *device_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetDeviceID(fpga_properties prop,
				      uint16_t device_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetNumSlots(const fpga_properties prop,
				      uint32_t *num_slots)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetNumSlots(fpga_properties prop,
				      uint32_t num_slots)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetBBSID(const fpga_properties prop,
				   uint64_t *bbs_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetBBSID(fpga_properties prop,
				   uint64_t bbs_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetBBSVersion(const fpga_properties prop,
					fpga_version *bbs_version)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetBBSVersion(fpga_properties prop,
					fpga_version version)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetVendorID(const fpga_properties prop,
				      uint16_t *vendor_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetVendorID(fpga_properties prop,
				      uint16_t vendor_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetModel(const fpga_properties prop,
				   char *model)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetModel(fpga_properties prop,
				   char *model)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetLocalMemorySize(const fpga_properties prop,
					     uint64_t *lms)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetLocalMemorySize(fpga_properties prop,
					     uint64_t lms)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetCapabilities(const fpga_properties prop,
					  uint64_t *capabilities)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetCapabilities(fpga_properties prop,
					  uint64_t capabilities)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetGUID(const fpga_properties prop,
				  fpga_guid *guid)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetGUID(fpga_properties prop,
				  fpga_guid guid)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetNumMMIO(const fpga_properties prop,
				     uint32_t *mmio_spaces)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetNumMMIO(fpga_properties prop,
				     uint32_t mmio_spaces)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetNumInterrupts(const fpga_properties prop,
					   uint32_t *num_interrupts)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetNumInterrupts(fpga_properties prop,
					   uint32_t num_interrupts)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetAcceleratorState(const fpga_properties prop,
					      fpga_accelerator_state *state)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetAcceleratorState(fpga_properties prop,
					      fpga_accelerator_state state)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetObjectID(const fpga_properties prop,
				      uint64_t *object_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetObjectID(const fpga_properties prop,
				      uint64_t object_id)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesGetNumErrors(const fpga_properties prop,
				       uint32_t *num_errors)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPropertiesSetNumErrors(const fpga_properties prop,
				       uint32_t num_errors)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaWriteMMIO64(fpga_handle handle,
			    uint32_t mmio_num,
			    uint64_t offset,
			    uint64_t value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReadMMIO64(fpga_handle handle,
			   uint32_t mmio_num,
			   uint64_t offset,
			   uint64_t *value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaWriteMMIO32(fpga_handle handle,
			    uint32_t mmio_num,
			    uint64_t offset,
			    uint32_t value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReadMMIO32(fpga_handle handle,
			   uint32_t mmio_num,
			   uint64_t offset,
			   uint32_t *value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaMapMMIO(fpga_handle handle,
			uint32_t mmio_num,
			uint64_t **mmio_ptr)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaUnmapMMIO(fpga_handle handle,
			  uint32_t mmio_num)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaEnumerate(const fpga_properties *filters,
			  uint32_t num_filters,
			  fpga_token *tokens,
			  uint32_t max_tokens,
			  uint32_t *num_matches)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaCloneToken(fpga_token src,
			   fpga_token *dst)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaDestroyToken(fpga_token *token)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetNumUmsg(fpga_handle handle,
			   uint64_t *value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaSetUmsgAttributes(fpga_handle handle,
				  uint64_t value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaTriggerUmsg(fpga_handle handle,
			    uint64_t value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetUmsgPtr(fpga_handle handle,
			   uint64_t **umsg_ptr)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaPrepareBuffer(fpga_handle handle,
			      uint64_t len,
			      void **buf_addr,
			      uint64_t *wsid,
			      int flags)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReleaseBuffer(fpga_handle handle,
			      uint64_t wsid)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetIOAddress(fpga_handle handle,
			     uint64_t wsid,
			     uint64_t *ioaddr)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetOPAECVersion(fpga_version *version)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetOPAECVersionString(char *version_str,
				      size_t len)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetOPAECBuildString(char *build_str,
				    size_t len)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReadError(fpga_token token,
			  uint32_t error_num,
			  uint64_t *value)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaClearError(fpga_token token,
			   uint32_t error_num)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaClearAllErrors(fpga_token token)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info)
{

	return FPGA_NOT_SUPPORTED;
}

const char * fpgaErrStr(fpga_result e)
{

	return 0;
}

fpga_result fpgaCreateEventHandle(fpga_event_handle *event_handle)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
					   int *fd)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaRegisterEvent(fpga_handle handle,
			      fpga_event_type event_type,
			      fpga_event_handle event_handle,
			      uint32_t flags)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaUnregisterEvent(fpga_handle handle,
				fpga_event_type event_type,
				fpga_event_handle event_handle)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaAssignPortToInterface(fpga_handle fpga,
				      uint32_t interface_num,
				      uint32_t slot_num,
				      int flags)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaAssignToInterface(fpga_handle fpga,
				  fpga_token accelerator,
				  uint32_t host_interface,
				  int flags)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReleaseFromInterface(fpga_handle fpga,
				     fpga_token accelerator)
{

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaReconfigureSlot(fpga_handle fpga,
				uint32_t slot,
				const uint8_t *bitstream,
				size_t bitstream_len,
				int flags)
{

	return FPGA_NOT_SUPPORTED;
}
