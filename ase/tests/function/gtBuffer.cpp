#ifdef __cplusplus
extern "C" {
#endif
#include "fpga/access.h"
#include <sys/mman.h>

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_1G_HUGEPAGE	(0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE)
#endif

#define NLB_DSM_SIZE          (2 * 1024 * 1024)
#define MAX_NLB_WKSPC_SIZE    CACHELINE(32768)
#define NLB_DSM_SIZE1         (2 * 1024 * 1024)
#define NLB_DSM_SIZE_1024         (10*1024)
/*#define ASE_TOKEN_MAGIC       0x46504741544f4b40
#define ASE_AFU_ID           "D8424DC4-A4A3-C413-F89E-433683F9040B"


void token_for_afu0(struct _fpga_token *_tok)
{
   // slot 0 AFU
   _tok->afc_id = (uint64_t)ASE_AFU_ID;
   _tok->magic = ASE_TOKEN_MAGIC;
}*/
/**
 * @test buffer_drv_no_prepare_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaReleaseBuffer must fail if fpga_buffer was not prepared.
 */
/*TEST(buffer, buffer_drv_no_prepare_01)
{

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Release buffer
	EXPECT_NE(FPGA_OK, fpgaReleaseBuffer(h, wsid));
//	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}*/


/**
 * @test buffer_drv_prepare_release_4kb_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaPrepareBuffer must allocate a shared memory buffer.
 * fpgaReleaseBuffer must release a shared memory buffer.
 */
TEST(buffer, buffer_drv_prepare_release_4kb_01)
{

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = 4 * 1024;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_prepare_release_2mb_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaPrepareBuffer must allocate a shared memory buffer.
 * fpgaReleaseBuffer must release a shared memory buffer.
 */
TEST(buffer, buffer_drv_prepare_release_2mb_01)
{

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = NLB_DSM_SIZE;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_write_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * Test writing to a shared memory buffer.
 */
TEST(buffer, buffer_drv_write_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	void *buf_addr;
	uint64_t wsid = 2;

	uint64_t offset;
	uint64_t value;
   // Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer
	buf_len = NLB_DSM_SIZE;
	ASSERT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Write test
	memset(buf_addr, 0, buf_len);
	for (offset = 0; offset < buf_len - sizeof(uint64_t); offset += sizeof(uint64_t))
	{
		value = offset;
		*((volatile uint64_t*) ((uint64_t) buf_addr + offset)) = value;
	}

	// Release buffer
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_write_read_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * Test writing and reading to/from a shared memory buffer
 */
TEST(buffer, buffer_drv_write_read_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	void *buf_addr;
	uint64_t wsid = 2;

	uint64_t offset;
	uint64_t value;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer
	buf_len = NLB_DSM_SIZE;
	ASSERT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Write test
	memset(buf_addr, 0, buf_len);
	for (offset = 0; offset < buf_len - sizeof(uint64_t); offset += sizeof(uint64_t))
	{
		value = offset;
		*((volatile uint64_t*) ((uint64_t) buf_addr + offset)) = value;
		EXPECT_EQ(*((volatile uint64_t*) ((uint64_t) buf_addr + offset)), offset);
	}

	// Release buffer
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}


/**
 * @test buffer_drv_preallocated_prepare_release_2mb_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaPrepareBuffer must allocate a shared memory buffer.
 * fpgaReleaseBuffer must release a shared memory buffer.
 */
/*TEST(buffer, buffer_drv_preallocated_prepare_release_2mb_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	EXPECT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = 4 * 1024 * 1024;
 	buf_addr = (uint64_t *) mmap(ADDR, buf_len, PROTECTION, FLAGS_2M, 0, 0);
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, FPGA_BUF_PREALLOCATED));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));

	// buf_addr was preallocated, do not touch it
	ASSERT_NE(buf_addr, (void*) NULL);
	munmap(buf_addr, buf_len);
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}*/
