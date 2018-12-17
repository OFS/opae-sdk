// Copyright(c) 2018-2019, Intel Corporation
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
 * @file dma.h
 * @brief APIs for DMA feature
 *
 * User should use the feature API to discover features which are DMA type.
 * With the feature token from the enumeration user can open the feature.
 * With the feature handle user can access and transfer data using the DMA.
 */

#ifndef __FPGA_DMA_H__
#define __FPGA_DMA_H__

#include <opae/types.h>
#include <opae/feature.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FPGA_DMA_MAX_CLIENT_NUM 8

/**
 * DMA properties
 *
 * Common properties for all DMA engines.
 * Output of fpgaDMAPropertiesGet.
 */
typedef struct {
	uint64_t max_channel_num; /**< Max number of channels that the DMA engine supports */
	uint64_t max_ring_size;   /**< Max number of buffers that the DMA can hold */
	uint64_t max_buffer_size; /**< Max size of one buffer */
	uint64_t addr_alignment_for_dma;    /**< Address alignment requirement for DMA */
	uint64_t minimum_xfer_size_for_dma; /**< DMA tranfer size should be multiples of this size */
	uint64_t capabilities_mask;         /**< Bit mask of fpga_dma_transfer_type */
	fpga_guid tx_endp_guid[FPGA_DMA_MAX_CLIENT_NUM]; /**< A table of guid of the connected IP to a Tx channels */
	fpga_guid rx_endp_guid[FPGA_DMA_MAX_CLIENT_NUM]; /**< A table of guid of the connected IP to a Rx channels */
	uint32_t tx_client_num; /**< Number of tx clients that the DMA is connected to */
	uint32_t rx_client_num; /**< Number of rx clients that the DMA is connected to */
} fpga_dma_properties;

/**
 * Get DMA feature properties.
 *
 * Get DMA properties from a feature token (DMA feature type)
 *
 * @param[in]   token   Feature token
 * @param[out]  prop    Pre-allocated fpga_dma_properties structure to
 *                      write information into
 *
 * @returns             FPGA_OK on success.
 *                      FPGA_INVALID_PARAM if invalid pointers are passed
 *                      into the function.
 */

fpga_result
fpgaDMAPropertiesGet(fpga_feature_token token, fpga_dma_properties *prop);

/**
 * DMA transfer
 *
 * Holds a DMA transaction information
 *
 * @note metadata is input for Tx stream and output for Rx stream
 * @note rx_len and rx_eop is output only
 */
typedef struct {
	void *priv_data; /**< Private data for internal use - must be first */
	uint64_t src;    /**< Source address */
	uint64_t dst;    /**< Destination address */
	uint64_t len;    /**< Transactions  length */
	uint64_t wsid;   /**< wsid of the host memory if it was allocated with prepareBuffer */
	void *metadata; /**< stream - user metadata for the receiving IP */
	uint64_t metadata_len;  /**< stream - length of the metadata */
	uint64_t metadata_wsid; /**< stream - wsid of the metadata */
	bool eop;      /**< Indicate last buffer for the stream */
	uint64_t rx_len; /**< Rx stream - length of Rx data */
} fpga_dma_transfer;

/**
 * DMA transfer list
 *
 * A list of DMA transfers with different buffers.
 */

typedef struct {
	uint64_t xfer_id;            /**< User ID for this transaction */
	fpga_dma_transfer *xfers;    /**< Pointer to a transfer array */
	uint32_t entries_num;        /**< number of entries in array */
	fpga_dma_transfer_type type; /**< Direction and streaming or memory */
	uint32_t ch_index; /**< in case of multi channel DMA, which channel to use */
} dma_transfer_list;

/**
 * Start a blocking transfer.
 *
 * Start a sync transfer and return only all the data was copied.
 *
 * @param[in]   dma_h         as populated by fpgaFeatureOpen()
 * @param[in]   xfer_list     transfer information
 *
 * @returns                   FPGA_OK on success.
 *                            FPGA_INVALID_PARAM if invalid pointers or objects
 *                            are passed into the function.
 *                            FPGA_NOT_SUPPORTED if the accelerator doesn't support
 *                            DMA sync tranfers.
 */
fpga_result
fpgaDMATransferSync(fpga_feature_handle dma_h, dma_transfer_list *xfer_list);

/**
 * FPGA DMA callback function for asynchronous operation
 */
typedef void (*fpga_dma_cb)(dma_transfer_list *xfer_list, void *context);

/**
 * Start a non-blocking transfer (callback).
 *
 * Start an Async transfer (Return immediately)
 * Callback will be invoked when the transfer is completed.
 *
 * @param[in]   dma_h         as populated by fpgaFeatureOpen()
 * @param[in]   dma_xfer      transfer information
 * @param[in]   cb            Call back function to call when the transfer is completed
 * @param[in]   context       argument to pass to the callback function
 *
 * @note For posting receive buffers to the DMA in Rx streaming mode,
 *       call this function with NULL back.
 *
 * @returns                   FPGA_OK on success.
 *                            FPGA_INVALID_PARAM if invalid pointers or objects
 *                            are passed into the function.
 *                            FPGA_NOT_SUPPORTED if the accelerator doesn't support
 *                            DMA async tranfers.
 */
fpga_result
fpgaDMATransferAsync(fpga_feature_handle dma_h, dma_transfer_list *dma_xfer,
		fpga_dma_cb cb, void *context);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_DMA_H__

