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
#include <opae/feature.h>
#include <opae/dma.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

fpga_result xfpga_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result xfpga_fpgaClose(fpga_handle handle);
fpga_result xfpga_fpgaReset(fpga_handle handle);
fpga_result xfpga_fpgaGetPropertiesFromHandle(fpga_handle handle,
					      fpga_properties *prop);
fpga_result xfpga_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result xfpga_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
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
fpga_result xfpga_fpgaTokenGetObject(fpga_token token, const char *name,
				     fpga_object *object, int flags);
fpga_result xfpga_fpgaHandleGetObject(fpga_token handle, const char *name,
				      fpga_object *object, int flags);
fpga_result xfpga_fpgaObjectGetObject(fpga_object parent, const char *name,
				      fpga_object *object, int flags);
fpga_result xfpga_fpgaDestroyObject(fpga_object *obj);
fpga_result xfpga_fpgaObjectGetSize(fpga_object obj, uint32_t *value,
				    int flags);
fpga_result xfpga_fpgaObjectRead(fpga_object obj, uint8_t *buffer,
				 size_t offset, size_t len, int flags);
fpga_result xfpga_fpgaObjectRead64(fpga_object obj, uint64_t *value, int flags);
fpga_result xfpga_fpgaObjectWrite64(fpga_object obj, uint64_t value, int flags);
fpga_result xfpga_fpgaSetUserClock(fpga_handle handle, uint64_t low_clk,
				   uint64_t high_clk, int flags);
fpga_result xfpga_fpgaGetUserClock(fpga_handle handle, uint64_t *low_clk,
				   uint64_t *high_clk, int flags);

fpga_result xfpga_fpgaGetNumMetrics(fpga_handle handle,
				   uint64_t *num_metrics);

fpga_result xfpga_fpgaGetMetricsInfo(fpga_handle handle,
				   fpga_metric_info *metric_info,
				   uint64_t *num_metrics);

fpga_result xfpga_fpgaGetMetricsByIndex(fpga_handle handle,
				    uint64_t *metric_num,
				    uint64_t num_metric_indexes,
				    fpga_metric *metrics);

fpga_result xfpga_fpgaGetMetricsByName(fpga_handle handle,
				    char **metrics_names,
				    uint64_t num_metric_names,
				    fpga_metric *metrics);

fpga_result xfpga_fpgaCloneFeatureToken(fpga_feature_token src,
					fpga_feature_token *dst);

fpga_result xfpga_fpgaFeatureEnumerate(fpga_handle handle,
					fpga_feature_properties *prop,
					fpga_feature_token *tokens,
					uint32_t max_tokens,
					uint32_t *num_matches);

fpga_result xfpga_fpgaDestroyFeatureToken(fpga_feature_token *token);
fpga_result xfpga_fpgaFeaturePropertiesGet(fpga_feature_token token,
					fpga_feature_properties *prop);
fpga_result xfpga_fpgaFeatureOpen(fpga_feature_token token,
					int flags,
					void *priv_config,
					fpga_feature_handle *handle);
fpga_result xfpga_fpgaFeatureClose(fpga_feature_handle handle);
fpga_result xfpga_fpgaDMAPropertiesGet(fpga_feature_token token,
					fpga_dma_properties *prop);
fpga_result xfpga_fpgaDMATransferSync(fpga_feature_handle dma_h,
					dma_transfer_list *xfer_list);
fpga_result xfpga_fpgaDMATransferAsync(fpga_feature_handle dma_h,
					dma_transfer_list *dma_xfer,
					fpga_dma_cb cb,
					void *context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __XFPGA_XFPGA_H__
