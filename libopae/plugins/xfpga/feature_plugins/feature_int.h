// Copyright(c) 2018-2019, Intel Corporation
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

#ifndef __FEATURE_INT_H__
#define __FEATURE_INT_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <uuid/uuid.h>

#include <opae/types.h>
#include <opae/log.h>
#include <opae/feature.h>
#include <opae/mmio.h>
#include <opae/dma.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#include <pthread.h>

#include "feature_adapter.h"

#define UNUSED_PARAM(x) ((void)x)

typedef struct _feature_adapter_table feature_adapter_table;
//                                  f e a t
#define FPGA_WRAPPED_FEATURE_TOKEN_MAGIC 0x66656174
#define FPGA_FEATURE_TOKEN_MAGIC 0x66656178

/** Device-wide unique FPGA feature resource identifier */
struct _fpga_feature_token {
	uint64_t magic;
	uint32_t feature_type;
	uint32_t mmio_num;
	fpga_guid feature_guid;
	uint64_t csr_offset;
	fpga_handle handle;
	struct _fpga_feature_token *next;
};

typedef struct _wrapped_feature_token {
	uint64_t magic;
	fpga_feature_token feature_token;
	feature_adapter_table *ftr_adapter_table;
} wrapped_feature_token;

wrapped_feature_token *
allocate_wrapped_feature_token(fpga_feature_token token,
			    const feature_adapter_table *adapter);

static inline wrapped_feature_token *validate_wrapped_feature_token(fpga_feature_token t)
{
	wrapped_feature_token *wt;
	if (!t)
		return NULL;
	wt = (wrapped_feature_token *)t;
	return (wt->magic == FPGA_WRAPPED_FEATURE_TOKEN_MAGIC) ? wt : NULL;
}

static inline void destroy_wrapped_feature_token(wrapped_feature_token *wt)
{
	wt->magic = 0;
	free(wt);
}
//                                   f e a h
#define FPGA_WRAPPED_FEATURE_HANDLE_MAGIC 0x66656168
#define FPGA_FEATURE_HANDLE_MAGIC 0x66656170


/** Process-wide unique FPGA feature handle */
typedef struct _wrapped_feature_handle {
	uint32_t magic;
	wrapped_feature_token *wrapped_feature_token;
	fpga_feature_handle feature_handle;
	feature_adapter_table *ftr_adapter_table;
} wrapped_feature_handle;

wrapped_feature_handle *
allocate_wrapped_feature_handle(wrapped_feature_token *t, fpga_feature_handle feature_handle,
			     feature_adapter_table *adapter);

static inline wrapped_feature_handle *validate_wrapped_feature_handle(fpga_feature_handle h)
{
	wrapped_feature_handle *wh;
	if (!h)
		return NULL;
	wh = (wrapped_feature_handle *)h;
	return (wh->magic == FPGA_WRAPPED_FEATURE_HANDLE_MAGIC) ? wh : NULL;
}

static inline void destroy_wrapped_feature_handle(wrapped_feature_handle *wh)
{
	wh->magic = 0;
	free(wh);
}

#endif //__FEATURE_INT_H__
