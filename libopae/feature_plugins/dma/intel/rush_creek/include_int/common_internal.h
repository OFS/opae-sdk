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
 * \common_internal.h
 * \brief FPGA DMA BBB Internal Header
 */

#ifndef __DMA_COMMON_INT_H__
#define __DMA_COMMON_INT_H__

//#define CHECK_DELAYSX

#ifdef CHECK_DELAYSX
#define WHENCE , int whence
#define WHENCE_VAR , whence
#define WAIT_MAGIC , 13
#define XFER_H2F , 1
#define XFER_H2F2 , 2
#define XFER_H2F3 , 3
#define M2S_TW , 4
#define M2S_TW2 , 5
#define S2M_TW , 6
#define S2M_TW2 , 7
#define S2M_TW3 , 8
#define WM_F2H , 9
#define WM_F2H2 , 10
#define WM_F2H3 , 11
#define WM_F2F , 12
#define WM_F2F2 , 0
#else
#define WHENCE
#define WHENCE_VAR
#define WAIT_MAGIC
#define XFER_H2F
#define XFER_H2F2
#define XFER_H2F3
#define M2S_TW
#define M2S_TW2
#define S2M_TW
#define S2M_TW2
#define S2M_TW3
#define WM_F2H
#define WM_F2H2
#define WM_F2H3
#define WM_F2F
#define WM_F2F2
#endif


// End of feature list
inline bool _fpga_dma_feature_eol(uint64_t dfh);

// Feature type is BBB
inline bool _fpga_dma_feature_is_bbb(uint64_t dfh);

// Offset to the next feature header
inline uint64_t _fpga_dma_feature_next(uint64_t dfh);

/**
 * _send_descriptor
 *
 * @brief                Queues a DMA descriptor to the FPGA
 * @param[in] dma_h      Handle to the FPGA DMA object
 * @param[in] desc       Pointer to a descriptor structure to send
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
fpga_result _send_descriptor(handle_common *dma_h, msgdma_ext_desc_t *desc);

fpga_result clear_interrupt(handle_common *dma_h);

fpga_result poll_interrupt(handle_common *dma_h WHENCE);

#endif // __DMA_COMMON_INT_H__
