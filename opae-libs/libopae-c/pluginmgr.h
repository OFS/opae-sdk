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

#ifndef __OPAE_PLUGINMGR_H__
#define __OPAE_PLUGINMGR_H__

#include "adapter.h"

// non-zero on failure.
int opae_plugin_mgr_initialize(const char *cfg_file);

// non-zero on failure.
int opae_plugin_mgr_finalize_all(void);

// iteration stops if callback returns non-zero.
#define OPAE_ENUM_STOP 1
#define OPAE_ENUM_CONTINUE 0
int opae_plugin_mgr_for_each_adapter(
	int (*callback)(const opae_api_adapter_table *, void *), void *context);

#define PLUGIN_SUPPORTED_DEVICES_MAX 256
#define PLUGIN_NAME_MAX 64
typedef struct _plugin_cfg {
	char name[PLUGIN_NAME_MAX];
	char plugin[PLUGIN_NAME_MAX];
	bool enabled;
	char *cfg;
	size_t cfg_size;
	uint32_t supported_devices[PLUGIN_SUPPORTED_DEVICES_MAX];
	struct _plugin_cfg *next;
} plugin_cfg;

#endif /* __OPAE_PLUGINMGR_H__ */
