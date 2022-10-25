// Copyright(c) 2022, Intel Corporation
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
#ifndef __OPAE_REMOTE_PLUGIN_H__
#define __OPAE_REMOTE_PLUGIN_H__
#include <opae/types.h>
#include "rmt-ifc.h"

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(x) ((void)x)
#endif // UNUSED_PARAM

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

fpga_result remote_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result remote_fpgaClose(fpga_handle handle);
fpga_result remote_fpgaReset(fpga_handle handle);
fpga_result remote_fpgaGetPropertiesFromHandle(fpga_handle handle,
					      fpga_properties *prop);
fpga_result remote_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result remote_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
fpga_result remote_fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint64_t value);
fpga_result remote_fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint64_t *value);
fpga_result remote_fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint32_t value);
fpga_result remote_fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint32_t *value);
fpga_result remote_fpgaWriteMMIO512(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, const void *value);
fpga_result remote_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
			      uint64_t **mmio_ptr);
fpga_result remote_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num);
fpga_result remote_fpgaEnumerate(const fpga_properties *filters,
				uint32_t num_filters, fpga_token *tokens,
				uint32_t max_tokens, uint32_t *num_matches);
fpga_result remote_fpgaCloneToken(fpga_token src, fpga_token *dst);
fpga_result remote_fpgaDestroyToken(fpga_token *token);
fpga_result remote_fpgaGetNumUmsg(fpga_handle handle, uint64_t *value);
fpga_result remote_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value);
fpga_result remote_fpgaTriggerUmsg(fpga_handle handle, uint64_t value);
fpga_result remote_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr);
fpga_result remote_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
				    void **buf_addr, uint64_t *wsid, int flags);
fpga_result remote_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid);
fpga_result remote_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
				   uint64_t *ioaddr);
fpga_result remote_fpgaGetOPAECVersion(fpga_version *version);
fpga_result remote_fpgaGetOPAECVersionString(char *version_str, size_t len);
fpga_result remote_fpgaGetOPAECBuildString(char *build_str, size_t len);
fpga_result remote_fpgaReadError(fpga_token token, uint32_t error_num,
				uint64_t *value);
fpga_result remote_fpgaClearError(fpga_token token, uint32_t error_num);
fpga_result remote_fpgaClearAllErrors(fpga_token token);
fpga_result remote_fpgaGetErrorInfo(fpga_token token, uint32_t error_num,
				   struct fpga_error_info *error_info);
fpga_result remote_fpgaCreateEventHandle(fpga_event_handle *event_handle);
fpga_result remote_fpgaDestroyEventHandle(fpga_event_handle *event_handle);
fpga_result remote_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						 int *fd);
fpga_result remote_fpgaRegisterEvent(fpga_handle handle,
				    fpga_event_type event_type,
				    fpga_event_handle event_handle,
				    uint32_t flags);
fpga_result remote_fpgaUnregisterEvent(fpga_handle handle,
				      fpga_event_type event_type,
				      fpga_event_handle event_handle);
fpga_result remote_fpgaAssignPortToInterface(fpga_handle fpga,
					    uint32_t interface_num,
					    uint32_t slot_num, int flags);
fpga_result remote_fpgaAssignToInterface(fpga_handle fpga,
					fpga_token accelerator,
					uint32_t host_interface, int flags);
fpga_result remote_fpgaReleaseFromInterface(fpga_handle fpga,
					   fpga_token accelerator);
fpga_result remote_fpgaReconfigureSlot(fpga_handle fpga, uint32_t slot,
				      const uint8_t *bitstream,
				      size_t bitstream_len, int flags);
fpga_result remote_fpgaTokenGetObject(fpga_token token, const char *name,
				     fpga_object *object, int flags);
fpga_result remote_fpgaHandleGetObject(fpga_token handle, const char *name,
				      fpga_object *object, int flags);
fpga_result remote_fpgaObjectGetObject(fpga_object parent, const char *name,
				      fpga_object *object, int flags);
fpga_result remote_fpgaObjectGetObjectAt(fpga_object parent, size_t idx,
					fpga_object *object);
fpga_result remote_fpgaDestroyObject(fpga_object *obj);
fpga_result remote_fpgaObjectGetType(fpga_object obj,
				    enum fpga_sysobject_type *type);
fpga_result remote_fpgaObjectGetName(fpga_object obj, char *name,
				    size_t max_len);
fpga_result remote_fpgaObjectGetSize(fpga_object obj, uint32_t *value,
				    int flags);
fpga_result remote_fpgaObjectRead(fpga_object obj, uint8_t *buffer,
				 size_t offset, size_t len, int flags);
fpga_result remote_fpgaObjectRead64(fpga_object obj, uint64_t *value, int flags);
fpga_result remote_fpgaObjectWrite64(fpga_object obj, uint64_t value, int flags);
fpga_result remote_fpgaSetUserClock(fpga_handle handle, uint64_t low_clk,
				   uint64_t high_clk, int flags);
fpga_result remote_fpgaGetUserClock(fpga_handle handle, uint64_t *low_clk,
				   uint64_t *high_clk, int flags);

fpga_result remote_fpgaGetNumMetrics(fpga_handle handle,
				   uint64_t *num_metrics);

fpga_result remote_fpgaGetMetricsInfo(fpga_handle handle,
				   fpga_metric_info *metric_info,
				   uint64_t *num_metrics);

fpga_result remote_fpgaGetMetricsByIndex(fpga_handle handle,
				    uint64_t *metric_num,
				    uint64_t num_metric_indexes,
				    fpga_metric *metrics);

fpga_result remote_fpgaGetMetricsByName(fpga_handle handle,
				    char **metrics_names,
				    uint64_t num_metric_names,
				    fpga_metric *metrics);

fpga_result remote_fpgaGetMetricsThresholdInfo(fpga_handle handle,
			metric_threshold *metric_threshold,
			uint32_t *num_thresholds);

#ifdef __cplusplus
}
#endif // __cplusplus

struct _remote_token {
	fpga_token_header header; //< Must appear at offset 0!
	opae_remote_client_ifc *ifc;
	int json_to_string_flags;
};

struct _remote_token *
opae_create_remote_token(fpga_token_header *hdr,
			 opae_remote_client_ifc *ifc,
			 int json_to_string_flags);

void opae_destroy_remote_token(struct _remote_token *t);

struct _remote_handle {

	struct _remote_token *token;

};

#endif // __OPAE_REMOTE_PLUGIN_H__
