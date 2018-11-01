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
 *
 * @file intel shell_referance_design_dma_int.h
 * @brief Header file for the implementation of the Intel shell reference design DMA
 *
 */

#ifndef __FPGA_ISRD__INTERNAL_DMA_H__
#define __FPGA_ISRD__INTERNAL_DMA_H__

#include <opae/fpga.h>
#include "shell_reference_design_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISRD_MAGIC_NUM			0x1ECCA5AD
#define ISRD_CH_MAGIC_NUM		0x7DEF18AA
#define DEFAULT_CHANNEL_NUMBEER		8
#define DEFAULT_TX_PD_RING_SIZE		1024
#define DEFAULT_RX_PD_RING_SIZE		1024
#define ISRD_CH_MEMBER_OFFSET(ch, member)  (((ch)->csr_base_offset) + (offsetof(ch_register_map_t, member)))
#define LOW_32B_OF_64B(x)	((uint32_t)(x))
#define HIGH_32B_OF_64B		((x) >> 32)

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)              \
	do {                                       \
		if ((res) != FPGA_OK) {            \
			print_err((desc), (res));  \
			goto label;                \
		}                                  \
	} while (0)

typedef struct {
	fpga_handle	fpga_h;
	uint64_t	csr_base_offset;
	uint32_t	mmio_num;
	uint32_t	pd_ring_size;
	uint32_t	pd_tail;	/* Next PD to write to */
	/* PD Ring buffer */
	volatile tx_pd_t	*pds_base_usr_addr;
	uint64_t		pds_base_wsid;
	uint64_t 		pds_base_iova;
	/* HW write the HW ring head to here */
	volatile uint32_t	*hw_ring_head_usr_addr;
	uint64_t		hw_ring_head_wsid;
	uint64_t		hw_ring_head_iova;
	fpga_dma_transfer_type	ch_type;
	pthread_mutex_t lock;
	uint32_t	magic_num;
} isrd_ch_t;



typedef struct {
	uint32_t	magic_num;
	fpga_handle	fpga_h;
	uint64_t	csr_base_offset;
	uint32_t	mmio_num;
	uint32_t	channel_number;
	isrd_ch_t	tx_channels[MAX_CHANNEL_SUPPORTED];
	isrd_ch_t	rx_channels[MAX_CHANNEL_SUPPORTED];
} isrd_dma_t;


isrd_dma_t *isrd_init_dma_handle(struct _fpga_feature_token *token, void *priv_config);
fpga_result isrd_init_ch(int ch_num, isrd_dma_t *isrd_handle,
			 fpga_dma_transfer_type ch_type, int ring_size);
fpga_result isrd_init_hw_ch(isrd_ch_t *ch);
fpga_result isrd_free_ch(isrd_ch_t *ch);
fpga_result isrd_reset_ch(isrd_ch_t *ch);
fpga_result isrd_ch_check_and_lock(isrd_ch_t *ch);
fpga_result isrd_xfer_tx_sync(isrd_ch_t *ch, transfer_list *dma_xfer);
fpga_result isrd_xfer_rx_sync(isrd_ch_t *ch, transfer_list *dma_xfer);







#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_ISRD__INTERNAL_DMA_H__
