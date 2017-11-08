#include <opae/access.h>
#include <opae/buffer.h>
#include <sys/mman.h>

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
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#endif

#define NLB_DSM_SIZE          (2 * 1024 * 1024)
#define MAX_NLB_WKSPC_SIZE    CACHELINE(32768)
#define NLB_DSM_SIZE1         (2 * 1024 * 1024)
#define NLB_DSM_SIZE_1024         (10*1024)

/** 
 * @test buffer_drv_no_prepare_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaReleaseBuffer must fail if fpga_buffer was not prepared.
 */

TEST(buffer, buffer_drv_no_prepare_01)
{

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Release buffer
	EXPECT_NE(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}


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
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = 4 * 1024;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_prepare_preallocated_0bytes
 *
 * When FPGA_BUF_PREALLOCATED is given and the buffer len is 0,
 * fpgaPrepareBuffer fails with FPGA_INVALID_PARAM.
 */
TEST(buffer, buffer_drv_prepare_preallocated_0bytes)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr = (uint64_t *) 1;
	uint64_t wsid = 1;
	int flags;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	buf_len = 0;
	flags = FPGA_BUF_PREALLOCATED;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h,
							buf_len,
							(void **) &buf_addr,
							&wsid,
							flags));
	// Close the device
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_prepare_preallocated_not_page_aligned
 *
 * When FPGA_BUF_PREALLOCATED is given and the buffer len is not
 * a multiple of the page size,
 * fpgaPrepareBuffer fails with FPGA_INVALID_PARAM.
 */
TEST(buffer, buffer_drv_prepare_preallocated_not_page_aligned)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr = (uint64_t *) 1;
	uint64_t wsid = 1;
	int flags;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	buf_len = (4 * 1024) - 1;
	flags = FPGA_BUF_PREALLOCATED;
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(h,
							buf_len,
							(void **) &buf_addr,
							&wsid,
							flags));
	// Close the device
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/** 
 * @test buffer_drv_prepare_0bytes
 *
 * When FPGA_BUF_PREALLOCATED is not given and the buffer len is 0,
 * fpgaPrepareBuffer allocates a one page buffer and returns
 * FPGA_OK.
 */
TEST(buffer, buffer_drv_prepare_0bytes)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;
	int flags;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	buf_len = 0;
	flags = 0;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h,
				 	     buf_len,
					     (void **) &buf_addr,
					     &wsid,
					     flags));

	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	// Close the device
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test buffer_drv_prepare_not_page_aligned
 *
 * When FPGA_BUF_PREALLOCATED is not given and the buffer len is not
 * a multiple of the page size,
 * fpgaPrepareBuffer allocates the next multiple of page size and
 * returns FPGA_OK.
 */
TEST(buffer, buffer_drv_prepare_not_page_aligned)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;
	int flags;

	// Open port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	buf_len = (4 * 1024) - 1;
	flags = 0;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h,
					     buf_len,
					     (void **) &buf_addr,
					     &wsid,
					     flags));

	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	// Close the device
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
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

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
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

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
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

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
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = 2 * 1024 * 1024;
 	buf_addr = (uint64_t *) mmap(ADDR, buf_len, PROTECTION, FLAGS_2M, 0, 0);
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, FPGA_BUF_PREALLOCATED));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));

	// buf_addr was preallocated, do not touch it
	ASSERT_NE(buf_addr, (void*) NULL);
	munmap(buf_addr, buf_len);
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}i*/

/**
 * @test buffer_drv_prepare_release_4mb_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaPrepareBuffer must allocate a shared memory buffer.
 * fpgaReleaseBuffer must release a shared memory buffer.
 */
/*
TEST(buffer, buffer_drv_prepare_release_4mb_01)
{

	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t buf_len;
	uint64_t *buf_addr;
	uint64_t wsid = 1;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));

	// Allocate buffer in MB range
	buf_len = 4 * 1024 *1024;
	EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(h, buf_len, (void **) &buf_addr, &wsid, 0));

	// Release buffer in MB range
	EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
*/

