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
#include <opae/cxx/core/errors.h>
#include <opae/cxx/core/events.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/sysobject.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>
#include <opae/fpga.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <atomic>
#include <exception>

namespace py = pybind11;
using opae::fpga::types::error;
using opae::fpga::types::event;
using opae::fpga::types::handle;
using opae::fpga::types::properties;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::sysobject;
using opae::fpga::types::token;
using opae::fpga::types::version;

class device {
 public:
  device() = delete;
  device(const device& other) {
    token_ = other.token_;
    handle_ = other.handle_;
  }

  device& operator=(const device& other) {
    if (this != &other) {
      token_ = other.token_;
      handle_ = other.handle_;
    }

    return *this;
  }

  static std::vector<device> enumerate() {
    std::vector<device> devices;
    for (auto t : token::enumerate({})) {
      devices.push_back(device(t));
    }
    return devices;
  }

 private:
  device(token::ptr_t t) : token_(t), handle_(nullptr){}
  token::ptr_t token_;
  handle::ptr_t handle_;
};

class region {
 public:
};

#ifdef OPAE_EMBEDDED
#include <pybind11/embed.h>
PYBIND11_EMBEDDED_MODULE(opae, m) {
  m.def("initialize", &fpgaInitialize);
#else
PYBIND11_MODULE(opae, m) {
  fpgaInitialize(nullptr);
#endif

  py::options opts;
  // opts.disable_function_signatures();

  m.doc() = "Open Programmable Acceleration Engine - Python bindings";

  m.def("devices", &device::enumerate, "get list of devices");
  py::class_<device> pydevice(m, "device", "device doc");

  py::class_<region> pyregion(m, "region", "region doc");
}
