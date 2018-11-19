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
 * @file shell_reference_design_DMA .h
 * @brief Plug in functions for Intel shell reference design DMA feature
 *
 * User should not use this API.
 * when OPAE discover this DMA it will load these functions.
 * when the user will use the DMA API the opae DMA plug-in will call these
 * functions.
 */

#ifndef __FPGA_SRD_DMA_H__
#define __FPGA_SRD_DMA_H__

#include <opae/types.h>
#include <stddef.h>
#include "/home/lab/workspace/amirh/opae-sdk/libopae/opae_int.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_CHANNEL_SUPPORTED			2048
#define CHANNEL_CONFIGURATION_SPACE_SIZE	(1<<16)  /* (2^16 ) */
#define MAX_MTU					9600
#define MSXI_VECTORS_PER_CHANNEL		4

#define DMA_TX_OFFSET				0
#define DMA_RX_OFFSET				0x8000
#define DMA_MSXI_BASE_ADDRESS			0xFFFE0000
#define DMA_GLOBAL_DEBUG_STATUS_BASE_ADDRESS	0xFFFF0000

#define TWO_16B_INTO_32B (h, l) ((((h) & 0xFFFF) << 16 ) | ((l) & 0xFFFF))

#define CTRL_EN				0x1
#define INT_CTRL_EN			0x1
#define INT_CTRL_ARM			0x2
#define RESET_RST			0x1

#pragma pack(push, 1)

/* Tx AND Rx register map */
typedef struct {
	uint32_t	ch_ctrl;
	uint32_t	res;
	uint32_t	ring_start_addr_l;
	uint32_t	ring_start_addr_h;
	uint32_t	ring_size;
	uint32_t	ring_panging_tail;
	uint32_t	ring_head;
	uint32_t	res2;
	uint32_t	ring_consumed_head_addr_l;
	uint32_t	ring_consumed_head_addr_h;
	uint32_t	packet_batch_size_delay;
	uint32_t	int_ctrl;
	uint32_t	int_threshold_delay;
	uint32_t	res3;
	uint32_t	debug_status_1;
	uint32_t	debug_status_2;
	uint32_t	debug_status_3;
	uint32_t	debug_status_4;
	uint32_t	reset;
	uint32_t	pipeline_timer_threshold;
} ch_register_map_t;

typedef struct {
	uint32_t	buffer_addr_hi;
	uint32_t	buffer_addr_lo;
	struct {
		uint32_t	res:14;
		uint32_t	eop:1;
		uint32_t	sop:1;
		uint32_t	len:16;
	};
	uint16_t	res2;
	uint16_t	metadata_len;
	uint32_t	metadata[4];
} tx_pd_t;

typedef struct {
	uint32_t	buffer_addr_hi;
	uint32_t	buffer_addr_lo;
	uint16_t	res;
	uint16_t	len;
	uint32_t	res2;
} rx_pd_t;

typedef struct {
	uint16_t	res;
	uint16_t	len;
	uint32_t	res2[3];
	char		*data[0];
} rx_buffer_header_t;

/* MSIX vector */
typedef struct {	uint32_t	message_addr_l;
			uint32_t	message_addr_h;
			uint16_t	need_to_zero_me;
			uint16_t	message_data;
			uint32_t	res;
} msix_vector;

/* MXIX vectors of one channel */
typedef struct {
	msix_vector	tx_completion;
	msix_vector	rx_completion;
	msix_vector	tx_event;
	msix_vector	rx_event;
} msix_channel_vectors_t;








/* global_debug_and_status_reg_map */
typedef	struct {
	uint32_t	tlp2hip:10;
	uint32_t	hip2tlp:10;
	uint32_t	res:12;
} arate_tlp_t;

typedef	struct {
	uint32_t	dmaWr2tlp:10;
	uint32_t	dmaRd2tlp:10;
	uint32_t	tlp2dmaRd:10;
	uint32_t	res:2;
} arate_dma_t;

typedef	struct {
	uint32_t	tlp2hip:10;
	uint32_t	hip2tlp:10;
	uint32_t	res:12;
} srate_tlp_t;

typedef	struct {
	uint32_t	dmaWr2tlp:10;
	uint32_t	dmaRd2tlp:10;
	uint32_t	res:12;
} srate_dma_t;

typedef	struct {
	uint32_t	np:10;
	uint32_t	p:10;
	uint32_t	cpl:10;
	uint32_t	res:2;
} srate_cred_t;

typedef	struct {
	uint32_t	rate:10;
	uint32_t	res:22;
} srate_tag_t;

typedef	struct {
	uint32_t	lvl:9;
	uint32_t	res:23;
} tag_lvl_t;

typedef	struct {
	uint32_t	hdr:8;
	uint32_t	res:24;
} cred_np_t;

typedef	struct {
	uint32_t	hdr:8;
	uint32_t	res:8;
	uint32_t	data:12;
	uint32_t	res2:4;
} cred_p_t;

typedef	struct {
	uint32_t	hdr:8;
	uint32_t	res:8;
	uint32_t	data:12;
	uint32_t	res2:4;
} cred_cpl_t;

typedef	struct {
	uint32_t	noRdTag:1;
	uint32_t	noWrCred:1;
	uint32_t	noRdCred:1;
	uint32_t	noCplCred:1;
	uint32_t	wrReqFifoOflow:1;
	uint32_t	wrTlpFifoOflow:1;
	uint32_t	rdReqFifoOflow:1;
	uint32_t	rdRspFifoOflow:1;
	uint32_t	wrReqProtErr:1;
	uint32_t	hiplfFifoOflow:1;
	uint32_t	avReqFifoOflow:1;
	uint32_t	avCtFifoOflow:1;
	uint32_t	avTlpFifoOflow;
	uint32_t	avBadRdReq:1;
	uint32_t	avBadWrRwq:1;
	uint32_t	avRdRspErr:1;
	uint32_t	tagFifoOflow:1;
	uint32_t	cplStatUR:1;
	uint32_t	cplStatCA:1;
	uint32_t	cplStatBad:1;
	uint32_t	cplStatEP:1;
	uint32_t	cplOoo:1;
	uint32_t	res:10;
} tlp_hold_t;

typedef	struct {
	uint32_t	disNpCredLa:1;
	uint32_t	disPCredLa:1;
	uint32_t	disCplCreadla:1;
	uint32_t	res:29;
} cfg_t;


typedef struct {
	uint8_t		res[0x180];
	uint32_t	arate_tlp;
	uint32_t	arate_dma;
	uint32_t	srate_tlp;
	uint32_t	srate_dma;
	uint32_t	srate_cred;
	uint32_t	srate_tag;
	uint32_t	tag_lvl;
	uint32_t	cred_np;
	uint32_t	cred_p;
	uint32_t	cred_cpl;
	uint32_t	tlp_hold;
	uint32_t	cfg;
} global_debug_and_status_reg_map;

#pragma pack(pop) // pack(push, 1)


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_SRD_DMA_H__

