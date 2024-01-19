// Copyright(c) 2024, Intel Corporation
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

//
// Multi-ported AFUs have a parent AFU that names child AFUs by GUID. These
// functions apply operations to all childen of a parent AFU.
//

#ifndef __OPAE_MULTI_PORT_AFU_H__
#define __OPAE_MULTI_PORT_AFU_H__

#include <stdint.h>
#include <opae/types.h>

fpga_result afu_open_children(opae_wrapped_handle *wrapped_parent_handle);
fpga_result afu_close_children(opae_wrapped_handle *wrapped_parent_handle);
fpga_result afu_pin_buffer(opae_wrapped_handle *wrapped_parent_handle,
			   void *buf_addr, uint64_t len, uint64_t wsid);
fpga_result afu_unpin_buffer(opae_wrapped_handle *wrapped_parent_handle,
			     uint64_t wsid);

#endif // __OPAE_MULTI_PORT_AFU_H__
