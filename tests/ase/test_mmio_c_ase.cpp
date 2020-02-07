// Copyright(c) 2019, Intel Corporation
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

extern "C" {
#include <json-c/json.h>
#include <uuid/uuid.h>
#include <opae/types.h>
#include "types_int.h"

}

#include "gtest/gtest.h"
#include "ase.h"

#define MMIO_AFU_OFFSET            (256*1024)
#define FPGA_HANDLE_MAGIC   0x46504741484e444c
extern uint64_t *mmio_afu_vbase;

/**
 * @test       nullhandle
 *
 * @brief      When the fpga_handle parameter is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST(mmio_c_ase, nullhandle) {
  uint32_t value = 0;
  uint64_t value2 = 0;
  uint64_t value512[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t *addr = nullptr;
  EXPECT_EQ(ase_fpgaWriteMMIO32(nullptr, 1, 0x4, 0xff),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaReadMMIO32(nullptr, 1, 0x4, &value),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaWriteMMIO64(nullptr, 1, 0x4, 0xff),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaReadMMIO64(nullptr, 1, 0x4, &value2),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaWriteMMIO512(nullptr, 1, 0x40, value512),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaMapMMIO(nullptr, 1, &addr),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaUnmapMMIO(nullptr, 1),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaReset(nullptr),
            FPGA_INVALID_PARAM);
}

/**
 * @test       mmio_afu_vbase_null
 *
 * @brief      If mmio_afu_vbase is nullptr, this test returns FFPGA_NOT_FOUND.
 */
TEST(mmio_c_ase, mmio_afu_vbase_null) {
  uint32_t value = 0;
  uint64_t value2 = 0;
  uint64_t value512[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  struct _fpga_handle handle_;
  fpga_handle accel_ = &handle_;
  
  handle_.fpgaMMIO_is_mapped = 0;
  mmio_afu_vbase = nullptr;
  EXPECT_EQ(ase_fpgaWriteMMIO32(accel_, 1, 0x4, 0xff),
            FPGA_NOT_FOUND);
  EXPECT_EQ(ase_fpgaReadMMIO32(accel_, 1, 0x4, &value),
            FPGA_NOT_FOUND);
  EXPECT_EQ(ase_fpgaWriteMMIO64(accel_, 1, 0x4, 0xff),
            FPGA_NOT_FOUND);
  EXPECT_EQ(ase_fpgaReadMMIO64(accel_, 1, 0x4, &value2),
            FPGA_NOT_FOUND);
  EXPECT_EQ(ase_fpgaWriteMMIO512(accel_, 1, 0x40, value512),
            FPGA_NOT_FOUND);
}

/**
 * @test       offset_misaligned 
 *
 * @brief      If the offset to MMIO functions is not 4 or 8 bytes aligned
 *             it will return FPGA_INVALID_PARAM
 */
TEST(mmio_c_ase, offset_misaligned) {
  uint32_t value = 0;
  uint64_t value2 = 0;
  uint64_t value512[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t mmio_data[16];

  struct _fpga_handle handle_;
  fpga_handle accel_ = &handle_;
  
  handle_.fpgaMMIO_is_mapped = 0;
  mmio_afu_vbase = mmio_data;
  EXPECT_EQ(ase_fpgaWriteMMIO32(accel_, 1, 0x3, 0xff),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaReadMMIO32(accel_, 1, 0x3, &value),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaWriteMMIO64(accel_, 1, 0x3, 0xff),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaReadMMIO64(accel_, 1, 0x3, &value2),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaWriteMMIO512(accel_, 1, 0x3, value512),
            FPGA_INVALID_PARAM);
}

/**
 * @test       offset_overflow 
 *
 * @brief      If the offset to MMIO functions is bigger than the MMIO_AFU_OFFSET
 *             it will return FPGA_INVALID_PARAM
 */
TEST(mmio_c_ase, offset_overflow) {
  uint32_t value = 0;
  uint64_t value2 = 0;
  uint64_t value512[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t mmio_data[16];

  struct _fpga_handle handle_;
  fpga_handle accel_ = &handle_;
  
  handle_.fpgaMMIO_is_mapped = 0;
  mmio_afu_vbase = mmio_data;
  EXPECT_EQ(ase_fpgaWriteMMIO32(accel_, 1, MMIO_AFU_OFFSET + 4, 0xff),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaReadMMIO32(accel_, 1, MMIO_AFU_OFFSET + 4, &value),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaWriteMMIO64(accel_, 1, MMIO_AFU_OFFSET + 4, 0xff),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaReadMMIO64(accel_, 1, MMIO_AFU_OFFSET + 4, &value2),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(ase_fpgaWriteMMIO512(accel_, 1, MMIO_AFU_OFFSET + 64, value512),
            FPGA_INVALID_PARAM);
}

/**
 * @test       mmio_nullptr
 *
 * @brief      If mmio_ptr parameter to fpgaMapMMIO() is nullptr, this test 
 *             returns FFPGA_OK; if a non-null pointer is passed, fpgaMapMMIO
 *             returns FPGA_NOT_SUPPORTED.
 */
TEST(mmio_c_ase, mmio_nullptr) {
  uint64_t value = 0;
  uint64_t *addr = &value;
  
  struct _fpga_handle handle_;
  fpga_handle accel_ = &handle_;
  
  handle_.fpgaMMIO_is_mapped = 0;
  EXPECT_EQ(ase_fpgaMapMMIO(accel_, 1, &addr), FPGA_NOT_SUPPORTED);
  EXPECT_EQ(ase_fpgaMapMMIO(accel_, 1, nullptr), FPGA_OK);
}

/**
 * @test       invalid_handle
 *
 * @brief      If fpga_hanle's magic is invalid, fpgaUnmapMMIO returns FPGA_INVALID_PARAM
 *             If fpga_hanle's magic is valid, fpgaUnmapMMIO returns FPGA_OK.
 *             If the fpgaMMIO_is_mapped is false, fpgaUnmapMMIO returns FPGA_INVALID_PARAM.
 */
TEST(mmio_c_ase, invalid_handle) {
  struct _fpga_handle handle_;
  handle_.magic = 0xFFFFFFFF;
  fpga_handle accel_ = &handle_;  

  EXPECT_EQ(ase_fpgaUnmapMMIO(accel_, 1), FPGA_INVALID_PARAM);

  handle_.magic = FPGA_HANDLE_MAGIC;
  handle_.fpgaMMIO_is_mapped = true;
  EXPECT_EQ(ase_fpgaUnmapMMIO(accel_, 1), FPGA_OK);
  
  handle_.fpgaMMIO_is_mapped = false;
  EXPECT_EQ(ase_fpgaUnmapMMIO(accel_, 1), FPGA_INVALID_PARAM);
}

/**
 * @test       reset
 *
 * @brief      If fpga_hanle is nullptr, fpgaReset returns FPGA_INVALID_PARAM
 *             If fpga_hanle's magic is invalid, fpgaReset returns FPGA_INVALID_PARAM.
 */
TEST(mmio_c_ase, reset) {
  struct _fpga_handle handle_;
  handle_.magic = 0xFFFFFFFF;
  fpga_handle accel_ = &handle_;  

  EXPECT_EQ(ase_fpgaReset(nullptr), FPGA_INVALID_PARAM);

  EXPECT_EQ(ase_fpgaReset(accel_), FPGA_INVALID_PARAM);
}

