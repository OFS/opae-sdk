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
#include <chrono>
#include <future>
#include <random>
#include <thread>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <opae/cxx/core.h>

const char *sbdf_pattern =
  "(([0-9a-fA-F]{4}):)?([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])";

enum {
  MATCHES_SIZE = 6
};


namespace opae {
namespace afu_test {

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
    if (reg_res)
      throw std::runtime_error("could not compile regex");

    reg_res = regexec(&re, s, MATCHES_SIZE, matches, 0);
    if (reg_res)
      throw std::runtime_error("pcie address not valid format");

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

class afu; // forward declaration

class command {
public:
  typedef std::shared_ptr<command> ptr_t;
  command(){}
  virtual ~command(){}
  virtual const char *name() const = 0;
  virtual const char *description() const = 0;
  virtual int run(afu *afu, CLI::App *app) = 0;
  virtual void add_options(CLI::App *app)
  {
    (void)app;
  }
  virtual const char *afu_id() const
  {
    return nullptr;
  }

private:

};

class afu {
public:
  typedef int (*command_fn)(afu *afu, CLI::App *app);
  enum exit_codes {
    success = 0,
    not_run,
    not_found,
    no_access,
    exception,
    error
  };

  afu(const char *name, const char *afu_id = nullptr)
  : name_(name)
  , afu_id_(afu_id ? afu_id : "")
  , app_(name_)
  , pci_addr_("")
  , log_level_("info")
  , shared_(false)
  , timeout_msec_(60000)
  , handle_(nullptr)
  , current_command_(nullptr)
  {
    if (!afu_id_.empty())
      app_.add_option("-g,--guid", afu_id_, "GUID")->default_val(afu_id_);
    app_.add_option("-p,--pci-address", pci_addr_,
                    "[<domain>:]<bus>:<device>.<function>");
    app_.add_option("-l,--log-level", log_level_, "stdout logging level")->
      default_val(log_level_)->
      check(CLI::IsMember(SPDLOG_LEVEL_NAMES));
    app_.add_flag("-s,--shared", shared_, "open in shared mode, default is off");
    app_.add_option("-t,--timeout", timeout_msec_, "test timeout (msec)")->default_val(timeout_msec_);
  }
  virtual ~afu() {}

  CLI::App & cli() { return app_; }

  int open_handle(const char *afu_id) {
    auto filter = fpga::properties::get();
    auto app_afu_id = afu_id ? afu_id : afu_id_.c_str();
    filter->type = FPGA_ACCELERATOR;
    try {
      filter->guid.parse(app_afu_id);
    } catch(opae::fpga::types::except & err) {
      return error;
    }
    if (!pci_addr_.empty()) {
      auto p = pcie_address::parse(pci_addr_.c_str());
      filter->segment = p.fields.domain;
      filter->bus = p.fields.bus;
      filter->device = p.fields.device;
      filter->function = p.fields.function;
    }

    auto tokens = fpga::token::enumerate({filter});
    if (tokens.size() < 1) {
      std::cerr << "no accelerator found with id: " << app_afu_id;
      if (!pci_addr_.empty()) {
        std::cerr << " at pcie address: " << pci_addr_;
      }
      std::cerr << "\n";
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
    if (!commands_.empty())
      app_.require_subcommand();
    CLI11_PARSE(app_, argc, argv);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::from_str(log_level_));

    command::ptr_t test(nullptr);
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

    int res = open_handle(test->afu_id());
    if (res != exit_codes::not_run) {
      return res;
    }

    std::stringstream ss;
    ss << name_ << "_" << test->name() << ".log";
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(ss.str(), true);
    file_sink->set_level(spdlog::level::trace);
    spdlog::sinks_init_list sinks({console_sink, file_sink});
    logger_ = std::make_shared<spdlog::logger>(test->name(), sinks);
    spdlog::register_logger(logger_);
    return run(app, test);
  }

  virtual int run(CLI::App *app, command::ptr_t test)
  {
    int res = exit_codes::not_run;

    current_command_ = test;

    try {
      std::future<int> f = std::async(std::launch::async,
        [this, test, app](){
          return test->run(this, app);
        });
      auto status = f.wait_for(std::chrono::milliseconds(timeout_msec_));
      if (status == std::future_status::timeout)
        throw std::runtime_error("timeout");
      res = f.get();
    } catch(std::exception &ex) {
      res = exit_codes::exception;
    }

    current_command_.reset();
    return res;
  }

  template<class T>
  CLI::App *register_command()
  {
    command::ptr_t cmd(new T());
    auto sub = app_.add_subcommand(cmd->name(),
                                   cmd->description());
    cmd->add_options(sub);
    commands_[sub] = cmd;
    return sub;
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

  command::ptr_t current_command() const {
    return current_command_;
  }

protected:
  std::string name_;
  std::string afu_id_;
  CLI::App app_;
  std::string pci_addr_;
  std::string log_level_;
  bool shared_;
  uint32_t timeout_msec_;
  fpga::handle::ptr_t handle_;
  command::ptr_t current_command_;
  std::map<CLI::App*, command::ptr_t> commands_;
  std::shared_ptr<spdlog::logger> logger_;
};


} // end of namespace afu_test
} // end of namespace opae

