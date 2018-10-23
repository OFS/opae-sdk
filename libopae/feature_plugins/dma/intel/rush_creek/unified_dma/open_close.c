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
 * \open_close.c
 * \brief FPGA Streaming DMA User-mode driver open, close, and enumerate
 * routines
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <opae_int.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <safe_string/safe_string.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

// Internal functions
static void addDMADescriptor(uint32_t *index, fpga_dma_channel_type_t type,
			     fpga_dma_channel_desc *descriptors,
			     uint32_t max_index, uint64_t mmio_va,
			     internal_channel_desc *int_desc, uint64_t offset)
{
	uint32_t ndx = *index;

	if (int_desc) {
		int_desc[ndx].desc.index = ndx;
		int_desc[ndx].desc.ch_type = type;
		int_desc[ndx].mmio_num = 0;
		int_desc[ndx].mmio_offset = 0;
		int_desc[ndx].mmio_va = mmio_va;
		int_desc[ndx].dma_base = offset;
		int_desc[ndx].dma_csr_base = offset + FPGA_DMA_CSR;
		int_desc[ndx].dma_desc_base = offset + FPGA_DMA_DESC;
	} else {
		if ((NULL == descriptors) || (*index >= max_index)) {
			*index = *index + 1;
			return;
		}

		descriptors[ndx].index = ndx;
		descriptors[ndx].ch_type = type;
	}

	*index = ndx + 1;

	return;
}


// Check for successful thread startup
static int checkThreadStart(sem_t *sem)
{
	struct timespec ts;
	int s;

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		FPGA_DMA_ST_ERR("clock_gettime failure");
		return -1;
	}

	ts.tv_sec++; // Wait ~1 second

	while (((s = sem_timedwait(sem, &ts)) == -1) && (errno == EINTR)) {
		continue; /* Restart if interrupted by handler */
	}

	if ((s == -1) && (errno == ETIMEDOUT)) {
		FPGA_DMA_ST_ERR("pthread_create thread startup timeout");
		return -2;
	}

	return 0;
}

static void releaseChannelResources(handle_common *hdr)
{
	fpga_result res = FPGA_OK;
	msgdma_ctrl_t ctrl = {0};

	res = fpgaUnregisterEvent(hdr->fpga_h, FPGA_EVENT_INTERRUPT, hdr->eh);
	ON_ERR_GOTO(res, out, "fpgaUnregisterEvent failed");

	res = fpgaDestroyEventHandle(&hdr->eh);
	ON_ERR_GOTO(res, out, "fpgaDestroyEventHandle failed");

	ctrl.ct.global_intr_en_mask = 0;
	res = MMIOWrite32Blk(hdr, CSR_CONTROL(hdr), (uint64_t)&ctrl.reg,
			     sizeof(ctrl.reg));

	// Make sure double-close fails
	hdr->dma_channel = INVALID_CHANNEL;
	hdr->magic_id = 0;

out:
	return;
}


// Public APIs
static fpga_result
fpgaDMAEnumerateChannelsInt(fpga_handle fpga, uint32_t max_descriptors,
			    fpga_dma_channel_desc *descriptors,
			    uint32_t *num_descriptors,
			    internal_channel_desc *int_desc)
{
	// Discover total# DMA channels by traversing the device feature list
	// We may encounter one or more BBBs during discovery
	// Populate the count
	fpga_result res = FPGA_OK;

	if (!fpga) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!num_descriptors) {
		FPGA_DMA_ST_ERR("Invalid pointer to num_descriptors");
		return FPGA_INVALID_PARAM;
	}

	*num_descriptors = 0;
	uint64_t offset = 0;
#ifndef USE_ASE
	uint64_t mmio_va = 0;

	res = fpgaMapMMIO(fpga, 0, (uint64_t **)&mmio_va);
	ON_ERR_GOTO(res, out, "fpgaMapMMIO");
#endif
	// Discover DMA BBB channels by traversing the device feature list
	bool end_of_list = false;
	uint64_t dfh = 0;
	do {
		uint64_t feature_uuid_lo, feature_uuid_hi;
#ifndef USE_ASE
		// Read the next feature header
		dfh = *((volatile uint64_t *)((uint64_t)mmio_va
					      + (uint64_t)(offset)));

		// Read the current feature's UUID
		feature_uuid_lo =
			*((volatile uint64_t *)((uint64_t)mmio_va
						+ (uint64_t)(offset + 8)));
		feature_uuid_hi =
			*((volatile uint64_t *)((uint64_t)mmio_va
						+ (uint64_t)(offset + 16)));
#else
		uint32_t mmio_no = 0;
		// Read the next feature header
		res = fpgaReadMMIO64(fpga, mmio_no, offset, &dfh);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");

		// Read the current feature's UUID
		res = fpgaReadMMIO64(fpga, mmio_no, offset + 8,
				     &feature_uuid_lo);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");

		res = fpgaReadMMIO64(fpga, mmio_no, offset + 16,
				     &feature_uuid_hi);
		ON_ERR_GOTO(res, out, "fpgaReadMMIO64");
#endif
		if (_fpga_dma_feature_is_bbb(dfh)) {
			if ((feature_uuid_lo == M2S_DMA_UUID_L)
			    && (feature_uuid_hi == M2S_DMA_UUID_H)) {
				// Found one. Record it.
				addDMADescriptor(num_descriptors, TX_ST,
						 descriptors, max_descriptors,
						 mmio_va, int_desc, offset);
			}
			if ((feature_uuid_lo == S2M_DMA_UUID_L)
			    && (feature_uuid_hi == S2M_DMA_UUID_H)) {
				// Found one. Record it.
				addDMADescriptor(num_descriptors, RX_ST,
						 descriptors, max_descriptors,
						 mmio_va, int_desc, offset);
			}
			if ((feature_uuid_lo == FPGA_DMA_UUID_L)
			    && (feature_uuid_hi == FPGA_DMA_UUID_H)) {
				// Found one. Record it.
				addDMADescriptor(num_descriptors, MM,
						 descriptors, max_descriptors,
						 mmio_va, int_desc, offset);
			}
		}

		// End of the list?
		end_of_list = _fpga_dma_feature_eol(dfh);
		// Move to the next feature header
		offset = offset + _fpga_dma_feature_next(dfh);
	} while (!end_of_list);

out:
	return res;
}

inline fpga_result fpgaDMAEnumerateChannels(fpga_dma_handle dma_ch,
					    uint32_t max_descriptors,
					    fpga_dma_channel_desc *descriptors,
					    uint32_t *num_descriptors)
{
	m2m_dma_handle_t *dma_h = (m2m_dma_handle_t *)dma_ch;

	if (!dma_h || IS_CHANNEL_HANDLE(dma_h)) {
		return FPGA_INVALID_PARAM;
	}
	return fpgaDMAEnumerateChannelsInt(dma_h->header.fpga_h,
					   max_descriptors, descriptors,
					   num_descriptors, NULL);
}

fpga_result fpgaDMAOpen(fpga_feature_token token, int flags, void *priv_config,
			fpga_feature_handle *handle)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h;
	struct _fpga_feature_token *token_ =
		(struct _fpga_feature_token *)token;
	fpga_handle fpga = token_->handle;
	struct _fpga_feature_handle *handle_ =
		(struct _fpga_feature_handle *)*handle;

	if (fpgaDMAIsOpen)
	{
		FPGA_DMA_ST_ERR("Attempt to open DMA multiple times");
		return FPGA_BUSY;
	}

	if (!fpga) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!handle) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA handle");
		return FPGA_INVALID_PARAM;
	}

	// init the dma handle - zero everything
	dma_h = (fpga_dma_handle_t *)calloc(1, sizeof(fpga_dma_handle_t));
	if (!dma_h) {
		return FPGA_NO_MEMORY;
	}

	dma_h->main_header.fpga_h = fpga;
	dma_h->main_header.dma_h = dma_h;

	if (pthread_spin_init(&dma_h->sem_mutex, 0)) {
		FPGA_DMA_ST_ERR("pthread_spin_init failed");
		goto free_dma_h;
	}

	if (pthread_spin_init(&dma_h->mutex_mutex, 0)) {
		FPGA_DMA_ST_ERR("pthread_spin_init failed");
		goto free_dma_h;
	}

	if (pthread_spin_init(&dma_h->buffer_mutex, 0)) {
		FPGA_DMA_ST_ERR("pthread_spin_init failed");
		goto free_dma_h;
	}

	res = fpgaDMAQueueInit(dma_h, &dma_h->transferCompleteq);
	if (res != FPGA_OK) {
		res = fpgaDMAQueueDestroy(dma_h, &dma_h->transferCompleteq,
					  false);
		ON_ERR_GOTO(FPGA_EXCEPTION, free_dma_h,
			    "fpgaDMAQueueDestroy failed");
	}

	uint32_t num_descs;

	res = fpgaDMAEnumerateChannelsInt(dma_h->main_header.fpga_h, 0, NULL,
					  &num_descs, NULL);
	ON_ERR_GOTO(res, rel_buf, "DMA fpgaDMAEnumerateChannelsInt failure");
	if (num_descs <= 0) {
		ON_ERR_GOTO(FPGA_NOT_FOUND, free_dma_h, "No descriptors");
	}

	dma_h->num_avail_channels = num_descs;
	dma_h->chan_descs = (internal_channel_desc *)calloc(
		1, sizeof(internal_channel_desc) * num_descs);
	if (!dma_h->chan_descs) {
		ON_ERR_GOTO(FPGA_NO_MEMORY, out,
			    "Error allocating descriptors");
	}

	// FIXME: Need some MMIO addresses for enumerate
	dma_h->main_header.chan_desc = &dma_h->chan_descs[0];

	res = fpgaDMAEnumerateChannelsInt(dma_h->main_header.fpga_h, 0, NULL,
					  &num_descs, dma_h->chan_descs);
	ON_ERR_GOTO(res, rel_buf, "DMA fpgaDMAEnumerateChannelsInt failure");

	if (fpgaDMA_setup_sig_handler(dma_h)) {
		ON_ERR_GOTO(FPGA_EXCEPTION, rel_buf,
			    "Error: failed to register signal handler.\n");
	}

	dma_h->open_channels = (void **)calloc(num_descs, sizeof(void *));
	if (!dma_h->open_channels) {
		res = FPGA_NO_MEMORY;
		ON_ERR_GOTO(res, restore_sig_handler,
			    "Error allocating open channels");
	}

	dma_h->main_header.magic_id = FPGA_DMA_MAGIC_ID;

	setNUMABindings(fpga);

	sem_init(&dma_h->completion_thread_sem, 0, 0);

	// Start the completion thread
	if (pthread_create(&dma_h->completion_thread_id, NULL,
			   TransactionCompleteWorker, (void *)dma_h)
	    != 0) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, restore_sig_handler, "pthread_create");
	}

	// Check for successful thread startup
	if (checkThreadStart(&dma_h->completion_thread_sem)) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, restore_sig_handler,
			    "pthread_create completion thread startup timeout");
	}

	handle_->feature_private = (void *)dma_h;
	fpgaDMAIsOpen = 1;

	return FPGA_OK;

restore_sig_handler:
	fpgaDMA_restore_sig_handler(dma_h);

rel_buf:
	free(dma_h->chan_descs);

out:
	fpgaDMAQueueDestroy(dma_h, &dma_h->transferCompleteq, false);

free_dma_h:
	free(dma_h);
	handle_->feature_private = NULL;
	return res;
}

fpga_result fpgaDMAClose(fpga_feature_handle *handle)
{
	fpga_result res = FPGA_OK;
	struct _fpga_feature_handle *handle_ =
		(struct _fpga_feature_handle *)*handle;
	fpga_dma_handle_t *dma_h =
		(fpga_dma_handle_t *)handle_->feature_private;
	fpga_dma_transfer_t kill_transfer = {.transfer_type = TERMINATE_THREAD};

	if (!dma_h || !IS_DMA_HANDLE(dma_h)) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!dma_h->main_header.fpga_h) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	// Kill all channels that are open
	uint32_t i;
	for (i = 0; i < dma_h->num_avail_channels; i++) {
		if (!dma_h->open_channels[i]) {
			continue;
		}

		res = fpgaDMACloseChannel(&dma_h->open_channels[i]);
		if (FPGA_OK != res) {
			FPGA_DMA_ST_ERR("fpgaDMAClose: closing channel")
		}

		assert(NULL == dma_h->open_channels[i]);
	}

	// Terminate the spawned threads by sending a termination transfer
	kill_transfer.ch_type = INVALID_TYPE;
	fpgaDMAEnqueue(&dma_h->transferCompleteq, &kill_transfer);

	// Wait for the threads to exit
	void *th_retval;
	if (pthread_join(dma_h->completion_thread_id, &th_retval)) {
		FPGA_DMA_ST_ERR("pthread_join for completion queue");
	}
	if (th_retval != (void *)dma_h) {
		FPGA_DMA_ST_ERR(
			"pthread_join for completion queue - bad return value");
	}

	fpgaDMA_restore_sig_handler(dma_h);

	if (fpgaDMAQueueDestroy(dma_h, &dma_h->transferCompleteq, false)
	    != FPGA_OK) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, out, "fpgaDMAQueueDestroy");
	}

	destroyAllPoolResources(&dma_h->main_header, false);

out:
	// Make sure double-close fails
	dma_h->main_header.dma_channel = INVALID_CHANNEL;
	dma_h->main_header.magic_id = 0;
	free((void *)dma_h);
	*handle = NULL;
	return res;
}

inline fpga_result fpgaDMAOpenChannel(fpga_dma_handle _dma_h,
				      uint64_t dma_channel_index,
				      fpga_dma_channel_handle *_dma_ch)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_hx = (fpga_dma_handle_t *)_dma_h;
	void *dma_ch = NULL;

	if ((!_dma_h) || !IS_DMA_HANDLE(dma_hx)) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!_dma_ch) {
		FPGA_DMA_ST_ERR("Invalid pointer to DMA channel handle");
		return FPGA_INVALID_PARAM;
	}

	if (dma_channel_index >= dma_hx->num_avail_channels) {
		FPGA_DMA_ST_ERR("Invalid channel index");
		return FPGA_INVALID_PARAM;
	}

	if (dma_hx->open_channels[dma_channel_index]) {
		FPGA_DMA_ST_ERR(
			"Attempt to open a channel that is already open");
		return FPGA_BUSY;
	}

	setNUMABindings(dma_hx->main_header.fpga_h);

	internal_channel_desc *desc_p = &dma_hx->chan_descs[dma_channel_index];

	// init the dma channel handle
	fpga_dma_channel_type_t ch_type = desc_p->desc.ch_type;

	handle_common **comm = (handle_common **)&dma_ch;
	sem_t *thread_sem = NULL;
	switch (ch_type) {
	case TX_ST:
		dma_ch = calloc(1, sizeof(m2s_dma_handle_t));
		if (!dma_ch) {
			return FPGA_NO_MEMORY;
		}
		m2s_dma_handle_t *m2s_dma_ch = (m2s_dma_handle_t *)dma_ch;
		m2s_dma_ch->header.magic_id = FPGA_DMA_TX_CHANNEL_MAGIC_ID;
		m2s_dma_ch->header.chan_desc = desc_p;
		m2s_dma_ch->header.ch_type = TX_ST;
		sem_init(&dma_hx->m2s_thread_sem, 0, 0);
		thread_sem = &dma_hx->m2s_thread_sem;
		break;

	case RX_ST:
		dma_ch = calloc(1, sizeof(s2m_dma_handle_t));
		if (!dma_ch) {
			return FPGA_NO_MEMORY;
		}
		s2m_dma_handle_t *s2m_dma_ch = (s2m_dma_handle_t *)dma_ch;
		s2m_dma_ch->header.magic_id = FPGA_DMA_RX_CHANNEL_MAGIC_ID;
		s2m_dma_ch->header.chan_desc = desc_p;
		s2m_dma_ch->header.ch_type = RX_ST;
		sem_init(&dma_hx->s2m_thread_sem, 0, 0);
		thread_sem = &dma_hx->s2m_thread_sem;
		s2m_dma_ch->dma_rsp_base = desc_p->dma_base + FPGA_DMA_RESPONSE;
		s2m_dma_ch->dma_streaming_valve_base =
			desc_p->dma_base + FPGA_DMA_STREAMING_VALVE;
		break;

	case MM:
		dma_ch = calloc(1, sizeof(m2m_dma_handle_t));
		if (!dma_ch) {
			return FPGA_NO_MEMORY;
		}
		m2m_dma_handle_t *m2m_dma_ch = (m2m_dma_handle_t *)dma_ch;
		m2m_dma_ch->header.magic_id = FPGA_MSGDMA_MAGIC_ID;
		m2m_dma_ch->header.chan_desc = desc_p;
		m2m_dma_ch->dma_ase_cntl_base =
			desc_p->dma_base + FPGA_DMA_ADDR_SPAN_EXT_CNTL;
		m2m_dma_ch->dma_ase_data_base =
			desc_p->dma_base + FPGA_DMA_ADDR_SPAN_EXT_DATA;
		m2m_dma_ch->cur_ase_page = 0xffffffffffffffffUll;

		// Allocate magic number buffer
		res = fpgaPrepareBuffer(dma_hx->main_header.fpga_h,
					FPGA_DMA_ALIGN_BYTES,
					(void **)&(m2m_dma_ch->magic_buf),
					&m2m_dma_ch->magic_wsid, 0);
		ON_ERR_GOTO(res, out, "fpgaPrepareBuffer");

		res = fpgaGetIOAddress(dma_hx->main_header.fpga_h,
				       m2m_dma_ch->magic_wsid,
				       &m2m_dma_ch->magic_iova);
		if (FPGA_OK != res) {
			res = fpgaReleaseBuffer(dma_hx->main_header.fpga_h,
						m2m_dma_ch->magic_wsid);
			ON_ERR_GOTO(res, out, "fpgaReleaseBuffer");
			goto out;
		}

		m2m_dma_ch->header.ch_type = MM;
		sem_init(&dma_hx->m2m_thread_sem, 0, 0);
		thread_sem = &dma_hx->m2m_thread_sem;
		break;

	default:
		FPGA_DMA_ST_ERR("Unknown channel type");
	}

	(*comm)->fpga_h = dma_hx->main_header.fpga_h;
	(*comm)->dma_h = dma_hx;
	(*comm)->dma_channel = dma_channel_index;

	res = fpgaDMAQueueInit(dma_ch, &(*comm)->transferRequestq);
	if (res != FPGA_OK) {
		res = fpgaDMAQueueDestroy(dma_ch, &(*comm)->transferRequestq,
					  false);
		ON_ERR_GOTO(FPGA_EXCEPTION, out, "fpgaDMAQueueDestroy failed");
	}

	// register interrupt event handle
	res = fpgaCreateEventHandle(&((*comm)->eh));
	ON_ERR_GOTO(res, out, "fpgaCreateEventHandle");
	res = fpgaRegisterEvent((*comm)->fpga_h, FPGA_EVENT_INTERRUPT,
				(*comm)->eh,
				(*comm)->dma_channel /*vector id*/);
	ON_ERR_GOTO(res, destroy_eh, "fpgaRegisterEvent");

	// turn on global interrupts
	msgdma_ctrl_t ctrl = {0};
	ctrl.ct.global_intr_en_mask = 1;
	res = MMIOWrite32Blk((*comm), CSR_CONTROL((*comm)), (uint64_t)&ctrl.reg,
			     sizeof(ctrl.reg));
	ON_ERR_GOTO(res, destroy_eh, "MMIOWrite32Blk");

	// Mark this channel as in-use
	dma_hx->open_channels[dma_channel_index] = dma_ch;

	if (ch_type == TX_ST) {
		if (pthread_create(&dma_hx->m2s_thread_id, NULL,
				   m2sTransactionWorker, (void *)dma_ch)
		    != 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, destroy_eh, "pthread_create");
		}
	} else if (ch_type == RX_ST) {
		if (pthread_create(&dma_hx->s2m_thread_id, NULL,
				   s2mTransactionWorker, (void *)dma_ch)
		    != 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, destroy_eh, "pthread_create");
		}
	} else if (ch_type == MM) {
		if (pthread_create(&dma_hx->m2m_thread_id, NULL,
				   m2mTransactionWorker, (void *)dma_ch)
		    != 0) {
			res = FPGA_EXCEPTION;
			ON_ERR_GOTO(res, destroy_eh, "pthread_create");
		}
	}

	// Check for successful thread startup
	if (checkThreadStart(thread_sem)) {
		res = FPGA_EXCEPTION;
		ON_ERR_GOTO(res, destroy_eh,
			    "pthread_create worker thread startup timeout");
	}

	*_dma_ch = dma_ch;
	dma_hx->num_open_channels++;

	return FPGA_OK;

destroy_eh:
	res = fpgaDestroyEventHandle(&(*comm)->eh);
	ON_ERR_GOTO(res, disable_ints, "fpgaRegisterEvent");

disable_ints:
	// turn off global interrupts
	ctrl.ct.global_intr_en_mask = 0;
	res = MMIOWrite32Blk((*comm), CSR_CONTROL((*comm)), (uint64_t)&ctrl.reg,
			     sizeof(ctrl.reg));
	ON_ERR_GOTO(res, destroy_eh, "MMIOWrite32Blk");

out:
	free(dma_ch);
	*_dma_ch = NULL;
	return res;
}

inline fpga_result fpgaDMACloseChannel(fpga_dma_channel_handle *_dma_h)
{
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h = (fpga_dma_handle_t *)*_dma_h;
	fpga_dma_transfer_t kill_transfer = {.transfer_type = TERMINATE_THREAD};
	m2m_dma_handle_t *m2m = (m2m_dma_handle_t *)*_dma_h;
	m2s_dma_handle_t *m2s = (m2s_dma_handle_t *)*_dma_h;
	s2m_dma_handle_t *s2m = (s2m_dma_handle_t *)*_dma_h;
	assert(&m2m->header == &s2m->header);
	assert(&m2m->header == &m2s->header);
	assert(&m2m->header == &dma_h->main_header);

	if (!dma_h || !IS_CHANNEL_HANDLE(m2m)) {
		FPGA_DMA_ST_ERR("Invalid DMA handle");
		return FPGA_INVALID_PARAM;
	}

	if (!m2m->header.fpga_h) {
		FPGA_DMA_ST_ERR("Invalid FPGA handle");
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	fpga_dma_handle_t *fme_dma_h = m2m->header.dma_h;

	if (!fme_dma_h->open_channels[dma_h->main_header.dma_channel]) {
		FPGA_DMA_ST_ERR(
			"Attempt to close DMA channel that was not open");
		return FPGA_INVALID_PARAM;
	}

	// Mark channel as no longer in use
	fme_dma_h->open_channels[dma_h->main_header.dma_channel] = NULL;

	kill_transfer.ch_type = m2m->header.ch_type;
	kill_transfer.transfer_type = TERMINATE_THREAD;

	// Wait for the threads to exit
	void *th_retval;

	switch (m2m->header.ch_type) {
	case TX_ST:
		// Terminate the spawned threads by sending a termination
		// transfer
		fpgaDMAEnqueue(&m2s->header.transferRequestq, &kill_transfer);

		if (pthread_join(m2s->header.dma_h->m2s_thread_id,
				 &th_retval)) {
			FPGA_DMA_ST_ERR("pthread_join for completion queue");
		}
		if (th_retval != (void *)dma_h) {
			FPGA_DMA_ST_ERR(
				"pthread_join for TX_ST - bad return value");
		}
		releaseChannelResources(&m2s->header);

		fpgaDMAQueueDestroy(m2s->header.dma_h,
				    &m2s->header.transferRequestq, false);
		break;
	case RX_ST:
		// Terminate the spawned threads by sending a termination
		// transfer
		fpgaDMAEnqueue(&s2m->header.transferRequestq, &kill_transfer);

		if (pthread_join(s2m->header.dma_h->s2m_thread_id,
				 &th_retval)) {
			FPGA_DMA_ST_ERR("pthread_join for completion queue");
		}
		if (th_retval != (void *)dma_h) {
			FPGA_DMA_ST_ERR(
				"pthread_join for RX_ST - bad return value");
		}
		releaseChannelResources(&s2m->header);

		fpgaDMAQueueDestroy(s2m->header.dma_h,
				    &s2m->header.transferRequestq, false);
		break;
	case MM:
		// Terminate the spawned threads by sending a termination
		// transfer
		fpgaDMAEnqueue(&m2m->header.transferRequestq, &kill_transfer);

		if (pthread_join(m2m->header.dma_h->m2m_thread_id,
				 &th_retval)) {
			FPGA_DMA_ST_ERR("pthread_join for completion queue");
		}
		if (th_retval != (void *)dma_h) {
			FPGA_DMA_ST_ERR(
				"pthread_join for MM - bad return value");
		}

		res = fpgaReleaseBuffer(m2m->header.fpga_h, m2m->magic_wsid);
		ON_ERR_GOTO(res, out, "fpgaReleaseBuffer");

		fpgaDMAQueueDestroy(m2m->header.dma_h,
				    &m2m->header.transferRequestq, false);

		releaseChannelResources(&m2m->header);
		break;

	default:
		FPGA_DMA_ST_ERR("Bad channel type in closeChannel");
		break;
	}

out:
	m2m->header.dma_h->num_open_channels--;
	free((void *)dma_h);
	*_dma_h = NULL;
	return res;
}
