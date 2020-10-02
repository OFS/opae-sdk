// Copyright(c) 2017-2018, Intel Corporation
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
#include <Python.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <opae/fpga.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <atomic>
#include <exception>
#include "pyerrors.h"
#include "pyevents.h"
#include "pyhandle.h"
#include "pyproperties.h"
#include "pyshared_buffer.h"
#include "pysysobject.h"
#include "pytoken.h"

namespace py = pybind11;
using opae::fpga::types::properties;
using opae::fpga::types::token;
using opae::fpga::types::handle;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::event;
using opae::fpga::types::error;
using opae::fpga::types::sysobject;
using opae::fpga::types::version;

const char *memory_barrier_doc = R"opaedoc(
  Place a memory barrier or fence to ensure that all preceding memory operations have completed before continuing.
)opaedoc";

#ifdef OPAE_EMBEDDED
#include <pybind11/embed.h>
PYBIND11_EMBEDDED_MODULE(_opae, m) {
  m.def("initialize", &fpgaInitialize);
#else
PYBIND11_MODULE(_opae, m) {
  fpgaInitialize(nullptr);
#endif

  py::options opts;
  // opts.disable_function_signatures();

  m.doc() = "Open Programmable Acceleration Engine - Python bindings";

  // define enumerations
  py::enum_<fpga_result>(m, "fpga_result", py::arithmetic(),
                         "OPAE return codes")
      .value("OK", FPGA_OK)
      .value("INVALID_PARAM", FPGA_INVALID_PARAM)
      .value("BUSY", FPGA_BUSY)
      .value("EXCEPTION", FPGA_EXCEPTION)
      .value("NOT_FOUND", FPGA_NOT_FOUND)
      .value("NO_MEMORY", FPGA_NO_MEMORY)
      .value("NOT_SUPPORTED", FPGA_NOT_SUPPORTED)
      .value("NO_DRIVER", FPGA_NO_DRIVER)
      .value("NO_DAEMON", FPGA_NO_DAEMON)
      .value("NO_ACCESS", FPGA_NO_ACCESS)
      .value("RECONF_ERROR", FPGA_RECONF_ERROR)
      .export_values();

  py::enum_<fpga_objtype>(m, "fpga_objtype", py::arithmetic(),
                          "OPAE resource objects")
      .value("DEVICE", FPGA_DEVICE)
      .value("ACCELERATOR", FPGA_ACCELERATOR)
      .export_values();

  py::enum_<fpga_open_flags>(m, "fpga_open_flags", py::arithmetic(),
                             "OPAE flags for opening resources")
      .value("OPEN_SHARED", FPGA_OPEN_SHARED)
      .export_values();

  py::enum_<fpga_event_type>(m, "fpga_event_type", py::arithmetic(),
                             "OPAE event type")
      .value("EVENT_INTERRUPT", FPGA_EVENT_INTERRUPT)
      .value("EVENT_ERROR", FPGA_EVENT_ERROR)
      .value("EVENT_POWER_THERMAL", FPGA_EVENT_POWER_THERMAL)
      .export_values();

  py::enum_<fpga_accelerator_state>(m, "fpga_accelerator_state",
                                    py::arithmetic(), "OPAE accelerator_state")
      .value("ACCELERATOR_ASSIGNED", FPGA_ACCELERATOR_ASSIGNED)
      .value("ACCELERATOR_UNASSIGNED", FPGA_ACCELERATOR_UNASSIGNED)
      .export_values();

  py::enum_<fpga_sysobject_flags>(m, "fpga_sysobject_flags", py::arithmetic(),
                                  "OPAE sysobject API flags.")
      .value("SYSOBJECT_SYNC", FPGA_OBJECT_SYNC)
      .value("SYSOBJECT_RAW", FPGA_OBJECT_RAW)
      .value("SYSOBJECT_GLOB", FPGA_OBJECT_GLOB)
      .value("SYSOBJECT_RECURSE_ONE", FPGA_OBJECT_RECURSE_ONE)
      .value("SYSOBJECT_RECURSE_ALL", FPGA_OBJECT_RECURSE_ALL)
      .export_values();

  py::enum_<fpga_reconf_flags>(
      m, "fpga_reconf_flags", py::arithmetic(),
      "Flags that define how an accelerator is opened.")
      .value("RECONF_FORCE", FPGA_RECONF_FORCE)
      .export_values();

  // version method
  m.def("version", &version::as_string,
        "Get the OPAE runtime version as a string");
  m.def("build", &version::build, "Get the OPAE runtime build hash");

  // define properties class
  py::class_<properties, properties::ptr_t> pyproperties(m, "properties",
                                                         properties_doc());
  pyproperties.def(py::init(&properties_get), properties_doc_get())
      .def(py::init(&properties_get_token), properties_doc_get_token())
      .def(py::init(&properties_get_handle), properties_doc_get_handle())
      .def_property("parent", properties_get_parent, properties_set_parent,
                    properties_doc_parent())
      .def_property("guid", properties_get_guid, properties_set_guid,
                    properties_doc_guid())
      .def_property("type", properties_get_type, properties_set_type,
                    properties_doc_type())
      .def_property("segment", properties_get_segment, properties_set_segment,
                    properties_doc_segment())
      .def_property("bus", properties_get_bus, properties_set_bus,
                    properties_doc_bus())
      .def_property("device", properties_get_device, properties_set_device,
                    properties_doc_device())
      .def_property("function", properties_get_function,
                    properties_set_function, properties_doc_function())
      .def_property("socket_id", properties_get_socket_id,
                    properties_set_socket_id, properties_doc_socket_id())
      .def_property("object_id", properties_get_object_id,
                    properties_set_object_id, properties_doc_object_id())
      .def_property("num_errors", properties_get_num_errors,
                    properties_set_num_errors, properties_doc_num_errors())
      .def_property("num_slots", properties_get_num_slots,
                    properties_set_num_slots, properties_doc_num_slots())
      .def_property("bbs_id", properties_get_bbs_id, properties_set_bbs_id,
                    properties_doc_bbs_id())
      .def_property("bbs_version", properties_get_bbs_version,
                    properties_set_bbs_version, properties_doc_bbs_version())
      .def_property("vendor_id", properties_get_vendor_id,
                    properties_set_vendor_id, properties_doc_vendor_id())
      .def_property("device_id", properties_get_device_id,
                    properties_set_device_id, properties_doc_device_id())
      .def_property("model", properties_get_model, properties_set_model,
                    properties_doc_model())
      .def_property("local_memory_size", properties_get_local_memory_size,
                    properties_set_local_memory_size,
                    properties_doc_local_memory_size())
      .def_property("capabilities", properties_get_capabilities,
                    properties_set_capabilities, properties_doc_capabilities())
      .def_property("num_mmio", properties_get_num_mmio,
                    properties_set_num_mmio, properties_doc_num_mmio())
      .def_property("num_interrupts", properties_get_num_interrupts,
                    properties_set_num_interrupts,
                    properties_doc_num_interrupts())
      .def_property("accelerator_state", properties_get_accelerator_state,
                    properties_set_accelerator_state,
                    properties_doc_accelerator_state());

  // memory fence
  m.def("memory_barrier",
        []() { std::atomic_thread_fence(std::memory_order_release); },
        memory_barrier_doc);
  // define token class
  m.def("enumerate", &token::enumerate, token_doc_enumerate())
      .def("enumerate", token_enumerate_kwargs, token_doc_enumerate_kwargs());
  py::class_<token, token::ptr_t> pytoken(m, "token", token_doc());
  pytoken.def("__getattr__", token_get_sysobject, sysobject_doc_token_get())
      .def("__getitem__", token_get_sysobject, sysobject_doc_token_get())
      .def("find", token_find_sysobject, sysobject_doc_token_find(),
           py::arg("name"), py::arg("flags") = FPGA_OBJECT_GLOB);

  // define handle class
  m.def("open", handle_open, handle_doc_open(), py::arg("tok"),
        py::arg("flags") = 0);
  py::class_<handle, handle::ptr_t> pyhandle(m, "handle");
  pyhandle.def("__enter__", handle_context_enter, handle_doc_context_enter())
      .def("__exit__", handle_context_exit, handle_doc_context_exit())
      .def("reconfigure", handle_reconfigure, handle_doc_reconfigure(),
           py::arg("slot"), py::arg("fd"), py::arg("flags") = 0)
      .def("__bool__", handle_valid, handle_doc_valid())
      .def("close", &handle::close, handle_doc_close())
      .def("reset", &handle::reset, handle_doc_reset())
      .def("read_csr32", &handle::read_csr32, handle_doc_read_csr32(),
           py::arg("offset"), py::arg("csr_space") = 0)
      .def("read_csr64", &handle::read_csr64, handle_doc_read_csr64(),
           py::arg("offset"), py::arg("csr_space") = 0)
      .def("write_csr32", &handle::write_csr32, handle_doc_write_csr32(),
           py::arg("offset"), py::arg("value"), py::arg("csr_space") = 0)
      .def("write_csr64", &handle::write_csr64, handle_doc_write_csr64(),
           py::arg("offset"), py::arg("value"), py::arg("csr_space") = 0)
      .def("__getattr__", handle_get_sysobject, sysobject_doc_handle_get())
      .def("__getitem__", handle_get_sysobject, sysobject_doc_handle_get())
      .def("find", handle_find_sysobject, sysobject_doc_handle_find(),
           py::arg("name"), py::arg("flags") = FPGA_OBJECT_GLOB);

  // define shared_buffer class
  m.def("allocate_shared_buffer", shared_buffer_allocate,
        shared_buffer_doc_allocate());
  py::class_<shared_buffer, shared_buffer::ptr_t> pybuffer(
      m, "shared_buffer", py::buffer_protocol(), shared_buffer_doc());
  pybuffer.def("size", &shared_buffer::size, shared_buffer_doc_size())
      .def("wsid", &shared_buffer::wsid, shared_buffer_doc_wsid())
      .def("io_address", &shared_buffer::io_address,
           shared_buffer_doc_io_address())
      .def("fill", &shared_buffer::fill, shared_buffer_doc_fill())
      .def("poll", shared_buffer_poll<uint8_t>,
           "Poll for an 8-bit value being set at given offset",
           py::arg("offset"), py::arg("value"), py::arg("mask") = 0,
           py::arg("timeout_usec") = 1000)
      .def("poll32", shared_buffer_poll<uint32_t>,
           "Poll for a 32-bit value being set at given offset",
           py::arg("offset"), py::arg("value"), py::arg("mask") = 0,
           py::arg("timeout_usec") = 1000)
      .def("poll64", shared_buffer_poll<uint64_t>,
           "Poll for a 64-bit value being set at given offset",
           py::arg("offset"), py::arg("value"), py::arg("mask"),
           py::arg("timeout_usec") = 1000)
      .def("compare", &shared_buffer::compare, shared_buffer_doc_compare())
      .def("copy", shared_buffer_copy, shared_buffer_doc_copy(),
           py::arg("other"), py::arg("size") = 0)
      .def_buffer([](shared_buffer &b) -> py::buffer_info {
        return py::buffer_info(
            const_cast<uint8_t *>(b.c_type()), sizeof(uint8_t),
            py::format_descriptor<uint8_t>::format(), b.size());
      })
      .def("read32", &shared_buffer::read<uint32_t>, shared_buffer_doc_read32())
      .def("read64", &shared_buffer::read<uint64_t>, shared_buffer_doc_read64())
      .def("write32", &shared_buffer::write<uint32_t>,
           shared_buffer_doc_write32())
      .def("write64", &shared_buffer::write<uint64_t>,
           shared_buffer_doc_write64())
      .def("split", shared_buffer_split, shared_buffer_doc_split())
      .def("__getitem__", shared_buffer_getitem, shared_buffer_doc_getitem())
      .def("__setitem__", shared_buffer_setitem, shared_buffer_doc_setitem())
      .def("__getitem__", shared_buffer_getslice, shared_buffer_doc_getslice());

  // define event class
  m.def("register_event", event_register_event, event_doc_register_event(),
        py::arg("handle"), py::arg("event_type"), py::arg("flags") = 0);
  py::class_<event, event::ptr_t> pyevent(m, "event", event_doc());

  pyevent.def("os_object", event_os_object, event_doc_os_object());

  py::class_<error, error::ptr_t> pyerror(m, "error", error_doc());
  pyerror.def_property_readonly("name", &error::name, error_doc_name())
      .def_property_readonly("can_clear", &error::can_clear,
                             error_doc_can_clear())
      .def("read_value", &error::read_value, error_doc_read_value());

  m.def("errors", error_errors, error_doc_errors());

  // define object class
  py::class_<sysobject, sysobject::ptr_t> pysysobject(m, "sysobject",
                                                      sysobject_doc());
  pysysobject
      .def("__getattr__", sysobject_get_sysobject, sysobject_doc_object_get())
      .def("__getitem__", sysobject_get_sysobject, sysobject_doc_object_get())
      .def("find", sysobject_find_sysobject, sysobject_doc_object_find(),
           py::arg("name"), py::arg("flags") = 0)
      .def("read64",
           [](sysobject::ptr_t obj) { return obj->read64(FPGA_OBJECT_SYNC); })
      .def("write64", &sysobject::write64)
      .def("size", &sysobject::size)
      .def("bytes", sysobject_bytes, sysobject_doc_bytes())
      .def("__getitem__", sysobject_getitem, sysobject_doc_getitem())
      .def("__getitem__", sysobject_getslice, sysobject_doc_getslice());
}
