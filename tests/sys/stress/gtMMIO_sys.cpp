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

#include <opae/fpga.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "common_utils.h"
#include "gtest/gtest.h"
#include "common_sys.h"

using namespace common_utils;
using namespace std;

#define CSR_SCRATCHPAD0 0x100  // 32b
#define CSR_SCRATCHPAD1 0x104  // 32b
#define CSR_SCRATCHPAD2 0x108  // 64b
#define MAX_MMIO_SIZE 1024 * 256
#define MMIO_TEST_OFFSET 0x18  // skip first 24 bytes of DFH heder
#define MMIO_OUT_REGION_ADDRESS 1024 * 1024 * 256

class StressLibopaecMmioFCommonHW : public BaseFixture, public ::testing::Test {
 public:
  uint64_t* mmio_ptr = NULL;
  uint32_t value32 = 0x12345678;
  uint64_t value64 = 0x1122334455667788;
  uint32_t read_value32 = 0;
  uint64_t read_value64 = 0;
  uint32_t offset32 = 0x40;
  uint64_t offset64 = 0x40;

 public:
  void MMIOReadWrite32(fpga_handle h) {
    offset32 = 0x40;
    value32 = 0x12345678;
    // Write value and verify
    while (offset32 < MAX_MMIO_SIZE) {
      read_value32 = 0;
      EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, offset32, value32));
      EXPECT_EQ(FPGA_OK, fpgaReadMMIO32(h, 0, offset32, &read_value32));
      // printf("%llx  %llx \n", offset32, read_value32);
      EXPECT_EQ(read_value32, value32);
      offset32 = offset32 + 4;
      value32 = value32 + 10;
    }
  }

  void MMIOReadWrite64(fpga_handle h) {
    uint64_t value64 = 0x1122334455667788;
    uint64_t offset64 = 0x40;
    // Write value and verify
    while (offset64 < MAX_MMIO_SIZE) {
      read_value64 = 0;
      EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, offset64, value64));
      EXPECT_EQ(FPGA_OK, fpgaReadMMIO64(h, 0, offset64, &read_value64));
      // printf("%llx  %llx \n", offset64, read_value64);
      EXPECT_EQ(read_value64, value64);
      offset64 = offset64 + 8;
      value64 = value64 + 10;
    }
  }
};

/**
 * @test       07
 *
 * @brief      Using MMIO bitstream, repeatly open, allocate MMIO
 *             region, Read/Write MMIO region, unmap, and close the
 *             device 2000 times with gtest repeat set to 1000 times.
 */

TEST_F(StressLibopaecMmioFCommonHW, 07) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    ASSERT_TRUE(checkReturnCodes(
        loadBitstream(config_map[BITSTREAM_MMIO], tokens[index]),
        LINE(__LINE__)));

    for (uint32_t i = 0; i < 2000; i++) {
      if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
        ASSERT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
        EXPECT_FALSE(mmio_ptr == NULL);

        // Write value and verify
        while (offset32 < MAX_MMIO_SIZE) {
          EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, offset32, value32));
          EXPECT_EQ(FPGA_OK, fpgaReadMMIO32(h, 0, offset32, &read_value32));
          // printf("%llx  %llx \n", offset32, read_value32);
          EXPECT_EQ(read_value32, value32);
          offset32 = offset32 + 4;
          value32 = value32 + 10;
        }

        // printf("%d complete32 \n", i);
        while (offset64 < MAX_MMIO_SIZE) {
          EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, offset64, value64));
          EXPECT_EQ(FPGA_OK, fpgaReadMMIO64(h, 0, offset64, &read_value64));
          // printf("%llx  %llx \n", offset64, read_value64);
          EXPECT_EQ(read_value64, value64);
          offset64 = offset64 + 8;
          value64 = value64 + 10;
        }

        // printf("%d complete64 \n", i);

        EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
        ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
      } else {
        cout << "open failed\n";
        FAIL();
      }
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       04
 *
 * @brief      Use NLB0 AFU, repeat MMIO access with 3 different scratch
 *             pad values 2000 times.
 */

TEST_F(StressLibopaecMmioFCommonHW, 04) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      ASSERT_EQ(FPGA_OK, fpgaMapMMIO(h, 0, &mmio_ptr));
      EXPECT_FALSE(mmio_ptr == NULL);

      for (uint32_t i = 0; i < 2000; i++) {
        // Write value and check correctness
        for (uint64_t value64 = 0; value64 < 100; value64 += 10) {
          EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD0, value64));
          EXPECT_EQ(value64, *((volatile uint64_t*)(mmio_ptr +
                                                    CSR_SCRATCHPAD0 /
                                                        sizeof(uint64_t))));
          EXPECT_EQ(FPGA_OK,
                    fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD0, &read_value64));
          EXPECT_EQ(read_value64, value64);

          value32 = (uint32_t)value64;
          EXPECT_EQ(FPGA_OK, fpgaWriteMMIO32(h, 0, CSR_SCRATCHPAD1, value32));
          EXPECT_EQ(FPGA_OK,
                    fpgaReadMMIO32(h, 0, CSR_SCRATCHPAD1, &read_value32));
          EXPECT_EQ(read_value32, value32);

          EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(h, 0, CSR_SCRATCHPAD2, value64));
          EXPECT_EQ(value64, *((volatile uint64_t*)(mmio_ptr +
                                                    CSR_SCRATCHPAD2 /
                                                        sizeof(uint64_t))));
          EXPECT_EQ(FPGA_OK,
                    fpgaReadMMIO64(h, 0, CSR_SCRATCHPAD2, &read_value64));
          EXPECT_EQ(read_value64, value64);
        }
        // printf("%d\n", i);
      }

      EXPECT_EQ(FPGA_OK, fpgaUnmapMMIO(h, 0));
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}
