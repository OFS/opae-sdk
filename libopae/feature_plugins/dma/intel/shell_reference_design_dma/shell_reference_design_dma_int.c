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
 * @brief helper functions for the 4 plugin function
 *
 */

#include "shell_reference_design_dma_int.h"



isrd_dma_t *isrd_init_dma_handle(struct _fpga_feature_token *token,
				 void *priv_config)
{
	isrd_dma_t *isrd_handle;

	if (token == NULL || token->handle == NULL || token->feature_type != DMA) {
		FPGA_MSG("FPGA_INVALID_PARAM\n");
		return NULL;
	}

	isrd_handle = calloc(1, sizeof(isrd_dma_t));
	if (isrd_handle == NULL) {
		FPGA_MSG("Failed to allocate DMA device");
		return NULL;
	}

	/* Device CSR start just after the DFH's 24B */
	isrd_handle->csr_base_offset = token->csr_offset + 24;
	if(isrd_handle->csr_base_offset & (CHANNEL_CONFIGURATION_SPACE_SIZE - 1)) {
		FPGA_MSG("CSR is not 0x%x aligment\n", CHANNEL_CONFIGURATION_SPACE_SIZE);
		goto csr_err;
	}
	isrd_handle->magic_num = ISRD_MAGIC_NUM;
	isrd_handle->fpga_h = token->handle;
	isrd_handle->channel_number = DEFAULT_CHANNEL_NUMBEER;
	isrd_handle->mmio_num = token->mmio_num;

	return isrd_handle;

csr_err:
	free(isrd_handle);
	return NULL;
}



fpga_result isrd_init_ch(int ch_num, isrd_dma_t *isrd_handle,
			 fpga_dma_transfer_type ch_type, int ring_size)
{
	isrd_ch_t *ch;
	pthread_mutexattr_t mattr;
	fpga_result res;
	uint32_t offset_in_ch;

	if (isrd_handle == NULL || ch_num >= isrd_handle->channel_number)
		return FPGA_INVALID_PARAM;

	switch (ch_type) {
		case HOST_MM_TO_FPGA_ST:
			offset_in_ch = DMA_TX_OFFSET;
			ch = &isrd_handle->tx_channels[ch_num];
			break;
		case FPGA_ST_TO_HOST_MM:
			offset_in_ch = DMA_RX_OFFSET;
			ch = &isrd_handle->rx_channels[ch_num];
			break;
		default:
			return FPGA_INVALID_PARAM;
	}

	if (pthread_mutexattr_init(&mattr)) {
		FPGA_MSG("Failed to initialized event handle mutex attributes");
		res = FPGA_EXCEPTION;
		goto out;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		FPGA_MSG("Failed to set event handle mutex attributes");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	ch->fpga_h = isrd_handle->fpga_h;
	ch->mmio_num = isrd_handle->mmio_num;
	ch->pd_ring_size = ring_size;
	ch->ch_type = ch_type;
	ch->csr_base_offset = isrd_handle->csr_base_offset
				   + ch_num * CHANNEL_CONFIGURATION_SPACE_SIZE
				   + offset_in_ch;
	if (pthread_mutex_init(&ch->lock, &mattr)) {
		FPGA_MSG("Failed to initialize Tx event handle mutex");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	res = isrd_init_hw_ch(ch);
	ON_ERR_GOTO(res, out_attr_destroy, "Failed to initialize HW for channel");

	pthread_mutexattr_destroy(&mattr);
	return FPGA_OK;

out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);
out:
	return res;
}

fpga_result isrd_init_hw_ch(isrd_ch_t *ch)
{
	fpga_result res;

	/* Clear HW head and tail */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_panging_tail) , 0);
	ON_ERR_GOTO(res, out, "writing ring_panging_tail");
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_head) , 0);
	ON_ERR_GOTO(res, out, "writing ring_head");

	/* allocate shared memory host/fpga for PDs */
	res = fpgaPrepareBuffer(ch->fpga_h,
			ch->pd_ring_size * sizeof(tx_pd_t),
			(void **)&ch->pds_base_usr_addr, &ch->pds_base_wsid, 0);
	ON_ERR_GOTO(res, out, "allocating PDs buffer");
	res = fpgaGetIOAddress(ch->fpga_h, ch->pds_base_wsid, &ch->pds_base_iova);
	ON_ERR_GOTO(res, out_free_pd_ring, "getting pds base IOVA");

	/* Allocatind memory for HW to write the HW head in the PDs ring */
	res = fpgaPrepareBuffer(ch->fpga_h, sizeof(*(ch->hw_ring_head_usr_addr)),
			(void **)&ch->hw_ring_head_usr_addr, &ch->hw_ring_head_wsid, 0);
	ON_ERR_GOTO(res, out_free_pd_ring, "allocating HW ring wirte back buffer");
	res = fpgaGetIOAddress(ch->fpga_h, ch->hw_ring_head_wsid, &ch->hw_ring_head_iova);
	ON_ERR_GOTO(res, out_free_hw_head_ring, "getting hw ring head IOVA");

	/* write to HW PD number */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_size), ch->pd_ring_size);
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring size");

	/* write shared memory host/fpga of PDs to HW */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_start_addr_l),
						LOW_32B_OF_64B(ch->pds_base_iova));
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring low addr");
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_start_addr_h),
						HIGH_32B_OF_64B(ch->pds_base_iova));
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring high addr");

	/* write location of HW head on system memory to HW */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_consumed_head_addr_l),
						LOW_32B_OF_64B(ch->hw_ring_head_iova));
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring low addr");
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_consumed_head_addr_h),
						HIGH_32B_OF_64B(ch->hw_ring_head_iova));
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring high addr");

	/* Enable ch */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ch_ctrl), CTRL_EN);
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring high addr");

	return FPGA_OK;

out_free_hw_head_ring:
	fpgaReleaseBuffer(ch->fpga_h, ch->hw_ring_head_wsid);
out_free_pd_ring:
	fpgaReleaseBuffer(ch->fpga_h, ch->pds_base_wsid);
out:
	return res;
}

fpga_result isrd_free_ch(isrd_ch_t *ch)
{
	fpga_result res;
	uint32_t val;

	if (ch == NULL) {
		FPGA_MSG("channel is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&ch->lock)) {
		FPGA_MSG("Failed to lock mutex");
		return FPGA_EXCEPTION;
	}

	res = isrd_reset_ch(ch);
	ON_ERR_GOTO(res, out, "Could not reset CH");

	res = fpgaReleaseBuffer(ch->fpga_h, ch->hw_ring_head_wsid);
	FPGA_MSG("Failed to release hw ring");
	res = fpgaReleaseBuffer(ch->fpga_h, ch->pds_base_wsid);
	FPGA_MSG("failed to release pds");
	ch->fpga_h = NULL;
	pthread_mutex_unlock(&ch->lock)
	pthread_mutex_destroy(&ch->lock);

	return FPGA_OK;
out:
	return res;
}


fpga_result isrd_reset_ch(isrd_ch_t *ch)
{
	fpga_result res;
	uint32_t val, timeout = 100000;

	if (ch == NULL) {
		FPGA_MSG("channel is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* Set reset and wait till it clears */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, reset) , RESET_RST);
	ON_ERR_GOTO(res, out, "writing reset faild");

	do {
		res = fpgaReadMMIO32(ch->fpga_h, ch->mmio_num,
			ISRD_CH_MEMBER_OFFSET(ch, reset), &val);
		ON_ERR_GOTO(res, out, "reading  reset faild");
		usleep(100);
		if (--timeout == 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, out, "reset timed out");
		}
	} while (val);

	return FPGA_OK;

out:
	return res;
}
