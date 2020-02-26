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

#include "pyproperties.h"
#include <sstream>

namespace py = pybind11;
using opae::fpga::types::properties;
using opae::fpga::types::token;

static inline fpga_version pytuple_to_fpga_version(py::tuple tpl) {
  fpga_version version{
      .major = tpl[0].cast<uint8_t>(),
      .minor = tpl[1].cast<uint8_t>(),
      .patch = tpl[2].cast<uint16_t>(),
  };
  return version;
}

const char *properties_doc() {
  return R"opaedoc(
    properties class is a container class for OPAE resource properties.
  )opaedoc";
}

const char *properties_doc_get() {
  return R"opaedoc(
    Create a new properties object. If kwargs is not included then the
    properties object is created with no property values set.
    If one of the kwargs keys is an OPAE property name then the kwargs
    value is used to initialize the corresponding value in the
    properties object.

    Kwargs:

      parent (token): Token object representing parent resource.

      guid (str): GUID (as a string) of the resource.

      type (fpga_objtype): The object type - DEVICE or ACCELERATOR.

      segment (uint16_t) : The PCIe segment (or domain) number.

      bus (uint8_t) : The PCIe bus number.

      device (uint8_t) : The PCIe device number.

      function (uint8_t) : The PCIe function number.

      socket_id (uint8_t): The socket ID encoded in the FIM.

      num_slots (uint32_t): Number of slots available in the FPGA.

      num_errors (uint32_t): Number of error registers in the resource.

      bbs_id (uint64_t): The BBS ID encoded in the FIM.

      bbs_version (tuple): The version of the BBS.

      vendor_id (uint16_t): The vendor ID in PCI config space.

      device_id (uint16_t): The device ID in PCI config space.

      model (str): The model of the FPGA.

      local_memory_size (uint64_t): The size (in bytes) of the FPGA local memory.

      num_mmio (uint32_t): The number of mmio spaces.

      num_interrupts (uint32_t): The number of interrupts supported by an accelerator.

      accelerator_state (fpga_accelerator_state): The state of the accelerator - ASSIGNED or UNASSIGNED.

      object_id (uint64_t): The 64-bit number unique within a single node or system.

   )opaedoc";
}
properties::ptr_t properties_get(py::kwargs kwargs) {
  auto props = properties::get();
  // if kwargs is empty, return a new (empty) properties object
  if (!kwargs) {
    return props;
  }

  if (kwargs.contains("parent")) {
    props->parent = *kwargs["parent"].cast<token::ptr_t>();
  }

  if (kwargs.contains("guid")) {
    props->guid.parse(kwargs["guid"].cast<std::string>().c_str());
  }

  kwargs_to_props<fpga_objtype>(props->type, kwargs, "type");
  kwargs_to_props<uint16_t>(props->segment, kwargs, "segment");
  kwargs_to_props<uint8_t>(props->bus, kwargs, "bus");
  kwargs_to_props<uint8_t>(props->device, kwargs, "device");
  kwargs_to_props<uint8_t>(props->function, kwargs, "function");
  kwargs_to_props<uint8_t>(props->socket_id, kwargs, "socket_id");
  kwargs_to_props<uint32_t>(props->num_errors, kwargs, "num_errors");
  kwargs_to_props<uint32_t>(props->num_slots, kwargs, "num_slots");
  kwargs_to_props<uint64_t>(props->bbs_id, kwargs, "bbs_id");

  if (kwargs.contains("bbs_version")) {
    py::tuple version_tuple = kwargs["bbs_version"].cast<py::tuple>();
    props->bbs_version = pytuple_to_fpga_version(version_tuple);
  }
  kwargs_to_props<uint16_t>(props->vendor_id, kwargs, "vendor_id");
  kwargs_to_props<uint16_t>(props->device_id, kwargs, "device_id");

  if (kwargs.contains("model")) {
    props->model =
        const_cast<char *>(kwargs["model"].cast<std::string>().c_str());
  }

  kwargs_to_props<uint64_t>(props->local_memory_size, kwargs,
                            "local_memory_size");
  kwargs_to_props<uint64_t>(props->capabilities, kwargs, "capabilities");
  kwargs_to_props<uint32_t>(props->num_mmio, kwargs, "num_mmio");
  kwargs_to_props<uint32_t>(props->num_interrupts, kwargs, "num_interrupts");
  kwargs_to_props<fpga_accelerator_state>(props->accelerator_state, kwargs,
                                          "accelerator_state");
  kwargs_to_props<uint64_t>(props->object_id, kwargs, "object_id");

  return props;
}

const char *properties_doc_get_token() {
  return R"opaedoc(
    Get properties from a token object.
    Args:
      tok (token): The token to read properties from.
   )opaedoc";
}

opae::fpga::types::properties::ptr_t properties_get_token(
    opae::fpga::types::token::ptr_t tok) {
  return properties::get(tok);
}

const char *properties_doc_get_handle() {
  return R"opaedoc(
    Get properties from a handle object.
    Args:
      h (handle): The handle to read properties from.
   )opaedoc";
}

opae::fpga::types::properties::ptr_t properties_get_handle(
    opae::fpga::types::handle::ptr_t h) {
  return properties::get(h);
}

const char *properties_doc_parent() {
  return R"opaedoc(
    Get or set the token representing a parent object of a resource.
    The resource must be of type ACCELERATOR
   )opaedoc";
}

token::ptr_t properties_get_parent(properties::ptr_t props) {
  auto token_struct = props->parent;
  auto parent_props = properties::get(token_struct);
  auto tokens = token::enumerate({parent_props});
  return tokens[0];
}

void properties_set_parent(properties::ptr_t props, token::ptr_t parent) {
  props->parent = *parent;
}

// guid
const char *properties_doc_guid() {
  return R"opaedoc(
    Get or set the guid property of a resource as a string.
 )opaedoc";
}

std::string properties_get_guid(properties::ptr_t props) {
  std::stringstream ss;
  ss << props->guid;
  return ss.str();
}

void properties_set_guid(properties::ptr_t props, const std::string &guid_str) {
  props->guid.parse(guid_str.c_str());
}

// object type
const char *properties_doc_type() {
  return R"opaedoc(
    Get or set the type property of a resource. The type must be
    either DEVICE or ACCELERATOR
   )opaedoc";
}

fpga_objtype properties_get_type(properties::ptr_t props) {
  return props->type;
}

void properties_set_type(properties::ptr_t props, fpga_objtype type) {
  props->type = type;
}
// pcie segment
const char *properties_doc_segment() {
  return R"opaedoc(
    Get or set the PCIe segment property of a resource.
   )opaedoc";
}

uint16_t properties_get_segment(properties::ptr_t props) {
  return props->segment;
}

void properties_set_segment(properties::ptr_t props, uint16_t segment) {
  props->segment = segment;
}

// pcie bus
const char *properties_doc_bus() {
  return R"opaedoc(
    Get or set the PCIe bus property of a resource.
   )opaedoc";
}

uint8_t properties_get_bus(properties::ptr_t props) { return props->bus; }

void properties_set_bus(properties::ptr_t props, uint8_t bus) {
  props->bus = bus;
}

// pcie device
const char *properties_doc_device() {
  return R"opaedoc(
    Get or set the PCIe device property of a resource.
   )opaedoc";
}

uint8_t properties_get_device(properties::ptr_t props) { return props->device; }

void properties_set_device(properties::ptr_t props, uint8_t device) {
  props->device = device;
}

// pcie function
const char *properties_doc_function() {
  return R"opaedoc(
    Get or set the PCIe function property of a resource.
   )opaedoc";
}

uint8_t properties_get_function(properties::ptr_t props) {
  return props->function;
}

void properties_set_function(properties::ptr_t props, uint8_t function) {
  props->function = function;
}

// socket id
const char *properties_doc_socket_id() {
  return R"opaedoc(
    Get or set the Socket ID  property of a resource. The socket id is
    encoded in of the FIM CSRs
   )opaedoc";
}

uint8_t properties_get_socket_id(properties::ptr_t props) {
  return props->socket_id;
}

void properties_set_socket_id(properties::ptr_t props, uint8_t socket_id) {
  props->socket_id = socket_id;
}

// object id
const char *properties_doc_object_id() {
  return R"opaedoc(
    Get or set the Object ID  property of a resource. The object id is
    a 64-bit identifier that is unique within a single node or system.
    I represents a similar concept as the token but can be serialized
    for use across processes
   )opaedoc";
}

uint64_t properties_get_object_id(properties::ptr_t props) {
  return props->object_id;
}

void properties_set_object_id(properties::ptr_t props, uint64_t object_id) {
  props->object_id = object_id;
}

// num errors
const char *properties_doc_num_errors() {
  return R"opaedoc(
    Get or set the number of error registers in the resource.
   )opaedoc";
}

uint32_t properties_get_num_errors(properties::ptr_t props) {
  return props->num_errors;
}

void properties_set_num_errors(properties::ptr_t props, uint32_t num_errors) {
  props->num_errors = num_errors;
}

// num slots
const char *properties_doc_num_slots() {
  return R"opaedoc(
    Get or set the number of slots property of a resource.
    The resource must be of type DEVICE
   )opaedoc";
}

uint32_t properties_get_num_slots(properties::ptr_t props) {
  return props->num_slots;
}

void properties_set_num_slots(properties::ptr_t props, uint32_t num_slots) {
  props->num_slots = num_slots;
}

// bbs id
const char *properties_doc_bbs_id() {
  return R"opaedoc(
    Get or set the BBS ID property of a resource.
    The resource must be of type DEVICE
   )opaedoc";
}

uint64_t properties_get_bbs_id(properties::ptr_t props) {
  return props->bbs_id;
}

void properties_set_bbs_id(properties::ptr_t props, uint64_t bbs_id) {
  props->bbs_id = bbs_id;
}

// bbs version
const char *properties_doc_bbs_version() {
  return R"opaedoc(
    Get or set the BBS version property of a resource.
    The resource must be of type DEVICE
   )opaedoc";
}

std::tuple<uint8_t, uint8_t, uint16_t> properties_get_bbs_version(
    properties::ptr_t props) {
  fpga_version version = props->bbs_version;
  return std::make_tuple(version.major, version.minor, version.patch);
}

void properties_set_bbs_version(properties::ptr_t props,
                                py::tuple bbs_version) {
  props->bbs_version = pytuple_to_fpga_version(bbs_version);
}

// vendor id
const char *properties_doc_vendor_id() {
  return R"opaedoc(
    Get or set the vendor ID  property of a resource.
    The vendor ID is part of the PCI ID and is assigned by the
    PCI SIG consortium.
   )opaedoc";
}

uint32_t properties_get_vendor_id(properties::ptr_t props) {
  return props->vendor_id;
}

void properties_set_vendor_id(properties::ptr_t props, uint32_t vendor_id) {
  props->vendor_id = vendor_id;
}

// device id
const char *properties_doc_device_id() {
  return R"opaedoc(
    Get or set the device ID  property of a resource.
    The device ID is part of the PCI ID and is assigned by the
    vendor.
   )opaedoc";
}

uint32_t properties_get_device_id(properties::ptr_t props) {
  return props->device_id;
}

void properties_set_device_id(properties::ptr_t props, uint32_t device_id) {
  props->device_id = device_id;
}

// model
const char *properties_doc_model() {
  return R"opaedoc(
    Get or set the model property of a resource.
   )opaedoc";
}

std::string properties_get_model(properties::ptr_t props) {
  return props->model;
}

void properties_set_model(properties::ptr_t props, char *model) {
  props->model = model;
}

// local memory size
const char *properties_doc_local_memory_size() {
  return R"opaedoc(
    Get or set the local memory size property of a resource.
   )opaedoc";
}

uint64_t properties_get_local_memory_size(properties::ptr_t props) {
  return props->local_memory_size;
}

void properties_set_local_memory_size(properties::ptr_t props, uint64_t size) {
  props->local_memory_size = size;
}

// capabilities
const char *properties_doc_capabilities() {
  return R"opaedoc(
    Get or set the capabilities property of a resource.
    This is taken directly from the capabilities CSR in the FIM.
   )opaedoc";
}

uint64_t properties_get_capabilities(properties::ptr_t props) {
  return props->capabilities;
}

void properties_set_capabilities(properties::ptr_t props, uint64_t caps) {
  props->capabilities = caps;
}

// num mmio
const char *properties_doc_num_mmio() {
  return R"opaedoc(
    Get or set the number of mmio spaces in a resource.
   )opaedoc";
}

uint32_t properties_get_num_mmio(properties::ptr_t props) {
  return props->num_mmio;
}

void properties_set_num_mmio(properties::ptr_t props, uint32_t num_mmio) {
  props->num_mmio = num_mmio;
}

// num interrupts
const char *properties_doc_num_interrupts() {
  return R"opaedoc(
    Get or set the number of interrupt vectors supported by a resource.
 )opaedoc";
}
uint32_t properties_get_num_interrupts(properties::ptr_t props) {
  return props->num_interrupts;
}
void properties_set_num_interrupts(properties::ptr_t props,
                                   uint32_t num_interrupts) {
  props->num_interrupts = num_interrupts;
}

// accelerator state
const char *properties_doc_accelerator_state() {
  return R"opaedoc(
    Get or set the state of an accelerator.
    The accelerator state is of type fpga_accelerator_state.
 )opaedoc";
}

fpga_accelerator_state properties_get_accelerator_state(
    properties::ptr_t props) {
  return props->accelerator_state;
}

void properties_set_accelerator_state(properties::ptr_t props,
                                      fpga_accelerator_state state) {
  props->accelerator_state = state;
}
