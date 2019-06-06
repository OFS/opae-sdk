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
#include <opae/cxx/core/except.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/enum.h>
#include <opae/manage.h>
#include <opae/mmio.h>
#include <opae/utils.h>

namespace opae {
namespace fpga {
namespace types {

handle::handle(fpga_handle h) : handle_(h), token_(nullptr) {}

handle::~handle() {
  close();
  // release the cloned token
  auto result = fpgaDestroyToken(&token_);
  if (result != FPGA_OK) {
    std::cerr << "Error while calling fpgaDestroyToken: " << fpgaErrStr(result)
              << "\n";
  }
}

handle::ptr_t handle::open(fpga_token token, int flags) {
  fpga_handle c_handle = nullptr;
  fpga_token c_token = nullptr;
  ptr_t p;
  // clone the token used to open the resource
  ASSERT_FPGA_OK(fpgaCloneToken(token, &c_token));
  auto res = fpgaOpen(c_token, &c_handle, flags);
  ASSERT_FPGA_OK(res);
  p.reset(new handle(c_handle));
  // stash the cloned token along with the handle object
  p->token_ = c_token;
  return p;
}

handle::ptr_t handle::open(token::ptr_t tok, int flags) {
  if (!tok) {
    throw std::invalid_argument("token object is null");
  }
  return handle::open(*tok, flags);
}

fpga_result handle::close() {
  if (handle_ != nullptr) {
    auto res = fpgaClose(handle_);
    ASSERT_FPGA_OK(res);
    handle_ = nullptr;
    return FPGA_OK;
  }

  return FPGA_EXCEPTION;
}

void handle::reconfigure(uint32_t slot, const uint8_t *bitstream, size_t size,
                         int flags) {
  ASSERT_FPGA_OK(fpgaReconfigureSlot(handle_, slot, bitstream, size, flags));
}

void handle::reset() {
  auto res = fpgaReset(handle_);
  ASSERT_FPGA_OK(res);
}

uint32_t handle::read_csr32(uint64_t offset, uint32_t csr_space) const {
  uint32_t value = 0;
  ASSERT_FPGA_OK(fpgaReadMMIO32(handle_, csr_space, offset, &value));
  return value;
}

uint64_t handle::read_csr64(uint64_t offset, uint32_t csr_space) const {
  uint64_t value = 0;
  ASSERT_FPGA_OK(fpgaReadMMIO64(handle_, csr_space, offset, &value));
  return value;
}

void handle::write_csr32(uint64_t offset, uint32_t value, uint32_t csr_space) {
  ASSERT_FPGA_OK(fpgaWriteMMIO32(handle_, csr_space, offset, value));
}

void handle::write_csr64(uint64_t offset, uint64_t value, uint32_t csr_space) {
  ASSERT_FPGA_OK(fpgaWriteMMIO64(handle_, csr_space, offset, value));
}

void handle::write_csr512(uint64_t offset, const void *value, uint32_t csr_space) {
  ASSERT_FPGA_OK(fpgaWriteMMIO512(handle_, csr_space, offset, value));
}

uint8_t *handle::mmio_ptr(uint64_t offset, uint32_t csr_space) const {
  uint8_t *base = nullptr;

  auto res =
      fpgaMapMMIO(handle_, csr_space, reinterpret_cast<uint64_t **>(&base));

  ASSERT_FPGA_OK(res);
  return base + offset;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
