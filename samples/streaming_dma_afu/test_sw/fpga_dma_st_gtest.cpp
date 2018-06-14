#include <string.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
extern "C" {
#include "fpga_dma_st_internal.h"
#include "fpga_dma.h"
#include "fpga_pattern_gen.h"
#include "fpga_pattern_checker.h"
}
#include "gtest/gtest.h"

#define DMA_AFU_ID				"EB59BF9D-B211-4A4E-B3E3-753CE68634BA"
#define TEST_BUF_SIZE (20*1024*1024)
#define ASE_TEST_BUF_SIZE (4*1024)
#define MAX_ST_DMA_CHANNELS 2
// Single pattern is represented as 64Bytes
#define PATTERN_WIDTH_BYTES 64
// No. of Patterns
#define PATTERN_LENGTH 32

#define RAND() (rand() % 0x7fffffff)
#define RAND_CNT() (rand() % 0x7fffff)
#define random_test_cnt 5
// Uncomment to enable bandwidth measurement
//#define FPGA_DMA_BANDWIDTH_TEST 1
#define DMA_DEBUG 0

// Convenience macros
#ifdef DMA_DEBUG
	#define debug_printk(fmt, ...) \
	do { if (DMA_DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#else
	#define debug_printk(...)
#endif
sem_t tx_cb_status;
sem_t rx_cb_status;
struct timeval start, stop;
double secs = 0;

static int err_cnt=0;
#define ON_ERR(res,label, desc)\
	do {\
		if ((res) != FPGA_OK) {\
			err_cnt++;\
			fprintf(stderr, "Error %s: %s\n", (desc), fpgaErrStr(res));\
			goto label;\
		}\
	} while (0)

//class DmaAfuTest : public ::testing::TestWithParam<test_param> {
class DmaAfuTest : public ::testing::Test {
 public:
	fpga_result InitRoutine(){
		int i;
		if(uuid_parse(DMA_AFU_ID, guid) < 0) {
			return (fpga_result)1;
		}
		sem_init(&tx_cb_status, 0, 0);
		sem_init(&rx_cb_status, 0, 0);
		res = fpgaGetProperties(NULL, &filter);
		ON_ERR(res, out, "fpgaGetProperties");
		res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
		ON_ERR(res, out, "fpgaPropertiesSetObjectType");
		res = fpgaPropertiesSetGUID(filter, guid);
		ON_ERR(res, out, "fpgaPropertiesSetGUID");
		res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
		ON_ERR(res, out, "fpgaEnumerate");
		if(num_matches < 1) {
			ON_ERR(FPGA_INVALID_PARAM, out, "num_matches<1");
			return (fpga_result)err_cnt;
		}
		res = fpgaOpen(afc_token, &afc_h, 0);
		ON_ERR(res, out, "fpgaOpen");

		if(!use_ase) {
			res = fpgaMapMMIO(afc_h, 0, (uint64_t**)&mmio_ptr);
			ON_ERR(res, out, "fpgaMapMMIO");
		}

		// reset AFC
		res = fpgaReset(afc_h);
		ON_ERR(res, out,"fpgaReset");

		// Enumerate DMA handles
		res = fpgaCountDMAChannels(afc_h, &ch_count);
		ON_ERR(res, out, "fpgaGetDMAChannels");

		if(ch_count < 1) {
			printf("Error: DMA channels not found\n");
			ON_ERR(FPGA_INVALID_PARAM, out, "count<1");
		}
		printf("No of DMA channels = %08lx\n", ch_count);

		dma_h = (fpga_dma_handle_t*)malloc(sizeof(fpga_dma_handle_t)*ch_count);

		for(i=0; i<ch_count; i++) {
			res = fpgaDMAOpen(afc_h, i, &dma_h[i]);
			ON_ERR(res, out, "fpgaDMAOpen");
		}

	out:
		return (fpga_result)err_cnt;
	}

	virtual void SetUp() {
		ASSERT_EQ(0,InitRoutine());
	}

	fpga_result exitRoutine(){
		int i;
		for(i=0; i<ch_count; i++){
			if(dma_h[i]) {
				res = fpgaDMAClose(dma_h[i]);
				ON_ERR(res, out, "fpgaDmaClose");
			}
		}
		free(dma_h);
		if(!use_ase) {
			res = fpgaUnmapMMIO(afc_h, 0);
			ON_ERR(res, out, "fpgaUnmapMMIO");
		}
		res = fpgaClose(afc_h);
		ON_ERR(res, out, "fpgaClose");
		res = fpgaDestroyToken(&afc_token);
		ON_ERR(res, out, "fpgaDestroyToken");
		res = fpgaDestroyProperties(&filter);
		ON_ERR(res, out, "fpgaDestroyProperties");
	out:
		return (fpga_result)err_cnt;
	}

	virtual void TearDown() {
		ASSERT_EQ(0,exitRoutine());
	}

	void fill_buffer(uint32_t *buf, size_t payload_size) {
		size_t i,j;
		uint32_t test_word = 0;
		while(payload_size) {
			test_word = 0x04030201;
			for (i = 0; i < PATTERN_LENGTH; i++) {
				for (j = 0; j < (PATTERN_WIDTH_BYTES/sizeof(test_word)); j++) {
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
	
	fpga_result verify_buffer(uint32_t *buf, size_t payload_size) {
	size_t i,j;
	uint32_t test_word = 0;
	while(payload_size) {
		test_word = 0x04030201;
		for (i = 0; i < PATTERN_LENGTH; i++) {
			for (j = 0; j < (PATTERN_WIDTH_BYTES/sizeof(test_word)); j++) {
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

	void clear_buffer(char *buf, uint64_t size) {
		memset(buf, 0, size);
	}

	fpga_result sendrxTransfer(fpga_dma_handle_t dma_h, fpga_dma_transfer_t rx_transfer, uint64_t src, uint64_t dst, uint64_t tf_len,fpga_dma_transfer_type_t tf_type, fpga_dma_rx_ctrl_t rx_ctrl, fpga_dma_transfer_cb cb) {
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

	fpga_result sendtxTransfer(fpga_dma_handle_t dma_h, fpga_dma_transfer_t tx_transfer, uint64_t src, uint64_t dst, uint64_t tf_len,fpga_dma_transfer_type_t tf_type, fpga_dma_tx_ctrl_t tx_ctrl, fpga_dma_transfer_cb cb) {
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

	// Callback
	static void rxtransferComplete(void *ctx) {
		sem_post(&rx_cb_status);
	}

	static void txtransferComplete(void *ctx) {
		sem_post(&tx_cb_status);
	}

	fpga_result M2S_transfer(fpga_dma_handle_t dma_h, uint64_t transfer_len, fpga_dma_transfer_cb cb){
		uint64_t *dma_tx_buf_ptr = NULL;

		dma_tx_buf_ptr = (uint64_t*)malloc(transfer_len);
		if(!dma_tx_buf_ptr) {
			res = FPGA_NO_MEMORY;
			ON_ERR(res, out, "Error allocating memory");
		}
		fill_buffer((uint32_t*)dma_tx_buf_ptr, transfer_len);

		// copy from host to fpga
		res = populate_pattern_checker(dma_h->fpga_h);
		ON_ERR(res, out, "populate_pattern_checker");

		res = stop_checker(dma_h->fpga_h);
		ON_ERR(res, out, "stop_checker");

		res = start_checker(dma_h->fpga_h, transfer_len);
		ON_ERR(res, out, "start checker");

		fpgaDMATransferInit(&tx_transfer);
		gettimeofday(&start, NULL);
		res = sendtxTransfer(dma_h, tx_transfer, (uint64_t)dma_tx_buf_ptr, 0, transfer_len, HOST_MM_TO_FPGA_ST, TX_NO_PACKET, cb);
		ON_ERR(res, out, "sendtxTransfer");
		if(cb)
			sem_wait(&tx_cb_status);
		res = wait_for_checker_complete(dma_h->fpga_h);
		ON_ERR(res, out, "wait_for_checker_complete");
		gettimeofday(&stop, NULL);
		secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
		if(secs>0){
			if(transfer_len == 4*1023*1024*1024l)
			debug_printk("Time taken Host To FPGA - %f s, BandWidth = %f MB/s \n",secs, ((unsigned long long)transfer_len/(float)secs/1000000));
		}
		fpgaDMATransferDestroy(tx_transfer);
		res = stop_checker(dma_h->fpga_h);
		ON_ERR(res, out, "stop_checker");
	out:
		free(dma_tx_buf_ptr);
		return (fpga_result)err_cnt;
	}

	fpga_result S2M_transfer(fpga_dma_handle_t dma_h, uint64_t transfer_len, int pkt_transfer, fpga_dma_transfer_cb cb){
		
		uint64_t *dma_rx_buf_ptr = NULL;

		dma_rx_buf_ptr = (uint64_t*)malloc(transfer_len);
		if(!dma_rx_buf_ptr) {
			res = FPGA_NO_MEMORY;
			ON_ERR(res, out, "Error allocating memory");
		}
		gettimeofday(&start, NULL);
		res = populate_pattern_generator(dma_h->fpga_h);
		ON_ERR(res, out, "populate_pattern_generator");

		res = stop_generator(dma_h->fpga_h);
		ON_ERR(res, out, "stop generator");

		res = start_generator(dma_h->fpga_h, transfer_len, pkt_transfer/*Not PACKET TRANSFER*/);
		ON_ERR(res, out, "start pattern generator");
		fpgaDMATransferInit(&rx_transfer);
		if(pkt_transfer == 1){
			res = sendrxTransfer(dma_h, rx_transfer, 0, (uint64_t)dma_rx_buf_ptr, transfer_len, FPGA_ST_TO_HOST_MM, END_ON_EOP, cb);
		} else {
			res = sendrxTransfer(dma_h, rx_transfer, 0, (uint64_t)dma_rx_buf_ptr, transfer_len, FPGA_ST_TO_HOST_MM, RX_NO_PACKET, cb);
		}
		ON_ERR(res, out, "fpgaDMATransfer");

		res = wait_for_generator_complete(dma_h->fpga_h);
		ON_ERR(res, out, "wait_for_generator_complete");
		if(cb)
			sem_wait(&rx_cb_status);
		gettimeofday(&stop, NULL);
		secs = ((double)(stop.tv_usec - start.tv_usec) / 1000000) + (double)(stop.tv_sec - start.tv_sec);
		if(secs>0){
			if(transfer_len == 4*1023*1024*1024l)
			debug_printk("Time taken Host To FPGA - %f s, BandWidth = %f MB/s \n",secs, ((unsigned long long)transfer_len/(float)secs/1000000));
		}
		res = verify_buffer((uint32_t*)dma_rx_buf_ptr, transfer_len);
		ON_ERR(res, out, "verify_buffer");
		clear_buffer((char*)dma_rx_buf_ptr, transfer_len);
		fpgaDMATransferDestroy(rx_transfer);
		res = stop_generator(dma_h->fpga_h);
		ON_ERR(res, out, "stop generator");
	out:
		free(dma_rx_buf_ptr);
		
		return (fpga_result)err_cnt;
	}


	fpga_result res = FPGA_OK;
	fpga_dma_handle_t *dma_h;
	fpga_properties filter = NULL;
	fpga_token afc_token;
	fpga_handle afc_h;
	fpga_guid guid;
	uint32_t num_matches;
	volatile uint64_t *mmio_ptr = NULL;
	uint32_t use_ase = 0;
	uint64_t dev_addr1;
	uint64_t dev_addr2;
	uint64_t count;
	uint64_t ch_count=0;
	bool pass;
	int pkt_transfer=0;
	fpga_dma_transfer_t rx_transfer;
	fpga_dma_transfer_t tx_transfer;

};

#if 0
// fpgaCountDMAChannels
TEST_F(DmaAfuTest, fpgaCountDMAChannels_InvalidFpgaHandle)
{
	// fpgaCountDMAChannels(NULL, &count) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaCountDMAChannels_InvalidCount)
{
	// fpgaCountDMAChannels(fpga_h, NULL) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaCountDMAChannels_ValidCount)
{
	// fpgaCountDMAChannels(afc_h, &count) must return FPGA_OK
	// count returned by fpgaCountDMAChannels(afc_h, &count) must equal 2
}

// fpgaDMAOpen
TEST_F(DmaAfuTest, fpgaDMAOpen_InvalidFpgaHandle)
{
	// fpgaDMAOpen(NULL, 0, &dma) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMAOpen_InvalidIndex)
{
	// fpgaDMAOpen(afc_h, -1, &dma) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMAOpen_InvalidPtrToDmaHandle)
{
	// fpgaDMAOpen(afc_h, -1, NULL) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMAOpen_IndexOutOfRange)
{
	// fpgaDMAOpen(afc_h, cnt, &dma) where cnt > 2 must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMAOpen_IndexOutOfRange)
{
	// fpgaDMAOpen(afc_h, cnt, &dma) where cnt > 2 must return INVALID_PARAM
}

// fpgaDMAClose
TEST_F(DmaAfuTest, fpgaDmaClose_InvalidDmaHandle)
{
	// fpgaDmaClose(NULL) OR fpgaDmaClose(invalid_dma_h) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDmaClose_ValidDmaHandle)
{
	// fpgaDmaClose(valid_dma_h) must return FPGA_OK
}

// fpgaGetDMAChannelType
TEST_F(DmaAfuTest, fpgaGetDMAChannelType_InvalidDmaHandle)
{
	// fpgaGetDMAChannelType(NULL, &ch_type) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaGetDMAChannelType_InvalidPtrToChannel)
{
	// fpgaGetDMAChannelType(dma_h, NULL) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaGetDMAChannelType_ValidTX)
{
	// fpgaGetDMAChannelType(dma_h[0], NULL) must return TX and FPGA_OK
}

TEST_F(DmaAfuTest, fpgaGetDMAChannelType_ValidRX)
{
	// fpgaGetDMAChannelType(dma_h[1], NULL) must return RX and FPGA_OK
}

// fpgaDMATransferInit
TEST_F(DmaAfuTest, fpgaDMATransferInit_InvalidPtrToTransfer)
{
	// fpgaDMATransferInit(NULL) must return INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferInit_ValidPtrToTransfer)
{
	// fpgaDMATransferInit(&transfer) must return FPGA_OK
}

// fpgaDMATransferDestroy
TEST_F(DmaAfuTest, fpgaDMATransferDestroy_InvalidTransfer)
{
	// fpgaDMATransferDestroy(NULL) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferDestroy_ValidTransfer)
{
	// fpgaDMATransferDestroy(transfer) must return FPGA_OK
}

// fpgaDMATransferSetSrc
TEST_F(DmaAfuTest, fpgaDMATransferSetSrc_InvalidTransfer)
{
	// fpgaDMATransferSetSrc(NULL, src) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetSrc_ValidTransfer)
{
	// fpgaDMATransferSetSrc(transfer, src) must return FPGA_OK
}	

// fpgaDMATransferSetDst
TEST_F(DmaAfuTest, fpgaDMATransferSetDst_InvalidTransfer)
{
	// fpgaDMATransferSetDst(NULL, dst) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetDst_ValidTransfer)
{
	// fpgaDMATransferSetDst(transfer, dst) must return FPGA_OK
}

// fpgaDMATransferSetLen
TEST_F(DmaAfuTest, fpgaDMATransferSetLen_InvalidTransfer)
{
	// fpgaDMATransferSetLen(NULL, dst) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetLen_ValidTransfer)
{
	// fpgaDMATransferSetLen(transfer, dst) must return FPGA_OK
}

// fpgaDMATransferSetTransferType
TEST_F(DmaAfuTest, fpgaDMATransferSetTransferType_InvalidTransfer)
{
	// fpgaDMATransferSetTransferType(NULL, type) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetTxCtrl_InvalidTransferType)
{
	// fpgaDMATransferSetTransferType(NULL, type) where type set to invalid value must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetTransferType_ValidTransfer)
{
	// fpgaDMATransferSetTransfer(transfer, type) must return FPGA_OK
}

// fpgaDMATransferSetTxCtrl
TEST_F(DmaAfuTest, fpgaDMATransferSetTxCtrl_InvalidTransfer)
{
	// fpgaDMATransferSetTxCtrl(NULL, tx_ctrl) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetTxCtrl_InvalidTxCtrl)
{
	// fpgaDMATransferSetTxCtrl(NULL, tx_ctrl) where tx_ctrl set to invalid value must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetTxCtrl_ValidTransfer)
{
	// fpgaDMATransferSetTxCtrl(transfer, tx_ctrl) must return FPGA_OK
}

// fpgaDMATransferSetRxCtrl
TEST_F(DmaAfuTest, fpgaDMATransferSetRxCtrl_InvalidTransfer)
{
	// fpgaDMATransferSetRxCtrl(NULL, rx_ctrl) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetRxCtrl_InvalidRxCtrl)
{
	// fpgaDMATransferSetRxCtrl(NULL, rx_ctrl) where rx_ctrl set to invalid value must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetRxCtrl_ValidTransfer)
{
	// fpgaDMATransferSetRxCtrl(transfer, rx_ctrl) must return FPGA_OK
}

// fpgaDMATransferSetTransferCallback
TEST_F(DmaAfuTest, fpgaDMATransferSetTransferCallback_InvalidTransfer)
{
	// fpgaDMATransferSetTransferCallback(NULL, cb) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransferSetTransferCallback_ValidTransfer)
{
	// fpgaDMATransferSetTransferCallback(transfer, cb) must return FPGA_OK
}

// fpgaDMATransfer
TEST_F(DmaAfuTest, fpgaDMATransfer_nullDMAHandle)
{
	// fpgaDMATransfer(NULL, transfer, cb, NULL) must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransfer_InvalidDMAHandle)
{
	// fpgaDMATransfer(dma_h, transfer, cb, NULL) must return FPGA_INVALID_PARAM) where dma_h was not opened using fpgaOpen must return FPGA_INVALID_PARAM
}

TEST_F(DmaAfuTest, fpgaDMATransfer_InvalidTransfer)
{
	// fpgaDMATransfer(dma_h, transfer, cb, NULL) must return FPGA_INVALID_PARAM if transfer is invalid
}

TEST_F(DmaAfuTest, fpgaDMATransfer_InvalidTransferType)
{
	// fpgaDMATransfer(dma_h, transfer, cb, NULL) must return FPGA_INVALID_PARAM if transfer type is invalid
}

TEST_F(DmaAfuTest, fpgaDMATransfer_ValidLen)
{
	// Deterministic DMA transfer must pass for all valid transfer types 
	// where length is < 1CL, 1CL, 2CL, 3CL and 4CL and any multiple of those lengths +- < 1CL
}
#endif

TEST_F(DmaAfuTest, fpgaDMATransfer_M2SBasicDeterministic)
{	
	// fpgaDMATransfer for any transfer type must return only after completion of the transfer if cb is set to NULL
	count = 20*1024*1024;
	debug_printk("count = %08lx \n", count);
	EXPECT_EQ(0,M2S_transfer(dma_h[0], count, NULL));
	err_cnt = 0;

}

TEST_F(DmaAfuTest, fpgaDMATransfer_S2MBasicDeterministic)
{
	// fpgaDMATransfer for any transfer type must return only after completion of the transfer if cb is set to NULL
	count = 10*1024*1024;
	debug_printk("count = %08lx \n", count);
	pkt_transfer = 0;
	EXPECT_EQ(0,S2M_transfer(dma_h[1], count, pkt_transfer, NULL));
	err_cnt = 0;
}

TEST_F(DmaAfuTest, fpgaDMATransfer_StressDeterministic)
{	
	// Issue 10,000 deterministic length fpgaDMATransfer with randomized packet length
}

TEST_F(DmaAfuTest, fpgaDMATransfer_S2MBasicNonDeterministic)
{
	// fpgaDMATransfer for any transfer type must return immediately if cb is not set to NULL. Cb must get called
	count = 15*1024*1024;
	debug_printk("count = %08lx \n", count);
	pkt_transfer = 1;
	EXPECT_EQ(0,S2M_transfer(dma_h[1], count, pkt_transfer, rxtransferComplete));
	err_cnt = 0;
}

#if 0
TEST_F(DmaAfuTest, fpgaDMATransfer_StressNonDeterministic)
{
	// Issue 10,000 non-deterministic length fpgaDMATransfer using different transfer objects, EOP set at random lengths	
}

TEST_F(DmaAfuTest, fpgaDMATransfer_StressNonDeterministicOnSameTransfer)
{	
	// Issue 10,000 non-deterministic length fpgaDMATransfer using the same transfer object, EOP set at random lengths	
	// This test will also verify transfer attributes can be correctly manipulated
}

TEST_F(DmaAfuTest, fpgaDMATransfer_ParallelChannelOperation)
{	
	// Issue 10,000 deterministic length transfers on Tx and Rx channels in parallel	
	// Use 4CL as the length (short transfers)
}

TEST_F(DmaAfuTest, fpgaDMATransfer_DescReuse)
{	
	// Cover various descriptor corner cases	
}

TEST_F(DmaAfuTest, fpgaDMATransfer_MultiThreaded_Channel)
{	
	// Issue 10,000 deterministic length transfers on Tx, half from Thread A, other half from thread B
	// use very long transfers	
}

TEST_F(DmaAfuTest, fpgaDMATransfer_Random)
{	
	// Issue 10,000 transfers, half from Thread A, other half from thread B
	// Use a random combination of deterministic and non-deterministc transfers
	// Use randomized length and ranomized EOP
}

TEST_F(DmaAfuTest, fpgaDMATransfer_BandwidthTest)
{	
	// Measure median bandwidth by running M2S and S2M transfer 30 times
}
#endif
/*
TEST_F(DmaAfuTest, fpgaDMAOpen)
{
	fpga_dma_handle_t dma;
	fpga_dma_handle_t *dma_handles;
	size_t count;
	
	ASSERT_EQ(FPGA_INVALID_PARAM, fpgaDMAOpen(NULL, 0, &dma));
	ASSERT_EQ(FPGA_INVALID_PARAM, fpgaDMAOpen(afc_h, -1, &dma));
	ASSERT_EQ(FPGA_INVALID_PARAM, fpgaDMAOpen(afc_h, -1, NULL));
	
	ASSERT_EQ(FPGA_OK, fpgaCountDMAChannels(afc_h, &count));
	dma_handles = malloc

	ASSERT_EQ(FPGA_OK, fpgaDMAOpen(afc_h, -1, NULL));
	for(int i = 0; i < count; i++) {
		ASSERT_EQ(FPGA_INVALID_PARAM, fpgaCountDMAChannels(afc_h, i, NULL));	
	}
	
	ASSERT_EQ(FPGA_OK, fpgaCountDMAChannels(afc_h, &count));
	ASSERT_EQ(MAX_ST_DMA_CHANNELS, count);
}
*/
int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
