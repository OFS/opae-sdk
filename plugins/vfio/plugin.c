// Copyright(c) 2020, Intel Corporation
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
#include "opae_vfio.h"


int __XFPGA_API__ vfio_plugin_initialize(void)
{
	int res = pci_discover();
	if (res) {
		printf("error with pci_discover\n");
	}
	res = features_discover();
	if (res) {
		printf("error discovering features\n");
	}

	return res;
}

int __XFPGA_API__ vfio_plugin_finalize(void)
{
	free_token_list();
	free_device_list();
	return 0;
}

bool __XFPGA_API__ vfio_plugin_supports_device(const char *device_type)
{
	(void)(device_type);
	return true;
}

bool __XFPGA_API__ vfio_plugin_supports_host(const char *hostname)
{
	(void)(hostname);
	return true;
}

int __XFPGA_API__ opae_plugin_configure(opae_api_adapter_table *adapter,
				       const char *jsonConfig)
{
	(void)(jsonConfig);

	adapter->fpgaOpen = dlsym(adapter->plugin.dl_handle, "vfio_fpgaOpen");
	adapter->fpgaClose =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaClose");
	adapter->fpgaReset =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReset");
	adapter->fpgaGetPropertiesFromHandle = dlsym(
		adapter->plugin.dl_handle, "vfio_fpgaGetPropertiesFromHandle");
	adapter->fpgaGetProperties =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetProperties");
	adapter->fpgaUpdateProperties =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaUpdateProperties");
	adapter->fpgaWriteMMIO64 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaWriteMMIO64");
	adapter->fpgaReadMMIO64 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReadMMIO64");
	adapter->fpgaWriteMMIO32 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaWriteMMIO32");
	adapter->fpgaReadMMIO32 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReadMMIO32");
	adapter->fpgaWriteMMIO512 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaWriteMMIO512");
	adapter->fpgaMapMMIO =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaMapMMIO");
	adapter->fpgaUnmapMMIO =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaUnmapMMIO");
	adapter->fpgaEnumerate =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaEnumerate");
	adapter->fpgaCloneToken =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaCloneToken");
	adapter->fpgaDestroyToken =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaDestroyToken");
	adapter->fpgaGetNumUmsg =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetNumUmsg");
	adapter->fpgaSetUmsgAttributes =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaSetUmsgAttributes");
	adapter->fpgaTriggerUmsg =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaTriggerUmsg");
	adapter->fpgaGetUmsgPtr =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetUmsgPtr");
	adapter->fpgaPrepareBuffer =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaPrepareBuffer");
	adapter->fpgaReleaseBuffer =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReleaseBuffer");
	adapter->fpgaGetIOAddress =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetIOAddress");
	/*
	**	adapter->fpgaGetOPAECVersion = dlsym(adapter->plugin.dl_handle,
	*"vfio_fpgaGetOPAECVersion");
	**	adapter->fpgaGetOPAECVersionString =
	*dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetOPAECVersionString"); *
	*adapter->fpgaGetOPAECBuildString = dlsym(adapter->plugin.dl_handle,
	*"vfio_fpgaGetOPAECBuildString");
	*/
	adapter->fpgaReadError =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReadError");
	adapter->fpgaClearError =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaClearError");
	adapter->fpgaClearAllErrors =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaClearAllErrors");
	adapter->fpgaGetErrorInfo =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetErrorInfo");
	adapter->fpgaCreateEventHandle =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaCreateEventHandle");
	adapter->fpgaDestroyEventHandle = dlsym(adapter->plugin.dl_handle,
						"vfio_fpgaDestroyEventHandle");
	adapter->fpgaGetOSObjectFromEventHandle =
		dlsym(adapter->plugin.dl_handle,
		      "vfio_fpgaGetOSObjectFromEventHandle");
	adapter->fpgaRegisterEvent =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaRegisterEvent");
	adapter->fpgaUnregisterEvent =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaUnregisterEvent");
	adapter->fpgaAssignPortToInterface = dlsym(
		adapter->plugin.dl_handle, "vfio_fpgaAssignPortToInterface");
	adapter->fpgaAssignToInterface =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaAssignToInterface");
	adapter->fpgaReleaseFromInterface = dlsym(
		adapter->plugin.dl_handle, "vfio_fpgaReleaseFromInterface");
	adapter->fpgaReconfigureSlot =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaReconfigureSlot");
	adapter->fpgaTokenGetObject =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaTokenGetObject");
	adapter->fpgaHandleGetObject =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaHandleGetObject");
	adapter->fpgaObjectGetObject =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectGetObject");
	adapter->fpgaObjectGetObjectAt =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectGetObjectAt");
	adapter->fpgaDestroyObject =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaDestroyObject");
	adapter->fpgaObjectRead =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectRead");
	adapter->fpgaObjectRead64 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectRead64");
	adapter->fpgaObjectGetSize =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectGetSize");
	adapter->fpgaObjectGetType =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectGetType");
	adapter->fpgaObjectWrite64 =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaObjectWrite64");
	adapter->fpgaSetUserClock =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaSetUserClock");
	adapter->fpgaGetUserClock =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetUserClock");

	adapter->initialize =
		dlsym(adapter->plugin.dl_handle, "vfio_plugin_initialize");
	adapter->finalize =
		dlsym(adapter->plugin.dl_handle, "vfio_plugin_finalize");

	adapter->supports_device = dlsym(adapter->plugin.dl_handle,
					 "vfio_plugin_supports_device");
	adapter->supports_host =
		dlsym(adapter->plugin.dl_handle, "vfio_plugin_supports_host");

	adapter->fpgaGetNumMetrics =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetNumMetrics");

	adapter->fpgaGetMetricsInfo =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetMetricsInfo");

	adapter->fpgaGetMetricsByIndex =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetMetricsByIndex");

	adapter->fpgaGetMetricsByName =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetMetricsByName");

	adapter->fpgaGetMetricsThresholdInfo =
		dlsym(adapter->plugin.dl_handle, "vfio_fpgaGetMetricsThresholdInfo");

	return 0;
}

