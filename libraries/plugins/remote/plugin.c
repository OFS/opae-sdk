// Copyright(c) 2022, Intel Corporation
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

#include "remote.h"
#include "adapter.h"

int __REMOTE_API__ remote_plugin_initialize(void)
{
	return 0;
}

int __REMOTE_API__ remote_plugin_finalize(void)
{
	return 0;
}

int __REMOTE_API__ opae_plugin_configure(opae_api_adapter_table *adapter,
					 const char *jsonConfig)
{
(void) jsonConfig;

	adapter->fpgaOpen = dlsym(adapter->plugin.dl_handle, "remote_fpgaOpen");
	adapter->fpgaClose =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaClose");
	adapter->fpgaReset =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReset");
	adapter->fpgaGetPropertiesFromHandle = dlsym(
		adapter->plugin.dl_handle, "remote_fpgaGetPropertiesFromHandle");
	adapter->fpgaGetProperties =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetProperties");
	adapter->fpgaUpdateProperties =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaUpdateProperties");
	adapter->fpgaWriteMMIO64 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaWriteMMIO64");
	adapter->fpgaReadMMIO64 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReadMMIO64");
	adapter->fpgaWriteMMIO32 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaWriteMMIO32");
	adapter->fpgaReadMMIO32 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReadMMIO32");
	adapter->fpgaWriteMMIO512 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaWriteMMIO512");
	adapter->fpgaMapMMIO =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaMapMMIO");
	adapter->fpgaUnmapMMIO =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaUnmapMMIO");
	adapter->fpgaEnumerate =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaEnumerate");
	adapter->fpgaCloneToken =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaCloneToken");
	adapter->fpgaDestroyToken =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaDestroyToken");
	adapter->fpgaGetNumUmsg =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetNumUmsg");
	adapter->fpgaSetUmsgAttributes =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaSetUmsgAttributes");
	adapter->fpgaTriggerUmsg =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaTriggerUmsg");
	adapter->fpgaGetUmsgPtr =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetUmsgPtr");
	adapter->fpgaPrepareBuffer =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaPrepareBuffer");
	adapter->fpgaReleaseBuffer =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReleaseBuffer");
	adapter->fpgaGetIOAddress =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetIOAddress");
	adapter->fpgaBufMemSet =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaBufMemSet");
	adapter->fpgaBufMemCpyToRemote =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaBufMemCpyToRemote");

	/*
	**	adapter->fpgaGetOPAECVersion = dlsym(adapter->plugin.dl_handle,
	*"remote_fpgaGetOPAECVersion");
	**	adapter->fpgaGetOPAECVersionString =
	*dlsym(adapter->plugin.dl_handle, "remote_fpgaGetOPAECVersionString"); *
	*adapter->fpgaGetOPAECBuildString = dlsym(adapter->plugin.dl_handle,
	*"remote_fpgaGetOPAECBuildString");
	*/
	adapter->fpgaReadError =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReadError");
	adapter->fpgaClearError =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaClearError");
	adapter->fpgaClearAllErrors =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaClearAllErrors");
	adapter->fpgaGetErrorInfo =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetErrorInfo");
	adapter->fpgaCreateEventHandle =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaCreateEventHandle");
	adapter->fpgaDestroyEventHandle = dlsym(adapter->plugin.dl_handle,
						"remote_fpgaDestroyEventHandle");
	adapter->fpgaGetOSObjectFromEventHandle =
		dlsym(adapter->plugin.dl_handle,
		      "remote_fpgaGetOSObjectFromEventHandle");
	adapter->fpgaRegisterEvent =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaRegisterEvent");
	adapter->fpgaUnregisterEvent =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaUnregisterEvent");
	adapter->fpgaAssignPortToInterface = dlsym(
		adapter->plugin.dl_handle, "remote_fpgaAssignPortToInterface");
	adapter->fpgaAssignToInterface =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaAssignToInterface");
	adapter->fpgaReleaseFromInterface = dlsym(
		adapter->plugin.dl_handle, "remote_fpgaReleaseFromInterface");
	/*adapter->fpgaReconfigureSlot =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReconfigureSlot");*/
	adapter->fpgaReconfigureSlotByName =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaReconfigureSlotByName");
	adapter->fpgaTokenGetObject =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaTokenGetObject");
	adapter->fpgaHandleGetObject =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaHandleGetObject");
	adapter->fpgaObjectGetObject =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectGetObject");
	adapter->fpgaObjectGetObjectAt =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectGetObjectAt");
	adapter->fpgaDestroyObject =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaDestroyObject");
	adapter->fpgaObjectRead =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectRead");
	adapter->fpgaObjectRead64 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectRead64");
	adapter->fpgaObjectGetSize =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectGetSize");
	adapter->fpgaObjectGetType =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectGetType");
	adapter->fpgaObjectGetName =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectGetName");
	adapter->fpgaObjectWrite64 =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaObjectWrite64");
	adapter->fpgaSetUserClock =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaSetUserClock");
	adapter->fpgaGetUserClock =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetUserClock");

	adapter->fpgaGetNumMetrics =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetNumMetrics");

	adapter->fpgaGetMetricsInfo =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetMetricsInfo");

	adapter->fpgaGetMetricsByIndex =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetMetricsByIndex");

	adapter->fpgaGetMetricsByName =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetMetricsByName");

	adapter->fpgaGetMetricsThresholdInfo =
		dlsym(adapter->plugin.dl_handle, "remote_fpgaGetMetricsThresholdInfo");

	adapter->initialize =
		dlsym(adapter->plugin.dl_handle, "remote_plugin_initialize");
	adapter->finalize =
		dlsym(adapter->plugin.dl_handle, "remote_plugin_finalize");

	return 0;
}
