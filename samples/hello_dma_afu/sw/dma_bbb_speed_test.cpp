#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <assert.h>

#include "dma_test_common.h"
#include "RC4memtest.h"

#ifdef INCLUDE_DMA_DRIVER
extern "C" {
#include "fpga_dma.h"
}
#endif

int usleep(unsigned);

#define DMA_TEST_AFU_ID              "331DB30C-9885-41EA-9081-F88B8F655CAA"
//opencl ddr bsp afu id
//#define DMA_TEST_AFU_ID              "18B79FFA-2EE5-4AA0-96EF-4230DAFACB5F"
#define SCRATCH_REG              0X80
#define SCRATCH_VALUE            0x0123456789ABCDEF
#define SCRATCH_RESET            0
#define BYTE_OFFSET              8

#define DMA_BUFFER_SIZE (1024*1024)

#define AFU_DFH_REG              0x0
#define AFU_ID_LO                0x8 
#define AFU_ID_HI                0x10
#define AFU_NEXT                 0x18
#define AFU_RESERVED             0x20

static int s_error_count = 0;

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)                    \
	do {                                       \
		if ((res) != FPGA_OK) {            \
			print_err((desc), (res));  \
			s_error_count += 1; \
			goto label;                \
		}                                  \
	} while (0)

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ASSERT_GOTO(condition, label, desc)                    \
	do {                                       \
		if (condition == 0) {            \
			fprintf(stderr, "Error %s\n", desc); \
			s_error_count += 1; \
			goto label;                \
		}                                  \
	} while (0)
		
/* Type definitions */
typedef struct {
	uint32_t uint[16];
} cache_line;

// High-resolution timer.
double getCurrentTimestamp() {
  timespec a;
  clock_gettime(CLOCK_MONOTONIC, &a);
  return (double(a.tv_nsec) * 1.0e-9) + double(a.tv_sec);
}

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

void mmio_read64(fpga_handle afc_handle, uint64_t addr, uint64_t *data, const char *reg_name)
{
	fpgaReadMMIO64(afc_handle, 0, addr, data);
	printf("Reading %s (Byte Offset=%08lx) = %08lx\n", reg_name, addr, *data);
}

void mmio_read64_silent(fpga_handle afc_handle, uint64_t addr, uint64_t *data)
{
	fpgaReadMMIO64(afc_handle, 0, addr, data);
}

void mmio_write64(fpga_handle afc_handle, uint64_t addr, uint64_t data, const char *reg_name)
{
	fpgaWriteMMIO64(afc_handle, 0, addr, data);
	printf("MMIO Write to %s (Byte Offset=%08lx) = %08lx\n", reg_name, addr, data);
}

int dma_memory_checker(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr
)
{
	int num_errors = 0;
	
#ifdef USE_ASE
	const long DMA_TEST_MEM_SIZE	 = 1024;
	const long DMA_BUF_SIZE = 256;
#else
	const long DMA_TEST_MEM_SIZE	 = 8l*1024l*1024l*1024l/32l;
	const long DMA_BUF_SIZE = 512*1024;
#endif

	//const long DMA_DEV_OFFSET = 8l*1024l*1024l*1024l;
	//const long DMA_DEV_OFFSET = 4l*1024l*1024l*1024l;
	const long DMA_DEV_OFFSET = 0;
	
	SimpleRandomMemtest memtest_obj;
	long byte_errors = 0;
	long page_errors = 0;
	const char *RC4_KEY = "mytestkey";
	double start = 0.0;
	double end = 0.0;
	double dma_write_time = 0.0;
	double dma_read_time = 0.0;
	
	
	assert(DMA_TEST_MEM_SIZE % DMA_BUF_SIZE == 0);
	
	const long NUM_DMA_TRANSFERS = DMA_TEST_MEM_SIZE/DMA_BUF_SIZE;
	
	memtest_obj.setup_key(RC4_KEY);
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		memtest_obj.write_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
		uint64_t dev_addr = i*DMA_BUF_SIZE + DMA_DEV_OFFSET;
		
		start = getCurrentTimestamp();
		copy_dev_to_dev_with_dma(afc_handle, dma_buf_iova | MSGDMA_BBB_HOST_MASK, dev_addr, DMA_BUF_SIZE);
		end = getCurrentTimestamp();
		dma_write_time += (end - start);
	}
	
	//reload rc4 state to begining
	memtest_obj.setup_key(RC4_KEY);
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		uint64_t dev_addr = i*DMA_BUF_SIZE + DMA_DEV_OFFSET;
		start = getCurrentTimestamp();
		copy_dev_to_dev_with_dma(afc_handle, dev_addr, dma_buf_iova | MSGDMA_BBB_HOST_MASK, DMA_BUF_SIZE);
		end = getCurrentTimestamp();
		dma_read_time += (end - start);
		long errors = memtest_obj.check_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
		if(errors)
			page_errors++;
		byte_errors += errors;
	}
	
	printf("mem_size=%ld dma_buf_size=%ld num_dma_buf=%ld\n", DMA_TEST_MEM_SIZE, DMA_BUF_SIZE, NUM_DMA_TRANSFERS);
	printf("byte_errors=%ld, page_errors=%ld\n", byte_errors, page_errors);
	if(byte_errors)
	{
		printf("ERROR: memtest FAILED!\n");
		s_error_count += 1;
		return 0;
	}
	
	#define TRANS_SPEED_FROM_TIME(x) ((int)((double)(DMA_TEST_MEM_SIZE)/1024.0d/1024.0d/x))
	#define TRANS_EFFICIENCY(x) ((double)(TRANS_SPEED_FROM_TIME(x))/6400.0d*100.0d)
	#define MIN_EFFICIENCY	60.0d
	#define CHECK_EFFICIENCT(x) { \
		if(TRANS_EFFICIENCY(x) < MIN_EFFICIENCY) { \
			printf("ERROR: measured speed is less than %.1f%!\n", MIN_EFFICIENCY); \
			num_errors += 1; \
		} \
	}
	
	printf("dma_read_time = %f\n", dma_read_time);
	printf("dma_write_time = %f\n", dma_write_time);
	printf("DMA_READ_SPEED = %d MB/sec (%f%%)\n", TRANS_SPEED_FROM_TIME(dma_read_time), TRANS_EFFICIENCY(dma_read_time));
	CHECK_EFFICIENCT(dma_read_time);
	printf("DMA_WRITE_SPEED = %d MB/sec (%f%%)\n", TRANS_SPEED_FROM_TIME(dma_write_time), TRANS_EFFICIENCY(dma_write_time));
	CHECK_EFFICIENCT(dma_write_time);
	printf("\n");

	#ifdef INCLUDE_DMA_DRIVER
	{
	fpga_dma_handle dma_h;
	fpga_result res = FPGA_OK;
	
	res = fpgaDmaOpen(afc_handle, &dma_h);
	if(res != FPGA_OK)
	{
		printf("Error: DMA init failed! code=%d \n", res);
		exit(1);
	}
	
	char *dma_buf_ptr2 = (char *)malloc(DMA_BUF_SIZE);
	
	dma_read_time = 0;
	dma_write_time = 0;
	
	memtest_obj.setup_key(RC4_KEY);
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		memtest_obj.write_bytes((char *)dma_buf_ptr2, (int)DMA_BUF_SIZE);
		uint64_t dev_addr = i*DMA_BUF_SIZE + DMA_DEV_OFFSET;;
		
		start = getCurrentTimestamp();
		res = fpgaDmaTransferSync(dma_h, dev_addr /*dst*/, (uint64_t)dma_buf_ptr2 /*src*/, DMA_BUF_SIZE, HOST_TO_FPGA_MM);
		end = getCurrentTimestamp();
		dma_write_time += (end - start);
	}
	
	//reload rc4 state to begining
	memtest_obj.setup_key(RC4_KEY);
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		uint64_t dev_addr = i*DMA_BUF_SIZE + DMA_DEV_OFFSET;;
		start = getCurrentTimestamp();
		res = fpgaDmaTransferSync(dma_h, (uint64_t)dma_buf_ptr2 /*dst*/, dev_addr /*src*/, DMA_BUF_SIZE, FPGA_TO_HOST_MM);
		end = getCurrentTimestamp();
		dma_read_time += (end - start);
		long errors = memtest_obj.check_bytes((char *)dma_buf_ptr2, (int)DMA_BUF_SIZE);
		if(errors)
			page_errors++;
		byte_errors += errors;
	}
	
	printf("fpga_dma_driver_read_time = %f\n", dma_read_time);
	printf("fpga_dma_driver_write_time = %f\n", dma_write_time);
	printf("FPGA_DMA_DRIVER_READ_SPEED = %d MB/sec (%f%%)\n", TRANS_SPEED_FROM_TIME(dma_read_time), TRANS_EFFICIENCY(dma_read_time));
	printf("FPGA_DMA_DRIVER_WRITE_SPEED = %d MB/sec (%f%%)\n", TRANS_SPEED_FROM_TIME(dma_write_time), TRANS_EFFICIENCY(dma_write_time));
	printf("\n");

	free(dma_buf_ptr2);
	res = fpgaDmaClose(dma_h);
	}
	#endif
	
	printf("num_errors = %d\n", num_errors);
	s_error_count += num_errors;
}

int dma_memory_checker(fpga_handle afc_handle)
{
	volatile uint64_t *dma_buf_ptr  = NULL;
	uint64_t        dma_buf_wsid;
	uint64_t dma_buf_iova;
	
	fpga_result     res = FPGA_OK;

	res = fpgaPrepareBuffer(afc_handle, DMA_BUFFER_SIZE,
		(void **)&dma_buf_ptr, &dma_buf_wsid, 0);
	ON_ERR_GOTO(res, release_buf, "allocating dma buffer");
	memset((void *)dma_buf_ptr,  0x0, DMA_BUFFER_SIZE);
	
	res = fpgaGetIOAddress(afc_handle, dma_buf_wsid, &dma_buf_iova);
	ON_ERR_GOTO(res, release_buf, "getting dma DMA_BUF_IOVA");
	
	printf("DMA_BUFFER_SIZE = %d\n", DMA_BUFFER_SIZE);
	
	dma_memory_checker(afc_handle, dma_buf_iova, dma_buf_ptr);

release_buf:
	res = fpgaReleaseBuffer(afc_handle, dma_buf_wsid);
}

int main(int argc, char *argv[])
{
	fpga_properties    filter = NULL;
	fpga_token         afc_token;
	fpga_handle        afc_handle;
	fpga_guid          guid;
	uint32_t           num_matches;
	uint64_t data = 0;
	uint64_t dfh_size = 0;
	bool found_dfh = 0;
	uint64_t msgdma_bbb_dfh_offset = 0;

	fpga_result     res = FPGA_OK;

	if (uuid_parse(DMA_TEST_AFU_ID, guid) < 0) {
		fprintf(stderr, "Error parsing guid '%s'\n", DMA_TEST_AFU_ID);
		goto out_exit;
	}

	/* Look for AFC with MY_AFC_ID */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_exit, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

	res = fpgaPropertiesSetGUID(filter, guid);
	ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");

	/* TODO: Add selection via BDF / device ID */

	res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "enumerating AFCs");

	if (num_matches < 1) {
		fprintf(stderr, "AFC not found.\n");
		res = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}

	/* Open AFC and map MMIO */
	res = fpgaOpen(afc_token, &afc_handle, 0);
	ON_ERR_GOTO(res, out_destroy_tok, "opening AFC");

	res = fpgaMapMMIO(afc_handle, 0, NULL);
	ON_ERR_GOTO(res, out_close, "mapping MMIO space");

	printf("Running Test\n");

	/* Reset AFC */
	res = fpgaReset(afc_handle);
	ON_ERR_GOTO(res, out_close, "resetting AFC");

	// Access mandatory AFU registers
	res = fpgaReadMMIO64(afc_handle, 0, AFU_DFH_REG, &data);
	ON_ERR_GOTO(res, out_close, "reading from MMIO");
	printf("AFU DFH REG = %08lx\n", data);
	
	res = fpgaReadMMIO64(afc_handle, 0, AFU_ID_LO, &data);
	ON_ERR_GOTO(res, out_close, "reading from MMIO");
	printf("AFU ID LO = %08lx\n", data);
	
	res = fpgaReadMMIO64(afc_handle, 0, AFU_ID_HI, &data);
	ON_ERR_GOTO(res, out_close, "reading from MMIO");
	printf("AFU ID HI = %08lx\n", data);
	
	res = fpgaReadMMIO64(afc_handle, 0, AFU_NEXT, &data);
	ON_ERR_GOTO(res, out_close, "reading from MMIO");
	printf("AFU NEXT = %08lx\n", data);
	
	res = fpgaReadMMIO64(afc_handle, 0, AFU_RESERVED, &data);
	ON_ERR_GOTO(res, out_close, "reading from MMIO");
	printf("AFU RESERVED = %08lx\n", data);
	
	found_dfh = find_dfh_by_guid(afc_handle, MSGDMA_BBB_GUID, &msgdma_bbb_dfh_offset, &dfh_size);
	set_msgdma_bbb_dfh_offset(msgdma_bbb_dfh_offset);
	assert(found_dfh);
	assert(dfh_size == MSGDMA_BBB_SIZE);

	dump_dfh_list(afc_handle);
	dma_memory_checker(afc_handle);

	printf("Done Running Test\n");

	/* Unmap MMIO space */
out_unmap:
	res = fpgaUnmapMMIO(afc_handle, 0);
	ON_ERR_GOTO(res, out_close, "unmapping MMIO space");
	
	/* Release accelerator */
out_close:
	res = fpgaClose(afc_handle);
	ON_ERR_GOTO(res, out_destroy_tok, "closing AFC");

	/* Destroy token */
out_destroy_tok:
	res = fpgaDestroyToken(&afc_token);
	ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

	/* Destroy properties object */
out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
	if(s_error_count > 0)
		printf("Test FAILED!\n");

	return s_error_count;

}
