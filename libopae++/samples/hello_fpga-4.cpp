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
#include "opae/cxx/accelerator.h"
#include "opae/cxx/buffers.h"
#include <uuid/uuid.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

using namespace opae::fpga::resources;
using namespace opae::fpga::application;

static const char* NLB0_AFUID = "D8424DC4-A4A3-C413-F89E-433683F9040B";
static const uint64_t CL = 64;
static const uint64_t KB = 1024;
static const uint64_t MB = KB * 1024;
static const uint64_t GB = MB * 1024;
static const uint64_t LOG2_CL = 6;
static const size_t LPBK1_DSM_SIZE = 4 * KB;
static const size_t LPBK1_BUFFER_SIZE = 1 * KB;
static const size_t LPBK1_BUFFER_ALLOCATION_SIZE = 4 * KB;
static const uint32_t DSM_STATUS_TEST_COMPLETE = 0x40;

static inline uint64_t cacheline_aligned_addr(uint64_t num) {
  return num >> LOG2_CL;
}


struct nlb0 : public component
{
  typedef std::shared_ptr<nlb0> ptr_t;

  nlb0(accelerator::ptr_t accel)
    : component(accel)
    , dsm_addr(this)
    , src_addr(this)
    , dst_addr(this) 
    , num_lines(this)
    , ctl(this)
    , cfg(this)
    , dsm(this)
    , inp(this)
    , out(this)
  {

  }

  virtual ~nlb0() {}

  csr64<0x0110> dsm_addr;
  csr64<0x0120> src_addr;
  csr64<0x0128> dst_addr;
  csr32<0x0130> num_lines;
  csr32<0x0138> ctl;
  csr64<0x0140> cfg;
  shared_buffer<LPBK1_BUFFER_SIZE>  dsm;
  shared_buffer<LPBK1_BUFFER_ALLOCATION_SIZE>  inp;
  shared_buffer<LPBK1_BUFFER_ALLOCATION_SIZE>  out;


private:

};

int main(int argc, char* argv[]) {
  // look for accelerator with NLB0_AFUID
  properties filter;
  filter.guid.parse(NLB0_AFUID);
  // Call the accelerator::run static function with the following args:
  // 1. The filter used to locate the accelerator
  // 2. The flags used when opening the accelerator
  // 3. The size of the initial buffer - more can be requested from accel
  // 4. The callback called once an accelerator is found and opened
  //    This callback takes as arguments the accelerator and the buffer allocated
  return accelerator::run<nlb0>(filter,
    FPGA_OPEN_SHARED,
    1 * GB,
    [](nlb0::ptr_t nlb)

    // initialize buffers
    std::fill_n(nlb->dsm->get(), LPBK1_DSM_SIZE, 0);
    std::fill_n(nlb->inp->get(), LPBK1_BUFFER_SIZE, 0xAF);
    std::fill_n(nlb->out->get(), LPBK1_BUFFER_SIZE, 0xBE);

    nlb->dsm_base = nlb->dsm->iova();
    nlb->ctl = 0;
    nlb->ctl = 1;
    nlb->src_addr = cacheline_aligned_addr(nlb->inp->iova());
    nlb->dst_addr = cacheline_aligned_addr(nlb->out->iova());
    nlb->num_lines = LPBK1_BUFFER_SIZE / (1 * CL);
    nlb->cfg = 0x42000;

    // start the test
    nlb->ctl = 3;

    // wait for test completion
    opae::fpga::memory::wait(
      nlb->dsm,                 // shared buffer
      DSM_STATUS_TEST_COMPLETE, // ofset
      100,                      // interval time
      10000,                    // timeout
      0x1,                      // mask to apply to memory
      0x1                       // expected value
    );

    // stop the device
    nlb->ctl = 7;

    // check output buffer contents
    auto mm = std::mismatch(nlb->inp->get(), nlb->inp->get() + LPBK1_BUFFER_SIZE, nlb->out->get());
    if (mm.second < nlb->out->get() + LPBK1_BUFFER_SIZE) {
      std::cerr << "output does NOT match input at offset: " << (mm.second - nlb->out->get()) << "\n";
      return -1;
    }

    return 0;
  });

}
