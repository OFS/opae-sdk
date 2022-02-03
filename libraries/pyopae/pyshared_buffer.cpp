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
#include <opae/cxx/core/handle.h>
#include "pycontext.h"

namespace py = pybind11;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::handle;

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

shared_buffer::ptr_t shared_buffer_allocate(handle::ptr_t hndl, size_t size) {
  auto buf = shared_buffer::allocate(hndl, size);
  buffer_registry::instance().add_buffer(hndl, buf);
  return buf;
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

const char *shared_buffer_doc_io_address() {
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

const char *shared_buffer_doc_getitem() {
  return R"opaedoc(
    Get the byte at the given offset.
  )opaedoc";
}

uint8_t shared_buffer_getitem(shared_buffer::ptr_t buf, uint32_t offset) {
  return *(buf->c_type() + offset);
}

const char *shared_buffer_doc_setitem() {
  return R"opaedoc(
    Set the bytes at the given offset using all bytes in the argument.
  )opaedoc";
}

void shared_buffer_setitem(opae::fpga::types::shared_buffer::ptr_t buf,
                           uint32_t offset, pybind11::int_ item) {
  int *ptr =
      reinterpret_cast<int *>(const_cast<uint8_t *>(buf->c_type() + offset));
  *ptr = item.cast<int>();
}

const char *shared_buffer_doc_getslice() {
  return R"opaedoc(
    Get a slice of the bytes as determined by the slice arguments ([start:stop:step])
    Args:
      start: start offset of buffer
      stop: end offset of the buffer (not incusive)
      step: step offset

    NOTE: This current implementation copies the data into a new list.
    )opaedoc";
}

py::list shared_buffer_getslice(shared_buffer::ptr_t buf, py::slice slice) {
  size_t start, stop, step, length;
  if (!slice.compute(buf->size(), &start, &stop, &step, &length))
    throw py::error_already_set();
  py::list list;
  for (size_t i = start; i < stop; i += step) {
    list.append(*(buf->c_type() + i));
  }
  return list;
}

const char *shared_buffer_doc_read32() {
  return R"opaedoc(
    Cast the memory at the given offset into a 32-bit integer
  )opaedoc";
}

const char *shared_buffer_doc_read64() {
  return R"opaedoc(
    Cast the memory at the given offset into a 64-bit integer
  )opaedoc";
}

const char *shared_buffer_doc_write32() {
  return R"opaedoc(
    Write a 32-bit integer at the memory at the given offset
  )opaedoc";
}

const char *shared_buffer_doc_write64() {
  return R"opaedoc(
    Write a 64-bit integer at the memory at the given offset
  )opaedoc";
}

const char *shared_buffer_doc_copy() {
  return R"opaedoc(
    Copy the given number of bytes from the current buffer to the buffer in the argument.
  )opaedoc";
}

void shared_buffer_copy(shared_buffer::ptr_t self, shared_buffer::ptr_t other,
                        size_t size) {
  uint8_t *src = const_cast<uint8_t *>(self->c_type());
  uint8_t *dst = const_cast<uint8_t *>(other->c_type());

  std::copy(src, src + (size ? size : self->size()), dst);
}

const char *shared_buffer_doc_split() {
  return R"opaedoc(
    Split the buffer into other shared_buffer objects.
    The arguments to this method make up a list of sizes to use when splitting the buffer.
    For example, say a shared_buffer object is 1024 bytes and split is called with sizes
    256, 256, 512 then the result is a list of shared_buffer objects with those sizes
    respectively.
  )opaedoc";
}

class split_buffer : public shared_buffer {
 public:
  typedef std::shared_ptr<split_buffer> ptr_t;
  split_buffer(const split_buffer &) = delete;
  split_buffer &operator=(const split_buffer &) = delete;

  split_buffer(shared_buffer::ptr_t parent, size_t len, uint8_t *virt,
               uint64_t wsid, uint64_t io_address)
      : shared_buffer(nullptr, len, virt, wsid, io_address), parent_(parent) {}

  virtual ~split_buffer() { parent_.reset(); }

 private:
  shared_buffer::ptr_t parent_;
};

std::vector<shared_buffer::ptr_t> shared_buffer_split(shared_buffer::ptr_t buf,
                                                      py::args args) {
  std::vector<shared_buffer::ptr_t> buffers;
  if (!args || py::len(args) == 1) {
    buffers.push_back(buf);
  } else {
    size_t offset = 0;
    uint8_t *virt = const_cast<uint8_t *>(buf->c_type());
    auto wsid = buf->wsid();
    auto io_address = buf->io_address();
    for (auto a : args) {
      auto len = a.cast<size_t>();
      if (offset + len > buf->size()) {
        throw std::invalid_argument("buffer not big enough to split this way");
      }
      buffers.push_back(
          std::make_shared<split_buffer>(buf, len, virt, wsid, io_address));
      offset += len;
      io_address += len;
      virt += len;
    }
  }
  return buffers;
}
