// Copyright(c) 2018-2019, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>

#include <uuid/uuid.h>

#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>

using namespace opae::fpga::types;

static const char* NLB0_AFUID = "D8424DC4-A4A3-C413-F89E-433683F9040B";
static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t LOG2_CL = 6;
static const size_t LPBK1_DSM_SIZE = 2 * MB;
static const size_t LPBK1_BUFFER_SIZE = 1 * MB;
static const size_t LPBK1_BUFFER_ALLOCATION_SIZE = 2 * MB;
static const uint64_t CSR_SRC_ADDR = 0x0120;
static const uint32_t CSR_DST_ADDR = 0x0128;
static const uint32_t CSR_CTL = 0x0138;
static const uint32_t CSR_CFG = 0x0140;
static const uint32_t CSR_NUM_LINES = 0x0130;
static const uint32_t DSM_STATUS_TEST_COMPLETE = 0x40;
static const uint64_t CSR_AFU_DSM_BASEL = 0x0110;
static const uint64_t CSR_AFU_DSM_BASEH = 0x0114;

static inline uint64_t cacheline_aligned_addr(uint64_t num) {
  return num >> LOG2_CL;
}

int main(int argc, char *argv[]) {
  if ((argc > 1) && ((std::string(argv[1]) == std::string("-v")) ||
                     (std::string(argv[1]) == std::string("--version")))) {
    std::cout << "hello_cxxcore " << INTEL_FPGA_API_VERSION << " "
              << INTEL_FPGA_API_HASH;
    if (INTEL_FPGA_TREE_DIRTY) std::cout << "*";
    std::cout << std::endl;
    return 0;
  }

  std::cout << "Using OPAE C++ Core library version '" << version::as_string()
            << "' build '" << version::build() << "'\n";
  // look for accelerator with NLB0_AFUID
  properties::ptr_t filter = properties::get();
  filter->guid.parse(NLB0_AFUID);
  filter->type = FPGA_ACCELERATOR;

  std::vector<token::ptr_t> tokens = token::enumerate({filter});

  // assert we have found at least one
  if (tokens.size() < 1) {
    std::cerr << "accelerator not found\n";
    return -1;
  }
  token::ptr_t tok = tokens[0];

  // open accelerator and map MMIO
  handle::ptr_t accel = handle::open(tok, FPGA_OPEN_SHARED);

  // allocate buffers
  shared_buffer::ptr_t dsm = shared_buffer::allocate(accel, LPBK1_DSM_SIZE);
  shared_buffer::ptr_t inp =
      shared_buffer::allocate(accel, LPBK1_BUFFER_ALLOCATION_SIZE);
  shared_buffer::ptr_t out =
      shared_buffer::allocate(accel, LPBK1_BUFFER_ALLOCATION_SIZE);

  std::cout << "Running Test\n";

  // initialize buffers
  std::fill_n(dsm->c_type(), LPBK1_DSM_SIZE, 0);
  std::fill_n(inp->c_type(), LPBK1_BUFFER_SIZE, 0xAF);
  std::fill_n(out->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

  accel->reset();
  accel->write_csr64(CSR_AFU_DSM_BASEL, dsm->io_address());
  accel->write_csr32(CSR_CTL, 0);
  accel->write_csr32(CSR_CTL, 1);
  accel->write_csr64(CSR_SRC_ADDR, cacheline_aligned_addr(inp->io_address()));
  accel->write_csr64(CSR_DST_ADDR, cacheline_aligned_addr(out->io_address()));

  accel->write_csr32(CSR_NUM_LINES, LPBK1_BUFFER_SIZE / (1 * CL));
  accel->write_csr32(CSR_CFG, 0x42000);

  // get ptr to device status memory - test complete
  // temporarily "borrow" a raw pointer to the buffer
  // status_ptr can be dangling pointer if dsm is the only reference
  // and it is reset or goes out of scope before status_ptr
  volatile uint8_t* status_ptr = dsm->c_type() + DSM_STATUS_TEST_COMPLETE;
  // start the test
  accel->write_csr32(CSR_CTL, 3);

  // wait for test completion
  while (0 == ((*status_ptr) * 0x1)) {
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  // stop the device
  accel->write_csr32(CSR_CTL, 7);

  // check output buffer contents
  std::pair<volatile uint8_t*, volatile uint8_t*> mm = std::mismatch(
      inp->c_type(), inp->c_type() + LPBK1_BUFFER_SIZE, out->c_type());
  if (mm.second < out->c_type() + LPBK1_BUFFER_SIZE) {
    std::cerr << "output does NOT match input at offset: "
              << (mm.second - out->c_type()) << "\n";
    return -1;
  }

  std::cout << "Done Running Test\n";

  return 0;
}
