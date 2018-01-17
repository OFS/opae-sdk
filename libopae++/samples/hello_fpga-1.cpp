// Copyright(c) 2017, Intel Corporation
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
#include "opae/cxx/properties.h"
#include "opae/cxx/token.h"
#include "opae/cxx/handle.h"
#include "opae/cxx/dma_buffer.h"
#include <uuid/uuid.h>
#include <algorithm>
#include <iostream>
#include <chrono>

using namespace opae::fpga::types;

static const char* NLB0_AFUID                      = "D8424DC4-A4A3-C413-F89E-433683F9040B";
static const uint64_t CL                           = 64;
static const uint64_t KB                           = 1024;
static const uint64_t MB                           = KB * 1024;
static const uint64_t LOG2_CL                      = 6;
static const size_t LPBK1_DSM_SIZE                 = 4*KB;
static const size_t LPBK1_BUFFER_SIZE              = 1*KB;
static const size_t LPBK1_BUFFER_ALLOCATION_SIZE   = 4*KB;
static const uint64_t CSR_SRC_ADDR                 = 0x0120;
static const uint32_t CSR_DST_ADDR                 = 0x0128;
static const uint32_t CSR_CTL                      = 0x0138;
static const uint32_t CSR_CFG                      = 0x0140;
static const uint32_t CSR_NUM_LINES                = 0x0130;
static const uint32_t DSM_STATUS_TEST_COMPLETE     = 0x40;
static const uint64_t CSR_AFU_DSM_BASEL            = 0x0110;
static const uint64_t CSR_AFU_DSM_BASEH            = 0x0114;

static inline uint64_t cacheline_aligned_addr(uint64_t num) {
  return num >> LOG2_CL;
}

int main(int argc, char* argv[]) {
  // look for accelerator with NLB0_AFUID
  properties filter;
  filter.guid.parse(NLB0_AFUID);
  filter.type = FPGA_ACCELERATOR;

  auto tokens = token::enumerate({ filter });
  
  // assert we have found at least one
  if (tokens.size() < 1) {
  	std::cerr << "accelerator not found\n";
  	return -1;
  }
  token::ptr_t tok = tokens[0];

  // open accelerator and map MMIO
  auto accel = handle::open(tok, FPGA_OPEN_SHARED);

  // allocate buffers
  auto dsm = dma_buffer::allocate(accel, LPBK1_DSM_SIZE);
  auto inp = dma_buffer::allocate(accel, LPBK1_BUFFER_ALLOCATION_SIZE);
  auto out = dma_buffer::allocate(accel, LPBK1_BUFFER_ALLOCATION_SIZE);

  // initialize buffers
  std::fill_n(dsm->get(), LPBK1_DSM_SIZE, 0);
  std::fill_n(inp->get(), LPBK1_BUFFER_SIZE, 0xAF);
  std::fill_n(out->get(), LPBK1_BUFFER_SIZE, 0xBE);

  //accel->reset();
  accel->write(CSR_AFU_DSM_BASEL, dsm->iova());
  accel->write(CSR_CTL, static_cast<uint32_t>(0));
  accel->write(CSR_CTL, static_cast<uint32_t>(1));
  accel->write(CSR_SRC_ADDR, cacheline_aligned_addr(inp->iova()));
  accel->write(CSR_DST_ADDR, cacheline_aligned_addr(out->iova()));

  accel->write(CSR_NUM_LINES, LPBK1_BUFFER_SIZE / 1*CL);
  accel->write(CSR_CFG, static_cast<uint32_t>(0x42000));

  // get ptr to device status memory - test complete
  auto status_ptr = dsm->get() + DSM_STATUS_TEST_COMPLETE / 8;

  // start the test
  accel->write(CSR_CTL, static_cast<uint32_t>(3));

  // wait for test completion
  while (0 == ((*status_ptr) * 0x1)) {
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  // stop the device
  accel->write(CSR_CTL, static_cast<uint32_t>(7));

  // check output buffer contents
  auto mm = std::mismatch(inp->get(), inp->get() + LPBK1_BUFFER_SIZE, out->get());
  if (mm.second < out->get() + LPBK1_BUFFER_SIZE) {
    std::cerr << "output does NOT match input at offset: " << (mm.second - out->get()) << "\n";
    return -1;
  }

  return 0;
}
