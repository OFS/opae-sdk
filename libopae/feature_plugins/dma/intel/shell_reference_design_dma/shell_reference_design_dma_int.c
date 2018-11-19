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

#include <unistd.h>
#include "safe_string/safe_string.h"
#include "shell_reference_design_dma_int.h"
#include "/home/lab/workspace/amirh/opae-sdk/libopae/plugins/xfpga/buffer_int.h"


isrd_dma_t *isrd_init_dma_handle(struct _fpga_feature_token *token,
				 void *priv_config)
{
	isrd_dma_t *isrd_handle;

	UNUSED_PARAM(priv_config);

	if (token == NULL || token->handle == NULL ||
			token->feature_type != DMA) {
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
	if(isrd_handle->csr_base_offset &
			(CHANNEL_CONFIGURATION_SPACE_SIZE - 1)) {
		FPGA_MSG("CSR is not 0x%x aligment\n",
			CHANNEL_CONFIGURATION_SPACE_SIZE);
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



fpga_result isrd_init_ch(uint32_t ch_num, isrd_dma_t *isrd_handle,
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
			ch->pd_size = sizeof(tx_pd_t);
			break;
		case FPGA_ST_TO_HOST_MM:
			offset_in_ch = DMA_RX_OFFSET;
			ch = &isrd_handle->rx_channels[ch_num];
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

	res = isrd_init_ch_buffers(ch);
	ON_ERR_GOTO(res, out_attr_destroy, "Failed to initialize buffers for channel");

	res = isrd_init_ch_hw(ch);
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



fpga_result isrd_init_ch_buffers(isrd_ch_t *ch)
{
	int j, pd_num, res, len, buffers_per_huge_page;
	uint64_t i, round_up_to = 8, huge_page_iova_base, huge_page_size = 2*1024*1024;
	void *huge_page_base_address = NULL;

	/* round up to nearest page boundary */
	len = ch->buffer_max_len;
	if (len & (round_up_to - 1)) {
		len = round_up_to + (len & ~(round_up_to - 1));
	}

	buffers_per_huge_page = huge_page_size / len;
	ch->pre_alloc_huge_pages_number = 1 + (ch->pd_ring_size /
							buffers_per_huge_page);

	/* room to save the wsid of each huge page */
	ch->pre_alloc_huge_pages_wsid_arr =
			calloc(ch->pre_alloc_huge_pages_number,
			       sizeof(uint64_t));

	/* room to save each PD buffer info */
	ch->pre_alloc_64k_mem_arr =
			calloc(ch->pd_ring_size, sizeof(pre_alloc_buf_t));

	if (ch->pre_alloc_huge_pages_wsid_arr == NULL ||
	    ch->pre_alloc_64k_mem_arr == NULL) {
		FPGA_MSG("Memory allocation for storing CH buffer info failed");
		return FPGA_NO_MEMORY;
	}

	for (i = 0; i < ch->pre_alloc_huge_pages_number; i++) {
		/* Allocate huge page */
		res = fpgaPrepareBuffer(ch->fpga_h, huge_page_size,
					(void **)&huge_page_base_address,
					&ch->pre_alloc_huge_pages_wsid_arr[i],
					0);
		ON_ERR_GOTO(res, out, "allocating huge buffer");

		res = fpgaGetIOAddress(ch->fpga_h,
				       ch->pre_alloc_huge_pages_wsid_arr[i],
				       &huge_page_iova_base);
		ON_ERR_GOTO(res, out2, "getting huge page IOVA");

		/* "Allocating" buffers */
		for (j = 0; j < buffers_per_huge_page; j++) {
			pd_num = j + i * buffers_per_huge_page;
			ch->pre_alloc_64k_mem_arr[pd_num].wsid =
					ch->pre_alloc_huge_pages_wsid_arr[i];
			ch->pre_alloc_64k_mem_arr[pd_num].usr_addr =
					huge_page_base_address + len * j;
			ch->pre_alloc_64k_mem_arr[pd_num].iova =
					huge_page_iova_base + len * j;
		}
	}

	return FPGA_OK;

out2:
	for (i = 0; i < ch->pre_alloc_huge_pages_number; i++)
		if (ch->pre_alloc_huge_pages_wsid_arr[i])
				fpgaReleaseBuffer(ch->fpga_h,
					ch->pre_alloc_huge_pages_wsid_arr[i]);
out:
	free(ch->pre_alloc_64k_mem_arr);
	free(ch->pre_alloc_huge_pages_wsid_arr);

	return res;



}



fpga_result isrd_init_ch_hw(isrd_ch_t *ch)
{
	fpga_result res;
	void **addr_for_usr_pd;

	if (ch->ch_type == HOST_MM_TO_FPGA_ST)
		addr_for_usr_pd = (void **)&ch->tx_pds_usr_addr;
	else
		addr_for_usr_pd = (void **)&ch->rx_pds_usr_addr;

	/* Clear HW head and tail */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
			      ISRD_CH_MEMBER_OFFSET(ch, ring_panging_tail),
			      0);
	ON_ERR_GOTO(res, out, "writing ring_panging_tail");

	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
			      ISRD_CH_MEMBER_OFFSET(ch, ring_head),
			      0);
	ON_ERR_GOTO(res, out, "writing ring_head");

	/* allocate shared host/fpga memory for PDs */
	res = fpgaPrepareBuffer(ch->fpga_h,
				ch->pd_ring_size * ch->pd_size,
				addr_for_usr_pd, &ch->pds_wsid, 0);
	ON_ERR_GOTO(res, out, "allocating PDs buffer");

	res = fpgaGetIOAddress(ch->fpga_h, ch->pds_wsid, &ch->pds_iova);
	ON_ERR_GOTO(res, out_free_pd_ring, "getting pds IOVA");

	/* Allocatind memory for HW to write the HW head in the host memory */
	res = fpgaPrepareBuffer(ch->fpga_h,
				sizeof(*(ch->hw_ring_head_usr_addr)),
				(void **)&ch->hw_ring_head_usr_addr,
				&ch->hw_ring_head_wsid, 0);
	ON_ERR_GOTO(res, out_free_pd_ring,
			"allocating HW ring wirte back buffer");

	res = fpgaGetIOAddress(ch->fpga_h,
			       ch->hw_ring_head_wsid, &ch->hw_ring_head_iova);
	ON_ERR_GOTO(res, out_free_hw_head_ring, "getting hw ring head IOVA");

	/* write to HW PD ring size */
	res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
			      ISRD_CH_MEMBER_OFFSET(ch, ring_size),
			      ch->pd_ring_size);
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
			      ISRD_CH_MEMBER_OFFSET(ch, ch_ctrl),
			      CTRL_EN);
	ON_ERR_GOTO(res, out_free_hw_head_ring, "writing CH EN bit");

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
	uint32_t i;

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
	for (i = 0; i < ch->pre_alloc_huge_pages_number; i++)
		if (ch->pre_alloc_huge_pages_wsid_arr[i])
				fpgaReleaseBuffer(ch->fpga_h,
					ch->pre_alloc_huge_pages_wsid_arr[i]);
	free(ch->pre_alloc_64k_mem_arr);
	free(ch->pre_alloc_huge_pages_wsid_arr);
	res = fpgaReleaseBuffer(ch->fpga_h, ch->hw_ring_head_wsid);
	FPGA_MSG("Failed to release hw ring");
	res = fpgaReleaseBuffer(ch->fpga_h, ch->pds_wsid);
	FPGA_MSG("failed to release pds");
	ch->fpga_h = NULL;
	ch->magic_num = 0;
	pthread_mutex_unlock(&ch->lock);
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



fpga_result update_pd_tail(isrd_ch_t *ch, uint32_t flags)
{
	fpga_result res;

	ch->unsubmitted_pds++;
	ch->pd_tail = (ch->pd_tail + 1) % ch->pd_ring_size;
	if ((flags & FORCE_SUBMIT_PD) ||
	     ch->unsubmitted_pds == PD_BATCH_BEST_SIZE)
	{
		res = fpgaWriteMMIO32(ch->fpga_h, ch->mmio_num,
				      ISRD_CH_MEMBER_OFFSET(ch, ring_panging_tail),
				      ch->pd_tail);
		ON_ERR_RETURN(res, "Failed to update HW tail");
		ch->unsubmitted_pds = 0;
	}

	return FPGA_OK;
}



int get_next_empty_pd_index(isrd_ch_t *ch)
{
	int timeout = 1000000;
	uint32_t one_over_next_pd;

	one_over_next_pd = (ch->pd_tail + 1) % ch->pd_ring_size;

	while (one_over_next_pd == *ch->hw_ring_head_usr_addr) {
		usleep(10);
		if (--timeout == 0) {
			FPGA_MSG("Get empty pd timed out");
			return -1;
		}
	}

	return ch->pd_tail;
}



fpga_result isrd_setup_buffer_for_pd(isrd_ch_t *ch, fpga_dma_transfer *xfer,
				     int pd_index, uint64_t data_xfered,
				     uint64_t data_len, uint64_t *buffer_iova)
{
	fpga_result res;
	isrd_xfer_priv_t *xfer_priv = (isrd_xfer_priv_t *)xfer->priv_data;
	int ret;

	if (xfer->wsid) {
		/* no need to copy data */
		if (xfer_priv->wsid_iova == 0) {
			/* expensive call - let's do it once */
			res = fpgaGetWsidInfo(ch->fpga_h, xfer->wsid,
					      &xfer_priv->wsid_useraddr,
					      &xfer_priv->wsid_iova);
			ON_ERR_RETURN(res, "Failed to find wsid");
		}

		*buffer_iova = xfer_priv->wsid_iova + data_xfered +
					(xfer->src - xfer_priv->wsid_useraddr);
	}
	else {
		/* Copy the data into the pre allocated buffer of this PD */
		ret = memcpy_s((void *)ch->pre_alloc_64k_mem_arr[pd_index].usr_addr,
			       ch->buffer_max_len,
			       (void *)xfer->src + data_xfered, data_len);
		ON_ERR_RETURN(ret, "Fail copy data");

	}

	return FPGA_OK;
}



/* Fill PDs with one Tx xfer */
fpga_result isrd_xfer_tx(isrd_ch_t *ch, fpga_dma_transfer *xfer)
{
	fpga_result res;
	volatile tx_pd_t *pd;
	int	next_pd_index, ret;
	isrd_xfer_priv_t xfer_priv = { 0, };
	uint64_t buff_data_len, data_xfered, metadata_left;
	uint64_t metadata_xfered, metadata_len_for_pd, data_left, buffer_iova;

// Should we put metadata as first buffer if it is WSID metadata ?
// if not, there is no point in having metadata with pre loocated buffer
//	if(!ch->ongoing_tx && xfer->len !=0 &&
//	   xfer->meta_data && xfer->meta_data_wsid) {
//		/* Send metadata from pre allocated mem in the first PD */
//		ch->wsid_metadata_xfer.src = (uint64_t)xfer->meta_data;
//		ch->wsid_metadata_xfer.len = xfer->meta_data_len;
//		ch->wsid_metadata_xfer.wsid = xfer->meta_data_wsid;
//		res = isrd_xfer_tx_wsid(ch, &ch->wsid_metadata_xfer);
//		ON_ERR_RETURN(res, failed to send metadata in first buffer");
//	}

	xfer->priv_data = (void *) &xfer_priv;

	next_pd_index = get_next_empty_pd_index(ch);
	if (next_pd_index < 0) {
		return FPGA_BUSY;
	}

	buff_data_len = xfer->len > ch->buffer_max_len ?
			ch->buffer_max_len : xfer->len;

	res = isrd_setup_buffer_for_pd(ch, xfer, next_pd_index, 0,
			buff_data_len, &buffer_iova);
	ON_ERR_RETURN(res, "Failed to setup buffers");

	pd = &ch->tx_pds_usr_addr[next_pd_index];
	pd->buffer_addr_hi = HIGH_32B_OF_64B(buffer_iova);
	pd->buffer_addr_lo = LOW_32B_OF_64B(buffer_iova);
	/* If SOP needed, SOP go in the first PD */
	if (ch->ongoing_tx)
		pd->sop = 0;
	else {
		pd->sop = 1;
		pd->metadata_len = 0;
		ch->ongoing_tx = 1;
	}
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
		pd->metadata_len = xfer->meta_data_len;
		metadata_len_for_pd =
				xfer->meta_data_len <= METADATA_AT_FIRST_PD ?
				xfer->meta_data_len : METADATA_AT_FIRST_PD;
		ret = memcpy_s((void *)pd->metadata, METADATA_AT_FIRST_PD,
			 xfer->meta_data, metadata_len_for_pd);
		ON_ERR_RETURN(ret, "Fail copy metadata");

		metadata_xfered = metadata_len_for_pd;
		res = update_pd_tail(ch, 0);
		ON_ERR_RETURN(res, "Failed to update tail");

		/* Copy metadata over 16B into next PDs. 32B in each PD */
		while (metadata_xfered < xfer->meta_data_len) {
			next_pd_index = get_next_empty_pd_index(ch);
			if (next_pd_index < 0)
				return FPGA_BUSY;

			pd = &ch->tx_pds_usr_addr[next_pd_index];
			metadata_left = xfer->meta_data_len - metadata_xfered;
			metadata_len_for_pd =
					metadata_left <= METADATA_AT_FULL_PD ?
					metadata_left : METADATA_AT_FULL_PD;
			ret = memcpy_s((void *)pd, sizeof(*pd),
				       xfer->meta_data + metadata_xfered,
				       metadata_len_for_pd);
			ON_ERR_RETURN(ret, "Fail copy metadata in seq PD");

			metadata_xfered += metadata_len_for_pd;
			res = update_pd_tail(ch, 0);
			ON_ERR_RETURN(res, "Failed to update tail");
		}
	} /* Xmit metadata */
	else {
		/* No metadata - xmit first PD */
		res = update_pd_tail(ch, 0);
		ON_ERR_RETURN(res, "Failed to update tail");
	}

	/* Large xmit can span over a few PDs */
	while (data_xfered < xfer->len) {
		next_pd_index = get_next_empty_pd_index(ch);
		if (next_pd_index < 0)
			return FPGA_BUSY;

		data_left = xfer->len - data_xfered;
		buff_data_len = data_left > ch->buffer_max_len ?
				ch->buffer_max_len : data_left;

		ret = isrd_setup_buffer_for_pd(ch, xfer, next_pd_index,
					       data_xfered, buff_data_len,
					       &buffer_iova);
		ON_ERR_RETURN(ret, "Failed to setup buffers for CH");

		pd = &ch->tx_pds_usr_addr[next_pd_index];
		pd->buffer_addr_hi = HIGH_32B_OF_64B(buffer_iova);
		pd->buffer_addr_lo = LOW_32B_OF_64B(buffer_iova);
		pd->sop = 0;
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
		ON_ERR_RETURN(res, "Failed to update tail");
	}

	return FPGA_OK;
}



fpga_result isrd_xfer_rx_sync(isrd_ch_t *ch, transfer_list *dma_xfer_list)
{
	UNUSED_PARAM(ch);
	UNUSED_PARAM(dma_xfer_list);

	return FPGA_NOT_SUPPORTED;

}



fpga_result isrd_xfer_tx_sync(isrd_ch_t *ch, transfer_list *dma_xfer_list)
{
	fpga_result res;
	fpga_dma_transfer *dma_one_xfer;
	int timeout = 1000000;
	uint32_t i;

	res = isrd_ch_check_and_lock(ch);
	ON_ERR_RETURN(res, "Failed ch check or ch lock");

	for (i = 0; i < dma_xfer_list->entries_num; i++) {
		dma_one_xfer = &dma_xfer_list->array[i];
		res = isrd_xfer_tx(ch, dma_one_xfer);
		ON_ERR_GOTO(res, out, "Failied to xfer tx");
	}

	/* Last xfer - update the HW tail */
	res = update_pd_tail(ch, FORCE_SUBMIT_PD);
	ON_ERR_GOTO(res, out, "Failied update tail");


	/* wait till xfer is done */
	/* TODO - do it outside the lock to allow addinital xfer
	 * 	Will be resolved with when Sync will call Async with callback
	 * NOTE - HW head update is not for every PD so we can "miss" it
	 */
	while (ch->pd_tail != *ch->hw_ring_head_usr_addr) {
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
	return res;
}
