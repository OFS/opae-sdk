// Copyright(c) 2021-2022, Intel Corporation
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

#include "pyopaeuio.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>

// open uio
int pyopae_uio::open(const std::string &uio_str) {
  int res = 0;
  num_regions = 0;

  // opae UIO device
  res = opae_uio_open(&uio_, uio_str.c_str());
  if (res) {
    std::cout << "Failed to open uio" << std::endl;
    throw std::invalid_argument("Failed to open uio");
  }

  // get uio region
  res = opae_uio_region_get(&uio_, 0, (uint8_t **)&uio_mmap_ptr_, nullptr);
  if (res) {
    std::cout << "Failed to git uio region" << std::endl;
    throw std::invalid_argument("Failed to git uio region");
  }

  struct opae_uio_device_region *r = uio_.regions;
  while (r) {
    ++num_regions;
    r = r->next;
  }

  return 0;
}

// close uio
void pyopae_uio::close(void) { return opae_uio_close(&uio_); }

// get uio region pointer
uint8_t *pyopae_uio::get_region(uint32_t region_index, uint32_t offset) {
  uint8_t *vptr = nullptr;
  size_t size = 0;

  if (opae_uio_region_get(&uio_, region_index, &vptr, &size)) {
    std::cout << "failed to get uio region" << std::endl;
    return nullptr;
  }

  if (offset >= size) {
    std::cout << "invalid offset" << std::endl;
    return nullptr;
  }

  return vptr + offset;
}

// read 8 bit value
uint8_t pyopae_uio::read8(uint32_t region_index, uint32_t offset) {
  uint8_t value = 0;
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  value = *reinterpret_cast<uint8_t *>(ptr);
  return value;
}

// read 16 bit value
uint16_t pyopae_uio::read16(uint32_t region_index, uint32_t offset) {
  uint16_t value = 0;
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  value = *reinterpret_cast<uint16_t *>(ptr);
  return value;
}

// read 32 bit value
uint32_t pyopae_uio::read32(uint32_t region_index, uint32_t offset) {
  uint32_t value = 0;
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  value = *reinterpret_cast<uint32_t *>(ptr);
  return value;
}

// read 64 bit value
uint64_t pyopae_uio::read64(uint32_t region_index, uint32_t offset) {
  uint64_t value = 0;
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  value = *reinterpret_cast<uint64_t *>(ptr);
  return value;
}

// write 8 bit value
uint64_t pyopae_uio::write8(uint32_t region_index, uint32_t offset,
                            uint8_t value) {
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  *reinterpret_cast<uint8_t *>(ptr) = value;
  return 0;
}

// write 16 bit value
uint64_t pyopae_uio::write16(uint32_t region_index, uint32_t offset,
                             uint16_t value) {
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  *reinterpret_cast<uint16_t *>(ptr) = value;
  return 0;
}

// write 32 bit value
uint64_t pyopae_uio::write32(uint32_t region_index, uint32_t offset,
                             uint32_t value) {
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  *reinterpret_cast<uint32_t *>(ptr) = value;
  return 0;
}

// write 64 bit value
uint64_t pyopae_uio::write64(uint32_t region_index, uint32_t offset,
                             uint64_t value) {
  uint8_t *ptr = get_region(region_index, offset);
  if (!ptr) {
    throw std::invalid_argument("Failed to get uio region");
  }

  *reinterpret_cast<uint64_t *>(ptr) = value;
  return 0;
}

// get uio region size
size_t pyopae_uio::get_size(uint32_t region_index) {
  size_t size = 0;
  uint8_t *vptr = nullptr;
  if (opae_uio_region_get(&uio_, region_index, &vptr, &size)) {
    std::cout << "failed to get uio region size:" << vptr << size << std::endl;
  }
  return size;
}

namespace py = pybind11;
PYBIND11_MODULE(pyopaeuio, m) {
  m.doc() = "pybind11 pyopaeuio plugin";
  py::class_<pyopae_uio>(m, "pyopaeuio")
      .def(py::init<>())
      .def("open", (int(pyopae_uio::*)(const std::string &)) & pyopae_uio::open)
      .def("close", (void(pyopae_uio::*)(void)) & pyopae_uio::close)
      .def("read8",
           (uint8_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset)) &
               pyopae_uio::read8)
      .def("read16",
           (uint16_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset)) &
               pyopae_uio::read16)
      .def("read32",
           (uint32_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset)) &
               pyopae_uio::read32)
      .def("read64",
           (uint64_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset)) &
               pyopae_uio::read64)
      .def("write8", (uint64_t(pyopae_uio::*)(uint32_t region_index,
                                              uint32_t offset, uint8_t value)) &
                         pyopae_uio::write8)
      .def("write16",
           (uint64_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset,
                                    uint16_t value)) &
               pyopae_uio::write16)
      .def("write32",
           (uint64_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset,
                                    uint32_t value)) &
               pyopae_uio::write32)
      .def("write64",
           (uint64_t(pyopae_uio::*)(uint32_t region_index, uint32_t offset,
                                    uint64_t value)) &
               pyopae_uio::write64)
      .def("getsize", (size_t(pyopae_uio::*)(uint32_t region_index)) &
                          pyopae_uio::get_size)
      .def_readonly("numregions", &pyopae_uio::num_regions);
}
