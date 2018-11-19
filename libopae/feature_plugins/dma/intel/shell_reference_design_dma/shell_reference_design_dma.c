// Copyright(c) 2017-2018, Intel Corporation
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

/**
 *
 * @file shell_referance_design_dma_int.c
 * @brief implementation of the Intel shell reference design DMA to plug into OPAE
 *
 */

#include "shell_reference_design_dma_int.h"



fpga_result fpgaDMAPropertiesGet(fpga_feature_token token,
				 fpgaDMAProperties *prop,
				 int max_ch)
{
	UNUSED_PARAM(token);
	UNUSED_PARAM(prop);
	UNUSED_PARAM(max_ch);

	return FPGA_NOT_SUPPORTED;
}

fpga_result fpgaFeatureOpen(fpga_feature_token	token,
			    int flags,
			    void *priv_config,
			    fpga_feature_handle *handle)
{
	struct _fpga_feature_token *_token = (struct _fpga_feature_token *)token;
	isrd_dma_t	*isrd_handle;
	fpga_result	res;
	uint32_t i, j;

	UNUSED_PARAM(flags);

	if (token == NULL || handle == NULL)
		return FPGA_INVALID_PARAM;

	isrd_handle = isrd_init_dma_handle(_token, priv_config);
	if (isrd_handle == NULL) {
			FPGA_MSG("Failed to allocate DMA device handle");
			return FPGA_NO_MEMORY;
	}

	/* Initialize Rx channels */
	for (i = 0; i < isrd_handle->channel_number; i++) {
		res = isrd_init_ch(i, isrd_handle, FPGA_ST_TO_HOST_MM, DEFAULT_RX_PD_RING_SIZE);
		ON_ERR_GOTO(res, out_free_rx_ch, "Failed to init Rx ch");
	}

	/* Initialize Tx channels */
	for (i = 0; i < isrd_handle->channel_number; i++) {
		res = isrd_init_ch(i, isrd_handle, HOST_MM_TO_FPGA_ST, DEFAULT_TX_PD_RING_SIZE);
		ON_ERR_GOTO(res, out_free_tx_ch, "Failed to init Tx ch");
	}

	*handle = (fpga_feature_handle)isrd_handle;

	return  FPGA_OK;


out_free_tx_ch:
	for (j = 0; j < i; j++) {
		isrd_free_ch(&isrd_handle->tx_channels[j]);
	}
	i = isrd_handle->channel_number;
out_free_rx_ch:
	for (j = 0; j < i; j++) {
		isrd_free_ch(&isrd_handle->rx_channels[j]);
	}

	free(isrd_handle);
	return res;
}

fpga_result fpgaDMATransferSync(fpga_feature_handle dma_handle,
				   transfer_list *dma_xfer_list)
{
	fpga_result res;
	isrd_dma_t *isrd_handle = (isrd_dma_t *)dma_handle;

	if (isrd_handle == NULL || isrd_handle->magic_num != ISRD_MAGIC_NUM ||
	    dma_xfer_list == NULL || dma_xfer_list->entries_num == 0 ||
	    dma_xfer_list->array == NULL ||
	    dma_xfer_list->ch_index >= isrd_handle->channel_number) {
		FPGA_MSG("Invalid param to xfer sync");
		return FPGA_INVALID_PARAM;
	}

	switch (dma_xfer_list->type) {
		case HOST_MM_TO_FPGA_ST:
			res = isrd_xfer_tx_sync(&isrd_handle->tx_channels[dma_xfer_list->ch_index],
						dma_xfer_list);
			break;
		case FPGA_ST_TO_HOST_MM:
			res = isrd_xfer_rx_sync(&isrd_handle->rx_channels[dma_xfer_list->ch_index],
						dma_xfer_list);
			break;
		default:
			FPGA_MSG("Invalid xfer type");
			return FPGA_INVALID_PARAM;
	}

	return res;
}


fpga_result fpgaDMATransferAsync(fpga_feature_handle dma,
				    transfer_list *dma_xfer_list,
				    fpga_dma_cb cb,
				    void *context)
{
		UNUSED_PARAM(dma);
		UNUSED_PARAM(dma_xfer_list);
		UNUSED_PARAM(cb);
		UNUSED_PARAM(context);

		return FPGA_NOT_SUPPORTED;

}

fpga_result fpgaFeatureClose(fpga_feature_handle dma_h)
{
	UNUSED_PARAM(dma_h);

	return FPGA_NOT_SUPPORTED;

}
