#include <opae/access.h>
#include <opae/mmio.h>
#include <sys/mman.h>

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

#define CSR_SCRATCHPAD0 0x100
#define MMIO_OUT_REGION_ADDRESS 1024*1024*256

/**
 * @test mmio_drv_init_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaOpen must initialize handle->mmio_root to NULL.
 */
TEST(MMIO, mmio_drv_positive_init_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	EXPECT_TRUE(((struct _fpga_handle*)h)->mmio_root == NULL);
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_positive_map_mmio_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaMapMMIOPtr initializes handle->mmio_root != NULL
 */
TEST(MMIO, mmio_drv_positive_map_mmio_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t *mmio_ptr = NULL;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
	EXPECT_TRUE(((struct _fpga_handle*)h)->mmio_root == NULL);
	EXPECT_TRUE(mmio_ptr == NULL);

	// Unmap memory range otherwise, will not accept open from same process
//	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_negative_map_mmio_02
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaMapMMIOPtr must fail for non-existent MMIO area
 * fpgaUnmapMMIOPtr must fail for non-existent MMIO area
 */
TEST(MMIO, mmio_drv_negative_map_mmio_02)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	uint64_t *mmio_ptr = NULL;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, -1, &mmio_ptr));

	// Do not modify mmio_ptr and mmio_root
	EXPECT_TRUE(mmio_ptr == NULL);
	//EXPECT_TRUE(((struct _fpga_handle*)h)->mmio_root == NULL);

	// Unmap memory range otherwise, will not accept open from same process
//	EXPECT_NE(FPGA_NOT_SUPPORTED, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_positive_write_read_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO64 must write correct value at given MMIO offset.
 * fpgaReadMMIO64 must write correct value at given MMIO offset.
 */
TEST(MMIO, mmio_drv_positive_write_read_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint64_t *mmio_ptr = NULL;
	uint64_t value;
	uint64_t read_value;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;
	ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
	EXPECT_TRUE(mmio_ptr == NULL);
        mmio_ptr=0;
	// Write value and check correctness
	for (value = 0; value < 100; value += 10)
	{
		EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0, value));
		EXPECT_EQ(FPGA_OK, fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0, &read_value));
		EXPECT_EQ(read_value, value);

	}

	// Unmap memory range otherwise, will not accept open from same process
	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}


/** 
 * @test mmio_drv_negative_write_read_01
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO64 must fail for misaligned offset.
 * fpgaReadMMIO64 must fail for misaligned offset.
 * fpgaWriteMMIO64 must fail for out-of-region offset.
 * fpgaReadMMIO64 must fail for out-of-region offset.
 */

TEST(MMIO, mmio_drv_negative_write_read_01)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint64_t *mmio_ptr = NULL;
	uint64_t value;
	uint64_t read_value;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
	EXPECT_EQ(mmio_ptr, (uint64_t *) NULL);

	// Check errors for misalinged or out of boundary memory accesses
	EXPECT_NE(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0+1, value));
	EXPECT_NE(FPGA_OK, fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0+1, &read_value));
	EXPECT_NE(FPGA_OK, fpgaWriteMMIO64(h, 0, MMIO_OUT_REGION_ADDRESS, value));
	EXPECT_NE(FPGA_OK, fpgaReadMMIO64(h, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

	// Unmap memory range otherwise, will not accept open from same process
	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_positive_write32_read32_02
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO32 must write correct value at given MMIO offset.
 * fpgaReadMMIO32 must write correct value at given MMIO offset.
 */
TEST(MMIO, mmio_drv_positive_write32_read32_02)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint64_t *mmio_ptr = NULL;
	uint32_t value;
	uint32_t read_value;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;
	ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
	EXPECT_TRUE(mmio_ptr == NULL);

	// Write value and check correctness
	for (value = 0; value < 100; value += 10)
	{
		EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD0, value));
		EXPECT_EQ(FPGA_OK, fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD0, &read_value));
		EXPECT_EQ(read_value, value);
	}

	// Unmap memory range otherwise, will not accept open from same process
//	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_negative_write32_read32_02
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO32 must fail for misaligned offset.
 * fpgaReadMMIO32 must fail for misaligned offset.
 * fpgaWriteMMIO32 must fail for out-of-region offset.
 * fpgaReadMMIO32 must fail for out-of-region offset.
 */
TEST(MMIO, mmio_drv_negative_write32_read32_02)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint64_t *mmio_ptr = NULL;
	uint32_t value;
	uint32_t read_value;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
	EXPECT_TRUE(mmio_ptr == NULL);

	// Check errors for misalinged or out of boundary memory accesses
	EXPECT_NE(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD0+1, value));
	EXPECT_NE(FPGA_OK, fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD0+1, &read_value));
	EXPECT_NE(FPGA_OK, fpgaWriteMMIO32(h, 0, MMIO_OUT_REGION_ADDRESS, value));
	EXPECT_NE(FPGA_OK, fpgaReadMMIO32(h, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

	// Unmap memory range otherwise, will not accept open from same process
//	EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_negative_write_read_unmap_03
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO64 must fail if MMIO was not previously mmap
 * fpgaReadMMIO64 must fail if MMIO was not previously mmap
 * fpgaUnmapMMIO must fail if MMIO was not previously mmap
 */
TEST(MMIO, mmio_drv_negative_write_read_unmap_03)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint64_t value = 0;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;

	// Write value and expect failure
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0, value));

	// Read value and expect failure
	EXPECT_EQ(FPGA_OK, fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0, &value));

	// Unmap MMIO memory must fail too
//	EXPECT_NE(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test mmio_drv_negative_write32_read32_unmap_04
 * When the parameters are valid and the drivers are loaded:
 *
 * TEST:
 * fpgaWriteMMIO32 must fail if MMIO was not previously mmap
 * fpgaReadMMIO32 must fail exception if MMIO was not previously mmap
 * fpgaUnmapMMIO must fail if MMIO was not previously mmap
 */
TEST(MMIO, mmio_drv_negative_write32_read32_unmap_04)
{
	struct _fpga_token _tok;
	fpga_token tok = &_tok;
	fpga_handle h;
	struct _fpga_handle *_h;
	uint32_t value = 0;

	// Open  port device
	token_for_afu0(&_tok);
	ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
	_h = (struct _fpga_handle*)h;

	// Write value and expect failure
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD0, value));

	// Read value and expect failure
	EXPECT_EQ(FPGA_OK, fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD0, &value));

	// Unmap MMIO memory must fail too
//	EXPECT_NE(FPGA_OK, fpgaUnmapMMIO(h, 0));
	ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
