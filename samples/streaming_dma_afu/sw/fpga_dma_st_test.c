// Copyright(c) 2018, Intel Corporation
//
// Redistribution  and	use  in source	and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//	 this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//	 this list of conditions and the following disclaimer in the documentation
//	 and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//	 may be used to  endorse or promote  products derived  from this  software
//	 without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.	IN NO EVENT  SHALL THE COPYRIGHT OWNER	OR CONTRIBUTORS BE
// LIABLE  FOR	ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,	EXEMPLARY,	OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,	BUT  NOT LIMITED  TO,  PROCUREMENT	OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,	DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,	WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,	EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <time.h>
#include "fpga_dma.h"
#include "fpga_pattern_gen.h"
#include "fpga_pattern_checker.h"
#include <unistd.h>
/**
 * \fpga_dma_st_test.c
 * \brief Streaming DMA test
 */

#include <stdlib.h>
#include <assert.h>
#include <semaphore.h>

#define DMA_AFU_ID				"EB59BF9D-B211-4A4E-B3E3-753CE68634BA"
#define TEST_BUF_SIZE (20*1024*1024)
#define ASE_TEST_BUF_SIZE (4*1024)
// Single pattern is represented as 64Bytes
#define PATTERN_WIDTH 64
// No. of Patterns
#define PATTERN_LENGTH 32

static uint64_t count=0;
sem_t rx_cb_status;
sem_t tx_cb_status;
static int err_cnt = 0;
/*
 * macro for checking return codes
 */
#define ON_ERR_GOTO(res, label, desc)\
	do {\
		if ((res) != FPGA_OK) {\
			err_cnt++;\
			fprintf(stderr, "Error %s: %s\n", (desc), fpgaErrStr(res));\
			goto label;\
		}\
	} while (0)
int sendrxTransfer(fpga_dma_handle_t dma_h, fpga_dma_transfer_t rx_transfer, uint64_t src, uint64_t dst, uint64_t tf_len,fpga_dma_transfer_type_t tf_type, fpga_dma_rx_ctrl_t rx_ctrl, fpga_dma_transfer_cb cb) {
	fpga_result res = FPGA_OK;
	fpgaDMATransferSetSrc(rx_transfer, src);
	fpgaDMATransferSetDst(rx_transfer, dst);
	fpgaDMATransferSetLen(rx_transfer, tf_len);
	fpgaDMATransferSetTransferType(rx_transfer, tf_type);
	fpgaDMATransferSetRxControl(rx_transfer, rx_ctrl);
	fpgaDMATransferSetTransferCallback(rx_transfer, cb, NULL);
	res = fpgaDMATransfer(dma_h, rx_transfer);
	return res;
}

int sendtxTransfer(fpga_dma_handle_t dma_h, fpga_dma_transfer_t tx_transfer, uint64_t src, uint64_t dst, uint64_t tf_len,fpga_dma_transfer_type_t tf_type, fpga_dma_tx_ctrl_t tx_ctrl, fpga_dma_transfer_cb cb) {
	fpga_result res = FPGA_OK;
	fpgaDMATransferSetSrc(tx_transfer, src);
	fpgaDMATransferSetDst(tx_transfer, dst);
	fpgaDMATransferSetLen(tx_transfer, tf_len);
	fpgaDMATransferSetTransferType(tx_transfer, tf_type);
	fpgaDMATransferSetTxControl(tx_transfer, tx_ctrl);
	fpgaDMATransferSetTransferCallback(tx_transfer, cb, NULL);
	res = fpgaDMATransfer(dma_h, tx_transfer);
	return res;
}

fpga_result verify_buffer(uint32_t *buf, size_t payload_size) {
	size_t i,j;
	uint32_t test_word = 0;
	while(payload_size) {
		test_word = 0x04030201;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH/sizeof(test_word)); j++) {
				if(!payload_size)
					goto out;
				if((*buf) != test_word) {
					printf("Invalid data at %zx Expected = %x Actual = %x\n",i,test_word,(*buf));
					return FPGA_INVALID_PARAM;
				}
				payload_size -= sizeof(test_word);
				buf++;
				test_word += 0x01010101;
			}
		}
	}
out:
	printf("S2M: Data Verification Success!\n");
	return FPGA_OK;
}

void clear_buffer(uint32_t *buf, size_t size) {
	memset(buf, 0, size);
}

// Callback
static void rxtransferComplete(void *ctx) {
	sem_post(&rx_cb_status);
}

static void txtransferComplete(void *ctx) {
	sem_post(&tx_cb_status);
}


static void fill_buffer(uint32_t *buf, size_t payload_size) {
	size_t i,j;
	uint32_t test_word = 0;
	while(payload_size) {
		test_word = 0x04030201;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH/sizeof(test_word)); j++) {
				if(!payload_size)
					return;
				*buf = test_word;
				payload_size -= sizeof(test_word);
				buf++;
				test_word += 0x01010101;
			}
		}
	}
}

void report_bandwidth(size_t size, double seconds) {
	double throughput = (double)size/((double)seconds*1000*1000);
	printf("\rMeasured bandwidth = %lf Megabytes/sec\n", throughput);
}

// return elapsed time
double getTime(struct timespec start, struct timespec end) {
	uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	return (double) diff/(double)1000000000L;
}

fpga_result run_bw_test(fpga_handle afc_h, fpga_dma_handle_t dma_m2s_h, fpga_dma_handle_t dma_s2m_h) {
	int res;
	struct timespec start, end;
	uint64_t *dma_tx_buf_ptr;
	uint64_t *dma_rx_buf_ptr;

	ssize_t total_mem_size = (uint64_t)(4*1024)*(uint64_t)(1024*1024);

	dma_tx_buf_ptr = (uint64_t*)malloc(total_mem_size);
	dma_rx_buf_ptr = (uint64_t*)malloc(total_mem_size);
	if(!dma_tx_buf_ptr || !dma_rx_buf_ptr) {
		res = FPGA_NO_MEMORY;
		goto out;
	}

	fill_buffer((uint32_t*)dma_tx_buf_ptr, total_mem_size);
	
	fpga_dma_transfer_t rx_transfer;
	fpgaDMATransferInit(&rx_transfer);
	fpga_dma_transfer_t tx_transfer;
	fpgaDMATransferInit(&tx_transfer);

	printf("Streaming from host memory to FPGA..\n");
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, out, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, out, "prepare_checker");

	res = start_checker(afc_h, total_mem_size);
	ON_ERR_GOTO(res, out, "start checker");
	
	clock_gettime(CLOCK_MONOTONIC, &start);
	res = sendtxTransfer(dma_m2s_h, tx_transfer, (uint64_t)dma_tx_buf_ptr, 0, total_mem_size, HOST_MM_TO_FPGA_ST, TX_NO_PACKET, txtransferComplete);
	if(res != FPGA_OK) {
		printf(" sendtxTransfer Host to FPGA failed with error %s", fpgaErrStr(res));
		free(dma_tx_buf_ptr);
		return FPGA_EXCEPTION;
	}

	sem_wait(&tx_cb_status);
	clock_gettime(CLOCK_MONOTONIC, &end);
	res = wait_for_checker_complete(afc_h);
	ON_ERR_GOTO(res, out, "wait_for_checker_complete");
	report_bandwidth(total_mem_size, getTime(start,end));

	printf("Streaming from FPGA to host memory..\n");
	clear_buffer((uint32_t*)dma_rx_buf_ptr, total_mem_size);
	
	res = populate_pattern_generator(afc_h);
	ON_ERR_GOTO(res, out, "populate_pattern_generator");

	res = stop_generator(afc_h);
	ON_ERR_GOTO(res, out, "prepare_checker");

	res = start_generator(afc_h, total_mem_size, 0);
	ON_ERR_GOTO(res, out, "start checker");
	
	clock_gettime(CLOCK_MONOTONIC, &start);
	res = sendrxTransfer(dma_s2m_h, rx_transfer, 0, (uint64_t)dma_rx_buf_ptr, total_mem_size, FPGA_ST_TO_HOST_MM, RX_NO_PACKET, rxtransferComplete);
	if(res != FPGA_OK) {
		printf(" sendrxTransfer FPGA to Host failed with error %s", fpgaErrStr(res));
		free(dma_rx_buf_ptr);
		return FPGA_EXCEPTION;
	}

	sem_wait(&rx_cb_status);
	clock_gettime(CLOCK_MONOTONIC, &end);
	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out, "wait_for_checker_complete");
	printf("Verifying buffer..\n");
	verify_buffer((uint32_t*)dma_rx_buf_ptr, total_mem_size);
	clear_buffer((uint32_t*)dma_rx_buf_ptr, total_mem_size);
	report_bandwidth(total_mem_size, getTime(start,end));

out:
	if(dma_tx_buf_ptr)
		free(dma_tx_buf_ptr);
	if(dma_rx_buf_ptr)
		free(dma_rx_buf_ptr);

	return FPGA_OK;
}

int main(int argc, char *argv[]) {
	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h;
	int i;
	uint64_t transfer_len = 0;
	fpga_properties filter = NULL;
	fpga_token afc_token;
	fpga_handle afc_h;
	fpga_guid guid;
	uint32_t num_matches;
	uint64_t bytes_rcvd;
#ifndef USE_ASE
	volatile uint64_t *mmio_ptr = NULL;
#endif

	uint64_t *dma_tx_buf_ptr = NULL;
	uint64_t *dma_rx_buf_ptr = NULL;
	int pkt_transfer=0;

	#ifdef USE_ASE
		printf("Running test in ASE mode\n");
		transfer_len = ASE_TEST_BUF_SIZE;
	#else
		printf("Running test in HW mode\n");
		transfer_len = TEST_BUF_SIZE;
	#endif

	// enumerate the afc
	if(uuid_parse(DMA_AFU_ID, guid) < 0) {
		return 1;
	}
	sem_init(&tx_cb_status, 0, 0);
	sem_init(&rx_cb_status, 0, 0);
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");

	res = fpgaPropertiesSetGUID(filter, guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");

	res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaEnumerate");

	if(num_matches < 1) {
		printf("Error: Number of matches < 1");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out_destroy_prop, "num_matches<1");
	}

	// open the AFC
	res = fpgaOpen(afc_token, &afc_h, 0);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaOpen");

	#ifndef USE_ASE
		res = fpgaMapMMIO(afc_h, 0, (uint64_t**)&mmio_ptr);
		ON_ERR_GOTO(res, out_close, "fpgaMapMMIO");
	#endif

	// reset AFC
	res = fpgaReset(afc_h);
	ON_ERR_GOTO(res, out_unmap, "fpgaReset");

	// Enumerate DMA handles
	res = fpgaCountDMAChannels(afc_h, &count);
	ON_ERR_GOTO(res, out_unmap, "fpgaGetDMAChannels");
	
	if(count < 1) {
		printf("Error: DMA channels not found\n");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out_unmap, "count<1");
	}
	printf("No of DMA channels = %08lx\n", count);

	dma_h = (fpga_dma_handle_t*)malloc(sizeof(fpga_dma_handle_t)*count);

	res = fpgaDMAOpen(afc_h, 0, &dma_h[0]);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAOpen");

	res = fpgaDMAOpen(afc_h, 1, &dma_h[1]);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAOpen");

	dma_tx_buf_ptr = (uint64_t*)malloc(transfer_len);
	dma_rx_buf_ptr = (uint64_t*)malloc(transfer_len);
	if(!dma_tx_buf_ptr || !dma_rx_buf_ptr) {
		res = FPGA_NO_MEMORY;
		ON_ERR_GOTO(res, out_dma_close, "Error allocating memory");
	}

	// Example DMA transfer (host to fpga, asynchronous)
	fpga_dma_transfer_t rx_transfer;
	fpgaDMATransferInit(&rx_transfer);
	fpga_dma_transfer_t tx_transfer;
	fpgaDMATransferInit(&tx_transfer);

	fill_buffer((uint32_t*)dma_tx_buf_ptr, transfer_len);

	// M2S deterministic length transfer
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "stop_checker");

	res = start_checker(afc_h, transfer_len);
	ON_ERR_GOTO(res, out_dma_close, "start checker");
	
	res = sendtxTransfer(dma_h[0], tx_transfer, (uint64_t)dma_tx_buf_ptr, 0, transfer_len, HOST_MM_TO_FPGA_ST, TX_NO_PACKET, txtransferComplete);
	ON_ERR_GOTO(res, out_dma_close, "sendtxTransfer");

	sem_wait(&tx_cb_status);
	res = wait_for_checker_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_checker_complete");

	// M2S non-deterministic length transfer
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "stop_checker");

	res = start_checker(afc_h, transfer_len);
	ON_ERR_GOTO(res, out_dma_close, "start checker");

	res = sendtxTransfer(dma_h[0], tx_transfer, (uint64_t)dma_tx_buf_ptr, 0, transfer_len, HOST_MM_TO_FPGA_ST, GENERATE_EOP, txtransferComplete);
	ON_ERR_GOTO(res, out_dma_close, "sendtxTransfer");
		
	sem_wait(&tx_cb_status);
	res = wait_for_checker_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_checker_complete");
	
	// S2M deterministic length transfer
	pkt_transfer = 0;
	res = populate_pattern_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "populate_pattern_generator");

	res = stop_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "stop generator");

	res = start_generator(afc_h, transfer_len, pkt_transfer/*Not PACKET TRANSFER*/);
	ON_ERR_GOTO(res, out_dma_close, "start pattern generator");

	res = sendrxTransfer(dma_h[1], rx_transfer, 0, (uint64_t)dma_rx_buf_ptr, transfer_len, FPGA_ST_TO_HOST_MM, RX_NO_PACKET, rxtransferComplete);
	ON_ERR_GOTO(res, out_dma_close, "sendrxTransfer");

	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_generator_complete");

	sem_wait(&rx_cb_status);

	res = fpgaDMATransferGetBytesTransferred(rx_transfer, &bytes_rcvd);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferGetBytesTransferred");

	verify_buffer((uint32_t*)dma_rx_buf_ptr, bytes_rcvd);
	clear_buffer((uint32_t*)dma_rx_buf_ptr, bytes_rcvd);

	// S2M non-deterministic length transfer
	pkt_transfer = 1;
	res = populate_pattern_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "populate_pattern_generator");

	res = stop_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "stop_generator");

	res = start_generator(afc_h, transfer_len, pkt_transfer/*PACKET TRANSFER*/);
	ON_ERR_GOTO(res, out_dma_close, "start pattern generator");

	res = sendrxTransfer(dma_h[1], rx_transfer, 0, (uint64_t)dma_rx_buf_ptr, transfer_len, FPGA_ST_TO_HOST_MM, END_ON_EOP, rxtransferComplete);
	ON_ERR_GOTO(res, out_dma_close, "sendrxTransfer");

	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_generator_complete");

	sem_wait(&rx_cb_status);
	res = fpgaDMATransferGetBytesTransferred(rx_transfer, &bytes_rcvd);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferGetBytesTransferred");

	verify_buffer((uint32_t*)dma_rx_buf_ptr, bytes_rcvd);
	clear_buffer((uint32_t*)dma_rx_buf_ptr, bytes_rcvd);

	fpgaDMATransferDestroy(rx_transfer);
	fpgaDMATransferDestroy(tx_transfer);

	#ifndef USE_ASE
		printf("Running Bandwidth Tests..\n");
		res = run_bw_test(afc_h, dma_h[0], dma_h[1]);
		ON_ERR_GOTO(res, out_dma_close, "run_bw_test");
	#endif

out_dma_close:
	free(dma_tx_buf_ptr);
	free(dma_rx_buf_ptr);
	for(i=0; i<count; i++){
		if(dma_h[i]) {
			res = fpgaDMAClose(dma_h[i]);
			ON_ERR_GOTO(res, out_unmap, "fpgaDmaClose");
		}
	}
	free(dma_h);

out_unmap:
	#ifndef USE_ASE
		res = fpgaUnmapMMIO(afc_h, 0);
		ON_ERR_GOTO(res, out_close, "fpgaUnmapMMIO");

out_close:
	#endif
	res = fpgaClose(afc_h);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaClose");

out_destroy_tok:
	res = fpgaDestroyToken(&afc_token);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaDestroyToken");

out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");

out:
	return err_cnt;
}
