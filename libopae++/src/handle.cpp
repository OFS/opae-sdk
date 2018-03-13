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
#include <opae/mmio.h>
#include <opae/utils.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/except.h>

namespace opae {
namespace fpga {
namespace types {

handle::handle(fpga_handle h)
    : handle_(h),
      log_("handle")  {}

handle::~handle() { close(); }

handle::ptr_t handle::open(fpga_token token, int flags) {
  fpga_handle c_handle = nullptr;
  ptr_t p;

  auto res = fpgaOpen(token, &c_handle, flags);
  if (res == FPGA_OK) {
    p.reset(new handle(c_handle));
  }

  return p;
}

handle::ptr_t handle::open(token::ptr_t tok, int flags) {
  return handle::open(*tok, flags);
}

fpga_result handle::close() {
  fpga_result res = FPGA_INVALID_PARAM;

  if (handle_ != nullptr) {
    res = fpgaClose(handle_);

    if (res == FPGA_OK) {
      handle_ = nullptr;
    } else {
      log_.error() << "fpgaClose() failed with (" << res
                   << ") " << fpgaErrStr(res);
      throw except(res, OPAECXX_HERE);
    }
  }

  return res;
}

void handle::reset() {
  auto res = fpgaReset(handle_);
  if (res != FPGA_OK) {
    log_.error() << "fpgaReset() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }
}

uint32_t handle::read_csr32(uint64_t offset, uint32_t csr_space) const {
  uint32_t value = 0;
  if (FPGA_OK == fpgaReadMMIO32(handle_, csr_space, offset, &value)){
    return value;
  }
  throw except(OPAECXX_HERE);
}

uint64_t handle::read_csr64(uint64_t offset, uint32_t csr_space) const {
  uint64_t value = 0;
  if (FPGA_OK == fpgaReadMMIO64(handle_, csr_space, offset, &value)){
    return value;
  }
  throw except(OPAECXX_HERE);
}

void handle::write_csr32(uint64_t offset, uint32_t value, uint32_t csr_space) {
  if (FPGA_OK != fpgaWriteMMIO32(handle_, csr_space, offset, value)){
    throw except(OPAECXX_HERE);
  }
}

void handle::write_csr64(uint64_t offset, uint64_t value, uint32_t csr_space) {
  if (FPGA_OK != fpgaWriteMMIO64(handle_, csr_space, offset, value)){
    throw except(OPAECXX_HERE);
  }
}

uint8_t * handle::mmio_ptr(uint64_t offset, uint32_t csr_space) const
{
  uint8_t *base = nullptr;
  fpga_result res;

  res = fpgaMapMMIO(handle_, csr_space,
                     reinterpret_cast<uint64_t **>(&base));
  if (res != FPGA_OK) {
    log_.error() << "fpgaMapMMIO() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }

  return base + offset;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
