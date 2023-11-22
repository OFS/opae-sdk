// Copyright(c) 2020-2022, Intel Corporation
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
#include <sstream>
#include <iomanip>

#include <opae/vfio.h>
#include "main.h"

namespace py = pybind11;


#ifdef LIBVFIO_EMBED
#include <pybind11/embed.h>
PYBIND11_EMBEDDED_MODULE(libvfio, m)
#else
PYBIND11_MODULE(libvfio, m)
#endif
{
  py::class_<vfio_device> pydevice(m, "device", "");
  pydevice.def_static("open", &vfio_device::open, py::return_value_policy::reference)
          .def("descriptor", &vfio_device::descriptor)
          .def("close", &vfio_device::close)
          .def("__getitem__", &vfio_device::config_read<uint32_t>)
          .def("__setitem__", &vfio_device::config_write<uint32_t>)
          .def("config_read32", &vfio_device::config_read<uint32_t>)
          .def("config_write32", &vfio_device::config_write<uint32_t>)
          .def("config_read16", &vfio_device::config_read<uint16_t>)
          .def("config_write16", &vfio_device::config_write<uint16_t>)
          .def("config_read8", &vfio_device::config_read<uint8_t>)
          .def("config_write8", &vfio_device::config_write<uint8_t>)
          .def("__repr__", &vfio_device::address)
          .def("allocate", &vfio_device::buffer_allocate)
          .def("set_vf_token", &vfio_device::set_vf_token)
          .def_property_readonly("pci_address", &vfio_device::address)
          .def_property_readonly("num_regions", &vfio_device::num_regions)
          .def_property_readonly("regions", &vfio_device::regions);

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
          .def_property_readonly("address", [](system_buffer *b) -> uint64_t { return reinterpret_cast<uint64_t>(b->buf); })
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
          .def("compare", &system_buffer::compare)
          .def("__repr__", [](system_buffer *b) -> std::string {
             std::ostringstream oss;
             oss << "size: " << b->size
                 << " virt: 0x" << std::hex << std::setfill('0') << reinterpret_cast<uint64_t>(b->buf)
                 << " io: 0x" << std::hex << std::setfill('0') << b->iova;
             return oss.str();
          });
}
