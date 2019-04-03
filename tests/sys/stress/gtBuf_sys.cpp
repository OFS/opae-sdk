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
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>
#include <algorithm>
#include <deque>

#include "common_utils.h"
#include "gtest/gtest.h"

using namespace common_utils;
using namespace std;

class StressLibopaecBufFCommonHW : public BaseFixture, public ::testing::Test {
 public:
  static const uint64_t len_min = 1024;
  static const uint64_t len_max = 4 * 1024;
  static const uint64_t len_2mb = 2 * 1024 * 1024;
  static const uint64_t len_1gb = 960 * 1024 * 1024;
  static const uint64_t accum_max = 2 * 1024UL * 1024UL * 1024UL;
};

/**
 * @test       05
 *
 * @brief      Repeat 100 times: Multiple buffer allocations with random
 *             sizes until total size >= 2GB. Two big allocations use
 *             1GB hugepages. 20 allocations use 2MB hugepages. Other
 *             allocations use regular pages. Free all the allocated
 *             buffers in random order.
 *
 * @details    Set-up instructions: 
 *
 *             1. Enable two 1GB hugepage with this
 *             command: sudo sh -c
 *             "echo 2 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages"
 *
 *             2. Verify the setting is successful: cat
 *             /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
 *             (The output should be 2)
 *
 *             3. Enable 20 2MB hugepage with this command: sudo sh -c
 *             "echo 20 > "/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"
 *
 *             4. Verify the setting is successful: cat
 *             /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
 *             (The output should be 20)
 *
 *             5. Make sure no other instance of libfpga app is running
 *             on the system.
 */

TEST_F(StressLibopaecBufFCommonHW, 05) {
  auto functor = [=]() -> void {

    // this handle was originally in a fixture
    fpga_handle h = NULL;
    // this deque was originally in a fixture
    std::deque<uint64_t> wsdeq;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(len_min, len_max);

    EXPECT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));

    // Repeat 100 times
    for (int i = 0; i < 100; ++i) {
      uint64_t* buf_addr = NULL;
      uint64_t wsid = 1;
      uint64_t len_accum = 0;

      dis.reset();

      // Allocate two buffers backed by 1GB page
      for (int j = 0; j < 2; ++j) {
        ASSERT_EQ(FPGA_OK,
                  fpgaPrepareBuffer(h, len_1gb, (void**)&buf_addr, &wsid, 0));
        wsdeq.push_back(wsid);
        len_accum += len_1gb;
      }

      // Allocate 20 buffers each backed by 2MB page
      for (int j = 0; j < 20; ++j) {
        ASSERT_EQ(FPGA_OK,
                  fpgaPrepareBuffer(h, len_2mb, (void**)&buf_addr, &wsid, 0));
        wsdeq.push_back(wsid);
        len_accum += len_2mb;
      }

      // Allocate buffers of random sizes backed by regular pages
      while (len_accum < accum_max) {
        uint64_t buf_len = dis(gen);
        len_accum += buf_len;
        ASSERT_EQ(FPGA_OK,
                  fpgaPrepareBuffer(h, buf_len, (void**)&buf_addr, &wsid, 0));
        wsdeq.push_back(wsid);
      }

      // Random shuffle list of buffers
      shuffle(wsdeq.begin(), wsdeq.end(), gen);

      // Walk the list to get physical IO address for each buffer, and then free
      // the buffer
      while (!wsdeq.empty()) {
        uint64_t ioaddr;

        wsid = wsdeq.front();
        wsdeq.pop_front();
        ASSERT_EQ(FPGA_OK, fpgaGetIOAddress(h, wsid, &ioaddr));
        ASSERT_EQ(FPGA_OK, fpgaReleaseBuffer(h, wsid));
      }
    }
    // this for loop was originally in a fixture TearDown()
    for (auto ws : wsdeq) {
      ASSERT_EQ(FPGA_OK, fpgaReleaseBuffer(h, ws));
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}
