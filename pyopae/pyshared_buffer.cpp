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
#include "pyshared_buffer.h"

namespace py = pybind11;
using opae::fpga::types::shared_buffer;

const char *shared_buffer_doc() {
  return R"opaedoc(
    shared_buffer represents a system memory buffer that can be shared with the accelerator.
    It implements the Python buffer protocol and can be converted to a native bytearray object.
  )opaedoc";
}

const char *shared_buffer_doc_allocate() {
  return R"opaedoc(
    shared_buffer factory method - allocate a shared buffer object.
    Args:
      handle: An accelerator handle object that identifies an open accelerator
      obect to share the buffer with.
      len: The length in bytes of the requested buffer.
  )opaedoc";
}

const char *shared_buffer_doc_size() {
  return R"opaedoc(
    Get the length of the buffer in bytes.
  )opaedoc";
}

const char *shared_buffer_doc_wsid() {
  return R"opaedoc(
    Get the underlying buffer's workspace ID.
  )opaedoc";
}

const char *shared_buffer_doc_iova() {
  return R"opaedoc(
    Get the address of the buffer suitable for programming into the
    accelerator device.
  )opaedoc";
}

const char *shared_buffer_doc_fill() {
  return R"opaedoc(
    Fill the buffer with a given value.

    Args:
      value: The value to use when filling the buffer.
  )opaedoc";
}

const char *shared_buffer_doc_compare() {
  return R"opaedoc(
    Compare this shared_buffer (the first len bytes)  object with another one.
    Returns 0 if the two buffers (up to len) are equal.
  )opaedoc";
}

const char *shared_buffer_doc_to_memoryview() {
  return R"opaedoc(
   Get a native Python memoryview object from this buffer.
  )opaedoc";
}

py::object shared_buffer_to_memoryview(shared_buffer::ptr_t buffer) {
  uint8_t *c_buffer = const_cast<uint8_t *>(buffer->c_type());
  return py::memoryview(py::buffer_info(c_buffer, buffer->size()));
}

const char *shared_buffer_doc_to_buffer() {
  return R"opaedoc(
   Get a native Python buffer object from this buffer.
  )opaedoc";
}

py::buffer_info shared_buffer_to_buffer(shared_buffer::ptr_t buffer) {
  uint8_t *c_buffer = const_cast<uint8_t *>(buffer->c_type());
  return py::buffer_info(c_buffer, buffer->size());
}
