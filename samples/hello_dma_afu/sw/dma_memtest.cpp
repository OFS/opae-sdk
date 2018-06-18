#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <assert.h>

#include "RC4memtest.h"
#include "dma_test_common.h"

int usleep(unsigned);

#define DMA_TEST_AFU_ID              "331DB30C-9885-41EA-9081-F88B8F655CAA"
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
static uint64_t msgdma_bbb_dfh_offset = -256*1024;

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

int test_main(int argc, char *argv[], fpga_handle afc_handle);

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

int dma_memory_checker(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr,
	long mem_size,
	int test_count = 1
)
{
	for(int j = 0; j < test_count; j++)
	{
		if(test_count != 1)
			printf("test count=%d\n", j);
		int num_errors = 0;
		
	#ifdef USE_ASE
		const long DMA_BUF_SIZE = 256;
	#else
		const long DMA_BUF_SIZE = 512*1024;
	#endif
	
		RC4Memtest rc4_obj;
		long byte_errors = 0;
		long page_errors = 0;
		const char *RC4_KEY = "mytestkey";
		
		rc4_obj.setup_key(RC4_KEY);
		
		assert(mem_size % DMA_BUF_SIZE == 0);
		
		const long NUM_DMA_TRANSFERS = mem_size/DMA_BUF_SIZE;
		printf("Starting memtest...\n");
		printf("Starting transfer from host to DDR - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			rc4_obj.write_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			
			copy_dev_to_dev_with_dma(afc_handle, dma_buf_iova | MSGDMA_BBB_HOST_MASK, dev_addr, DMA_BUF_SIZE);
		}
		printf("Finished transfer from host to DDR\n");
		
		//reload rc4 state to begining
		rc4_obj.setup_key(RC4_KEY);
		
		printf("Starting transfer from DDR to host - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			copy_dev_to_dev_with_dma(afc_handle, dev_addr, dma_buf_iova | MSGDMA_BBB_HOST_MASK, DMA_BUF_SIZE);
			long errors = rc4_obj.check_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
			if(errors)
				page_errors++;
			byte_errors += errors;
		}
		printf("Finished transfer from DDR to host\n");
		
		printf("mem_size=%ld dma_buf_size=%ld num_dma_buf=%ld\n", mem_size, DMA_BUF_SIZE, NUM_DMA_TRANSFERS);
		printf("byte_errors=%ld, page_errors=%ld\n", byte_errors, page_errors);
		if(byte_errors)
		{
			printf("ERROR: memtest FAILED!\n");
			s_error_count += 1;
			return 0;
		}
	
		printf("num_errors = %d\n", num_errors);
		s_error_count += num_errors;
	}
}

int dma_read_test_fast(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr,
	long mem_size,
	int test_count = 1
)
{

	int num_errors = 0;
	
#ifdef USE_ASE
	const long DMA_BUF_SIZE = 256;
#else
	const long DMA_BUF_SIZE = 512*1024;
#endif

	RC4Memtest rc4_obj;
	long byte_errors = 0;
	long page_errors = 0;
	const char *RC4_KEY = "mytestkey";
	
	rc4_obj.setup_key(RC4_KEY);
	char check_buffer[DMA_BUF_SIZE];
	rc4_obj.write_bytes((char *)check_buffer, (int)DMA_BUF_SIZE);
	
	assert(mem_size % DMA_BUF_SIZE == 0);
	
	const long NUM_DMA_TRANSFERS = mem_size/DMA_BUF_SIZE;
	printf("Starting memtest...\n");
	printf("Starting transfer from host to DDR - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		rc4_obj.setup_key(RC4_KEY);
		rc4_obj.write_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
		uint64_t dev_addr = i*DMA_BUF_SIZE;
		
		//copy_dev_to_dev_with_dma(afc_handle, dma_buf_iova | MSGDMA_BBB_HOST_MASK, dev_addr, DMA_BUF_SIZE);
		copy_to_dev_with_mmio(afc_handle, (uint64_t *)dma_buf_ptr, dev_addr, DMA_BUF_SIZE);
	}
	printf("Finished transfer from host to DDR\n");
	
	for(int j = 0; j < test_count; j++)
	{
		if(test_count != 1)
			printf("test count=%d\n", j);
	
		//reload rc4 state to begining
		rc4_obj.setup_key(RC4_KEY);
		
		printf("Starting transfer from DDR to host - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			copy_dev_to_dev_with_dma(afc_handle, dev_addr, dma_buf_iova | MSGDMA_BBB_HOST_MASK, DMA_BUF_SIZE);
			//long errors = rc4_obj.check_bytes((char *)check_buffer, (int)DMA_BUF_SIZE);
			if(memcmp((void *)dma_buf_ptr, (void *)check_buffer, DMA_BUF_SIZE) != 0)
				page_errors++;
		}
		printf("Finished transfer from DDR to host\n");
		
		printf("mem_size=%ld dma_buf_size=%ld num_dma_buf=%ld\n", mem_size, DMA_BUF_SIZE, NUM_DMA_TRANSFERS);
		printf("byte_errors=%ld, page_errors=%ld\n", byte_errors, page_errors);
		if(byte_errors)
		{
			printf("ERROR: memtest FAILED!\n");
			s_error_count += 1;
			return 0;
		}
	}

	printf("num_errors = %d\n", num_errors);
	s_error_count += num_errors;
}


int dma_read_test(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr,
	long mem_size,
	int test_count = 1
)
{

	int num_errors = 0;
	
#ifdef USE_ASE
	const long DMA_BUF_SIZE = 256;
#else
	const long DMA_BUF_SIZE = 512*1024;
#endif

	IncrMemtest rc4_obj;
	long byte_errors = 0;
	long page_errors = 0;
	const char *RC4_KEY = "mytestkey";
	
	rc4_obj.setup_key(RC4_KEY);
	
	assert(mem_size % DMA_BUF_SIZE == 0);
	
	const long NUM_DMA_TRANSFERS = mem_size/DMA_BUF_SIZE;
	printf("Starting memtest...\n");
	printf("Starting transfer from host to DDR - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
	for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
	{
		rc4_obj.write_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
		uint64_t dev_addr = i*DMA_BUF_SIZE;
		
		//copy_dev_to_dev_with_dma(afc_handle, dma_buf_iova | MSGDMA_BBB_HOST_MASK, dev_addr, DMA_BUF_SIZE);
		copy_to_dev_with_mmio(afc_handle, (uint64_t *)dma_buf_ptr, dev_addr, DMA_BUF_SIZE);
	}
	printf("Finished transfer from host to DDR\n");
	
	for(int j = 0; j < test_count; j++)
	{
		if(test_count != 1)
			printf("test count=%d\n", j);
	
		//reload rc4 state to begining
		rc4_obj.setup_key(RC4_KEY);
		
		printf("Starting transfer from DDR to host - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			copy_dev_to_dev_with_dma(afc_handle, dev_addr, dma_buf_iova | MSGDMA_BBB_HOST_MASK, DMA_BUF_SIZE);
			long errors = rc4_obj.check_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
			if(errors)
				page_errors++;
			byte_errors += errors;
		}
		printf("Finished transfer from DDR to host\n");
		
		printf("mem_size=%ld dma_buf_size=%ld num_dma_buf=%ld\n", mem_size, DMA_BUF_SIZE, NUM_DMA_TRANSFERS);
		printf("byte_errors=%ld, page_errors=%ld\n", byte_errors, page_errors);
		if(byte_errors)
		{
			printf("ERROR: memtest FAILED!\n");
			s_error_count += 1;
			return 0;
		}
	}

	printf("num_errors = %d\n", num_errors);
	s_error_count += num_errors;
}

int dma_write_test(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr,
	long mem_size,
	int test_count = 1
)
{

	int num_errors = 0;
	
#ifdef USE_ASE
	const long DMA_BUF_SIZE = 256;
#else
	const long DMA_BUF_SIZE = 512*1024;
#endif

	RC4Memtest rc4_obj;
	long byte_errors = 0;
	long page_errors = 0;
	const char *RC4_KEY = "mytestkey";
	
	rc4_obj.setup_key(RC4_KEY);
	rc4_obj.write_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
	
	assert(mem_size % DMA_BUF_SIZE == 0);
	
	const long NUM_DMA_TRANSFERS = mem_size/DMA_BUF_SIZE;
	printf("Starting memread_test...\n");
	for(int j = 0; j < test_count; j++)
	{
		if(test_count != 1)
			printf("test count=%d\n", j);
		printf("Starting transfer from host to DDR - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			
			copy_dev_to_dev_with_dma(afc_handle, dma_buf_iova | MSGDMA_BBB_HOST_MASK, dev_addr, DMA_BUF_SIZE);
		}
		printf("Finished transfer from host to DDR\n");
	}
	
	printf("num_errors = %d\n", num_errors);
	s_error_count += num_errors;
}

int dma_clear_test(
	fpga_handle afc_handle,
	uint64_t dma_buf_iova,
	volatile uint64_t *dma_buf_ptr,
	long mem_size,
	int test_count = 1
)
{

	int num_errors = 0;
	
#ifdef USE_ASE
	const long DMA_BUF_SIZE = 256;
#else
	const long DMA_BUF_SIZE = 512*1024;
#endif

	ClearMemtest rc4_obj;
	long byte_errors = 0;
	long page_errors = 0;
	
	
	assert(mem_size % DMA_BUF_SIZE == 0);
	
	const long NUM_DMA_TRANSFERS = mem_size/DMA_BUF_SIZE;
	printf("Starting DDR clar memtest...\n");
	for(int j = 0; j < test_count; j++)
	{
		if(test_count != 1)
			printf("test count=%d\n", j);
		
		printf("Starting transfer from DDR to host - size %d MBytes\n", (int)(mem_size/(1024l*1024l)));
		for(long i = 0; i < NUM_DMA_TRANSFERS; i++)
		{
			uint64_t dev_addr = i*DMA_BUF_SIZE;
			copy_dev_to_dev_with_dma(afc_handle, dev_addr, dma_buf_iova | MSGDMA_BBB_HOST_MASK, DMA_BUF_SIZE);
			long errors = rc4_obj.check_bytes((char *)dma_buf_ptr, (int)DMA_BUF_SIZE);
			if(errors)
				page_errors++;
			byte_errors += errors;
		}
		printf("Finished transfer from DDR to host\n");
		
		printf("mem_size=%ld dma_buf_size=%ld num_dma_buf=%ld\n", mem_size, DMA_BUF_SIZE, NUM_DMA_TRANSFERS);
		printf("byte_errors=%ld, page_errors=%ld\n", byte_errors, page_errors);
		if(byte_errors)
		{
			printf("ERROR: memtest FAILED!\n");
			s_error_count += 1;
			return 0;
		}
	}

	printf("num_errors = %d\n", num_errors);
	s_error_count += num_errors;
}



class DMABuffer
{
private:
	fpga_handle m_afc_handle;
	
public:
	volatile uint64_t *dma_buf_ptr;
	uint64_t        dma_buf_wsid;
	uint64_t dma_buf_iova;
	
	DMABuffer(fpga_handle afc_handle, int buffer_size = DMA_BUFFER_SIZE)
	{
		m_afc_handle = afc_handle;
		dma_buf_ptr  = NULL;
		fpga_result     res = FPGA_OK;
	
		res = fpgaPrepareBuffer(m_afc_handle, buffer_size,
			(void **)&dma_buf_ptr, &dma_buf_wsid, 0);
		if (res != FPGA_OK) {
			print_err("allocating dma buffer", res);
			res = fpgaReleaseBuffer(m_afc_handle, dma_buf_wsid);
			exit(1);
		}
		
		memset((void *)dma_buf_ptr,  0x0, buffer_size);
		
		res = fpgaGetIOAddress(m_afc_handle, dma_buf_wsid, &dma_buf_iova);
		if (res != FPGA_OK) {
			print_err("getting dma DMA_BUF_IOVA", res);
			res = fpgaReleaseBuffer(m_afc_handle, dma_buf_wsid);
			exit(1);
		}
	}
	
	virtual ~DMABuffer()
	{
		if(dma_buf_ptr)
		{
			fpga_result     res = FPGA_OK;
			res = fpgaReleaseBuffer(m_afc_handle, dma_buf_wsid);
		}
	}
	
private:
	//not supported
    DMABuffer(const DMABuffer& other); // copy constructor
    DMABuffer& operator=(const DMABuffer& other); // copy assignment
};

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

	test_main(argc, argv, afc_handle);

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
	else
		printf("Test PASSED!\n");

	return s_error_count;

}

int test_main(int argc, char *argv[], fpga_handle afc_handle)
{
	int current_arg = 1;
	
	long mem_mult = 8;
	int repeat = 1;
	
	enum {
		MEMTEST_MODE,
		MEMWRITE_TEST,
		MEMREAD_TEST,
		MEMREAD_FAST_TEST,
		MEMCLEAR_TEST
	} test_mode = MEMTEST_MODE;
	
	while(current_arg < argc)
	{
		//printf(argv[current_arg]);
		if(strcmp(argv[current_arg], "-s") == 0) {
			current_arg++;
			assert(current_arg < argc);
			mem_mult = (long)atoi(argv[current_arg]);
		}
		else if(strcmp(argv[current_arg], "-r") == 0) {
			current_arg++;
			assert(current_arg < argc);
			repeat = atoi(argv[current_arg]);
		}
		else if(strcmp(argv[current_arg], "-m") == 0) {
			current_arg++;
			assert(current_arg < argc);
			if(strcmp(argv[current_arg], "memtest") == 0)
				test_mode = MEMTEST_MODE;
			else if(strcmp(argv[current_arg], "memwrite") == 0)
				test_mode = MEMWRITE_TEST;
			else if(strcmp(argv[current_arg], "memread") == 0)
				test_mode = MEMREAD_TEST;
			else if(strcmp(argv[current_arg], "memread_fast") == 0)
				test_mode = MEMREAD_FAST_TEST;
			else if(strcmp(argv[current_arg], "memclear") == 0)
				test_mode = MEMCLEAR_TEST;
			else
			{
				printf("ERROR: Unexpected mode '%s'\n", argv[current_arg]);
				s_error_count++;
				return s_error_count;
			}
			//
		}
		else {
			printf("ERROR: Unexpected argument '%s'\n", argv[current_arg]);
			s_error_count++;
			return s_error_count;
		}
		current_arg++;
	}
	
	#ifdef USE_ASE
		const long mem_size	 = 128*mem_mult;
	#else
		const long mem_size	 = mem_mult*1024l*1024l;
	#endif
	
	DMABuffer dma_buf(afc_handle);
	printf("DMA Buffer IOVA %p\n", dma_buf.dma_buf_iova);
	printf("DMA Buffer Virtual %p\n", dma_buf.dma_buf_ptr);
	
	switch(test_mode)
	{
	case MEMTEST_MODE:
		dma_memory_checker(afc_handle, dma_buf.dma_buf_iova, dma_buf.dma_buf_ptr, mem_size, repeat);
		break;
	case MEMWRITE_TEST:
		dma_write_test(afc_handle, dma_buf.dma_buf_iova, dma_buf.dma_buf_ptr, mem_size, repeat);
		break;
	case MEMREAD_TEST:
		dma_read_test(afc_handle, dma_buf.dma_buf_iova, dma_buf.dma_buf_ptr, mem_size, repeat);
		break;
	case MEMREAD_FAST_TEST:
		dma_read_test_fast(afc_handle, dma_buf.dma_buf_iova, dma_buf.dma_buf_ptr, mem_size, repeat);
		break;
	case MEMCLEAR_TEST:
		dma_clear_test(afc_handle, dma_buf.dma_buf_iova, dma_buf.dma_buf_ptr, mem_size, repeat);
		break;
	}
}
