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
#include <opae/cxx/accelerator.h>

using std::vector;

using namespace opae::fpga::types;

namespace opae {
namespace fpga {
namespace resource {

accelerator::accelerator(token::ptr_t t)
    : token_(t), props_(properties::read(t)) {}

accelerator::list_t accelerator::enumerate(vector<properties> filter) {
  list_t accelerators;

  for (auto& f : filter) {
    f.type = FPGA_ACCELERATOR;
  }
  auto tokens = token::enumerate({filter});
  for (token::ptr_t t : tokens) {
    accelerators.push_back(accelerator::ptr_t(new accelerator(t)));
  }
  return accelerators;
}

accelerator::~accelerator() {}

void accelerator::open(int flags) { handle_ = handle::open(token_, flags); }

dma_buffer::ptr_t accelerator::allocate_buffer(size_t size) {
  if (handle_) {
    return dma_buffer::allocate(handle_, size);
  } else {
    throw except(FPGA_EXCEPTION, OPAECXX_HERE);
  }
}

void accelerator::reset() {
  if (handle_) {
    handle_->reset();
  } else {
    throw except(FPGA_EXCEPTION, OPAECXX_HERE);
  }
}

}  // end of namespace resource
}  // end of namespace fpga
}  // end of namespace opae
