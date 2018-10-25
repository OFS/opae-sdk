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

/**
 * \fpga_dma.h
 * \brief FPGA DMA BBB API Header
 *
 * Known Limitations
 * - Supports only synchronous (blocking) transfers
 */

#ifndef __DMA_FPGA_DMA_H__
#define __DMA_FPGA_DMA_H__

#include <opae/fpga.h>
#include "fpga_dma_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * The DMA driver supports host to FPGA, FPGA to host and FPGA
 * to FPGA transfers. The FPGA interface can be streaming
 * or memory-mapped.
 */

/* clang-format off */
/**
 * Enumerate DMA channel resources present in the FPGA
 *
 * This call allows the user to query the FPGA for DMA channel resources.
 *
 * fpgaDMAEnumerateChannels() will create a number of `fpga_dma_channel_desc`s
 * describing available DMA channel resources and populate the array
 * `descriptors`. The `max_descriptors` argument can be used to
 * limit the number of descriptors allocated/returned by
 * fpgaDMAEnumerateChannels(); i.e., the number of descriptors in the returned
 * `descriptors` array will be either `max_descriptors` or `num_descriptors`
 * (the number of resources available), whichever is smaller.
 *
 * To query the number of resources available (e.g. to allocate a `descriptors`
 * array of the appropriate size), call fpgaDMAEnumerateChannels() with the
 * parameter `descriptors` set to NULL; this will only return the number of
 * matches in `num_descriptors`.
 *
 * @param[in] fpga              Handle to the FPGA AFU object obtained via fpgaOpen().
 * @param[in] max_descriptors   Maximum number of tokens that fpgaDMAEnumerateChannels()
 *                              shall return (length of `descriptors` array). There may
 *                              be more or fewer matches than this number; `num_descriptors`
 *                              is set to the number of actual matches.
 * @param[out] descriptors      Pointer to an array of fpga_dma_channel_desc structures to be
 *                              populated.  If NULL is supplied, fpgaDMAEnumerateChannels()
 *                              will not create any descriptors, but it will return the
 *                              number of resources in `num_descriptors`.
 * @param[out] num_descriptors  Number of resources available. This number can be higher
 *                              than the number of tokens returned in the `descriptors`
 *                              array (depending on the value of `max_tokens`).
 * @returns                     FPGA_OK on success.
 *                              FPGA_INVALID_PARAM if invalid pointers or objects
 *                              are passed into the function.
 *                              FPGA_NO_DRIVER if OPAE can't find the respective
 *                              enumeration data structures usually provided by the
 *                              driver.
 */
/* clang-format on */
inline fpga_result fpgaDMAEnumerateChannels(fpga_dma_handle dma_ch,
					    uint32_t max_descriptors,
					    fpga_dma_channel_desc *descriptors,
					    uint32_t *num_descriptors);

/**
 * fpgaDMAOpen
 *
 * @brief                      Open DMA device
 *
 * @param[in]  fpga            Handle to the FPGA AFU object obtained via
 *                             fpgaOpen()
 * @param[out] dma             DMA object handle
 * @returns                    FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMAOpen(fpga_handle fpga, fpga_dma_handle *dma);

/**
 * fpgaDMAOpenChannel
 *
 * @brief                      Open DMA channel handle
 *
 * @param[in]  dma_h           Handle to the DMA object obtained via
 *                             fpgaDMAOpen()
 * @param[in]  dma_channel_idx Index of the DMA channel that must be opened
 * @param[out] dma             DMA object handle
 * @returns                    FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMAOpenChannel(fpga_dma_handle dma_h,
				      uint64_t dma_channel_idx,
				      fpga_dma_channel_handle *dma_ch);

/**
 * fpgaDMACloseChannel
 *
 * @brief                  Close DMA channel handle
 *
 * @param[in]  dma         DMA channel handle
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMACloseChannel(fpga_dma_channel_handle *dma);

/**
 * fpgaDMAClose
 *
 * @brief                  Close DMA handle.  Will close all channels
 *                         opened for this DMA device.
 *
 * @param[in]  dma         DMA channel handle
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMAClose(fpga_dma_handle *dma);

/**
 * fpgaDMAGetChannelType
 *
 * @brief                  Query DMA channel type
 *
 *                         Possible type of channels are TX streaming (TX_ST),
 *                         and RX streaming (RX_ST)
 *
 * @param[in]  dma         DMA channel handle
 * @param[out] ch_type     Pointer to channel type
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMAGetChannelType(fpga_dma_channel_handle dma,
					 fpga_dma_channel_type_t *ch_type);

/**
 * fpgaDMATransferInit
 *
 * @brief                  Initialize an object that represents the DMA
 * transfer.
 *
 *                         The driver will reset all transfer attributes to
 * default values upon successful initialization
 *
 * @param[out]  transfer   Pointer to transfer attribute struct
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferInit(fpga_dma_channel_handle dma,
				       fpga_dma_transfer *transfer);

/**
 * fpgaDMATransferReset
 *
 * @brief                  Reset DMA transfer attribute object to default
 * values.
 *
 *                         If same transfer object is reused for transfers, the
 * stale values need to be reset to default values. Eg: rx_bytes, eop_status
 *
 * @param[in]  transfer    Pointer to transfer attribute struct
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferReset(fpga_dma_channel_handle dma,
					fpga_dma_transfer transfer);

/**
 * fpgaDMATransferDestroy
 *
 * @brief                 Destroy DMA transfer attribute object.
 *
 * @param[in]  transfer   Pointer to transfer attribute struct
 * @returns               FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferDestroy(fpga_dma_channel_handle dma,
					  fpga_dma_transfer *transfer);


/**
* fpgaDMATransferSetSrc
*
* @brief                  Set source address of the transfer
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in]  src         Source address

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result fpgaDMATransferSetSrc(fpga_dma_transfer transfer,
					 uint64_t src);

/**
* fpgaDMATransferSetDst
*
* @brief                  Set destination address of the transfer
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in]  dst         Destination address

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result fpgaDMATransferSetDst(fpga_dma_transfer transfer,
					 uint64_t dst);

/**
* fpgaDMATransferSetLen
*
* @brief                  Set transfer length in bytes
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in]  len         Length of the transfer in bytes

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result fpgaDMATransferSetLen(fpga_dma_transfer transfer,
					 uint64_t len);

/**
* fpgaDMATransferSetTransferType
*
* @brief                  Set transfer type
*
*                         Legal values are
*                         HOST_MM_TO_FPGA_ST (host to AFU sreaming)
*                         FPGA_ST_TO_HOST_MM (AFU to host streaming)
*                         FPGA_MM_TO_FPGA_ST (local mem to AFU streaming. Not
supported in the current version)
*                         FPGA_ST_TO_FPGA_MM (AFU to local mem streaming. Not
supported in the current version)
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in]  type        Type of transfer

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result
fpgaDMATransferSetTransferType(fpga_dma_transfer transfer,
			       fpga_dma_transfer_type_t type);

/**
* fpgaDMATransferSetTxControl
*
* @brief                  Set TX control
*
*                         TX Control allows the driver to optionally generate
*                         in-band start and end of packet (SOP/EOP) in the data
*                         stream sent from the TX DMA.
*
*                         TX Control is valid only for HOST_MM_TO_FPGA_ST and
*                         FPGA_MM_TO_FPGA_ST transfers.
*
*                         Valid values are:
*
*                         TX_NO_PACKET (deterministic length transfer)
*                         GENERATE_SOP
*                         GENERATE_EOP
*                         GENERATE_SOP_AND_EOP
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in]  tx_ctrl     TX Control value

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result fpgaDMATransferSetTxControl(fpga_dma_transfer transfer,
					       fpga_dma_tx_ctrl_t tx_ctrl);

/**
* fpgaDMATransferSetRxControl
*
* @brief                  Set RX control
*
*                         RX control allows the driver to handle an unknown
*                         amount of receive data from the FPGA. When END_ON_EOP
*                         is set, the RX DMA will end the transfer when EOP
arrives
*                         in the receive stream or when rx_count bytes have been
*                         received (whichever occurs first)
*
*                         RX Control is valid only for FPGA_ST_TO_HOST_MM and
*                         FPGA_MM_TO_FPGA_ST transfers.
*
*                         Valid values are:
*
*                         RX_NO_PACKET (deterministic length transfer)
•                         END_ON_EOP
*
* @param[in]  transfer    Pointer to transfer attribute struct
* @param[in] rx_ctrl      RX Control value

* @returns                FPGA_OK on success, return code otherwise
*/
inline fpga_result fpgaDMATransferSetRxControl(fpga_dma_transfer transfer,
					       fpga_dma_rx_ctrl_t rx_ctrl);

/**
 * fpgaDMATransferSetTransferCallback
 *
 * @brief                  Register callback for notification on asynchronous
 *                         transfer completion
 *                         If a callback is specified, fpgaDMATransfer
 *                         returns immediately (asynchronous transfer).
 *
 *                         If a callback is not specified, fpgaDMATransfer
 *                         returns after the transfer is complete (synchronous/
 *                         blocking transfer).
 *
 *
 * @param[in]  transfer    Pointer to transfer attribute struct
 * @param[in]  cb          Notification callback; You must set notification
 *                         callback to NULL for synchronous transfers
 * @param[in]  ctxt        Callback context
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result
fpgaDMATransferSetTransferCallback(fpga_dma_transfer transfer,
				   fpga_dma_transfer_cb cb, void *ctxt);

/**
 * fpgaDMATransferGetBytesTransferred
 *
 * @brief                  Retrieve number of bytes completed by RX DMA
 *
 *                         Pointer to the number of bytes the RX DMA transferred
 *                         to memory. RX transfer from streaming sources will
 *                         have an unknown amount of data to transfer when
 *                         rx_control is set to END_ON_EOP.
 *
 * @param[in]  transfer    Pointer to transfer attribute struct
 * @param[out] rx_bytes    Pointer to the number of bytes RX DMA has
 *                         transferred to memory
 *
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result
fpgaDMATransferGetBytesTransferred(fpga_dma_transfer transfer,
				   size_t *rx_bytes);

/**
 * fpgaDMATransferCheckEopArrived
 *
 * @brief                  Retrieve EOP status
 *
 *                         Legal value are:
 *                         0: EOP not arrived
 *                         1: EOP arrived
 *
 * @param[in]  transfer    Pointer to transfer attribute struct
 * @param[out] eop_arrived Pointer to the eop status
 *
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferCheckEopArrived(fpga_dma_transfer transfer,
						  bool *eop_arrived);

/**
 * fpgaDMATransfer
 *
 * @brief                  Perform a DMA transfer
 *
 * @param[dma] dma         DMA handle
 * @param[in]  transfer    Transfer attribute object
 *
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferStart(fpga_dma_channel_handle dma,
					const fpga_dma_transfer transfer);

/**
 * fpgaDMATransferInitSmall
 *
 * @brief                  Initialize an object that represents the DMA
 *                         transfer.
 *
 *                         This is an advanced API (experimental) and may
 *                         disappear in the future.
 *
 *                         The driver will reset all transfer attributes to
 *                         default values upon successful initialization.
 *
 * This API allocates a buffer suitable for the DMA engine to access directly.
 * The size of the buffer is currently limited to 2MB (0x00200000) bytes.
 *
 * The intended use is to provide an application with a high-speed DMA option
 * aimed at "small" transfers.  The number of small transfers is limited to
 * four 2MB buffers - that is, calling fpgaDMATransferInitSmall five times
 * without an intervening fpgaDMATransferDestroy will fail with FPGA_NO_MEMORY.
 *
 * Note that the application can sub-partition this buffer at will, and manage
 * it however they wish.
 *
 * NOTE: The FPGA will be reading and writing to this buffer asynchronously.
 * Once a transfer has started, it is the application's responsibility to
 * not read from or write to the memory identified by the transaction.
 *
 * @param[in] dma          DMA channel handle
 * @param[in/out] size     Size of the buffer to allocate.  Returns the actual
 *                         size allocated (0 on failure).
 * @param[out]  buf_ptr    Address of a pointer to hold the address of
 *                         the first byte of the allocated buffer
 * @param[out]  transfer   Pointer to transfer attribute struct
 * @returns                FPGA_OK on success, return code otherwise
 */
inline fpga_result fpgaDMATransferInitSmall(fpga_dma_channel_handle dma,
					    uint64_t *size, void **buf_ptr,
					    fpga_dma_transfer *transfer);


#ifdef __cplusplus
}
#endif

#endif // __DMA_FPGA_DMA_H__
