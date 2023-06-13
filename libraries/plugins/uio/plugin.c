// Copyright(c) 2023, Intel Corporation
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

#include <stdlib.h>
#include <dlfcn.h>

#include <opae/types_enum.h>

#include "adapter.h"
#include "opae_uio.h"
#include "cfg-file.h"
#include "mock/opae_std.h"

#ifndef __UIO_API__
#define __UIO_API__
#endif

extern libopae_config_data *opae_u_supported_devices;

int __UIO_API__ uio_plugin_initialize(void)
{
	int res;
	char *raw_cfg = NULL;
	char *cfg_file;

	cfg_file = opae_find_cfg_file();
	if (cfg_file)
		raw_cfg = opae_read_cfg_file(cfg_file);

	opae_u_supported_devices = opae_parse_libopae_config(cfg_file, raw_cfg);

	if (cfg_file) {
		opae_free(cfg_file);
		cfg_file = NULL;
	}

	res = uio_pci_discover();
	if (res) {
		OPAE_ERR("error with uio_pci_discover\n");
	}

	return res;
}

int __UIO_API__ uio_plugin_finalize(void)
{
	uio_free_device_list();

	opae_free_libopae_config(opae_u_supported_devices);
	opae_u_supported_devices = NULL;

	return 0;
}

int __UIO_API__ opae_plugin_configure(opae_api_adapter_table *adapter,
				       const char *jsonConfig)
{
	(void)(jsonConfig);

	adapter->fpgaOpen = dlsym(adapter->plugin.dl_handle, "uio_fpgaOpen");
	adapter->fpgaClose =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaClose");
	adapter->fpgaReset =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaReset");
	adapter->fpgaGetPropertiesFromHandle = dlsym(
		adapter->plugin.dl_handle, "uio_fpgaGetPropertiesFromHandle");
	adapter->fpgaGetProperties =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaGetProperties");
	adapter->fpgaUpdateProperties =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaUpdateProperties");
	adapter->fpgaWriteMMIO64 =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaWriteMMIO64");
	adapter->fpgaReadMMIO64 =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaReadMMIO64");
	adapter->fpgaWriteMMIO32 =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaWriteMMIO32");
	adapter->fpgaReadMMIO32 =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaReadMMIO32");
	adapter->fpgaWriteMMIO512 =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaWriteMMIO512");
	adapter->fpgaMapMMIO =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaMapMMIO");
	adapter->fpgaUnmapMMIO =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaUnmapMMIO");
	adapter->fpgaEnumerate =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaEnumerate");
	adapter->fpgaCloneToken =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaCloneToken");
	adapter->fpgaDestroyToken =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaDestroyToken");
	adapter->fpgaCreateEventHandle =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaCreateEventHandle");
	adapter->fpgaDestroyEventHandle =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaDestroyEventHandle");
	adapter->fpgaGetOSObjectFromEventHandle =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaGetOSObjectFromEventHandle");
	adapter->fpgaRegisterEvent =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaRegisterEvent");
	adapter->fpgaUnregisterEvent =
		dlsym(adapter->plugin.dl_handle, "uio_fpgaUnregisterEvent");

	adapter->initialize =
		dlsym(adapter->plugin.dl_handle, "uio_plugin_initialize");
	adapter->finalize =
		dlsym(adapter->plugin.dl_handle, "uio_plugin_finalize");

	return 0;
}
