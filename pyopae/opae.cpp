#include <Python.h>

#include <opae/cxx/core/token.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "pyevents.h"
#include "pyhandle.h"
#include "pyproperties.h"
#include "pyshared_buffer.h"

namespace py = pybind11;
using opae::fpga::types::properties;
using opae::fpga::types::token;
using opae::fpga::types::handle;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::event;

const char *token_doc() {
  return R"opaedoc(
    Token for referencing an OPAE resource.

    A token object serves as a reference so a specific resource in the system.
    Holding a token does not constitute ownership of an OPAE resource.
    It is used to query information about a resource,
    or to acquire ownership by calling fpga.open module method.
  )opaedoc";
}

const char *token_doc_enumerate() {
  return R"opaedoc(
    Get a list of tokens for the given search criteria.

    Args:

      props(list): A list of properties objects that define the search criteria.
                   All OPAE properties in each properties object make up one filter.
                   All properties objects are combined in a union.
  )opaedoc";
}

PYBIND11_MODULE(_opae, m) {
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

  py::enum_<fpga_reconf_flags>(
      m, "fpga_reconf_flags", py::arithmetic(),
      "Flags that define how an accelerator is opened.")
      .value("RECONF_FORCE", FPGA_RECONF_FORCE)
      .export_values();

  // define properties class
  py::class_<properties, properties::ptr_t> pyproperties(m, "properties",
                                                         properties_doc());
  pyproperties.def(py::init(&properties_get), properties_doc_get())
      .def(py::init(&properties_get_token), properties_doc_get_token())
      .def_property("parent", properties_get_parent, properties_get_parent,
                    properties_doc_parent())
      .def_property("guid", properties_get_guid, properties_set_guid,
                    properties_doc_guid())
      .def_property("type", properties_get_type, properties_set_type,
                    properties_doc_type())
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
      .def_property("num_slots", properties_get_num_slots,
                    properties_set_num_slots, properties_doc_num_slots())
      .def_property("bbs_id", properties_get_bbs_id, properties_set_bbs_id,
                    properties_doc_bbs_id())
      .def_property("bbs_version", properties_get_bbs_version,
                    properties_set_bbs_id, properties_doc_bbs_id())
      .def_property("vendor_id", properties_get_vendor_id,
                    properties_set_vendor_id, properties_doc_vendor_id())
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

  // define token class
  m.def("enumerate", &token::enumerate, token_doc_enumerate());
  py::class_<token, token::ptr_t> pytoken(m, "token", token_doc());

  // define handle class
  m.def("open", handle_open, handle_doc_open(), py::arg("tok"),
        py::arg("flags") = 0);
  py::class_<handle, handle::ptr_t> pyhandle(m, "handle");
  pyhandle
      .def("__enter__", handle_context_enter, handle_doc_context_enter())
      .def("__exit__", handle_context_exit, handle_doc_context_exit())
      .def("reconfigure", handle_reconfigure, handle_doc_reconfigure(),
           py::arg("slot"), py::arg("fd"), py::arg("flags") = 0)
      .def("__bool__", handle_valid, handle_doc_valid())
      .def("close", &handle::close, handle_doc_close())
      .def("reset", &handle::reset)
      .def("read_csr32", &handle::read_csr32, handle_doc_read_csr32(),
           py::arg("offset"), py::arg("csr_space") = 0)
      .def("read_csr64", &handle::read_csr64, handle_doc_read_csr64(),
           py::arg("offset"), py::arg("csr_space") = 0)
      .def("write_csr32", &handle::write_csr32, handle_doc_write_csr32(),
           py::arg("offset"), py::arg("value"), py::arg("csr_space") = 0)
      .def("write_csr64", &handle::write_csr64, handle_doc_write_csr64(),
           py::arg("offset"), py::arg("value"), py::arg("csr_space") = 0);

  // define shared_buffer class
  m.def("allocate_shared_buffer", &shared_buffer::allocate,
        shared_buffer_doc_allocate());
  py::class_<shared_buffer, shared_buffer::ptr_t> pybuffer(
      m, "shared_buffer", py::buffer_protocol(), shared_buffer_doc());
  pybuffer.def("size", &shared_buffer::size, shared_buffer_doc_size())
      .def("wsid", &shared_buffer::wsid, shared_buffer_doc_wsid())
      .def("iova", &shared_buffer::iova, shared_buffer_doc_iova())
      .def("fill", &shared_buffer::fill, shared_buffer_doc_fill())
      .def("compare", &shared_buffer::compare, shared_buffer_doc_compare())
      .def("memoryview", shared_buffer_to_memoryview,
           shared_buffer_doc_to_memoryview())
      .def_buffer([](shared_buffer &b) -> py::buffer_info {
    return py::buffer_info(const_cast<uint8_t *>(b.c_type()), b.size());
      });

  // define event class
  m.def("register_event", event_register_event,
               event_doc_register_event(), py::arg("handle"),
               py::arg("event_type"), py::arg("flags") = 0);
  py::class_<event, event::ptr_t> pyevent(m, "event", event_doc());

  pyevent.def("os_object", event_os_object, event_doc_os_object());
}
