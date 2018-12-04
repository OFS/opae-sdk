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

#ifndef __FEATURE_ADAPTER_H__
#define __FEATURE_ADAPTER_H__
#include <stdbool.h>

#include <opae/types.h>
#include <opae/feature.h>
#include <opae/dma.h>

typedef struct _feature_plugin {
	char *path;      // location on file system
	void *dl_handle; // handle to the loaded library instance
} feature_plugin;

typedef struct _feature_adapter_table {

	struct _feature_adapter_table *next;
	feature_plugin plugin;
	fpga_guid guid;

	fpga_result(*fpgaDMAPropertiesGet)(fpga_feature_token token, fpgaDMAProperties *prop,
				int max_ch);
	fpga_result (*fpgaDMATransferSync)(fpga_feature_handle dma_handle,
			transfer_list *dma_xfer);
	fpga_result (*fpgaDMATransferAsync)(fpga_feature_handle dma,
			transfer_list *dma_xfer, fpga_dma_cb cb, void *context);
	fpga_result (*fpgaFeatureOpen)(fpga_feature_token token, int flags,
				   void *priv_config, fpga_feature_handle *handle);
	fpga_result (*fpgaFeatureClose)(fpga_feature_handle *_dma_h);
	// configuration functions
	int (*initialize)(void);
	int (*finalize)(void);

} feature_adapter_table;

#endif /* __FEATURE_ADAPTER_H__ */
