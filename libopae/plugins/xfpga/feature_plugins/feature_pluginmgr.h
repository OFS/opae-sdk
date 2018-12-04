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

#ifndef __OPAE_FEATURE_PLUGINMGR_H__
#define __OPAE_FEATURE_PLUGINMGR_H__

#include "feature_adapter.h"

// BBB Feature ID (refer CCI-P spec)
#define FPGA_DMA_BBB 0x2
#define DMA_ID1 "E7E3E958-F2E8-739D-E04C-48C158698187"

#define AFU_DFH_REG 0x0
#define AFU_DFH_NEXT_OFFSET 16
#define AFU_DFH_EOL_OFFSET 40
#define AFU_DFH_TYPE_OFFSET 60

struct DFH {
	union {
		uint64_t csr;
		struct {
			uint64_t id : 12;
			uint64_t  revision : 4;
			uint64_t next_header_offset : 24;
			uint64_t eol : 1;
			uint64_t reserved : 19;
			uint64_t  type : 4;
		};
	};
};

/* Non-zero on failure */
int feature_plugin_mgr_initialize(fpga_handle handle);

/* Non-zero on failure */
int feature_plugin_mgr_finalize_all(void);

/* Iteration stops if callback returns non-zero */
#define OPAE_ENUM_STOP 1
#define OPAE_ENUM_CONTINUE 0
feature_adapter_table *get_feature_plugin_adapter(fpga_guid guid);

void get_guid(uint64_t uuid_lo, uint64_t uuid_hi, fpga_guid *guid);

#endif /* __OPAE_FEATURE_PLUGINMGR_H__ */
