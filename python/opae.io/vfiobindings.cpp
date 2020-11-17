// Copyright(c) 2020, Intel Corporation
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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <sstream>
#include <vector>

#include <opae/vfio.h>
#include "main.h"

namespace py = pybind11;

typedef struct opae_vfio *device_ptr;

device_ptr device_open_pciaddress(const char *pci_addr)
{
    device_ptr v = new struct opae_vfio();

    if (opae_vfio_open(v, pci_addr)) {
        delete v;
        v = nullptr;
    }

    return v;
}

void device_close(device_ptr v)
{
    if (v) {
        opae_vfio_close(v);
        delete v;
    }
}

static inline void assert_config_op(uint64_t offset,
                                    ssize_t expected,
				    ssize_t actual,
				    const char *op)
{
  if (actual != expected) {
    std::stringstream ss;
    ss << "error: [pci_config:" << op << " @0x" << std::hex << offset
       << " expected " << std::dec << expected
       << ", processed " << actual << "\n";
    throw std::length_error(ss.str().c_str());
  }
}

template <typename T>
T device_config_read(device_ptr v, uint64_t offset)
{
  T value = 0;
  ssize_t bytes;
  bytes = pread(v->device.device_fd,
                &value,
                sizeof(T),
                v->device.device_config_offset + offset);
  assert_config_op(offset, sizeof(T), bytes, "read");
  return value;
}

template <typename T>
void device_config_write(device_ptr v, uint64_t offset, T value)
{
  ssize_t bytes;
  bytes = pwrite(v->device.device_fd,
                 &value,
                 sizeof(T),
		 v->device.device_config_offset + offset);
  assert_config_op(offset, sizeof(T), bytes, "write");
}

uint32_t device_num_regions(device_ptr v)
{
  uint32_t count = 0;
  struct opae_vfio_device_region *r = v->device.regions;
  while (r) {
    ++count;
    r = r->next;
  }
  return count;
}

std::vector<mmio_region> device_regions(device_ptr v)
{
  std::vector<mmio_region> vect;
  struct opae_vfio_device_region *r = v->device.regions;
  while (r) {
    mmio_region region = { r->region_index, r->region_ptr, r->region_size };
    vect.push_back(region);
    r = r->next;
  }
  return vect;
}

system_buffer * device_buffer_allocate(device_ptr v, size_t sz)
{
  system_buffer *b = new struct system_buffer();

  b->size = sz;
  b->buf = nullptr;
  b->iova = 0;

  if (opae_vfio_buffer_allocate(v, &b->size, &b->buf, &b->iova)) {
    delete b;
    b = nullptr;
  }

  b->vfio = v;

  return b;
}

#ifdef LIBVFIO_EMBED
#include <pybind11/embed.h>
PYBIND11_EMBEDDED_MODULE(libvfio, m)
#else
PYBIND11_MODULE(libvfio, m)
#endif
{
  py::class_<opae_vfio> pydevice(m, "device", "");
  pydevice.def_static("open", device_open_pciaddress)
          .def("descriptor", [](device_ptr v) -> int { return v->cont_fd; })
          .def("close", device_close)
          .def("__getitem__", device_config_read<uint32_t>)
          .def("__setitem__", device_config_write<uint32_t>)
	  .def("config_read32", device_config_read<uint32_t>)
          .def("config_write32", device_config_write<uint32_t>)
          .def("config_read16", device_config_read<uint16_t>)
          .def("config_write16", device_config_write<uint16_t>)
          .def("config_read8", device_config_read<uint8_t>)
          .def("config_write8", device_config_write<uint8_t>)
          .def("__repr__", [](device_ptr v) -> const char * { return v->cont_pciaddr; })
	  .def("allocate", device_buffer_allocate)
          .def_property_readonly("pci_address", [](device_ptr v) -> const char * { return v->cont_pciaddr; })
          .def_property_readonly("num_regions", device_num_regions)
          .def_property_readonly("regions", device_regions);

  py::class_<mmio_region> pyregion(m, "region", "");
  pyregion.def("write32", &mmio_region::write32)
          .def("write64", &mmio_region::write64)
          .def("read32", &mmio_region::read32)
          .def("read64", &mmio_region::read64)
          .def("index", [](mmio_region *r) { return r->index; })
          .def("__repr__", [](mmio_region *r) { return std::to_string(r->index); })
          .def("__len__", [](mmio_region *r) { return r->size; });

  py::class_<system_buffer> pybuffer(m, "system_buffer", "");
  pybuffer.def_property_readonly("size", [](system_buffer *b) -> size_t { return b->size; })
          .def_property_readonly("address", [](system_buffer *b) -> uint8_t * { return b->buf; })
          .def_property_readonly("io_address", [](system_buffer *b) -> uint64_t { return b->iova; })
          .def("__getitem__", &system_buffer::get_uint64)
          .def("__setitem__", &system_buffer::set_uint64)
          .def("read8", &system_buffer::get<uint8_t>)
          .def("read16", &system_buffer::get<uint16_t>)
          .def("read32", &system_buffer::get<uint32_t>)
          .def("read64", &system_buffer::get<uint64_t>)
          .def("fill8", &system_buffer::fill<uint8_t>)
          .def("fill16", &system_buffer::fill<uint16_t>)
          .def("fill32", &system_buffer::fill<uint32_t>)
          .def("fill64", &system_buffer::fill<uint64_t>)
          .def("compare", &system_buffer::compare);
}
