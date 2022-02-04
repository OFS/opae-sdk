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
#include <Python.h>
#include "pyhandle.h"
#include "pycontext.h"
#include <sstream>

namespace py = pybind11;
using opae::fpga::types::handle;
using opae::fpga::types::token;

const char *handle_doc_open() {
  return R"opaedoc(
    Create a new handle object from a token.
  )opaedoc";
}

handle::ptr_t handle_open(token::ptr_t tok, int flags) {
  return handle::open(tok, flags);
}

const char *handle_doc_reconfigure() {
  return R"opaedoc(
    Reconfigure an accelerator resource. By default, an attempt will be
    made to open the accelerator in exclusive mode which will result in
    an exception being thrown if that accelerator is currently in use.
    Use FPGA_RECONF_FORCE to bypass this behavior and program the GBS.
    Args:
      slot: The slot number to program.
      fd(file): The file object obtained by openeing the GBS file.
      flags: Flags that control behavior of reconfiguration.
             Value of 0 indicates no flags. FPGA_RECONF_FORCE indicates
             that the bitstream be programmed into the slot without
             checking if the resource is currently in use.

  )opaedoc";
}

void handle_reconfigure(handle::ptr_t handle, uint32_t slot, py::object file,
                        int flags) {
  PyObject *obj = file.ptr();
#if PY_MAJOR_VERSION == 3
  int fd = PyObject_AsFileDescriptor(obj);
  FILE *fp = fdopen(fd, "r");
#else
  if (!PyFile_Check(obj)) {
    throw std::invalid_argument("fd argument is not a file object");
  }
  FILE *fp = PyFile_AsFile(obj);
#endif
  if (!fp) {
    throw std::runtime_error("could not convert fd to FILE*");
  }
  // PyFile_IncUseCount(obj);
  // is fd object already holding a reference count while in this function?
  fseek(fp, 0L, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  std::vector<char> buffer(size);
  if (!fread(buffer.data(), size, 1, fp)) {
    fclose(fp);
    throw std::runtime_error("error reading from file object");
  }
  fclose(fp);
  handle->reconfigure(slot, reinterpret_cast<const uint8_t *>(buffer.data()),
                      size, flags);
}

const char *handle_doc_valid() {
  return R"opaedoc(
    "Boolean protocol to test if a handle is open or not."
  )opaedoc";
}

bool handle_valid(opae::fpga::types::handle::ptr_t handle) {
  return *handle != nullptr;
}

const char *handle_doc_context_enter() {
  return R"opaedoc(
    Context manager protocol enter function.
    Simply returns the handle object.
  )opaedoc";
}

handle::ptr_t handle_context_enter(handle::ptr_t hnd) {
  buffer_registry::instance().register_handle(hnd);
  return hnd;
}

const char *handle_doc_context_exit() {
  return R"opaedoc(
    Context manager protocol exit function.
    Closes the resource identified by this handle and currently does nothing with the exit arguments.
  )opaedoc";
}

void handle_context_exit(opae::fpga::types::handle::ptr_t hnd, py::args args) {
  // TODO: Use args for logging exceptions
  (void)args;
  buffer_registry::instance().unregister_handle(hnd);
  hnd->close();
}

const char *handle_doc_close() {
  return R"opaedoc(
    "Close an accelerator associated with handle."
  )opaedoc";
}

const char *handle_doc_reset() {
  return R"opaedoc(
    Reset the accelerator associated with this handle.
    The accelerator must be opened.
  )opaedoc";
}

const char *handle_doc_read_csr32() {
  return R"opaedoc(
    Read 32 bits from a CSR belonging to a resource associated with a handle.
    Args:
      offset: The register offset.
      csr_space: The CSR space to read from. Default is 0.
  )opaedoc";
}

const char *handle_doc_read_csr64() {
  return R"opaedoc(
    Read 64 bits from a CSR belonging to a resource associated with a handle.
    Args:
      offset: The register offset.
      csr_space: The CSR space to read from. Default is 0.
  )opaedoc";
}

const char *handle_doc_write_csr32() {
  return R"opaedoc(
    Write 32 bits to a CSR belonging to a resource associated with a handle.
    Args:
      offset: The register offset.
      value: The 32-bit value to write to the register.
      csr_space: The CSR space to write from. Default is 0.
  )opaedoc";
}

const char *handle_doc_write_csr64() {
  return R"opaedoc(
    Write 64 bits to a CSR belonging to a resource associated with a handle.
    Args:
      offset: The register offset.
      value: The 64-bit value to write to the register.
      csr_space: The CSR space to write from. Default is 0.
  )opaedoc";
}
