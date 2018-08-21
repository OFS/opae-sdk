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

#include <opae/fpga.h>

#include "adapter.h"

int opae_plugin_initialize(void)
{

	return 0;
}

int opae_plugin_finalize(void)
{

	return 0;
}

bool opae_plugin_supports_device(const char *device_type)
{

	return true;
}

bool opae_plugin_supports_host(const char *hostname)
{

	return true;
}


int opae_plugin_configure(opae_api_adapter_table *adapter,
			  const char *jsonConfig)
{
	adapter->fpgaOpen = fpgaOpen;
	adapter->fpgaClose = fpgaClose;
	adapter->fpgaReset = fpgaReset;
	adapter->fpgaGetPropertiesFromHandle = fpgaGetPropertiesFromHandle;
	adapter->fpgaGetProperties = fpgaGetProperties;
	adapter->fpgaUpdateProperties = fpgaUpdateProperties;
	adapter->fpgaClearProperties = fpgaClearProperties;
	adapter->fpgaCloneProperties = fpgaCloneProperties;
	adapter->fpgaDestroyProperties = fpgaDestroyProperties;
	adapter->fpgaPropertiesGetParent = fpgaPropertiesGetParent;
	adapter->fpgaPropertiesSetParent = fpgaPropertiesSetParent;
	adapter->fpgaPropertiesGetObjectType = fpgaPropertiesGetObjectType;
	adapter->fpgaPropertiesSetObjectType = fpgaPropertiesSetObjectType;
	adapter->fpgaPropertiesGetSegment = fpgaPropertiesGetSegment;
	adapter->fpgaPropertiesSetSegment = fpgaPropertiesSetSegment;
	adapter->fpgaPropertiesGetBus = fpgaPropertiesGetBus;
	adapter->fpgaPropertiesSetBus = fpgaPropertiesSetBus;
	adapter->fpgaPropertiesGetDevice = fpgaPropertiesGetDevice;
	adapter->fpgaPropertiesSetDevice = fpgaPropertiesSetDevice;
	adapter->fpgaPropertiesGetFunction = fpgaPropertiesGetFunction;
	adapter->fpgaPropertiesSetFunction = fpgaPropertiesSetFunction;
	adapter->fpgaPropertiesGetSocketID = fpgaPropertiesGetSocketID;
	adapter->fpgaPropertiesSetSocketID = fpgaPropertiesSetSocketID;
	adapter->fpgaPropertiesGetDeviceID = fpgaPropertiesGetDeviceID;
	adapter->fpgaPropertiesSetDeviceID = fpgaPropertiesSetDeviceID;
	adapter->fpgaPropertiesGetNumSlots = fpgaPropertiesGetNumSlots;
	adapter->fpgaPropertiesSetNumSlots = fpgaPropertiesSetNumSlots;
	adapter->fpgaPropertiesGetBBSID = fpgaPropertiesGetBBSID;
	adapter->fpgaPropertiesSetBBSID = fpgaPropertiesSetBBSID;
	adapter->fpgaPropertiesGetBBSVersion = fpgaPropertiesGetBBSVersion;
	adapter->fpgaPropertiesSetBBSVersion = fpgaPropertiesSetBBSVersion;
	adapter->fpgaPropertiesGetVendorID = fpgaPropertiesGetVendorID;
	adapter->fpgaPropertiesSetVendorID = fpgaPropertiesSetVendorID;
	adapter->fpgaPropertiesGetModel = fpgaPropertiesGetModel;
	adapter->fpgaPropertiesSetModel = fpgaPropertiesSetModel;
	adapter->fpgaPropertiesGetLocalMemorySize =
		fpgaPropertiesGetLocalMemorySize;
	adapter->fpgaPropertiesSetLocalMemorySize =
		fpgaPropertiesSetLocalMemorySize;
	adapter->fpgaPropertiesGetCapabilities = fpgaPropertiesGetCapabilities;
	adapter->fpgaPropertiesSetCapabilities = fpgaPropertiesSetCapabilities;
	adapter->fpgaPropertiesGetGUID = fpgaPropertiesGetGUID;
	adapter->fpgaPropertiesSetGUID = fpgaPropertiesSetGUID;
	adapter->fpgaPropertiesGetNumMMIO = fpgaPropertiesGetNumMMIO;
	adapter->fpgaPropertiesSetNumMMIO = fpgaPropertiesSetNumMMIO;
	adapter->fpgaPropertiesGetNumInterrupts =
		fpgaPropertiesGetNumInterrupts;
	adapter->fpgaPropertiesSetNumInterrupts =
		fpgaPropertiesSetNumInterrupts;
	adapter->fpgaPropertiesGetAcceleratorState =
		fpgaPropertiesGetAcceleratorState;
	adapter->fpgaPropertiesSetAcceleratorState =
		fpgaPropertiesSetAcceleratorState;
	adapter->fpgaPropertiesGetObjectID = fpgaPropertiesGetObjectID;
	adapter->fpgaPropertiesSetObjectID = fpgaPropertiesSetObjectID;
	adapter->fpgaPropertiesGetNumErrors = fpgaPropertiesGetNumErrors;
	adapter->fpgaPropertiesSetNumErrors = fpgaPropertiesSetNumErrors;
	adapter->fpgaWriteMMIO64 = fpgaWriteMMIO64;
	adapter->fpgaReadMMIO64 = fpgaReadMMIO64;
	adapter->fpgaWriteMMIO32 = fpgaWriteMMIO32;
	adapter->fpgaReadMMIO32 = fpgaReadMMIO32;
	adapter->fpgaMapMMIO = fpgaMapMMIO;
	adapter->fpgaUnmapMMIO = fpgaUnmapMMIO;
	adapter->fpgaEnumerate = fpgaEnumerate;
	adapter->fpgaCloneToken = fpgaCloneToken;
	adapter->fpgaDestroyToken = fpgaDestroyToken;
	adapter->fpgaGetNumUmsg = fpgaGetNumUmsg;
	adapter->fpgaSetUmsgAttributes = fpgaSetUmsgAttributes;
	adapter->fpgaTriggerUmsg = fpgaTriggerUmsg;
	adapter->fpgaGetUmsgPtr = fpgaGetUmsgPtr;
	adapter->fpgaPrepareBuffer = fpgaPrepareBuffer;
	adapter->fpgaReleaseBuffer = fpgaReleaseBuffer;
	adapter->fpgaGetIOAddress = fpgaGetIOAddress;
	adapter->fpgaGetOPAECVersion = fpgaGetOPAECVersion;
	adapter->fpgaGetOPAECVersionString = fpgaGetOPAECVersionString;
	adapter->fpgaGetOPAECBuildString = fpgaGetOPAECBuildString;
	adapter->fpgaReadError = fpgaReadError;
	adapter->fpgaClearError = fpgaClearError;
	adapter->fpgaClearAllErrors = fpgaClearAllErrors;
	adapter->fpgaGetErrorInfo = fpgaGetErrorInfo;
	adapter->fpgaCreateEventHandle = fpgaCreateEventHandle;
	adapter->fpgaDestroyEventHandle = fpgaDestroyEventHandle;
	adapter->fpgaGetOSObjectFromEventHandle =
		fpgaGetOSObjectFromEventHandle;
	adapter->fpgaRegisterEvent = fpgaRegisterEvent;
	adapter->fpgaUnregisterEvent = fpgaUnregisterEvent;
	adapter->fpgaAssignPortToInterface = fpgaAssignPortToInterface;
	adapter->fpgaAssignToInterface = fpgaAssignToInterface;
	adapter->fpgaReleaseFromInterface = fpgaReleaseFromInterface;
	adapter->fpgaReconfigureSlot = fpgaReconfigureSlot;

	adapter->initialize = opae_plugin_initialize;
	adapter->finalize = opae_plugin_finalize;

	adapter->supports_device = opae_plugin_supports_device;
	adapter->supports_host = opae_plugin_supports_host;

	return 0;
}
