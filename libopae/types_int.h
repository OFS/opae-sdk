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

#ifndef __OPAE_TYPES_INT_H__
#define __OPAE_TYPES_INT_H__

#include <stdint.h>

#include <opae/types.h>

#include "adapter.h"

typedef struct _opae_api_adapter_table opae_api_adapter_table;

#define OPAE_WRAPPED_TOKEN_MAGIC 0x00000001

typedef struct _opae_wrapped_token {
	uint32_t magic;
	fpga_token opae_token;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_token;

inline opae_wrapped_token * opae_validate_wrapped_token(fpga_token t)
{
	opae_wrapped_token *wt;
	if (!t)
		return NULL;
	wt = (opae_wrapped_token *) t;
	return (wt->magic == OPAE_WRAPPED_TOKEN_MAGIC) ? wt : NULL;
}

#define OPAE_WRAPPED_HANDLE_MAGIC 0x00000002

typedef struct _opae_wrapped_handle {
	uint32_t magic;
	opae_wrapped_token wrapped_token;
	fpga_handle opae_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_handle;

inline opae_wrapped_handle * opae_validate_wrapped_handle(fpga_handle h)
{
	opae_wrapped_handle *wh;
	if (!h)
		return NULL;
	wh = (opae_wrapped_handle *) h;
	return (wh->magic == OPAE_WRAPPED_HANDLE_MAGIC) ? wh : NULL;
}

#endif /* __OPAE_TYPES_INT_H__ */
