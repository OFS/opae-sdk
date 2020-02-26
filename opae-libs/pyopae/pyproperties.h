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
#pragma once
#include <Python.h>

#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/pvalue.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>
#include <pybind11/pybind11.h>
#include <tuple>

template <typename T>
static inline void kwargs_to_props(opae::fpga::types::pvalue<T> &prop,
                                   pybind11::kwargs kwargs, const char *key) {
  if (kwargs.contains(key)) {
    prop = kwargs[key].cast<T>();
  }
}

const char *properties_doc();
const char *properties_doc_get();
opae::fpga::types::properties::ptr_t properties_get(pybind11::kwargs kwargs);

const char *properties_doc_get_token();
opae::fpga::types::properties::ptr_t properties_get_token(
    opae::fpga::types::token::ptr_t tok);

const char *properties_doc_get_handle();
opae::fpga::types::properties::ptr_t properties_get_handle(
    opae::fpga::types::handle::ptr_t hndl);

const char *properties_doc_parent();
opae::fpga::types::token::ptr_t properties_get_parent(
    opae::fpga::types::properties::ptr_t props);
void properties_set_parent(opae::fpga::types::properties::ptr_t props,
                           opae::fpga::types::token::ptr_t parent);

const char *properties_doc_guid();
std::string properties_get_guid(opae::fpga::types::properties::ptr_t props);
void properties_set_guid(opae::fpga::types::properties::ptr_t props,
                         const std::string &guid_str);

const char *properties_doc_type();
fpga_objtype properties_get_type(opae::fpga::types::properties::ptr_t props);
void properties_set_type(opae::fpga::types::properties::ptr_t props,
                         fpga_objtype type);

const char *properties_doc_segment();
uint16_t properties_get_segment(opae::fpga::types::properties::ptr_t props);
void properties_set_segment(opae::fpga::types::properties::ptr_t props,
                            uint16_t segment);

const char *properties_doc_bus();
uint8_t properties_get_bus(opae::fpga::types::properties::ptr_t props);
void properties_set_bus(opae::fpga::types::properties::ptr_t props,
                        uint8_t bus);

const char *properties_doc_device();
uint8_t properties_get_device(opae::fpga::types::properties::ptr_t props);
void properties_set_device(opae::fpga::types::properties::ptr_t props,
                           uint8_t device);

const char *properties_doc_function();
uint8_t properties_get_function(opae::fpga::types::properties::ptr_t props);
void properties_set_function(opae::fpga::types::properties::ptr_t props,
                             uint8_t function);

const char *properties_doc_socket_id();
uint8_t properties_get_socket_id(opae::fpga::types::properties::ptr_t props);
void properties_set_socket_id(opae::fpga::types::properties::ptr_t props,
                              uint8_t socket_id);

const char *properties_doc_object_id();
uint64_t properties_get_object_id(opae::fpga::types::properties::ptr_t props);
void properties_set_object_id(opae::fpga::types::properties::ptr_t props,
                              uint64_t object_id);

const char *properties_doc_num_errors();
uint32_t properties_get_num_errors(opae::fpga::types::properties::ptr_t props);
void properties_set_num_errors(opae::fpga::types::properties::ptr_t props,
                               uint32_t num_errors);

const char *properties_doc_num_slots();
uint32_t properties_get_num_slots(opae::fpga::types::properties::ptr_t props);
void properties_set_num_slots(opae::fpga::types::properties::ptr_t props,
                              uint32_t num_slots);

const char *properties_doc_bbs_id();
uint64_t properties_get_bbs_id(opae::fpga::types::properties::ptr_t props);
void properties_set_bbs_id(opae::fpga::types::properties::ptr_t props,
                           uint64_t bbs_id);

const char *properties_doc_bbs_version();
std::tuple<uint8_t, uint8_t, uint16_t> properties_get_bbs_version(
    opae::fpga::types::properties::ptr_t props);
void properties_set_bbs_version(opae::fpga::types::properties::ptr_t props,
                                pybind11::tuple bbs_version);

const char *properties_doc_vendor_id();
uint32_t properties_get_vendor_id(opae::fpga::types::properties::ptr_t props);
void properties_set_vendor_id(opae::fpga::types::properties::ptr_t props,
                              uint32_t vendor_id);

const char *properties_doc_device_id();
uint32_t properties_get_device_id(opae::fpga::types::properties::ptr_t props);
void properties_set_device_id(opae::fpga::types::properties::ptr_t props,
                              uint32_t device_id);

const char *properties_doc_model();
std::string properties_get_model(opae::fpga::types::properties::ptr_t props);
void properties_set_model(opae::fpga::types::properties::ptr_t props,
                          char *model);

const char *properties_doc_local_memory_size();
uint64_t properties_get_local_memory_size(
    opae::fpga::types::properties::ptr_t props);
void properties_set_local_memory_size(
    opae::fpga::types::properties::ptr_t props, uint64_t size);

const char *properties_doc_capabilities();
uint64_t properties_get_capabilities(
    opae::fpga::types::properties::ptr_t props);
void properties_set_capabilities(opae::fpga::types::properties::ptr_t props,
                                 uint64_t caps);

const char *properties_doc_num_mmio();
uint32_t properties_get_num_mmio(opae::fpga::types::properties::ptr_t props);
void properties_set_num_mmio(opae::fpga::types::properties::ptr_t props,
                             uint32_t num_mmio);

const char *properties_doc_num_interrupts();
uint32_t properties_get_num_interrupts(
    opae::fpga::types::properties::ptr_t props);
void properties_set_num_interrupts(opae::fpga::types::properties::ptr_t props,
                                   uint32_t num_interrupts);

const char *properties_doc_accelerator_state();
fpga_accelerator_state properties_get_accelerator_state(
    opae::fpga::types::properties::ptr_t props);
void properties_set_accelerator_state(
    opae::fpga::types::properties::ptr_t props, fpga_accelerator_state state);
