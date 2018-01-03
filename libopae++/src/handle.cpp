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
#include "opae/cxx/handle.h"
#include "opae/cxx/properties.h"
#include "opae/cxx/except.h"

namespace opae {
namespace fpga {
namespace types {

handle::handle(fpga_handle h, uint32_t mmio_region, uint8_t *mmio_base)
    : handle_(h), mmio_region_(mmio_region), mmio_base_(mmio_base),
      log_("handle")  {}

handle::~handle() { close(); }

bool handle::write(uint64_t offset, uint32_t value) {
  return FPGA_OK == fpgaWriteMMIO32(handle_, mmio_region_, offset, value);
}

bool handle::write(uint64_t offset, uint64_t value) {
  return FPGA_OK == fpgaWriteMMIO64(handle_, mmio_region_, offset, value);
}

bool handle::read(uint64_t offset, uint32_t &value) const {
  return FPGA_OK == fpgaReadMMIO32(handle_, mmio_region_, offset, &value);
}

bool handle::read(uint64_t offset, uint64_t &value) const {
  return FPGA_OK == fpgaReadMMIO64(handle_, mmio_region_, offset, &value);
}

handle::ptr_t handle::open(fpga_token token, int flags, uint32_t mmio_region) {
  fpga_handle c_handle = nullptr;
  uint8_t *mmio_base = nullptr;
  ptr_t p;
  opae::fpga::internal::logger log("handle::open()");

  auto res = fpgaOpen(token, &c_handle, flags);
  if (res == FPGA_OK) {
    properties::ptr_t props = properties::read(token);

    if (FPGA_ACCELERATOR == props->type) {
      res = fpgaMapMMIO(c_handle, mmio_region,
                        reinterpret_cast<uint64_t **>(&mmio_base));

      if (res != FPGA_OK) {
        log.error() << "fpgaMapMMIO() failed with (" << res
                    << ") " << fpgaErrStr(res);
        throw except(res, OPAECXX_HERE);
      }
    }

    p.reset(new handle(c_handle, mmio_region, mmio_base));
  }

  return p;
}

handle::ptr_t handle::open(token::ptr_t tok, int flags, uint32_t mmio_region) {
  return handle::open(*tok, flags, mmio_region);
}

fpga_result handle::close() {
  fpga_result res = FPGA_INVALID_PARAM;

  if (handle_ != nullptr) {
    if (mmio_base_ != nullptr) {
      res = fpgaUnmapMMIO(handle_, mmio_region_);

      if (res == FPGA_OK) {
        mmio_base_ = nullptr;
      } else {
        log_.error() << "fpgaUnmapMMIO() failed with (" << res
                     << ") " << fpgaErrStr(res);
        throw except(res, OPAECXX_HERE);
      }
    }

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

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
