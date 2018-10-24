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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTOR."AS ."
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
#include "pysysobject.h"

using namespace opae::fpga::types;
namespace py = pybind11;

const char *sysobject_doc() {
  return R"doc(
    Wraps the OPAE fpga_object primitive as a Python object.
  )doc";
}

const char *sysobject_doc_token_get() {
  return R"doc(
    Get a sysobject instance from a valid token object.
  )doc";
}

const char *sysobject_doc_handle_get() {
  return R"doc(
    Get a sysobject instance from a valid handle object.
  )doc";
}

const char *sysobject_doc_object_get() {
  return R"doc(
    Get a sysobject instance from a valid sysobject.
    The parent sysobject must be a container type object.
  )doc";
}

const char *sysobject_doc_token_find() {
  return R"doc(
    Find a sysobject instance from a valid token object.
    Args:
      flags: Flags that control behavior of finding sub-objects.
      SYSOBJECT_GLOB is used to indicate that wildcard patterns (*) are allowed.
      SYSOBJECT_RECURSE_ONE is used to indicate that the find routine should recurse one level.
      SYSOBJECT_RECURSE_ALL is used to indicate that the find routine should recurse to all children.
  )doc";
}

const char *sysobject_doc_handle_find() {
  return R"doc(
    Find a sysobject instance from a valid handle object.
  )doc";
}

const char *sysobject_doc_object_find() {
  return R"doc(
    Find a sysobject instance from a valid sysobject.
    The parent sysobject must be a container type object.
  )doc";
}

const char *sysobject_doc_bytes() {
  return R"doc(
    Get bytes from the sysobject.
    Raises `RuntimeError` if the sysobject instance is a container type.
  )doc";
}

const char *sysobject_doc_getitem() {
  return R"doc(
    Get a byte from the sysobject at a given index.
    Raises `RuntimeError` if the sysobject instance is a container type.
  )doc";
}

const char *sysobject_doc_getslice() {
  return R"doc(
    Get a slice of bytes from the sysobject at a given offset.
    Raises `RuntimeError` if the sysobject instance is a container type.
  )doc";
}

sysobject::ptr_t token_get_sysobject(token::ptr_t tok, const std::string &name) {
  return sysobject::get(tok, name, 0);
}

sysobject::ptr_t handle_get_sysobject(handle::ptr_t h, const std::string &name) {
  return sysobject::get(h, name, 0);
}

sysobject::ptr_t sysobject_get_sysobject(sysobject::ptr_t o, const std::string &name) {
  return o->get(name, 0);
}

sysobject::ptr_t token_find_sysobject(token::ptr_t tok, const std::string &name,
                                     int flags) {
  return sysobject::get(tok, name, flags);
}

sysobject::ptr_t handle_find_sysobject(handle::ptr_t h, const std::string &name,
                                      int flags) {
  return sysobject::get(h, name, flags);
}

sysobject::ptr_t sysobject_find_sysobject(sysobject::ptr_t o, const std::string &name,
                                      int flags) {
  return o->get(name, flags);
}

std::string sysobject_bytes(sysobject::ptr_t obj) {
  auto bytes = obj->bytes(FPGA_OBJECT_SYNC);
  return std::string(bytes.begin(), bytes.end());
}

uint8_t sysobject_getitem(sysobject::ptr_t obj, uint32_t offset) {
  auto bytes = obj->bytes(offset, 1, FPGA_OBJECT_SYNC);
  return bytes[0];
}

std::string sysobject_getslice(sysobject::ptr_t obj, py::slice slice) {
  size_t start, stop, step, length;
  if (!slice.compute(obj->size(), &start, &stop, &step, &length))
    throw py::error_already_set();
  auto bytes = obj->bytes(FPGA_OBJECT_SYNC);
  std::string buf('\0', bytes.size());
  for (size_t i = start, j = 0; i < stop; i += step, ++j) {
    buf[j] = bytes[i];
  }
  return buf;
}
