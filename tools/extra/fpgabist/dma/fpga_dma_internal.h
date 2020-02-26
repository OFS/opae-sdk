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
 * \fpga_dma_internal.h
 * \brief FPGA DMA BBB Internal Header
 */

#ifndef __FPGA_DMA_INT_H__
#define __FPGA_DMA_INT_H__
#include <opae/fpga.h>
#include "fpga_dma_types.h"
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "tbb/concurrent_queue.h"
#include "x86-sse2.h"
#include <iostream>
#include <fstream>


using namespace std;
using namespace tbb;

#define FPGA_DMA_ERR(msg_str) \
		fprintf(stderr, "Error %s: %s\n", __FUNCTION__, msg_str);

#define MIN(X,Y) (X<Y)?X:Y

#define QWORD_BYTES 8
#define DWORD_BYTES 4
#define IS_ALIGNED_DWORD(addr) (addr%4==0)
#define IS_ALIGNED_QWORD(addr) (addr%8==0)

#define M2S_DMA_UUID_H                0xfee69b442f7743ed
#define M2S_DMA_UUID_L                0x9ff49b8cf9ee6335
#define S2M_DMA_UUID_H                0xf118209ad59a4b3f
#define S2M_DMA_UUID_L                0xa66cd700a658a015
#define M2M_DMA_UUID_H                0xef82def7f6ec40fc
#define M2M_DMA_UUID_L                0xa9149a35bace01ea
#define FPGA_DMA_HOST_MASK            0x2000000000000

#define AFU_DFH_REG 0x0
#define AFU_DFH_NEXT_OFFSET 16
#define AFU_DFH_EOL_OFFSET 40
#define AFU_DFH_TYPE_OFFSET 60

// BBB Feature ID (refer CCI-P spec)
#define FPGA_DMA_BBB 0x2

// Feature ID for DMA BBB
#define FPGA_DMA_BBB_FEATURE_ID 0x765

// DMA Register offsets from base
#define FPGA_DMA_ST_CSR 0x40
#define FPGA_DMA_MM_CSR 0x40
#define FPGA_DMA_PREFETCHER 0x80

//#define FPGA_DMA_CSR 0x40

#define CSR_BASE(dma_h) ((uint64_t)dma_h->dma_csr_base)
#define CSR_STATUS(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, status))
#define PREFETCHER_BASE(dma_h) ((uint64_t)dma_h->dma_prefetcher_base)

#define PREFETCHER_CTRL(dma_h) (PREFETCHER_BASE(dma_h) + offsetof(msgdma_prefetcher_csr_t, ctrl))
#define PREFETCHER_START(dma_h) (PREFETCHER_BASE(dma_h) + offsetof(msgdma_prefetcher_csr_t, start_loc))
#define PREFETCHER_FETCH(dma_h) (PREFETCHER_BASE(dma_h) + offsetof(msgdma_prefetcher_csr_t, fetch_loc))
#define PREFETCHER_STATUS(dma_h) (PREFETCHER_BASE(dma_h) + offsetof(msgdma_prefetcher_csr_t, status))

#define CSR_STATUS(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, status))
#define CSR_CONTROL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, ctrl))
#define CSR_FILL_LEVEL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, fill_level))
#define CSR_RSP_FILL_LEVEL(dma_h) (CSR_BASE(dma_h) + offsetof(msgdma_csr_t, rsp_level)

#define HOST_MMIO_32_ADDR(dma_handle,offset) ((volatile uint32_t *)((uint64_t)(dma_handle)->mmio_va + (uint64_t)(offset)))
#define HOST_MMIO_64_ADDR(dma_handle,offset) ((volatile uint64_t *)((uint64_t)(dma_handle)->mmio_va + (uint64_t)(offset)))
#define HOST_MMIO_32(dma_handle,offset) (*HOST_MMIO_32_ADDR(dma_handle,offset))
#define HOST_MMIO_64(dma_handle,offset) (*HOST_MMIO_64_ADDR(dma_handle,offset))

#define FPGA_DMA_MASK_32_BIT 0xFFFFFFFF

#define FPGA_DMA_CSR_BUSY (1<<0)
#define FPGA_DMA_DESC_BUFFER_EMPTY 0x2
#define FPGA_DMA_DESC_BUFFER_FULL 0x4

#define FPGA_DMA_ALIGN_BYTES 64
#define IS_DMA_ALIGNED(addr) (addr%FPGA_DMA_ALIGN_BYTES==0)

#define FPGA_DMA_BUF_SIZE (1024*1024)
#define FPGA_DMA_BUF_ALIGN_SIZE FPGA_DMA_BUF_SIZE

#define MIN_SSE2_SIZE 4096
#define CACHE_LINE_SIZE 64
#define ALIGN_TO_CL(x) ((uint64_t)(x) & ~(CACHE_LINE_SIZE - 1))
#define IS_CL_ALIGNED(x) (((uint64_t)(x) & (CACHE_LINE_SIZE - 1)) == 0)

#define HOST_MEM_MASK(dma_h) (dma_h->ch_type == MM ? 0x1000000000000 : 0x0)

// Convenience macros
#ifdef FPGA_DMA_DEBUG
#define debug_print(fmt, ...) \
do { \
	if (FPGA_DMA_DEBUG) {\
		fprintf(stderr, "%s (%d) : ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#define error_print(fmt, ...) \
do { \
	fprintf(stderr, "%s (%d) : ", __FUNCTION__, __LINE__); \
	fprintf(stderr, fmt, ##__VA_ARGS__); \
	err_cnt++; \
 } while (0)
#else
#define debug_print(...)
#define error_print(...)
#endif

// Channel types
typedef enum {
	TRANSFER_PENDING = 0,
	TRANSFER_COMPLETE = 1
} fpga_transf_status_t;

// DFH Features
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

// Host memory metadata for a block of descriptors
typedef struct {
	// block va
	uint64_t *block_va;
	// block wsid
	uint64_t block_wsid;
	// block pa
	uint64_t block_iova;
} msgdma_block_mem_t;

// Descriptor control fields
typedef union {
	uint32_t reg;
	struct {
		uint32_t tx_channel:8;
		uint32_t generate_sop:1;
		uint32_t generate_eop:1;
		uint32_t park_reads:1;
		uint32_t park_writes:1;
		uint32_t end_on_eop:1;
		uint32_t eop_rvcd_irq_en:1;
		uint32_t transfer_irq_en:1;
		uint32_t early_term_irq_en:1;
		uint32_t trans_error_irq_en:8;
		uint32_t early_done_en:1;
		uint32_t wait_for_wr_rsp:1;
		uint32_t reserved_2:5;
		uint32_t go:1;
	};
} msgdma_desc_ctrl_t;

typedef union {
	uint32_t volatile reg;
	volatile struct {
		uint32_t busy:1;
		uint32_t desc_buf_empty:1;
		uint32_t desc_buf_full:1;
		uint32_t rsp_buf_empty:1;
		uint32_t rsp_buf_full:1;
		uint32_t stopped:1;
		uint32_t resetting:1;
		uint32_t stopped_on_errror:1;
		uint32_t stopped_on_early_term:1;
		uint32_t irq:1;
		uint32_t reserved:22;
	} st;
} msgdma_status_t;

typedef union {
	uint32_t reg;
	struct {
		uint32_t stop_dispatcher:1;
		uint32_t reset_dispatcher:1;
		uint32_t stop_on_error:1;
		uint32_t stopped_on_early_term:1;
		uint32_t global_intr_en_mask:1;
		uint32_t stop_descriptors:1;
		uint32_t flush_descriptors:1;
		uint32_t flush_rd_master:1;
		uint32_t flush_wr_master:1;
		uint32_t rsvd:19;
	} ct;
} msgdma_ctrl_t;

typedef union {
	uint32_t reg;
	struct {
		uint32_t rd_fill_level:16;
		uint32_t wr_fill_level:16;
	} fl;
} msgdma_fill_level_t;

typedef union {
	uint32_t reg;
	struct {
		uint32_t rsp_fill_level:16;
		uint32_t rsvd:16;
	} rsp;
} msgdma_rsp_level_t;

typedef union {
	uint32_t reg;
	struct {
		uint32_t rd_seq_num:16;
		uint32_t wr_seq_num:16;
	} seq;
} msgdma_seq_num_t;

typedef struct __attribute__((__packed__)) {
	// 0x0
	msgdma_status_t status;
	// 0x4
	msgdma_ctrl_t ctrl;
	// 0x8
	msgdma_fill_level_t fill_level;
	// 0xc
	msgdma_rsp_level_t rsp_level;
	// 0x10
	msgdma_seq_num_t seq_num;
} msgdma_csr_t;

// Hardware descriptor
typedef struct __attribute__((__packed__)) {
	// word 0
	uint8_t format;
	uint8_t block_size;
	volatile uint8_t owned_by_hw;
	uint8_t rsvd1;	
	msgdma_desc_ctrl_t ctrl;
	// word 1
	uint64_t src;
	// word 2
	uint64_t dst;
	// word 3
	uint32_t len;
	uint32_t rsvd2;
	// word 4
	uint16_t seq_num;
	uint8_t rd_burst;
	uint8_t wr_burst;
	uint16_t rd_stride;
	uint16_t wr_stride;
	// word 5
	volatile uint32_t bytes_transferred;
	volatile uint8_t error;
	volatile uint8_t eop_arrived;
	volatile uint8_t early_term;
	uint8_t rsvd3;
	// word 6
	uint64_t rsvd4;
	// word 7
	uint64_t next_desc;
} msgdma_hw_desc_t;

// Buffer
struct fpga_dma_transfer {
	volatile uint64_t src;
	uint64_t dst;
	uint64_t len;
	fpga_dma_transfer_type_t transfer_type;
	fpga_dma_tx_ctrl_t tx_ctrl;
	fpga_dma_rx_ctrl_t rx_ctrl;
	fpga_dma_transfer_cb cb;
	bool eop_arrived;
	void *context;
	size_t bytes_transferred;
	pthread_mutex_t tf_mutex;	
	bool is_last_buf;
};

// Pointer to hardware descriptor, with additional metadata for use by driver
typedef struct msgdma_hw_descp {
	//metadata for debug
	uint64_t hw_block_id;
	uint64_t hw_desc_id;
	msgdma_hw_desc_t *hw_desc; // ptr to desc in hw chain
} msgdma_hw_descp_t;

// Software descriptor
typedef struct msgdma_sw_desc {
	uint64_t id;
	msgdma_hw_descp_t *hw_descp; // assigned hw descriptor
	struct fpga_dma_transfer *transfer;
	sem_t tf_status; // When locked, the transfer in progress
	bool kill_worker;
	uint64_t last;
} msgdma_sw_desc_t;

// DMA handle
struct fpga_dma_handle {
	fpga_handle fpga_h;
	uint32_t mmio_num;
	uint64_t mmio_offset;
	uint64_t mmio_va;
	uint64_t dma_base;
	uint64_t dma_offset;
	uint64_t dma_csr_base;
	uint64_t dma_desc_base;
	uint64_t dma_prefetcher_base;
	// pointer to hardware descriptor block-chain
	msgdma_block_mem_t *block_mem;
	concurrent_queue<struct msgdma_sw_desc*> ingress_queue;
	concurrent_queue<struct msgdma_sw_desc*> pending_queue;	
	concurrent_queue<struct msgdma_hw_descp*> free_desc;
	concurrent_queue<struct msgdma_hw_descp*> invalid_desc_queue;
	// channel type
	fpga_dma_channel_type_t ch_type;
        #define INVALID_CHANNEL (0x7fffffffffffffffULL)
	uint64_t dma_channel;
	pthread_t ingress_id;
	pthread_t pending_id;
	pthread_mutex_t dma_mutex;
	sem_t dma_init;
	volatile bool invalidate;
	volatile bool terminate;
};

// Prefetcher ctrl register
typedef union {
	uint64_t reg;
	struct {
		uint64_t fetch_en:1;
		uint64_t flush_desc:1;
		uint64_t irq_mask:1;
		uint64_t timeout_en:1;
		uint64_t rsvd1:12;
		uint64_t timeout_val:16;
		uint64_t rsvd2:32;
	} ct;
} msgdma_prefetcher_ctrl_t;

// MSGDMA status register
typedef union {
	uint64_t volatile reg;
	volatile struct {
		uint64_t irq:1;
		uint64_t fetch_idle:1;
		uint64_t store_idle:1;
		uint64_t rsvd1:5;
		uint64_t fetch_state:3;
		uint64_t store_state:2;
		uint64_t rsvd2:3;
		uint64_t outstanding_fetches:16;
		uint64_t rsvd3:32;
	} st;
} msgdma_prefetcher_status_t;

// MSGDMA fill levels
typedef union {
	uint64_t reg;
	struct {
		uint64_t fetch_desc_watermark:16;
		uint64_t store_desc_watermark:16;
		uint64_t store_resp_watermark:16;
		uint64_t rsvd:16;
	} fl;
} msgdma_prefetcher_fill_level_t;

typedef struct __attribute__((__packed__)) {
	msgdma_prefetcher_ctrl_t ctrl; // 0x0
	uint64_t start_loc; // 0x8
	uint64_t fetch_loc; // 0x10
	uint64_t store_loc; // 0x18
	msgdma_prefetcher_status_t status; // 0x20
	msgdma_prefetcher_fill_level_t fill_level; // 0x28
	uint64_t rsvd1; // 0x30
	uint64_t rsvd2; // 0x38
} msgdma_prefetcher_csr_t;

#endif // __FPGA_DMA_INT_H__
