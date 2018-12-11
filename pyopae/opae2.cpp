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
#include <sstream>

namespace py = pybind11;
using opae::fpga::types::error;
using opae::fpga::types::event;
using opae::fpga::types::handle;
using opae::fpga::types::properties;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::sysobject;
using opae::fpga::types::token;
using opae::fpga::types::version;

template <typename T>
void unparse_guid(const T &guid, std::string &id) {
  if (id.empty()) {
    std::stringstream ss;
    ss << guid;
    id = ss.str();
  }
}

union pfid_u {
  struct {
    uint32_t domain : 16;
    uint32_t bus : 8;
    uint32_t device : 5;
    uint32_t function : 3;
  } u;
  uint32_t value;
};

class device {
 public:
  typedef std::shared_ptr<device> ptr_t;
  device() = delete;
  device(token::ptr_t t) : token_(t), handle_(nullptr) {
    auto props = properties::get(t);
    pfid_u pfid{props->segment, props->bus, props->device, props->function};
    pfid_ = pfid.value;
    device_id_ = props->device_id;
    vendor_id_ = props->vendor_id;
  }

  device(const device &other) = delete;

  device &operator=(const device &other) = delete;

  static std::vector<device::ptr_t> enumerate() {
    std::vector<device::ptr_t> devices;
    auto p = properties::get(FPGA_DEVICE);
    for (auto t : token::enumerate({p})) {
      devices.push_back(std::make_shared<device>(t));
    }
    return devices;
  }
  uint32_t get_pfid() const { return pfid_; }
  uint32_t get_device_id() const { return device_id_; }
  uint32_t get_vendor_id() const { return vendor_id_; }

 private:
  token::ptr_t token_;
  handle::ptr_t handle_;
  uint32_t pfid_;
  uint32_t device_id_;
  uint32_t vendor_id_;
};

class buffer {
 public:
  typedef std::shared_ptr<buffer> ptr_t;
  buffer() = delete;
  buffer(shared_buffer::ptr_t b, handle::ptr_t h) : buffer_(b), handle_(h), released_(false) {}

  buffer(const buffer &other) = delete;
  buffer &operator=(const buffer &other) = delete;

  ~buffer() {
    release();
  }

  void release() {
    if (!released_) {
        buffer_->release();
        buffer_.reset();
        handle_.reset();
        released_ = true;
    }
  }

  int64_t poll(uint64_t offset, uint64_t mask, uint64_t value, uint64_t timeout_usec) {
    using hrc = std::chrono::high_resolution_clock;
    auto ptr = buffer_->c_type();
    auto begin = hrc::now();
    std::chrono::nanoseconds elapsed(0);
    std::chrono::nanoseconds timeout(timeout_usec*1000);
    if (!mask) {
      mask = ~mask;
    }

    while ((*reinterpret_cast<volatile uint64_t *>(ptr + offset) & mask) != value) {
      elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        hrc::now() - begin);
      if (elapsed >= timeout) {
        return -1;
      }
    }
    return elapsed.count();
 }



 private:
  shared_buffer::ptr_t buffer_;
  handle::ptr_t handle_;
  bool released_;
};

class accelerator {
 public:
  typedef std::shared_ptr<accelerator> ptr_t;
  accelerator() = delete;
  accelerator(token::ptr_t t) : token_(t), handle_(nullptr) {
    unparse_guid(properties::get(t)->guid, id_);
  }

  accelerator(const accelerator &other) = delete;
  accelerator &operator=(const accelerator &other) = delete;
  // accelerator(const accelerator &other) {
  //   token_ = other.token_;
  //   handle_ = other.handle_;
  // }

  // accelerator &operator=(const accelerator &other) {
  //   if (this != &other) {
  //     token_ = other.token_;
  //     handle_ = other.handle_;
  //   }

  //   return *this;
  // }

  // static std::vector<accelerator> enumerate() {
  //   std::vector<accelerator> accelerators;
  //   auto p = properties::get(FPGA_DEVICE);
  //   for (auto t : token::enumerate({p})) {
  //     accelerators.push_back(accelerator(t));
  //   }
  //   return accelerators;
  // }

  void open() {
    handle_ = handle::open(token_, FPGA_OPEN_SHARED);
  }

  void assert_handle() {
    if (!handle_)
      throw std::domain_error("I/O Operation on closed accelerator");
  }

  uint32_t read_csr32(uint64_t addr) {
    assert_handle();
    return handle_->read_csr32(addr);
  }

  uint64_t read_csr64(uint64_t addr) {
    assert_handle();
    return handle_->read_csr64(addr);
  }

  void write_csr32(uint64_t addr, uint32_t value) {
    assert_handle();
    handle_->write_csr32(addr, value);
  }

  void write_csr64(uint64_t addr, uint64_t value) {
    assert_handle();
    handle_->write_csr64(addr, value);
  }

  std::string get_id() { return id_; }

  int64_t buffer_poll(buffer::ptr_t buf, uint64_t offset, uint64_t mask, uint64_t value, uint64_t timeout_usec) {
    return buf->poll(offset, mask, value, timeout_usec);
  }

  accelerator &enter() {
    open();
    return *this;
  }

  void exit(py::args args) {
    (void)args;
    for (auto b : buffers_) {
      b->release();
    }
    buffers_.clear();
    handle_->close();
    handle_.reset();
  }

  buffer::ptr_t allocate_buffer(size_t size, uint64_t reg_addr = 0,
                         bool cl_align = false) {
    assert_handle();
    auto buf = shared_buffer::allocate(handle_, size);
    if (!buf) {
      throw std::bad_alloc();
    }
    if (reg_addr) {
      auto io_addr = buf->io_address();
      if (cl_align) {
        io_addr = io_addr >> 6;
      }
      handle_->write_csr64(reg_addr, io_addr);
    }
    buffers_.push_back(std::make_shared<buffer>(buf, handle_));
    return buffers_.back();
  }
  
  void run(py::object func, py::args args, py::kwargs kwargs) {
    func(this, *args, **kwargs);
  }

 private:
  token::ptr_t token_;
  handle::ptr_t handle_;
  std::string id_;
  std::vector<buffer::ptr_t> buffers_;
};

class region {
 public:
  region() = delete;
  region(token::ptr_t d, token::ptr_t a, uint32_t slot)
      : device_token_(d), accelerator_token_(a), id_(), slot_(slot) {
    device_props_ = properties::get(device_token_);
    unparse_guid(device_props_->guid, id_);
  }
  region(const region &other) {
    device_token_ = other.device_token_;
    accelerator_token_ = other.accelerator_token_;
    device_props_ = other.device_props_;
    id_ = other.id_;
    slot_ = other.slot_;
  }

  region &operator=(const region &other) {
    if (this != &other) {
      device_token_ = other.device_token_;
      accelerator_token_ = other.accelerator_token_;
      device_props_ = other.device_props_;
      id_ = other.id_;
      slot_ = other.slot_;
    }

    return *this;
  }

  static std::vector<region> enumerate(py::kwargs kwargs) {
    std::vector<region> regions;
    auto dev_props = properties::get(FPGA_DEVICE);
    if (kwargs.contains("type_id")) {
        dev_props->guid.parse(kwargs["type_id"].cast<std::string>().c_str());
    }

    if (kwargs.contains("device_id")) {
        dev_props->device_id = kwargs["device_id"].cast<uint32_t>();
    }
    if (kwargs.contains("vendor_id")) {
        dev_props->vendor_id = kwargs["vendor_id"].cast<uint32_t>();
    }

    auto device_tokens = token::enumerate({dev_props});
    for (auto t : device_tokens) {
      auto tok_props = properties::get(t);
      auto acc_props = properties::get(FPGA_ACCELERATOR);
      acc_props->parent = *t;
      if (kwargs.contains("function_id")) {
          acc_props->guid.parse(kwargs["function_id"].cast<std::string>().c_str());
      }
      auto acc_tokens = token::enumerate({acc_props});
      auto num_slots = static_cast<uint32_t>(tok_props->num_slots);
      uint32_t slot = 0;
      for (auto at : acc_tokens) {
        regions.push_back(region(t, at, slot++));
        --num_slots;
      }
      if (!kwargs) {
        for (int i = 0; i < num_slots; i++) {
          regions.push_back(region(t, nullptr, slot++));
        }
      }
    }
    if (kwargs) {

    }
    return regions;
  }

  device::ptr_t get_device() { return std::make_shared<device>(device_token_); }

  void program(py::object file) {
    int flags = 0;
    auto handle = handle::open(device_token_, 0);
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
      throw std::runtime_error("error reading from file object");
    }
    handle->reconfigure(slot_, reinterpret_cast<const uint8_t *>(buffer.data()),
                        size, flags);
    // update our internal accelerator token
    // TODO: Use slot id to get the right token
    auto acc_props = properties::get(FPGA_ACCELERATOR);
    acc_props->parent = *device_token_;
    auto acc_tokens = token::enumerate({acc_props});
    if (acc_tokens.size() == 1) {
        accelerator_token_ = acc_tokens[0];
    }

  }

  std::string get_id() { return id_; }

  accelerator::ptr_t get_accelerator() {
    return std::make_shared<accelerator>(accelerator_token_);
  }



 private:
  token::ptr_t device_token_;
  token::ptr_t accelerator_token_;
  properties::ptr_t device_props_;
  std::string id_;
  uint32_t slot_;
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
  py::class_<device, device::ptr_t> pydevice(m, "device", "device doc");
  pydevice.def_property_readonly("pfid", &device::get_pfid)
          .def_property_readonly("device_id", &device::get_device_id)
          .def_property_readonly("vendor_id", &device::get_vendor_id);

  m.def("regions", &region::enumerate, "get list of regions");
  py::class_<region> pyregion(m, "region", "region doc");
  pyregion.def("type_id", &region::get_id, "get the region type id")
      .def_property_readonly("function", &region::get_accelerator,
           "get the accelerator function in the region")
      .def_property_readonly("device", &region::get_device)
      .def("program", &region::program);

  py::class_<accelerator, accelerator::ptr_t> pyaccelerator(m, "accelerator", "accelerator doc");
  pyaccelerator.def("function_id", &accelerator::get_id, "")
      .def("run", &accelerator::run, "")
      .def("read_csr32", &accelerator::read_csr32, "")
      .def("read_csr64", &accelerator::read_csr64, "")
      .def("write_csr32", &accelerator::write_csr32, "")
      .def("write_csr64", &accelerator::write_csr64, "")
      .def("buffer", &accelerator::allocate_buffer, "", py::arg("size"), py::arg("register_addr") = 0, py::arg("cl_align") = false)
      .def("poll", &accelerator::buffer_poll, "", py::arg("buffer"), py::arg("offset"), py::arg("mask"), py::arg("value"), py::arg("timeout_usec") = 100)
      .def("__enter__", &accelerator::enter, "")
      .def("__exit__", &accelerator::exit, "");

  py::class_<buffer, buffer::ptr_t> pybuffer(m, "buffer", "buffer doc");

}
