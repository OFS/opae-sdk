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
#include <cstring>

#include <opae/cxx/core/dma_buffer.h>

namespace opae {
namespace fpga {
namespace types {

dma_buffer::~dma_buffer() {
  // If the allocation was successful.
  if (virt_) {
    ASSERT_FPGA_OK(fpgaReleaseBuffer(handle_->get(), wsid_));
  }
}

dma_buffer::ptr_t dma_buffer::allocate(handle::ptr_t handle, size_t len) {
  ptr_t p;

  if (!len) {
    throw except(OPAECXX_HERE);
  }

  uint8_t *virt = nullptr;
  uint64_t iova = 0;
  uint64_t wsid = 0;

  fpga_result res = fpgaPrepareBuffer(
      handle->get(), len, reinterpret_cast<void **>(&virt), &wsid, 0);
  ASSERT_FPGA_OK(res);
  res = fpgaGetIOAddress(handle->get(), wsid, &iova);
  ASSERT_FPGA_OK(res);
  p.reset(new dma_buffer(handle, len, virt, wsid, iova));

  return p;
}

dma_buffer::ptr_t dma_buffer::attach(handle::ptr_t handle, uint8_t *base,
                                     size_t len) {
  ptr_t p;

  uint8_t *virt = base;
  uint64_t iova = 0;
  uint64_t wsid = 0;

  fpga_result res =
      fpgaPrepareBuffer(handle->get(), len, reinterpret_cast<void **>(&virt),
                        &wsid, FPGA_BUF_PREALLOCATED);

  ASSERT_FPGA_OK(res);
  res = fpgaGetIOAddress(handle->get(), wsid, &iova);
  ASSERT_FPGA_OK(res);
  p.reset(new dma_buffer(handle, len, virt, wsid, iova));

  return p;
}

void dma_buffer::fill(int c) { ::memset(virt_, c, len_); }

int dma_buffer::compare(dma_buffer::ptr_t other, size_t len) const {
  return ::memcmp(virt_, other->virt_, len);
}

dma_buffer::dma_buffer(handle::ptr_t handle, size_t len, uint8_t *virt,
                       uint64_t wsid, uint64_t iova)
    : handle_(handle), len_(len), virt_(virt), wsid_(wsid), iova_(iova) {}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
