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
#define FPGA_DMA_CSR_BUSY          (1<<0)
#define FPGA_DMA_DESC_BUFFER_EMPTY 0x2
#define FPGA_DMA_DESC_BUFFER_FULL  0x4

// Alignment byte sizes
#define FPGA_DMA_ALIGN_BYTES 64
#define CACHE_LINE_SIZE 64

// DMA transfer
struct _fpga_dma_transfer {
	uint64_t	       src;
	uint64_t	       dst;
	uint64_t	       len;
	fpga_dma_transfer_type type;
	fpga_dma_tx_ctrl       tx_ctrl;
	fpga_dma_rx_ctrl       rx_ctrl;
	fpga_dma_transfer_cb   cb;
	bool		       eop_status;
	void		       *context;
	size_t		       rx_bytes;
	pthread_mutex_t	       tf_mutex;
	sem_t		       tf_status;
};

typedef struct _dma_channel_desc {
	uint32_t               mmio_num;
	uint64_t               mmio_offset;
	uint64_t               mmio_va;
	uint64_t               dma_base;
	uint64_t               dma_csr_base;
	uint64_t               dma_desc_base;
};

// DMA channel capabilities
struct _fpga_dma_token {
	fpga_token	       parent_token;
	_dma_channel_desc      desc;
	uint32_t	       index;
	fpga_dma_channel_type  type;
	int		       ring_size;
	int		       max_sg_buffer_size;
	int		       dma_alignment;
	bool		       sg_support;
	bool		       interrupts_support;
	bool		       streaming_support;
	uint64_t	       dma_ase_cntl_base;
	uint64_t	       dma_ase_data_base;
};

// DMA handle
struct fpga_dma_buffer {
	void     *dma_buf_ptr;
	uint64_t dma_buf_wsid;
	uint64_t dma_buf_iova;
};

struct _fpga_dma_channel_handle {
	_fpga_dma_token	  token;
	uint64_t	  cur_ase_page;
	fpga_event_handle eh;
	fpga_dma_buffer	  *buffer;
	pthread_t	  thread_id;
	qinfo_t		  qinfo;
};

#endif //__FPGA__DMA_TYPES_INTERNAL_H__
