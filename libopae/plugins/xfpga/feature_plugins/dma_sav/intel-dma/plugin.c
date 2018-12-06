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

/*#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
*/

#include <dlfcn.h>

#include "feature_adapter.h"

#define UNUSED_PARAM(x) ((void)x)
/* Macro for defining symbol visibility */
#define __FPGA_API__ __attribute__((visibility("default")))
//#define __FIXME_MAKE_VISIBLE__ __attribute__((visibility("default")))

int __FPGA_API__ feature_initialize(void)
{

	return 0;
}

int __FPGA_API__ feature_finalize(void)
{

	return 0;
}

fpga_result __FPGA_API__ feature_open(fpga_feature_token token, int flags,
				   fpga_feature_handle *handle)
{
	UNUSED_PARAM(token);
	UNUSED_PARAM(flags); 
	UNUSED_PARAM(handle); 
	return FPGA_OK;
}
fpga_result __FPGA_API__ feature_close(fpga_feature_handle *_dma_h)
{
	UNUSED_PARAM(_dma_h); 
	return FPGA_OK;
}
fpga_result __FPGA_API__ dma_propertiesGet(fpga_feature_token token, fpga_dma_properties *prop,
				int max_ch)
{
	UNUSED_PARAM(token);
	UNUSED_PARAM(prop);
	UNUSED_PARAM(max_ch);
	return FPGA_OK;
}

fpga_result __FPGA_API__ dma_transferSync(fpga_feature_handle dma_handle,
			dma_transfer_list *dma_xfer)
{
	UNUSED_PARAM(dma_handle);
	UNUSED_PARAM(dma_xfer);

	return FPGA_OK;
}
fpga_result __FPGA_API__ dma_transferAsync(fpga_feature_handle dma,
			dma_transfer_list *dma_xfer, fpga_dma_cb cb, void *context)
{
	UNUSED_PARAM(dma);
	UNUSED_PARAM(dma_xfer);
	UNUSED_PARAM(cb);
	UNUSED_PARAM(context);
	return FPGA_OK;
}

int __FPGA_API__ feature_plugin_configure(feature_adapter_table *adapter,
				      const char *jsonConfig)
{
	UNUSED_PARAM(jsonConfig);

	adapter->fpgaFeatureOpen =
		dlsym(adapter->plugin.dl_handle, "feature_open"); 
	adapter->fpgaFeatureClose =
		dlsym(adapter->plugin.dl_handle, "feature_close");
		
	adapter->fpgaDMAPropertiesGet =
		dlsym(adapter->plugin.dl_handle, "dma_propertiesGet");
	adapter->fpgaDMATransferSync =
		dlsym(adapter->plugin.dl_handle, "dma_transferSync");
	adapter->fpgaDMATransferAsync =
		dlsym(adapter->plugin.dl_handle, "dma_transferAsync");

	adapter->initialize =
		dlsym(adapter->plugin.dl_handle, "feature_initialize");
	adapter->finalize =
		dlsym(adapter->plugin.dl_handle, "feature_finalize");

	return 0;
}
