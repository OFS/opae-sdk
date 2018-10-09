// Copyright(c) 2017, Intel Corporation
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
 * These APIs come after a feature DMA was discovered
 */

#ifndef __FPGA_DMA_H__
#define __FPGA_DMA_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DMA properties
 *
 * Common properties for all DMA engines.
 */
typedef struct {uint64_t    maxChannelNum;
                uint64_t    maxRingSize;
                uint64_t    maxBufferSize;
                uint64_t    addrAlignmentForDMA;
                uint64_t    minimumXferSizeForDMA;
                uint64_t    capabilities_mask;      // Bit mask of fpga_dma_transfer_type
                uint64_t    reserved [32];
                fpga_guid   *txEndPGuid;     // Pointer to a table of Guid of the connected IP to a Tx channels
                fpga_guid   *rxEndPGuid;     // Pointer to a table of Guid of the connected IP to a Rx channels
}fpgaDMAProperties;

/**
 * Get DMA feature properties.
 *
 * Get DMA properties from a feature token (DMA feature type)
 *
 * @param[in]   token   Feature token
 * @param[out]  prop    fpgaDMAProperties structure to fill
 * @param[in]   max_ch  Entry number in the Tx/Rx end point GUID array
 *
 * @returns FPGA_OK on success.
 */

fpga_result
fpgaDMAPropertiesGet(fpga_feature_token token, fpgaDMAProperties *prop, int max_ch);

typedef struct {    void        *priv_data;
                    uint64_t    src;
                    uint64_t    dst;
                    uint64_t    len;
                    uint64_t    wsid;           // For pre allocated host memory
                    fpga_dma_transfer_type  type;
                    int         ch_index;
                    /* For Rx streaming */
                    uint64_t    *rx_len;
                    bool        *rx_eop;
                    uint64_t    reserved[8];
}fpga_dma_transfer_t;

/**
 * Start a blocking transfer.
 *
 * Start a sync transfer and return only all the data was copied.
 *
 * @param[in]   dma_handle      as populated by fpgaDMAOpen()
 * @param[in]   dma_xfer        encapsulation of all the information about the transfer
 *                              as populated by fpgaDMATransferInit and set by fpgaDMATransferSet functions
 *
 * @returns FPGA_OK on success.
 */
fpga_result
fpgaDMATransferSync(fpga_feature_handle dma_h, fpga_dma_transfer_t *dma_xfer);

/**
 * Start a none blocking transfer (callback).
 *
 * Start an Async transfer (Return immediately)
 * Callback will be invoke when the transfer is completed.
 *
 * @param[in]   dma_handle      as populated by fpgaFeatureOpen()
 * @param[in]   dma_xfer        information about the transfer
 * @param[in]   cb              Function to call when the transfer is completed
 * @param[in]   context         value to pass to the callback function
 *
 * @returns FPGA_OK on success.
 */
fpga_result
fpgaDMATransferCB(fpga_feature_handle dma_h,
                fpga_dma_transfer_t *dma_xfer,
                fpga_dma_transfer_cb cb,
                void *context);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_DMA_H__

