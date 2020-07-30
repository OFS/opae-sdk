// Copyright(c) 2020, Intel Corporation
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
#include <poll.h>
#include <regex.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <opae/cxx/core/events.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/shared_buffer.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/version.h>

const char *sbdf_pattern =
  "(([0-9a-fA-F]{4}):)?([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])";

enum {
  MATCHES_SIZE = 6
};


namespace opae {
namespace app {

namespace fpga = fpga::types;

template<typename T>
inline bool parse_match_int(const char *s,
                            regmatch_t m, T &v, int radix=10)
{
  if (m.rm_so == -1 || m.rm_eo == -1) return false;
  errno = 0;
  v = std::strtoul(s + m.rm_so, NULL, radix);
  return errno == 0;
}

union pcie_address {
  struct {
    uint32_t function: 3;
    uint32_t device: 5;
    uint32_t bus : 8;
    uint32_t domain : 16;
  } fields;
  uint32_t value;

  static pcie_address parse(const char *s)
  {
    regex_t re;
    regmatch_t matches[MATCHES_SIZE];

    int reg_res = regcomp(&re, sbdf_pattern, REG_EXTENDED | REG_ICASE);
    if (reg_res) {
      throw std::runtime_error("could not compile regex");
    }
    reg_res = regexec(&re, s, MATCHES_SIZE, matches, 0);
    uint16_t domain, bus, device, function;
    if (!parse_match_int(s, matches[2], domain, 16))
      domain = 0;
    if (!parse_match_int(s, matches[3], bus, 16))
      throw std::runtime_error("error parsing pcie address");
    if (!parse_match_int(s, matches[4], device, 16))
      throw std::runtime_error("error parsing pcie address");
    if (!parse_match_int(s, matches[5], function))
      throw std::runtime_error("error parsing; pcie address");
    pcie_address a;
    a.fields.domain=domain;
    a.fields.bus=bus;
    a.fields.device=device;
    a.fields.function=function;
    return a;
  }

};

class test_afu;
class test_command {
public:
  typedef std::shared_ptr<test_command> ptr_t;
  test_command(){}
  virtual ~test_command(){}
  virtual const char *name() const = 0;
  virtual const char *description() const = 0;
  virtual int run(test_afu *afu, CLI::App *app) = 0;
  virtual void add_options(CLI::App *app)
  {
    (void)app;
  }
private:

};

class test_afu {
public:
  typedef int (*command_fn)(test_afu *afu, CLI::App *app);
  enum exit_codes {
    success = 0,
    not_run,
    not_found,
    no_access,
    exception,
    error
  };

  test_afu(const char *name, const char *afu_id)
  : app_(name)
  , afu_id_(afu_id)
  , pci_addr_("")
  , handle_(nullptr)
  , shared_(false)
  {
    app_.add_option("-g,--guid", afu_id_, "GUID")->default_val(afu_id_);
    app_.add_option("-p,--pci-address", pci_addr_,
                    "[<domain>:]<bus>:<device>.<function>");
    app_.add_flag("-s,--shared", shared_, "open in shared mode, default is off");
  }
  virtual ~test_afu(){}

  int open_handle() {
    auto filter = fpga::properties::get();
    filter->type = FPGA_ACCELERATOR;
    filter->guid.parse(afu_id_.c_str());
    if (!pci_addr_.empty()) {
      auto p = pcie_address::parse(pci_addr_.c_str());
      filter->segment = p.fields.domain;
      filter->bus = p.fields.bus;
      filter->device = p.fields.device;
      filter->function = p.fields.function;
    }

    auto tokens = fpga::token::enumerate({filter});
    if (tokens.size() < 1) {
      return exit_codes::not_found;
    }
    if (tokens.size() > 1) {
      std::cerr << "more than one accelerator found matching filter\n";
    }
    int flags = shared_ ? FPGA_OPEN_SHARED : 0;
    try {
      handle_ = fpga::handle::open(tokens[0], flags);
    } catch (fpga::no_access &err) {
      std::cerr << err.what() << "\n";
      return exit_codes::no_access;
    }
    return exit_codes::not_run;
  }



  int main(int argc, char *argv[])
  {
    app_.require_subcommand();
    CLI11_PARSE(app_, argc, argv);
    int res = open_handle();
    if (res != exit_codes::not_run) {
      return res;
    }

    test_command::ptr_t test(nullptr);
    CLI::App *app = nullptr;
    for (auto kv : commands_) {
      if (*kv.first) {
        app = kv.first;
        test = kv.second;
        break;
      }
    }
    if (!test) {
      std::cerr << "no command specified\n";
      return exit_codes::not_run;
    }
    try {
      res = test->run(this, app);
    } catch(std::exception &ex) {
      std::cerr << "error running command "
                << test->name()
                << ": " << ex.what() << "\n";
      res = exit_codes::exception;
    }
    auto pass = res == exit_codes::success ? "PASS" : "FAIL";
    std::cout << "Test " << test->name() << ": "
              << pass << "\n";
    return res;
  }

  template<class T>
  CLI::App *register_command()
  {
    test_command::ptr_t cmd(new T());
    auto sub = app_.add_subcommand(cmd->name(),
                                   cmd->description());
    cmd->add_options(sub);
    commands_[sub] = cmd;
    return sub;
  }

  template<typename T>
  inline T read(uint32_t offset) const {
    return *reinterpret_cast<T*>(handle_->mmio_ptr(offset));
  }

  template<typename T>
  inline void write(uint32_t offset, T value) const {
    *reinterpret_cast<T*>(handle_->mmio_ptr(offset)) = value;
  }

  template<typename T>
  T read(uint32_t offset, uint32_t i) const {
    return read<T>(get_offset(offset, i));
  }

  template<typename T>
  void write(uint32_t offset, uint32_t i, T value) const {
    write<T>(get_offset(offset, i), value);
  }

  uint64_t read64(uint32_t offset) const {
    return handle_->read_csr64(offset);
  }

  void write64(uint32_t offset, uint64_t value) const {
    return handle_->write_csr64(offset, value);
  }

  uint32_t read32(uint32_t offset) const {
    return handle_->read_csr32(offset);
  }

  void write32(uint32_t offset, uint32_t value) const {
    return handle_->write_csr32(offset, value);
  }

  uint64_t read64(uint32_t offset, uint32_t i) const {
    return read64(get_offset(offset, i));
  }

  void write64(uint32_t offset, uint32_t i, uint64_t value) const {
    write64(get_offset(offset, i), value);
  }

  fpga::shared_buffer::ptr_t allocate(size_t size)
  {
    return fpga::shared_buffer::allocate(handle_, size);
  }

  void fill(fpga::shared_buffer::ptr_t buffer)
  {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(1, 4096);
    auto sz = sizeof(uint32_t);
    for (uint32_t i = 0; i < buffer->size()/sz; i+=sz){
      buffer->write<uint32_t>(dist(mt), i);
    }

  }

  void fill(fpga::shared_buffer::ptr_t buffer, uint32_t value)
  {
    buffer->fill(value);
  }

  fpga::event::ptr_t register_interrupt()
  {
    auto event = fpga::event::register_event(handle_, FPGA_EVENT_INTERRUPT);
    return event;
  }

  void interrupt_wait(fpga::event::ptr_t event, int timeout=-1)
  {
    struct pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = event->os_object();
    auto ret = poll(&pfd, 1, timeout);

    if (ret < 0)
      throw std::runtime_error(strerror(errno));
    if (ret == 0)
      throw std::runtime_error("timeout error");
    std::cout << "got interrupt\n";

  }

  void compare(fpga::shared_buffer::ptr_t b1,
               fpga::shared_buffer::ptr_t b2, uint32_t count = 0)
  {
      if (b1->compare(b2, count ? count : b1->size())) {
        throw std::runtime_error("buffers mismatch");
      }

  }

  template<class T>
  T read_register()
  {
    return *reinterpret_cast<T*>(handle_->mmio_ptr(T::offset));
  }

  template<class T>
  volatile T* register_ptr(uint32_t offset)
  {
    return reinterpret_cast<volatile T*>(handle_->mmio_ptr(offset));
  }

  template<class T>
  void write_register(uint32_t offset, T* reg)
  {
    return *reinterpret_cast<T*>(handle_->mmio_ptr(offset)) = *reg;
  }

protected:
private:
  CLI::App app_;
  std::string afu_id_;
  std::string pci_addr_;
  fpga::handle::ptr_t handle_;
  bool shared_;
  std::map<CLI::App*, test_command::ptr_t> commands_;
  uint32_t max_;
  std::map<uint32_t, uint32_t> limits_;

  uint32_t get_offset(uint32_t base, uint32_t i) const {
    auto limit = limits_.find(base);
    auto offset = base + sizeof(uint64_t)*i;
    if (limit != limits_.end() &&
        offset > limit->second - sizeof(uint64_t)) {
      throw std::out_of_range("offset out range in csr space");
    }
    return offset;
  }



};

}
}
