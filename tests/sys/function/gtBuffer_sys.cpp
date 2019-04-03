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
#include <opae/buffer.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>
#include <algorithm>
#include <deque>
#include <random>

#include "gtest/gtest.h"
#include "common_sys.h"
#include "common_utils.h"

using namespace std;
using namespace common_utils;

class LibopaecBufFCommonHW : public BaseFixture, public ::testing::Test {
 public:
  static const uint64_t len_min = 1024;
  static const uint64_t len_max = 4 * 1024;
  static const uint64_t len_2mb = 2 * 1024 * 1024;
  static const uint64_t len_1gb = 960 * 1024 * 1024;
  static const uint64_t accum_max = 2 * 1024UL * 1024UL * 1024UL;
};

/**
 * @test       LibopaecBufFCommonHW02
 *
 * @brief      fpgaPrepareBuffer must return the requested amount of
 *             memory when memory is available. It must return false
 *             when no memory is available when the request is over the
 *             limit (the single 1GB page was consumed by previous
 *             allocation).
 *
 * @details    Set-up instructions:
 *
 *             1. Enable a single 1GB hugepage with this command: sudo
 *             sh -c
 *             "echo 1 >
 * /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages"
 *
 *             2. Verify the setting is successful: cat
 *             /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
 *             (The output should be 1)
 *
 *             3. Make sure no other instance of libfpga app is running
 *             on the system.
 */

TEST_F(LibopaecBufFCommonHW, 02) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    uint64_t buf_len;
    uint64_t *buf_addr1, *buf_addr2;
    uint64_t wsid1 = 1, wsid2 = 2;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      // Allocate buffer of 8MB
      buf_len = 8 * 1024 * 1024;
      ASSERT_EQ(FPGA_OK,
                fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr1, &wsid1, 0));

      // Allocate another buffer of 8MB
      ASSERT_EQ(FPGA_NO_MEMORY,
                fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr2, &wsid2, 0));

      // Release buffer
      EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid1));
      ASSERT_EQ(FPGA_OK, fpgaClose(h));
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR, true, functor);
}

/**
 * @test       LibopaecBufFCommonHW04
 *
 * @brief      Verify that the IOVA operation returns the correct value
 *             for known buffers. E.g. allocate a buffer B of length N
 *             mapped at user virtual address of V and given IOVA(V) is
 *             P. Get various IOVA values and ensure that they are what
 *             is expected. Specifically: P=IOVA(V), then P+1=IOVA(V+1);
 *             P+L-1=IOVA(V+L-1); 0=IOVA(V+L) [indicating failure]
 *
 * @details    Set-up instructions:
 *
 *             1. Enable a single 1GB hugepage with
 *             this command: sudo sh -c
 *             "echo 1 >
 * /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages"
 *
 *
 *             2. Verify the setting is successful: cat
 *             /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
 *             (The output should be 1)
 *
 *
 *             3. Make sure no other instance of libfpga app is running
 *             on the system.
 */

TEST_F(LibopaecBufFCommonHW, 04) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    uint64_t* buf_addr;
    uint64_t wsid = 1;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      // Allocate buffer of 8MB
      uint64_t offset;
      uint64_t buf_len = 4 * 1024 * 1024;
      ASSERT_EQ(FPGA_OK,
                fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

      // file buffer with data
      memset(buf_addr, 0, buf_len);

      for (offset = 0; offset < buf_len - sizeof(uint64_t);
           offset += sizeof(uint64_t)) {
        uint64_t value = offset;
        *((volatile uint64_t*)((uint64_t)buf_addr + offset)) = value;
      }

      // Release buffer
      EXPECT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
      ASSERT_EQ(FPGA_OK, fpgaClose(h));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR, true, functor);
}

TEST_F(LibopaecBufFCommonHW, 07) {
  /**
   * @test       LibopaecBufFCommonHW07
   *
   * @brief      Configure kernel to reserve 1GB from OS and ensure
   *             allocation of 1.5 GB attempts to access fail.  Attempt
   *             to allocate 1.5 GB buffer, expect 'no buffer
   *             allocated'message.
   */

  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    uint64_t buf_len;
    uint64_t* buf_addr;
    uint64_t wsid = 1;

    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));

    // Allocate buffer in MB range
    buf_len = 1.5 * 1024 * 1024 * 1024;
    EXPECT_EQ(FPGA_NO_MEMORY,
              fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

    // Release buffer in MB range
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(h, wsid));
    ASSERT_EQ(FPGA_OK, fpgaClose(h));
  };

  TestAllFPGA(FPGA_ACCELERATOR, true, functor);
}

TEST_F(LibopaecBufFCommonHW, 08) {
  /**
   * @test       LibopaecBufFCommonHW08
   *
   * @brief      Negative test:  proper failure of 1 gig kernel
   *             allocation (DMA memory) based on smaller reserved
   *             physical memory (20 2M pages)
   */

  auto functor = [=]() -> void {

    fpga_handle h = NULL;

    uint64_t buf_len;
    uint64_t* buf_addr;
    uint64_t wsid = 1;

    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));

    // Allocate buffer in MB range
    buf_len = 1 * 1024 * 1024 * 1024;
    EXPECT_EQ(FPGA_NO_MEMORY,
              fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));

    // Release buffer in MB range
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(h, wsid));
    ASSERT_EQ(FPGA_OK, fpgaClose(h));
  };

  TestAllFPGA(FPGA_ACCELERATOR, true, functor);
}

