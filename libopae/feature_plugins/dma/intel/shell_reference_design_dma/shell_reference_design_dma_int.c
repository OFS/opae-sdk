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
#include "buffer_int.h"



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
			ch->ch_type = HOST_MM_TO_FPGA_ST;
			ch->pd_size = sizeof(tx_pd_t);
			break;
		case FPGA_ST_TO_HOST_MM:
			offset_in_ch = DMA_RX_OFFSET;
			ch = &isrd_handle->rx_channels[ch_num];
			ch->ch_type = FPGA_ST_TO_HOST_MM;
			ch->pd_size = sizeof(rx_pd_t);
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

	ch->magic_num = ISRD_CH_MAGIC_NUM;
	ch->fpga_h = isrd_handle->fpga_h;
	ch->mmio_num = isrd_handle->mmio_num;
	ch->pd_ring_size = ring_size;
	ch->ch_type = ch_type;
	ch->buffer_max_len = ISRD_BUFFER_MAX_LEN;
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
	ch->magic_num = 0;
	ch->fpga_h = NULL;
	return res;
}

fpga_result isrd_init_hw_ch(isrd_ch_t *ch)
{
	fpga_result res;
	void **add_for_usr_pd;

	if (ch->ch_type == HOST_MM_TO_FPGA_ST)
		add_for_usr_pd = (void **)&ch->tx_pds_usr_addr;
	else
		add_for_usr_pd = (void **)&ch->rx_pds_usr_addr;

	/* Clear HW head and tail */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_panging_tail) , 0);
	ON_ERR_GOTO(res, out, "writing ring_panging_tail");
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_head) , 0);
	ON_ERR_GOTO(res, out, "writing ring_head");

	/* allocate shared memory host/fpga for PDs */
	res = fpgaPrepareBuffer(ch->fpga_h,
			ch->pd_ring_size * ch->pd_size,
			add_for_usr_pd, &ch->pds_wsid, 0);
	ON_ERR_GOTO(res, out, "allocating PDs buffer");
	res = fpgaGetIOAddress(ch->fpga_h, ch->pds_wsid, &ch->pds_iova);
	ON_ERR_GOTO(res, out_free_pd_ring, "getting pds IOVA");

	/* Allocatind memory for HW to write the HW head in the host memory */
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
						LOW_32B_OF_64B(ch->pds_iova));
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing ring low addr");
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, ring_start_addr_h),
						HIGH_32B_OF_64B(ch->pds_iova));
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
	fpgaReleaseBuffer(ch->fpga_h, ch->pds_wsid);
out:
	return res;
}

fpga_result isrd_free_ch(isrd_ch_t *ch)
{
	fpga_result res;
	uint32_t val;

	if (ch == NULL || ch->fpga_h == NULL) {
		FPGA_MSG("channel or fpga_h is NULL");
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
	res = fpgaReleaseBuffer(ch->fpga_h, ch->pds_wsid);
	FPGA_MSG("failed to release pds");
	ch->fpga_h = NULL;
	ch->magic_num = 0;
	pthread_mutex_unlock(&ch->lock)
	pthread_mutex_destroy(&ch->lock);

	return FPGA_OK;
out:
	return res;
}

/* Call this function with mutex lock */
fpga_result isrd_reset_ch(isrd_ch_t *ch)
{
	fpga_result res;
	uint32_t val, timeout = 100000;

	if (ch == NULL || ch->fpga_h == NULL) {
		FPGA_MSG("channel or handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* Set reset and wait till it clears */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
		ISRD_CH_MEMBER_OFFSET(ch, reset) , RESET_RST);
	ON_ERR_GOTO(res, out, "writing reset faild");

	while(true) {
		res = fpgaReadMMIO32(ch->fpga_h, ch->mmio_num,
			ISRD_CH_MEMBER_OFFSET(ch, reset), &val);
		ON_ERR_GOTO(res, out, "reading  reset faild");

		if (!(val & RESET_RST))
			break;

		usleep(100);
		if (--timeout == 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, out, "reset timed out");
		}
	}

	return FPGA_OK;

out:
	return res;
}



/*
 * Check handle object for validity and lock its mutex
 * If handle_check_and_lock() returns FPGA_OK, assume the mutex to be locked.
 */
fpga_result isrd_ch_check_and_lock(isrd_ch_t *ch)
{
	if (ch == NULL || pthread_mutex_lock(&ch->lock)) {
		FPGA_MSG("Failed to lock mutex");
		return FPGA_EXCEPTION;
	}

	if (ch->magic_num != ISRD_CH_MAGIC_NUM) {
		FPGA_MSG("Invalid handle object");
		pthread_mutex_unlock(&ch->lock);

		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}


fpga_result isrd_xfer_tx_fill_pd()
{
 return -1;

}



fpga_result update_pd_tail(isrd_ch_t ch, uint32_t flags)
{
	fpga_result res;

	ch->unsubmitted_pds++;
	ch->pd_tail = (ch->pd_tail + 1) % ch->pd_ring_size;
	if ((flags & FORCE_SUBMIT_PD) || ch->unsubmitted_pds == PD_BATCH_BEST_SIZE)
	{
		res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
			ISRD_CH_MEMBER_OFFSET(ch, ring_panging_tail),
			ch->pd_tail);
		if (res != FPGA_OK) {
			FPGA_MSG("Failed to update HW tail");
			return res;
		}
		ch->unsubmitted_pds = 0;
	}

	return FPGA_OK;
}



int get_next_empty_pd_index(isrd_ch_t ch)
{
	int one_over_next_pd, timeout = 1000000;

	one_over_next_pd = (ch->pd_tail + 1) % ch->pd_ring_size;

	while (one_over_next_pd == ch->hw_ring_head_usr_addr) {
		usleep(10);
		if (--timeout == 0) {
			FPGA_MSG("Get empty pd timed out");
			return -1;
		}
	}

	return ch->pd_tail;
}


/* Fill PDs with Tx xfer of one pre allocated memory buffer */
fpga_result isrd_xfer_tx_wsid(isrd_ch_t *ch, fpga_dma_transfer *xfer)
{
	fpga_result res;
	tx_pd_t	*pd;
	int	ret;
	uint64_t wsid_iova, wsid_useraddr, src_iova, next_pd_index, buff_data_len, data_xfered;
	uint64_t metadata_xfered, metadata_len_for_pd, data_left, metadata_left;

	if(!ch->ongoing_tx && xfer->meta_data && xfer->meta_data_wsid) {
		/* Send metadata from pre allocated mem in the first PD */
		ch->wsid_metadata_xfer.src = (uint64_t)xfer->meta_data;
		ch->wsid_metadata_xfer.len = xfer->meta_data_len;
		ch->wsid_metadata_xfer.wsid = xfer->meta_data_wsid;
		res = isrd_xfer_tx_sync(ch, &ch->wsid_metadata_xfer);
		if (res)
			return res;
	}

	res = fpgaGetWsidInfo(ch->fpga_h, xfer->wsid,
			      &wsid_useraddr, &wsid_iova);
	if (res != FPGA_OK) {
		FPGA_MSG("Failed to find wsid");
		return res;
	}
	src_iova = wsid_iova + (xfer->src - wsid_useraddr);
	next_pd_index = get_next_empty_pd_index(ch);
	if (next_pd_index < 0)
		return FPGA_BUSY;

	pd = &ch->tx_pds_usr_addr[next_pd_index];
	pd->buffer_addr_hi = HIGH_32B_OF_64B(src_iova);
	pd->buffer_addr_lo = LOW_32B_OF_64B(src_iova);
	/* If SOP needed, SOP go in the first PD */
	if (ch->ongoing_tx)
		pd->sop = 0;
	else {
		pd->sop = 1;
		ch->ongoing_tx = 1;
	}

	buff_data_len = xfer->len > ch->buffer_max_len ? ch->buffer_max_len : xfer->len;
	pd->len = buff_data_len;
	data_xfered = buff_data_len;

	/* If EOP needed, EOP go in the last PD
	 * In case of muliple PDs for one xfer with EOP
	 * EOP need to be set only in the last PD */
	if (xfer->tx_eop && data_xfered == xfer->len) {
		pd->eop = 1;
		/* Will set SOP at next PD */
		ch->ongoing_tx = 0;
	}
	else {
		pd->eop = 0;
	}

	/* Metadata xmit
	 * First 16B of metadata go in the first PD
	 * Then evey 32B go into the PD after */
	if (pd->sop && xfer->meta_data_len && !(xfer->meta_data_wsid)) {
		metadata_len_for_pd = xfer->meta_data_len <= METADATA_AT_FIRST_PD ?
				xfer->meta_data_len : METADATA_AT_FIRST_PD;
		// *** TODO - set metadata len in PD ***
		ret = memcpy_s(pd->metadata, METADATA_AT_FIRST_PD,
			 xfer->meta_data, metadata_len_for_pd);
		if (ret) {
			FPGA_MSG("Failied to copy metadata in first PD. ERR %d", ret);
			return FPGA_EXCEPTION;
		}
		metadata_xfered = metadata_len_for_pd;
		res = update_pd_tail(ch, 0);
		if (res)
			return res;

		/* Copy metadata over 16B into next PDs. 32B in each PD */
		while (metadata_xfered < xfer->meta_data_len) {
			next_pd_index = get_next_empty_pd_index(ch);
			if (next_pd_index < 0)
				return FPGA_BUSY;

			pd = &ch->tx_pds_usr_addr[next_pd_index];
			metadata_left = xfer->meta_data_len - metadata_xfered;
			metadata_len_for_pd = metadata_left <= METADATA_AT_FULL_PD ?
					metadata_left : METADATA_AT_FULL_PD;
			ret = memcpy_s(pd, sizeof(*pd), xfet->meta_data + metadata_xfered,
				 metadata_len_for_pd);
			if (ret) {
				FPGA_MSG("Failied to copy metadata in seq PD. ERR %d", ret);
				return FPGA_EXCEPTION;
			}
			metadata_xfered += metadata_len_for_pd;
			res = update_pd_tail(ch, 0);
			if (res)
				return res;
		}
	} /* Xmit metadata */
	else {
		/* No metadata - xmit first PD */
		// *** TODO - set metadata len in PD tp zero ***
		res = update_pd_tail(ch, 0);
		if (res)
			return res;
	}

	/* Large xmit can span over a few PDs */
	while (data_xfered < xfer->len) {
		next_pd_index = get_next_empty_pd_index(ch);
		if (next_pd_index < 0)
			return FPGA_BUSY;

		pd = &ch->tx_pds_usr_addr[next_pd_index];
		pd->buffer_addr_hi = HIGH_32B_OF_64B(src_iova + data_xfered);
		pd->buffer_addr_lo = LOW_32B_OF_64B(src_iova + data_xfered);
		pd->sop = 0;
		/* TODO set metadata to zero */
		data_left = xfer->len - data_xfered;
		buff_data_len = data_left > ch->buffer_max_len ?
				ch->buffer_max_len : data_left;
		pd->len = buff_data_len;
		data_xfered += buff_data_len;

		/* If EOP needed, EOP go in the last PD
		 * In case of muliple PDs for one xfer with EOP
		 * EOP needs to be set only in the last PD */
		if (xfer->tx_eop && data_xfered == xfer->len) {
			pd->eop = 1;
			/* Will set SOP at next PD */
			ch->ongoing_tx = 0;
		}
		else {
			pd->eop = 0;
		}
		res = update_pd_tail(ch, 0);
		if (res)
			return res;
	}

	return FPGA_OK;
}



fpga_result isrd_xfer_tx_sync(isrd_ch_t *ch, transfer_list *dma_xfer_list)
{
	fpga_result res;
	fpga_dma_transfer *dma_xfer_arr, *dma_one_xfer;
	int i, timeout = 1000000;

	res = isrd_ch_check_and_lock(ch);
	if (res)
		return res;

	dma_xfer_arr =  dma_xfer_list->array;
	for (i = 0; i < dma_xfer_list->entries_num; i++) {
		dma_one_xfer = &dma_xfer_arr[i];
		if (dma_one_xfer->wsid) {
			res = isrd_xfer_tx_wsid(ch, dma_one_xfer);
		}
		else {
			res = isrd_xfer_tx_user_mem(ch, dma_one_xfer);
		}
	}

	/* Last xfer - update the HW tail */
	res = update_pd_tail(ch, FORCE_SUBMIT_PD);


	/* wait till xfer is done */
	/* TODO - do it outside the lock to allow addinital xfer
	 * NOTE - HW head update is not for every PD so we can "miss" it
	 */
	while (ch->pd_tail != ch->hw_ring_head_usr_addr) {
		usleep(10);
		if (--timeout == 0) {
			FPGA_MSG("xfer timeout");
			goto out;
		}
	}

	pthread_mutex_unlock(&ch->lock);

	return FPGA_OK;

out:
	pthread_mutex_unlock(&ch->lock);
	return -1;
}
