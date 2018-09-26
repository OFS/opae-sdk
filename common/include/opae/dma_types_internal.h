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

#ifndef __FPGA__DMA_TYPES_INTERNAL_H__
#define __FPGA__DMA_TYPES_INTERNAL_H__

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <opae/fpga.h>
#include "dma_types.h"

#define QWORD_BYTES 8
#define DWORD_BYTES 4
#define IS_ALIGNED_DWORD(addr) (addr%4==0)
#define IS_ALIGNED_QWORD(addr) (addr%8==0)

// BBB Feature ID (refer CCI-P spec)
#define FPGA_DMA_BBB 0x2

// Reuse following values
#define FPGA_DMA_CSR_BUSY (1<<0)
#define FPGA_DMA_DESC_BUFFER_EMPTY 0x2
#define FPGA_DMA_DESC_BUFFER_FULL 0x4

// Alignment byte sizes
#define FPGA_DMA_ALIGN_BYTES 64
#define FPGA_DMA_BUF_SIZE (2*1024*1024)
#define CACHE_LINE_SIZE 64

// Granularity of DMA transfer (maximum bytes that can be packed
// in a single descriptor).This value must match configuration of
// the DMA IP. Larger transfers will be broken down into smaller
// transactions.
#define FPGA_DMA_BUF_ALIGN_SIZE FPGA_DMA_BUF_SIZE

// Maximum ring size
#define FPGA_DMA_MAX_BUF 8

// Max. async transfers in progress
#define FPGA_DMA_MAX_INFLIGHT_TRANSACTIONS 1024

// Architecture specific masks
#define FPGA_DMA_MASK_32_BIT 0xFFFFFFFF

// Helper functions
#define CSR_BASE(dma_handle) ((uint64_t)dma_handle->dma_csr_base)
#define RSP_BASE(dma_handle) ((uint64_t)dma_handle->dma_rsp_base)
#define ST_VALVE_BASE(dma_handle) ((uint64_t)dma_handle->dma_streaming_valve_base)
#define HOST_MMIO_32_ADDR(dma_handle,offset) ((volatile uint32_t *)((uint64_t)(dma_handle)->mmio_va + (uint64_t)(offset)))
#define HOST_MMIO_64_ADDR(dma_handle,offset) ((volatile uint64_t *)((uint64_t)(dma_handle)->mmio_va + (uint64_t)(offset)))
#define HOST_MMIO_32(dma_handle,offset) (*HOST_MMIO_32_ADDR(dma_handle,offset))
#define HOST_MMIO_64(dma_handle,offset) (*HOST_MMIO_64_ADDR(dma_handle,offset))

#define CSR_STATUS(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, status))
#define CSR_CONTROL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, ctrl))
#define CSR_FILL_LEVEL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, fill_level))
#define CSR_RSP_FILL_LEVEL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, rsp_level))
#define RSP_BYTES_TRANSFERRED(dma_h) (RSP_BASE(dma_h) + offsetof(msgdma_rsp_t, actual_bytes_tf))
#define RSP_STATUS(dma_h) (RSP_BASE(dma_h) + offsetof(msgdma_rsp_t, rsp_status))
#define ST_VALVE_CONTROL(dma_h) (ST_VALVE_BASE(dma_h) + offsetof(msgdma_st_valve_t, control))
#define ST_VALVE_STATUS(dma_h) (ST_VALVE_BASE(dma_h) + offsetof(msgdma_st_valve_t, status))

#define IS_DMA_ALIGNED(addr) (addr%FPGA_DMA_ALIGN_BYTES==0)

#define ALIGN_TO_CL(x) ((uint64_t)(x) & ~(CACHE_LINE_SIZE - 1))
#define IS_CL_ALIGNED(x) (((uint64_t)(x) & (CACHE_LINE_SIZE - 1)) == 0)

typedef struct __attribute__ ((__packed__)) {
	uint64_t dfh;
	uint64_t feature_uuid_lo;
	uint64_t feature_uuid_hi;
} dfh_feature_t;

typedef union {
	uint64_t reg;
	struct {
		uint64_t feature_type:4;
		uint64_t reserved_8:8;
		uint64_t afu_minor:4;
		uint64_t reserved_7:7;
		uint64_t end_dfh:1;
		uint64_t next_dfh:24;
		uint64_t afu_major:4;
		uint64_t feature_id:12;
	} bits;
} dfh_reg_t;

// DMA transfer
typedef enum {
	TRANSFER_IN_PROGRESS = 0,
	TRANSFER_NOT_IN_PROGRESS = 1
} fpga_dma_transfer_status_t;

struct fpga_dma_transfer {
	uint64_t src;
	uint64_t dst;
	uint64_t len;
	fpga_dma_transfer_type_t transfer_type;
	fpga_dma_tx_ctrl_t tx_ctrl;
	fpga_dma_rx_ctrl_t rx_ctrl;
	fpga_dma_transfer_cb cb;
	bool eop_status;
	void *context;
	size_t rx_bytes;
	pthread_mutex_t tf_mutex;
	sem_t tf_status; // When locked, the transfer in progress
};

// Channel types
typedef enum {
	TX_ST = 0,
	RX_ST,
	MM
} fpga_dma_channel_type_t;

// DMA channel capabilities
struct fpga_dma_channel {
	uint32_t index;
	fpga_dma_channel_type_t ch_type;
	int   ring_size;
	int   max_sg_buffer_size;
	int   dma_alignment;
	bool  sg_support;
	bool  interrupts_support;
	bool  streaming_support;
};

// DMA handle
struct fpga_dma_buffer {
	void     *dma_buf_ptr;
	uint64_t dma_buf_wsid;
	uint64_t dma_buf_iova;
};

struct fpga_dma_mmio {
	uint32_t mmio_num;
	uint64_t mmio_offset;
	uint64_t mmio_va;
}

struct fpga_dma_handle {
	fpga_handle fpga_h;
	// DMA CSRs
	uint64_t dma_base;
	uint64_t dma_offset;
	uint64_t dma_csr_base;
	uint64_t dma_desc_base;
	uint64_t dma_rsp_base;
	uint64_t dma_streaming_valve_base;
	// for ASE
	uint64_t dma_ase_cntl_base;
	uint64_t dma_ase_data_base;
	// Interrupt event handle
	fpga_event_handle eh;
	// Managed buffers
	fpga_dma_buffer *buffer;
	// FPGA DMA mmio region
	fpga_dma_mmio mmio;
	// FPGA DMA channel
	fpga_dma_channel channel;
	// Worker thread
	pthread_t thread_id;
	// Transaction queue (model as a fixed-size circular buffer)
	qinfo_t qinfo;
};

#endif //__FPGA__DMA_TYPES_INTERNAL_H__
