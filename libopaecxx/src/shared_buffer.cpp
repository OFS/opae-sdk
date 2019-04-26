// Copyright(c) 2018, Intel Corporation
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
#include <algorithm>
#include <cstring>

#include <opae/cxx/core/shared_buffer.h>
#include <exception>

namespace opae {
namespace fpga {
namespace types {

shared_buffer::~shared_buffer() { release(); }

shared_buffer::ptr_t shared_buffer::allocate(handle::ptr_t handle, size_t len,
                                             bool read_only) {
  ptr_t p;

  if (!handle) {
    throw std::invalid_argument("handle object is null");
  }

  if (!len) {
    throw except(OPAECXX_HERE);
  }

  uint8_t *virt = nullptr;
  uint64_t io_address = 0;
  uint64_t wsid = 0;

  int flags = 0;
  if (read_only) {
      flags |= FPGA_BUF_READ_ONLY;
  }

  fpga_result res = fpgaPrepareBuffer(
      handle->c_type(), len, reinterpret_cast<void **>(&virt), &wsid, flags);
  ASSERT_FPGA_OK(res);
  res = fpgaGetIOAddress(handle->c_type(), wsid, &io_address);
  ASSERT_FPGA_OK(res);
  p.reset(new shared_buffer(handle, len, virt, wsid, io_address));

  return p;
}

shared_buffer::ptr_t shared_buffer::attach(handle::ptr_t handle, uint8_t *base,
                                           size_t len,
                                           bool read_only) {
  ptr_t p;

  uint8_t *virt = base;
  uint64_t io_address = 0;
  uint64_t wsid = 0;

  int flags = FPGA_BUF_PREALLOCATED;
  if (read_only) {
      flags |= FPGA_BUF_READ_ONLY;
  }

  fpga_result res =
      fpgaPrepareBuffer(handle->c_type(), len, reinterpret_cast<void **>(&virt),
                        &wsid, flags);

  ASSERT_FPGA_OK(res);
  res = fpgaGetIOAddress(handle->c_type(), wsid, &io_address);
  ASSERT_FPGA_OK(res);
  p.reset(new shared_buffer(handle, len, virt, wsid, io_address));

  return p;
}

void shared_buffer::release() {
  // If the allocation was successful.
  if (virt_ && handle_) {
    auto res = fpgaReleaseBuffer(handle_->c_type(), wsid_);
    if (res == FPGA_OK) {
      virt_ = nullptr;
      len_ = 0;
      wsid_ = 0;
      io_address_ = 0;
    } else {
      std::cerr << "Error while calling fpgaReleaseBuffer: " << fpgaErrStr(res)
                << "\n";
    }
  }
}

void shared_buffer::fill(int c) { std::fill(virt_, virt_ + len_, c); }

int shared_buffer::compare(shared_buffer::ptr_t other, size_t len) const {
  return std::equal(virt_, virt_ + len, other->virt_) ? 0 : 1;
}

shared_buffer::shared_buffer(handle::ptr_t handle, size_t len, uint8_t *virt,
                             uint64_t wsid, uint64_t io_address)
    : handle_(handle),
      len_(len),
      virt_(virt),
      wsid_(wsid),
      io_address_(io_address) {}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
