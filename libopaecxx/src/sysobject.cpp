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
#include <opae/cxx/core/sysobject.h>
#include <opae/sysobject.h>

namespace opae {
namespace fpga {
namespace types {

sysobject::sysobject(fpga_object sysobj, token::ptr_t tok, handle::ptr_t hnd)
    : sysobject_(sysobj), token_(tok), handle_(hnd) {}
sysobject::~sysobject() {
  if (sysobject_ != nullptr) {
    auto res = fpgaDestroyObject(&sysobject_);
    if (res != FPGA_OK) {
      std::cerr << "Error while calling fpgaDestroyObject: " << fpgaErrStr(res)
                << "\n";
    }
  }
}

uint32_t sysobject::size() const {
  uint32_t size;
  ASSERT_FPGA_OK(fpgaObjectGetSize(sysobject_, &size, FPGA_OBJECT_SYNC));
  return size;
}

sysobject::ptr_t sysobject::get(token::ptr_t tok, const std::string &path,
                                int flags) {
  fpga_object sysobj;
  sysobject::ptr_t obj;
  auto res = fpgaTokenGetObject(tok->c_type(), path.c_str(), &sysobj, flags);
  if (!res) {
    obj.reset(new sysobject(sysobj, tok, nullptr));
  } else if (res != FPGA_NOT_FOUND) {
    ASSERT_FPGA_OK(res);
  }
  return obj;
}

sysobject::ptr_t sysobject::get(handle::ptr_t hnd, const std::string &path,
                                int flags) {
  fpga_object sysobj;
  sysobject::ptr_t obj;
  auto res = fpgaHandleGetObject(hnd->c_type(), path.c_str(), &sysobj, flags);
  if (!res) {
    obj.reset(new sysobject(sysobj, nullptr, hnd));
  } else if (res != FPGA_NOT_FOUND) {
    ASSERT_FPGA_OK(res);
  }
  return obj;
}

sysobject::ptr_t sysobject::get(const std::string &path, int flags) {
  fpga_object sysobj;
  sysobject::ptr_t obj;
  auto res = fpgaObjectGetObject(sysobject_, path.c_str(), &sysobj, flags);
  if (!res) {
    obj.reset(new sysobject(sysobj, token_, handle_));
  } else if (res != FPGA_NOT_FOUND) {
    ASSERT_FPGA_OK(res);
  }
  return obj;
}

uint64_t sysobject::read64(int flags) const {
  uint64_t value = 0;
  ASSERT_FPGA_OK(fpgaObjectRead64(sysobject_, &value, flags));
  return value;
}

void sysobject::write64(uint64_t value, int flags) const {
  ASSERT_FPGA_OK(fpgaObjectWrite64(sysobject_, value, flags));
}

std::vector<uint8_t> sysobject::bytes(int flags) const {
  uint32_t size;
  ASSERT_FPGA_OK(fpgaObjectGetSize(sysobject_, &size, flags));
  std::vector<uint8_t> bytes(size);
  ASSERT_FPGA_OK(fpgaObjectRead(sysobject_, bytes.data(), 0, size, flags));
  return bytes;
}

std::vector<uint8_t> sysobject::bytes(uint32_t offset, uint32_t size,
                                      int flags) const {
  std::vector<uint8_t> bytes(size);
  ASSERT_FPGA_OK(fpgaObjectRead(sysobject_, bytes.data(), offset, size, flags));
  return bytes;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
