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

#include <opae/access.h>
#include <opae/mmio.h>
#include <sys/mman.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define CSR_SCRATCHPAD0 0x100
//#define MAX_MMIO_SIZE  1024*256
//#define MMIO_TEST_OFFSET 0x18  //skip first 24 bytes of DFH heder
#define MMIO_OUT_REGION_ADDRESS 1024 * 1024 * 256
#define MAX_MMIO_SIZE 1024 * 256

using namespace common_test;


/*
 * The mmio map is a hash table.
 */
static bool mmio_map_is_empty(struct wsid_tracker *root) {
  if (!root || (root->n_hash_buckets == 0))
    return true;

  for (uint32_t i = 0; i < root->n_hash_buckets; i += 1) {
    if (root->table[i])
      return false;
  }

  return true;
}


/**
 * @test       mmio_drv_init_01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaOpen must initialize handle->mmio_root to NULL.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_positive_init_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifndef BUILD_ASE
  // On HW we build a hash table
  EXPECT_FALSE(((struct _fpga_handle*)h)->mmio_root == NULL);
  EXPECT_FALSE(((struct _fpga_handle*)h)->mmio_root->n_hash_buckets == 0);
#endif
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)h)->mmio_root));
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_positive_map_mmio_01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaMapMMIOPtr initializes handle->mmio_root != NULL
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_positive_map_mmio_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifndef BUILD_ASE
  ASSERT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);
  EXPECT_FALSE(mmio_map_is_empty(((struct _fpga_handle*)h)->mmio_root));
#else
  // ASE
  ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_TRUE(((struct _fpga_handle *)h)->mmio_root == NULL);
  EXPECT_TRUE(mmio_ptr == NULL);
#endif

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_negative_map_mmio_02
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaMapMMIOPtr must fail for non-existent MMIO area,
 *             fpgaUnmapMMIOPtr must fail for non-existent MMIO area.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_negative_map_mmio_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifndef BUILD_ASE
  EXPECT_NE(FPGA_OK, fpgaMapMMIO(h, -1, &mmio_ptr));
#else
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, -1, &mmio_ptr));
#endif

  // Do not modify mmio_ptr and mmio_root
  EXPECT_TRUE(mmio_ptr == NULL);
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)h)->mmio_root));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaMapMMIO(NULL, 0, NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnmapMMIO(NULL, 0));
  EXPECT_NE(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_positive_write_read_01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaWriteMMIO64 must write correct value at given MMIO
 *             offset.  fpgaReadMMIO64 must write correct value at given
 *             MMIO offset.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_positive_write_read_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;
  uint64_t value = 0;
  uint64_t read_value = 0;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifdef BUILD_ASE
  ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_TRUE(mmio_ptr == NULL);
  mmio_ptr = 0;
#else
  EXPECT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);
#endif
  // Write value and check correctness
  for (value = 0; value < 100; value += 10) {
    EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0, value));
#ifndef BUILD_ASE
    EXPECT_EQ(
        value,
        *((volatile uint64_t*)(mmio_ptr + CSR_SCRATCHPAD0 / sizeof(uint64_t))));
#endif
    EXPECT_EQ(FPGA_OK, fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0, &read_value));
    EXPECT_EQ(read_value, value);
  }

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_negative_write_read_01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaWriteMMIO64 must fail for misaligned offset.
 *             fpgaReadMMIO64 must fail for misaligned offset.
 *             fpgaWriteMMIO64 must fail for out-of-region offset.
 *             fpgaReadMMIO64 must fail for out-of-region offset.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_negative_write_read_01) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;
  uint64_t value = 0;
  uint64_t read_value = 0;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifdef BUILD_ASE
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_EQ(mmio_ptr, (uint64_t *)NULL);
#else
  EXPECT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr, (uint64_t *)NULL);
#endif

  // Check errors for misalinged or out of boundary memory accesses
  EXPECT_NE(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0 + 1, value));
  EXPECT_NE(FPGA_OK, fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0 + 1, &read_value));
  EXPECT_NE(FPGA_OK, fpgaWriteMMIO64(h, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_NE(FPGA_OK,
            fpgaReadMMIO64(h, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReadMMIO64(NULL, 0, CSR_SCRATCHPAD0, &read_value));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaWriteMMIO64(NULL, 0, CSR_SCRATCHPAD0, value));
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_positive_write32_read32_02
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaWriteMMIO32 must write correct value at given MMIO
 *             offset.  fpgaReadMMIO32 must write correct value at given
 *             MMIO offset.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_positive_write32_read32_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;
  uint32_t value = 0;
  uint32_t read_value = 0;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifdef BUILD_ASE
  ASSERT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_TRUE(mmio_ptr == NULL);
#else
  ASSERT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);
#endif

  // Write value and check correctness
  for (value = 0; value < 100; value += 10) {
    EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD0, value));
    EXPECT_EQ(FPGA_OK, fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD0, &read_value));
    EXPECT_EQ(read_value, value);
  }

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}

/**
 * @test       mmio_drv_negative_write32_read32_02
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaWriteMMIO32 must fail for misaligned offset.
 *             fpgaReadMMIO32 must fail for misaligned offset.
 *             fpgaWriteMMIO32 must fail for out-of-region offset.
 *             fpgaReadMMIO32 must fail for out-of-region offset.
 *
 */
TEST(LibopaecMmioCommonALL, mmio_drv_negative_write32_read32_02) {
  struct _fpga_token _tok;
  fpga_token tok = &_tok;
  fpga_handle h = NULL;
  uint64_t* mmio_ptr = NULL;
  uint32_t value = 0;
  uint32_t read_value = 0;

  // Open  port device
  token_for_afu0(&_tok);
  ASSERT_EQ(FPGA_OK, fpgaOpen(tok, &h, 0));
#ifdef BUILD_ASE
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_TRUE(mmio_ptr == NULL);
#else
  EXPECT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);
#endif

  // Check errors for misaligned or out of boundary memory accesses
  EXPECT_NE(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD0 + 1, value));
  EXPECT_NE(FPGA_OK, fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD0 + 1, &read_value));
  EXPECT_NE(FPGA_OK, fpgaWriteMMIO32(h, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_NE(FPGA_OK,
            fpgaReadMMIO32(h, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReadMMIO32(NULL, 0, CSR_SCRATCHPAD0, &read_value));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaWriteMMIO32(NULL, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
#endif
  ASSERT_EQ(FPGA_OK, fpgaClose(h));
}
