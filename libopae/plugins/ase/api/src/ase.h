// Copyright(c) 2019, Intel Corporation
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

#ifndef __ASE_FPGA_H__
#define __ASE_FPGA_H__

#include <stdint.h>

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

fpga_result ase_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result ase_fpgaClose(fpga_handle handle);
fpga_result ase_fpgaReset(fpga_handle handle);
fpga_result ase_fpgaGetPropertiesFromHandle(fpga_handle handle,
					      fpga_properties *prop);
fpga_result ase_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result ase_fpgaDestroyProperties(fpga_properties *prop);
fpga_result ase_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
fpga_result ase_fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint64_t value);
fpga_result ase_fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint64_t *value);
fpga_result ase_fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
				  uint64_t offset, uint32_t value);
fpga_result ase_fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
				 uint64_t offset, uint32_t *value);
fpga_result ase_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
			      uint64_t **mmio_ptr);
fpga_result ase_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num);
fpga_result ase_fpgaEnumerate(const fpga_properties *filters,
				uint32_t num_filters, fpga_token *tokens,
				uint32_t max_tokens, uint32_t *num_matches);
fpga_result ase_fpgaCloneToken(fpga_token src, fpga_token *dst);
fpga_result ase_fpgaDestroyToken(fpga_token *token);
fpga_result ase_fpgaGetNumUmsg(fpga_handle handle, uint64_t *value);
fpga_result ase_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value);
fpga_result ase_fpgaTriggerUmsg(fpga_handle handle, uint64_t value);
fpga_result ase_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr);
fpga_result ase_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
				    void **buf_addr, uint64_t *wsid, int flags);
fpga_result ase_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid);
fpga_result ase_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
				   uint64_t *ioaddr);
fpga_result ase_fpgaGetOPAECVersion(fpga_version *version);
fpga_result ase_fpgaGetOPAECVersionString(char *version_str, size_t len);
fpga_result ase_fpgaGetOPAECBuildString(char *build_str, size_t len);
fpga_result ase_fpgaReadError(fpga_token token, uint32_t error_num,
				uint64_t *value);
fpga_result ase_fpgaClearError(fpga_token token, uint32_t error_num);
fpga_result ase_fpgaClearAllErrors(fpga_token token);
fpga_result ase_fpgaGetErrorInfo(fpga_token token, uint32_t error_num,
				   struct fpga_error_info *error_info);
fpga_result ase_fpgaCreateEventHandle(fpga_event_handle *event_handle);
fpga_result ase_fpgaDestroyEventHandle(fpga_event_handle *event_handle);
fpga_result ase_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						 int *fd);
fpga_result ase_fpgaRegisterEvent(fpga_handle handle,
				    fpga_event_type event_type,
				    fpga_event_handle event_handle,
				    uint32_t flags);
fpga_result ase_fpgaUnregisterEvent(fpga_handle handle,
				      fpga_event_type event_type,
				      fpga_event_handle event_handle);
fpga_result ase_fpgaAssignPortToInterface(fpga_handle fpga,
					    uint32_t interface_num,
					    uint32_t slot_num, int flags);
fpga_result ase_fpgaAssignToInterface(fpga_handle fpga,
					fpga_token accelerator,
					uint32_t host_interface, int flags);
fpga_result ase_fpgaReleaseFromInterface(fpga_handle fpga,
					   fpga_token accelerator);
fpga_result ase_fpgaReconfigureSlot(fpga_handle fpga, uint32_t slot,
				      const uint8_t *bitstream,
				      size_t bitstream_len, int flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __ASE_FPGA_H__
