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

#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <time.h>
#include "fpga_dma.h"
#include "fpga_pattern_gen.h"
#include "fpga_pattern_checker.h"
#include <unistd.h>
#ifndef USE_ASE
#include <hwloc.h>
#include <assert.h>
#endif
/**
 * \fpga_dma_st_test.c
 * \brief Streaming DMA test
 */

#include <stdlib.h>
#include <assert.h>
#include <semaphore.h>

#define ST_DMA_AFU_ID "EB59BF9D-B211-4A4E-B3E3-753CE68634BA"
#define MM_DMA_AFU_ID "331DB30C-9885-41EA-9081-F88B8F655CAA"
#define TEST_BUF_SIZE (20 * 1024 * 1024)
#define ASE_TEST_BUF_SIZE (4 * 1024)
// Single pattern is represented as 64Bytes
#define PATTERN_WIDTH 64
// No. of Patterns
#define PATTERN_LENGTH 32

static uint32_t count = 0;
static sem_t rx_cb_status;
static sem_t tx_cb_status;
static int err_cnt = 0;

void do_mm_test(fpga_handle afc_h, uint32_t channel, uint32_t use_ase);

/*
 * macro for checking return codes
 */
#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			err_cnt++;                                             \
			fprintf(stderr, "Error %s: %s\n", (desc),              \
				fpgaErrStr(res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)
int sendrxTransfer(fpga_dma_channel_handle dma_h, fpga_dma_transfer rx_transfer,
		   uint64_t src, uint64_t dst, uint64_t tf_len,
		   fpga_dma_transfer_type_t tf_type, fpga_dma_rx_ctrl_t rx_ctrl,
		   fpga_dma_transfer_cb cb, int attr_reset)
{
	fpga_result res = FPGA_OK;
	if (attr_reset)
		fpgaDMATransferReset(dma_h, rx_transfer);
	fpgaDMATransferSetSrc(rx_transfer, src);
	fpgaDMATransferSetDst(rx_transfer, dst);
	fpgaDMATransferSetLen(rx_transfer, tf_len);
	fpgaDMATransferSetTransferType(rx_transfer, tf_type);
	fpgaDMATransferSetRxControl(rx_transfer, rx_ctrl);
	fpgaDMATransferSetTransferCallback(rx_transfer, cb, NULL);
	res = fpgaDMATransferStart(dma_h, rx_transfer);
	return res;
}

int sendtxTransfer(fpga_dma_channel_handle dma_h, fpga_dma_transfer tx_transfer,
		   uint64_t src, uint64_t dst, uint64_t tf_len,
		   fpga_dma_transfer_type_t tf_type, fpga_dma_tx_ctrl_t tx_ctrl,
		   fpga_dma_transfer_cb cb, int attr_reset)
{
	fpga_result res = FPGA_OK;
	if (attr_reset)
		fpgaDMATransferReset(dma_h, tx_transfer);
	fpgaDMATransferSetSrc(tx_transfer, src);
	fpgaDMATransferSetDst(tx_transfer, dst);
	fpgaDMATransferSetLen(tx_transfer, tf_len);
	fpgaDMATransferSetTransferType(tx_transfer, tf_type);
	fpgaDMATransferSetTxControl(tx_transfer, tx_ctrl);
	fpgaDMATransferSetTransferCallback(tx_transfer, cb, NULL);
	res = fpgaDMATransferStart(dma_h, tx_transfer);
	return res;
}

fpga_result verify_buffer(uint8_t *buf, size_t payload_size)
{
	size_t i, j;
	uint8_t test_word = 0;
	while (payload_size) {
		test_word = 0x01;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH / sizeof(test_word));
			     j++) {
				if (!payload_size)
					goto out;
				if ((*buf) != test_word) {
					printf("Invalid data at %zx Expected = %x Actual = %x\n",
					       i, test_word, (*buf));
					return FPGA_INVALID_PARAM;
				}
				payload_size -= sizeof(test_word);
				buf++;
				test_word += 0x01;
				if (test_word == 0xfd)
					test_word = 0x01;
			}
		}
	}
out:
	printf("S2M: Data Verification Success!\n");
	return FPGA_OK;
}

void clear_buffer(uint32_t *buf, size_t size)
{
	memset(buf, 0, size);
}

#define UNUSED(x) (void)(x)
// Callback
static void rxtransferComplete(void *ctx)
{
	UNUSED(ctx);
	sem_post(&rx_cb_status);
}

static void txtransferComplete(void *ctx)
{
	UNUSED(ctx);
	sem_post(&tx_cb_status);
}


static void fill_buffer(uint8_t *buf, size_t payload_size)
{
	size_t i, j;
	uint8_t test_word = 0;
	while (payload_size) {
		test_word = 0x01;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH / sizeof(test_word));
			     j++) {
				if (!payload_size)
					return;
				*buf = test_word;
				payload_size -= sizeof(test_word);
				buf++;
				test_word += 0x01;
				if (test_word == 0xfd)
					test_word = 0x01;
			}
		}
	}
}

void report_bandwidth(size_t size, double seconds)
{
	double throughput = (double)size / ((double)seconds * 1000 * 1000);
	printf("\rMeasured bandwidth = %lf Megabytes/sec\n", throughput);
}

// return elapsed time
double getTime(struct timespec start, struct timespec end)
{
	uint64_t diff = 1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec
			- start.tv_nsec;
	return (double)diff / (double)1000000000L;
}

fpga_result run_bw_test(fpga_handle afc_h, fpga_dma_channel_handle dma_m2s_h,
			fpga_dma_channel_handle dma_s2m_h)
{
	int res;
	struct timespec start, end;
	uint64_t *dma_tx_buf_ptr;
	uint64_t *dma_rx_buf_ptr;

	ssize_t total_mem_size = (uint64_t)(4 * 1024) * (uint64_t)(1024 * 1024);

	dma_tx_buf_ptr = (uint64_t *)malloc(total_mem_size);
	dma_rx_buf_ptr = (uint64_t *)malloc(total_mem_size);
	if (!dma_tx_buf_ptr || !dma_rx_buf_ptr) {
		res = FPGA_NO_MEMORY;
		goto out;
	}

	fill_buffer((uint8_t *)dma_tx_buf_ptr, total_mem_size);

	fpga_dma_transfer rx_transfer = NULL;
	fpgaDMATransferInit(dma_s2m_h, &rx_transfer);
	fpga_dma_transfer tx_transfer = NULL;
	fpgaDMATransferInit(dma_m2s_h, &tx_transfer);

	printf("Streaming from host memory to FPGA..\n");
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, out, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, out, "prepare_checker");

	res = start_checker(afc_h, total_mem_size);
	ON_ERR_GOTO(res, out, "start checker");

	clock_gettime(CLOCK_MONOTONIC, &start);
	res = sendtxTransfer(dma_m2s_h, tx_transfer, (uint64_t)dma_tx_buf_ptr,
			     0, total_mem_size, HOST_MM_TO_FPGA_ST,
			     TX_NO_PACKET, txtransferComplete,
			     1 /*reset transfer attributes*/);
	if (res != FPGA_OK) {
		printf(" sendtxTransfer Host to FPGA failed with error %s",
		       fpgaErrStr(res));
		free(dma_tx_buf_ptr);
		return FPGA_EXCEPTION;
	}

	sem_wait(&tx_cb_status);
	clock_gettime(CLOCK_MONOTONIC, &end);
	res = wait_for_checker_complete(afc_h);
	ON_ERR_GOTO(res, out, "wait_for_checker_complete");
	report_bandwidth(total_mem_size, getTime(start, end));

	printf("Streaming from FPGA to host memory..\n");
	clear_buffer((uint32_t *)dma_rx_buf_ptr, total_mem_size);

	res = populate_pattern_generator(afc_h);
	ON_ERR_GOTO(res, out, "populate_pattern_generator");

	res = stop_generator(afc_h);
	ON_ERR_GOTO(res, out, "prepare_checker");

	res = start_generator(afc_h, total_mem_size, 0);
	ON_ERR_GOTO(res, out, "start checker");

	clock_gettime(CLOCK_MONOTONIC, &start);
	res = sendrxTransfer(
		dma_s2m_h, rx_transfer, 0, (uint64_t)dma_rx_buf_ptr,
		total_mem_size, FPGA_ST_TO_HOST_MM, RX_NO_PACKET,
		rxtransferComplete, 1 /*reset transfer attributes*/);
	if (res != FPGA_OK) {
		printf(" sendrxTransfer FPGA to Host failed with error %s",
		       fpgaErrStr(res));
		free(dma_rx_buf_ptr);
		return FPGA_EXCEPTION;
	}

	sem_wait(&rx_cb_status);
	clock_gettime(CLOCK_MONOTONIC, &end);
	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out, "wait_for_checker_complete");
	printf("Verifying buffer..\n");
	verify_buffer((uint8_t *)dma_rx_buf_ptr, total_mem_size);
	clear_buffer((uint32_t *)dma_rx_buf_ptr, total_mem_size);
	report_bandwidth(total_mem_size, getTime(start, end));

out:
	if (dma_tx_buf_ptr)
		free(dma_tx_buf_ptr);
	if (dma_rx_buf_ptr)
		free(dma_rx_buf_ptr);
	if (rx_transfer)
		fpgaDMATransferDestroy(dma_s2m_h, &rx_transfer);
	if (tx_transfer)
		fpgaDMATransferDestroy(dma_m2s_h, &tx_transfer);

	return FPGA_OK;
}

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	fpga_result res = FPGA_OK;
	fpga_dma_handle dma_handle;
	int i;
	uint64_t transfer_len = 0;
	fpga_properties filter[2] = {NULL, NULL};
	fpga_token afc_token;
	fpga_handle afc_h;
	fpga_guid guid;
	fpga_guid mm_guid;
	uint32_t num_matches;
	uint64_t bytes_rcvd;
	bool eop_arrived;
#ifndef USE_ASE
	volatile uint64_t *mmio_ptr = NULL;
#endif

	uint64_t *dma_tx_buf_ptr = NULL;
	uint64_t *dma_rx_buf_ptr = NULL;
	int pkt_transfer = 0;

#ifdef USE_ASE
	printf("Running test in ASE mode\n");
	transfer_len = ASE_TEST_BUF_SIZE;
#else
	printf("Running test in HW mode\n");
	transfer_len = TEST_BUF_SIZE;
#endif

	// enumerate the afc
	if (uuid_parse(ST_DMA_AFU_ID, guid) < 0) {
		return 1;
	}
	if (uuid_parse(MM_DMA_AFU_ID, mm_guid) < 0) {
		return 1;
	}

	sem_init(&tx_cb_status, 0, 0);
	sem_init(&rx_cb_status, 0, 0);
	res = fpgaGetProperties(NULL, &filter[0]);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");
	res = fpgaGetProperties(NULL, &filter[1]);
	ON_ERR_GOTO(res, out, "fpgaGetProperties");

	res = fpgaPropertiesSetObjectType(filter[0], FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");
	res = fpgaPropertiesSetObjectType(filter[1], FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetObjectType");

	res = fpgaPropertiesSetGUID(filter[0], guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");
	res = fpgaPropertiesSetGUID(filter[1], mm_guid);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaPropertiesSetGUID");

	res = fpgaEnumerate(&filter[0], 2, &afc_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "fpgaEnumerate");

	if (num_matches < 1) {
		printf("Error: Number of matches < 1\n");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out_destroy_prop,
			    "num_matches<1");
	}

	// open the AFC
	res = fpgaOpen(afc_token, &afc_h, 0);
	ON_ERR_GOTO(res, out_destroy_tok, "fpgaOpen");

#if 0
#ifndef USE_ASE
	// Set up proper affinity if requested
	if (cpu_affinity || memory_affinity) {
		unsigned dom = 0, bus = 0, dev = 0, func = 0;
		fpga_properties props;
		int retval;
#if (FPGA_DMA_DEBUG)
		char str[4096];
#endif
		res = fpgaGetProperties(afc_token, &props);
		ON_ERR_GOTO(res, out_destroy_tok, "fpgaGetProperties");
		res = fpgaPropertiesGetBus(props, (uint8_t *)&bus);
		ON_ERR_GOTO(res, out_destroy_tok, "fpgaPropertiesGetBus");
		res = fpgaPropertiesGetDevice(props, (uint8_t *)&dev);
		ON_ERR_GOTO(res, out_destroy_tok, "fpgaPropertiesGetDevice");
		res = fpgaPropertiesGetFunction(props, (uint8_t *)&func);
		ON_ERR_GOTO(res, out_destroy_tok, "fpgaPropertiesGetFunction");

		// Find the device from the topology
		hwloc_topology_t topology;
		hwloc_topology_init(&topology);
		hwloc_topology_set_flags(topology,
					 HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
		hwloc_topology_load(topology);
		hwloc_obj_t obj = hwloc_get_pcidev_by_busid(topology, dom, bus,
							    dev, func);
		hwloc_obj_t obj2 = hwloc_get_non_io_ancestor_obj(topology, obj);
#if (FPGA_DMA_DEBUG)
		hwloc_obj_type_snprintf(str, 4096, obj2, 1);
		printf("%s\n", str);
		hwloc_obj_attr_snprintf(str, 4096, obj2, " :: ", 1);
		printf("%s\n", str);
		hwloc_bitmap_taskset_snprintf(str, 4096, obj2->cpuset);
		printf("CPUSET is %s\n", str);
		hwloc_bitmap_taskset_snprintf(str, 4096, obj2->nodeset);
		printf("NODESET is %s\n", str);
#endif
		if (memory_affinity) {
#if HWLOC_API_VERSION > 0x00020000
			retval = hwloc_set_membind(
				topology, obj2->nodeset, HWLOC_MEMBIND_THREAD,
				HWLOC_MEMBIND_MIGRATE
					| HWLOC_MEMBIND_BYNODESET);
#else
			retval = hwloc_set_membind_nodeset(
				topology, obj2->nodeset, HWLOC_MEMBIND_THREAD,
				HWLOC_MEMBIND_MIGRATE);
#endif
			ON_ERR_GOTO(retval, out_destroy_tok,
				    "hwloc_set_membind");
		}
		if (cpu_affinity) {
			retval = hwloc_set_cpubind(topology, obj2->cpuset,
						   HWLOC_CPUBIND_STRICT);
			ON_ERR_GOTO(retval, out_destroy_tok,
				    "hwloc_set_cpubind");
		}
	}
#endif
#endif

#ifndef USE_ASE
	res = fpgaMapMMIO(afc_h, 0, (uint64_t **)&mmio_ptr);
	ON_ERR_GOTO(res, out_close, "fpgaMapMMIO");
#endif

	// reset AFC
	res = fpgaReset(afc_h);
	ON_ERR_GOTO(res, out_unmap, "fpgaReset");

	res = fpgaDMAOpen(afc_h, &dma_handle);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAOpen");

	// Enumerate DMA handles
	res = fpgaDMAEnumerateChannels(dma_handle, 0, NULL, &count);
	ON_ERR_GOTO(res, dma_close, "fpgaDMAEnumerateChannels");

	if (count < 1) {
		printf("Error: DMA channels not found\n");
		ON_ERR_GOTO(FPGA_INVALID_PARAM, dma_close, "count<1");
	}
	printf("No of DMA channels = %08x\n", count);

	fpga_dma_channel_desc *descs = (fpga_dma_channel_desc *)malloc(
		sizeof(fpga_dma_channel_desc) * count);
	res = fpgaDMAEnumerateChannels(dma_handle, count, descs, &count);
	ON_ERR_GOTO(res, free_descs, "fpgaDMAEnumerateChannels");

	fpga_dma_channel_handle *dma_h;

	dma_h = (fpga_dma_handle *)malloc(sizeof(fpga_dma_handle) * count);

	uint32_t k;
	uint32_t tx_index = 0xffffffff;
	uint32_t rx_index = 0xffffffff;
	uint32_t mm_index = 0xffffffff;
	for (k = 0; k < count; k++) {
		assert(k == descs[k].index);
		if (descs[k].ch_type == TX_ST) {
			tx_index = descs[k].index;
		}
		if (descs[k].ch_type == RX_ST) {
			rx_index = descs[k].index;
		}
		if (descs[k].ch_type == MM) {
			mm_index = descs[k].index;
		}
	}

	if (mm_index != 0xffffffff) {
#ifndef USE_ASE
		do_mm_test(dma_handle, mm_index, 0);
#else
		do_mm_test(dma_handle, mm_index, 1);
#endif
		return err_cnt;
	}

	assert(!((tx_index == 0xffffffff) || (rx_index == 0xffffffff)));

	res = fpgaDMAOpenChannel(dma_handle, tx_index,
				 &dma_h[tx_index]); // TX handle
	ON_ERR_GOTO(res, free_descs, "fpgaDMAOpen");

	res = fpgaDMAOpenChannel(dma_handle, rx_index,
				 &dma_h[rx_index]); // RX handle
	ON_ERR_GOTO(res, free_descs, "fpgaDMAOpen");

	dma_tx_buf_ptr = (uint64_t *)malloc(transfer_len);
	dma_rx_buf_ptr = (uint64_t *)malloc(transfer_len);
	if (!dma_tx_buf_ptr || !dma_rx_buf_ptr) {
		res = FPGA_NO_MEMORY;
		ON_ERR_GOTO(res, free_descs, "Error allocating memory");
	}

	// Example DMA transfer (host to fpga, asynchronous)
	fpga_dma_transfer rx_transfer = NULL;
	fpgaDMATransferInit(dma_h[rx_index], &rx_transfer);
	fpga_dma_transfer tx_transfer = NULL;
	fpgaDMATransferInit(dma_h[tx_index], &tx_transfer);

	fill_buffer((uint8_t *)dma_tx_buf_ptr, transfer_len);

	// M2S deterministic length transfer
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, free_descs, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, free_descs, "stop_checker");

	res = start_checker(afc_h, transfer_len);
	ON_ERR_GOTO(res, free_descs, "start checker");

	res = sendtxTransfer(
		dma_h[tx_index], tx_transfer, (uint64_t)dma_tx_buf_ptr, 0,
		transfer_len, HOST_MM_TO_FPGA_ST, TX_NO_PACKET,
		txtransferComplete, 1 /*reset transfer attributes*/);
	ON_ERR_GOTO(res, free_descs, "sendtxTransfer");

	sem_wait(&tx_cb_status);
	res = wait_for_checker_complete(afc_h);
	ON_ERR_GOTO(res, free_descs, "wait_for_checker_complete");

	// M2S non-deterministic length transfer
	res = populate_pattern_checker(afc_h);
	ON_ERR_GOTO(res, free_descs, "populate_pattern_checker");

	res = stop_checker(afc_h);
	ON_ERR_GOTO(res, free_descs, "stop_checker");

	res = start_checker(afc_h, transfer_len);
	ON_ERR_GOTO(res, free_descs, "start checker");

	res = sendtxTransfer(
		dma_h[tx_index], tx_transfer, (uint64_t)dma_tx_buf_ptr, 0,
		transfer_len, HOST_MM_TO_FPGA_ST, GENERATE_EOP,
		txtransferComplete, 1 /*reset transfer attributes*/);
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

	res = start_generator(afc_h, transfer_len,
			      pkt_transfer /*Not PACKET TRANSFER*/);
	ON_ERR_GOTO(res, out_dma_close, "start pattern generator");

	res = sendrxTransfer(
		dma_h[rx_index], rx_transfer, 0, (uint64_t)dma_rx_buf_ptr,
		transfer_len, FPGA_ST_TO_HOST_MM, RX_NO_PACKET,
		rxtransferComplete, 1 /*reset transfer attributes*/);
	ON_ERR_GOTO(res, out_dma_close, "sendrxTransfer");

	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_generator_complete");

	sem_wait(&rx_cb_status);

	res = fpgaDMATransferGetBytesTransferred(rx_transfer, &bytes_rcvd);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferGetBytesTransferred");

	verify_buffer((uint8_t *)dma_rx_buf_ptr, bytes_rcvd);
	clear_buffer((uint32_t *)dma_rx_buf_ptr, bytes_rcvd);

	// S2M non-deterministic length transfer
	pkt_transfer = 1;
	res = populate_pattern_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "populate_pattern_generator");

	res = stop_generator(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "stop_generator");

	res = start_generator(afc_h, transfer_len,
			      pkt_transfer /*PACKET TRANSFER*/);
	ON_ERR_GOTO(res, out_dma_close, "start pattern generator");

	res = sendrxTransfer(dma_h[rx_index], rx_transfer, 0,
			     (uint64_t)dma_rx_buf_ptr, transfer_len,
			     FPGA_ST_TO_HOST_MM, END_ON_EOP, rxtransferComplete,
			     1 /*reset transfer attributes*/);
	ON_ERR_GOTO(res, out_dma_close, "sendrxTransfer");

	res = wait_for_generator_complete(afc_h);
	ON_ERR_GOTO(res, out_dma_close, "wait_for_generator_complete");

	sem_wait(&rx_cb_status);
	res = fpgaDMATransferGetBytesTransferred(rx_transfer, &bytes_rcvd);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferGetBytesTransferred");

	res = fpgaDMATransferCheckEopArrived(rx_transfer, &eop_arrived);
	ON_ERR_GOTO(res, out_dma_close, "fpgaDMATransferGetCheckEopArrived");

	if (eop_arrived) {
		verify_buffer((uint8_t *)dma_rx_buf_ptr, bytes_rcvd);
		clear_buffer((uint32_t *)dma_rx_buf_ptr, bytes_rcvd);
	}

	fpgaDMATransferDestroy(dma_h[rx_index], &rx_transfer);
	fpgaDMATransferDestroy(dma_h[tx_index], &tx_transfer);

#ifndef USE_ASE
	printf("Running Bandwidth Tests..\n");
	res = run_bw_test(afc_h, dma_h[tx_index], dma_h[rx_index]);
	ON_ERR_GOTO(res, out_dma_close, "run_bw_test");
#endif

out_dma_close:
	free(dma_tx_buf_ptr);
	free(dma_rx_buf_ptr);
	for (i = 0; i < (int)count; i++) {
		if (dma_h[i]) {
			res = fpgaDMACloseChannel(&dma_h[i]);
			ON_ERR_GOTO(res, out_unmap, "fpgaDMACloseChannel");
		}
	}
	free(dma_h);

free_descs:
	free(descs);

dma_close:
	res = fpgaDMAClose(&dma_handle);
	ON_ERR_GOTO(res, out_unmap, "fpgaDMAClose");

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
	res = fpgaDestroyProperties(&filter[0]);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");
	res = fpgaDestroyProperties(&filter[1]);
	ON_ERR_GOTO(res, out, "fpgaDestroyProperties");

out:
	return err_cnt;
}
