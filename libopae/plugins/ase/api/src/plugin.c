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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <dlfcn.h>
#include "common_int.h"
#include "adapter.h"

int __FPGA_API__ ase_plugin_initialize(void)
{
	return 0;
}

int __FPGA_API__ ase_plugin_finalize(void)
{
	return 0;
}

bool __FPGA_API__ ase_plugin_supports_device(const char *device_type)
{
	UNUSED_PARAM(device_type);
	return true;
}

bool __FPGA_API__ ase_plugin_supports_host(const char *hostname)
{
	UNUSED_PARAM(hostname);
	return true;
}

int __FPGA_API__ opae_plugin_configure(opae_api_adapter_table *adapter,
				       const char *jsonConfig)
{
	UNUSED_PARAM(jsonConfig);

	adapter->fpgaOpen = dlsym(adapter->plugin.dl_handle, "ase_fpgaOpen");
	adapter->fpgaClose =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaClose");
	adapter->fpgaReset =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReset");
	adapter->fpgaGetPropertiesFromHandle = dlsym(
		adapter->plugin.dl_handle, "ase_fpgaGetPropertiesFromHandle");
	adapter->fpgaGetProperties =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetProperties");
	adapter->fpgaUpdateProperties =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaUpdateProperties");
	adapter->fpgaWriteMMIO64 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaWriteMMIO64");
	adapter->fpgaReadMMIO64 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReadMMIO64");
	adapter->fpgaWriteMMIO32 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaWriteMMIO32");
	adapter->fpgaReadMMIO32 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReadMMIO32");
	adapter->fpgaMapMMIO =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaMapMMIO");
	adapter->fpgaUnmapMMIO =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaUnmapMMIO");
	adapter->fpgaEnumerate =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaEnumerate");
	adapter->fpgaCloneToken =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaCloneToken");
	adapter->fpgaDestroyToken =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaDestroyToken");
	adapter->fpgaGetNumUmsg =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetNumUmsg");
	adapter->fpgaSetUmsgAttributes =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaSetUmsgAttributes");
	adapter->fpgaTriggerUmsg =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaTriggerUmsg");
	adapter->fpgaGetUmsgPtr =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetUmsgPtr");
	adapter->fpgaPrepareBuffer =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaPrepareBuffer");
	adapter->fpgaReleaseBuffer =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReleaseBuffer");
	adapter->fpgaGetIOAddress =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetIOAddress");
	/*
	**	adapter->fpgaGetOPAECVersion = dlsym(adapter->plugin.dl_handle,
	*"ase_fpgaGetOPAECVersion");
	**	adapter->fpgaGetOPAECVersionString =
	*dlsym(adapter->plugin.dl_handle, "ase_fpgaGetOPAECVersionString"); *
	*adapter->fpgaGetOPAECBuildString = dlsym(adapter->plugin.dl_handle,
	*"ase_fpgaGetOPAECBuildString");
	*/
	adapter->fpgaReadError =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReadError");
	adapter->fpgaClearError =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaClearError");
	adapter->fpgaClearAllErrors =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaClearAllErrors");
	adapter->fpgaGetErrorInfo =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetErrorInfo");
	adapter->fpgaCreateEventHandle =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaCreateEventHandle");
	adapter->fpgaDestroyEventHandle = dlsym(adapter->plugin.dl_handle,
						"ase_fpgaDestroyEventHandle");
	adapter->fpgaGetOSObjectFromEventHandle =
		dlsym(adapter->plugin.dl_handle,
		      "ase_fpgaGetOSObjectFromEventHandle");
	adapter->fpgaRegisterEvent =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaRegisterEvent");
	adapter->fpgaUnregisterEvent =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaUnregisterEvent");
	adapter->fpgaAssignPortToInterface = dlsym(
		adapter->plugin.dl_handle, "ase_fpgaAssignPortToInterface");
	adapter->fpgaAssignToInterface =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaAssignToInterface");
	adapter->fpgaReleaseFromInterface = dlsym(
		adapter->plugin.dl_handle, "ase_fpgaReleaseFromInterface");
	adapter->fpgaReconfigureSlot =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaReconfigureSlot");
	adapter->fpgaTokenGetObject =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaTokenGetObject");
	adapter->fpgaHandleGetObject =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaHandleGetObject");
	adapter->fpgaObjectGetObject =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaObjectGetObject");
	adapter->fpgaDestroyObject =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaDestroyObject");
	adapter->fpgaObjectRead =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaObjectRead");
	adapter->fpgaObjectRead64 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaObjectRead64");
	adapter->fpgaObjectGetSize =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaObjectGetSize");
	adapter->fpgaObjectWrite64 =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaObjectWrite64");
	adapter->fpgaSetUserClock =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaSetUserClock");
	adapter->fpgaGetUserClock =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetUserClock");

	adapter->initialize =
		dlsym(adapter->plugin.dl_handle, "ase_plugin_initialize");
	adapter->finalize =
		dlsym(adapter->plugin.dl_handle, "ase_plugin_finalize");

	adapter->supports_device = dlsym(adapter->plugin.dl_handle,
					 "ase_plugin_supports_device");
	adapter->supports_host =
		dlsym(adapter->plugin.dl_handle, "ase_plugin_supports_host");

	adapter->fpgaGetNumMetrics =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetNumMetrics");

	adapter->fpgaGetMetricsInfo =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetMetricsInfo");

	adapter->fpgaGetMetricsByIndex =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetMetricsByIndex");

	adapter->fpgaGetMetricsByName =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetMetricsByName");

	adapter->fpgaGetMetricsThresholdInfo =
		dlsym(adapter->plugin.dl_handle, "ase_fpgaGetMetricsThresholdInfo");

	return 0;
}

