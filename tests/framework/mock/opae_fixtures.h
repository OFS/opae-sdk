// Copyright(c) 2017-2022, Intel Corporation
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

extern "C" {
#include <json-c/json.h>
#include <uuid/uuid.h>
}

#include <opae/fpga.h>
#include "adapter.h"
#include "opae_int.h"
#include "gtest/gtest.h"
#include "test_system.h"

extern "C" {
#ifndef NO_OPAE_C
extern opae_api_adapter_table *adapter_list;
#endif // NO_OPAE_C
}

#include <dlfcn.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <algorithm>
#include <sstream>
#include <cstdarg>

namespace opae {
namespace testing {

extern const char xfpga_[] = "xfpga_";
extern const char vfio_[] = "vfio_";
extern const char none_[] = "";

typedef fpga_result (*enumerate_fn_t)(const fpga_properties *filters,
                                      uint32_t num_filters, fpga_token *tokens,
                                      uint32_t max_tokens,
                                      uint32_t *num_matches);
typedef fpga_result (*get_properties_fn_t)(fpga_token token,
                                           fpga_properties *prop);
typedef fpga_result (*destroy_token_fn_t)(fpga_token *token);
typedef fpga_result (*open_fn_t)(fpga_token token, fpga_handle *handle, int flags);
typedef fpga_result (*close_fn_t)(fpga_handle handle);

union pcie_address
{
  pcie_address(uint16_t s, uint8_t b, uint8_t d, uint8_t f) :
    segment(s),
    bus(b),
    device(d),
    function(f) {}

  std::string to_string() const
  {
    std::ostringstream oss;

    oss << std::setw(4) << std::setfill('0') << std::hex << segment << ":" <<
           std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned>(bus) << ":" <<
           std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned>(device) << "." <<
           std::setw(1) << std::setfill('0') << std::hex << static_cast<unsigned>(function);

    return oss.str();
  }

  struct
  {
    uint16_t segment;
    uint8_t bus;
    uint8_t device : 5;
    uint8_t function : 3;
  };
  uint32_t address;
};

template <const char *_P = none_>
class opae_base_p : public ::testing::TestWithParam<std::string> {
 public:

  class compare
  {
   public:
    compare(const std::string &fn_prefix, void *dl_handle) :
      fn_prefix_(fn_prefix),
      dl_handle_(dl_handle)
    {}

    bool operator () (const fpga_token &lhs, const fpga_token &rhs)
    {
      fpga_properties prop = nullptr;
      uint16_t segment = 0;
      uint8_t bus = 0;
      uint8_t device = 0;
      uint8_t function = 0;

      std::string fn_name = fn_prefix_ + std::string("fpgaGetProperties");
      get_properties_fn_t get_properties = reinterpret_cast<get_properties_fn_t>(dlsym(dl_handle_, fn_name.c_str()));
      if (!get_properties)
        return false;

      get_properties(lhs, &prop);
      fpgaPropertiesGetSegment(prop, &segment);
      fpgaPropertiesGetBus(prop, &bus);
      fpgaPropertiesGetDevice(prop, &device);
      fpgaPropertiesGetFunction(prop, &function);
      fpgaDestroyProperties(&prop);

      pcie_address lhs_addr(segment, bus, device, function);

      segment = 0;
      bus = device = function = 0;
      prop = nullptr;

      get_properties(rhs, &prop);
      fpgaPropertiesGetSegment(prop, &segment);
      fpgaPropertiesGetBus(prop, &bus);
      fpgaPropertiesGetDevice(prop, &device);
      fpgaPropertiesGetFunction(prop, &function);
      fpgaDestroyProperties(&prop);

      pcie_address rhs_addr(segment, bus, device, function);

      return lhs_addr.address < rhs_addr.address;
    }

   private:
    std::string fn_prefix_;
    void *dl_handle_;
  };

  enum states
  {
    got_device_tokens      = 0x00000001,
    got_accelerator_tokens = 0x00000002,
  };
 
  opae_base_p() :
    state_(0),
    fn_prefix_(_P),
    plugin_hint_(_P),
    system_(nullptr)
  {
    if (plugin_hint_ == std::string(xfpga_))
      plugin_hint_.pop_back(); // "xfpga"
    else if (plugin_hint_ == std::string(vfio_))
      plugin_hint_ = "opae-v";
  }

  void * get_dl_handle(const std::string &hint) const
  {
    if (hint.empty())
      return nullptr;

#ifndef NO_OPAE_C
    opae_api_adapter_table *p = adapter_list;

    while (p) {
      std::string library(p->plugin.path);

      if (library.find(hint) != std::string::npos)
        return p->plugin.dl_handle;

      p = p->next;
    }
#endif // NO_OPAE_C

    return nullptr;
  }

  fpga_result Enumerate(const fpga_properties *filters,
                        uint32_t num_filters, fpga_token *tokens,
                        uint32_t max_tokens,
                        uint32_t *num_matches) const
  {
    std::string fn_name = fn_prefix_ + std::string("fpgaEnumerate");
    void *dl_handle = get_dl_handle(plugin_hint_);
    enumerate_fn_t enumerate = reinterpret_cast<enumerate_fn_t>(dlsym(dl_handle, fn_name.c_str()));
    if (!enumerate)
      return FPGA_EXCEPTION;
    return enumerate(filters, num_filters, tokens, max_tokens, num_matches);
  }

  fpga_result GetProperties(fpga_token token, fpga_properties *prop) const
  {
    std::string fn_name = fn_prefix_ + std::string("fpgaGetProperties");
    void *dl_handle = get_dl_handle(plugin_hint_);
    get_properties_fn_t get_properties = reinterpret_cast<get_properties_fn_t>(dlsym(dl_handle, fn_name.c_str()));
    if (!get_properties)
      return FPGA_EXCEPTION;
    return get_properties(token, prop);
  }

  fpga_result DestroyToken(fpga_token *token) const
  {
    std::string fn_name = fn_prefix_ + std::string("fpgaDestroyToken");
    void *dl_handle = get_dl_handle(plugin_hint_);
    destroy_token_fn_t destroy_token = reinterpret_cast<destroy_token_fn_t>(dlsym(dl_handle, fn_name.c_str()));
    if (!destroy_token)
      return FPGA_EXCEPTION;
    return destroy_token(token);
  }

  fpga_result Open(fpga_token token, fpga_handle *handle, int flags) const
  {
    std::string fn_name = fn_prefix_ + std::string("fpgaOpen");
    void *dl_handle = get_dl_handle(plugin_hint_);
    open_fn_t open_fn = reinterpret_cast<open_fn_t>(dlsym(dl_handle, fn_name.c_str()));
    if (!open_fn)
      return FPGA_EXCEPTION;
    return open_fn(token, handle, flags);
  }

  fpga_result Close(fpga_handle handle) const
  {
    std::string fn_name = fn_prefix_ + std::string("fpgaClose");
    void *dl_handle = get_dl_handle(plugin_hint_);
    close_fn_t close_fn = reinterpret_cast<close_fn_t>(dlsym(dl_handle, fn_name.c_str()));
    if (!close_fn)
      return FPGA_EXCEPTION;
    return close_fn(handle);
  }

  virtual void OPAEInitialize()
  {
    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);
  }

  virtual void OPAEFinalize()
  {
    ASSERT_EQ(fpgaFinalize(), FPGA_OK);
  }

  virtual void SetUp() override
  {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();
    OPAEInitialize();
  }

  virtual void TearDown() override
  {
    cleanup_accelerator_tokens();
    cleanup_device_tokens();
    OPAEFinalize();
    EXPECT_EQ(system_->remove_sysfs(), 0) << "error removing tmpsysfs: "
                                          << strerror(errno);
    system_->finalize();

#ifndef NO_OPAE_C
#ifdef LIBOPAE_DEBUG
    EXPECT_EQ(opae_wrapped_tokens_in_use(), 0);
#endif // LIBOPAE_DEBUG
#endif // NO_OPAE_C
  }

  fpga_token get_device_token(size_t i)
  {
    enumerate_device_tokens();
    if (i >= device_tokens_.size())
      return nullptr;
    return device_tokens_[i];
  }

  fpga_token get_accelerator_token(fpga_token parent, size_t i)
  {
    std::vector<fpga_token> accelerators =
      get_accelerator_tokens(parent);

    if (i >= accelerators.size())
      return nullptr;

    return accelerators[i];
  }

  std::vector<fpga_token> get_accelerator_tokens(fpga_token parent)
  {
    std::vector<fpga_token> empty;

    enumerate_accelerator_tokens();

    std::map<fpga_token, std::vector<fpga_token>>::iterator it;
    it = device_to_accelerator_map_.find(parent);
    if (it == device_to_accelerator_map_.end())
      return empty;

    return it->second;
  }

  fpga_token get_parent_token(fpga_token child)
  {
    enumerate_accelerator_tokens();

    std::map<fpga_token, fpga_token>::iterator it;
    it = accelerator_to_parent_map_.find(child);
    if (it == accelerator_to_parent_map_.end())
      return nullptr;

    return it->second;
  }

  pcie_address get_pcie_address(fpga_token token) const
  {
    uint16_t segment = 0;
    uint8_t bus = 0;
    uint8_t device = 0;
    uint8_t function = 0;
    fpga_properties prop = nullptr;

    GetProperties(token, &prop);
    fpgaPropertiesGetSegment(prop, &segment);
    fpgaPropertiesGetBus(prop, &bus);
    fpgaPropertiesGetDevice(prop, &device);
    fpgaPropertiesGetFunction(prop, &function);
    fpgaDestroyProperties(&prop);

    return pcie_address(segment, bus, device, function);
  }

  std::string get_token_type_str(fpga_token token) const
  {
    fpga_objtype objtype = FPGA_DEVICE;
    fpga_properties prop = nullptr;

    GetProperties(token, &prop);
    fpgaPropertiesGetObjectType(prop, &objtype);
    fpgaDestroyProperties(&prop);

    return (objtype == FPGA_DEVICE) ? std::string("device") :
                                      std::string("accelerator");
  }

  virtual fpga_properties device_filter() const
  {
    fpga_properties filter = nullptr;

    if (GetProperties(nullptr, &filter) != FPGA_OK)
      return nullptr;

    if (fpgaPropertiesSetObjectType(filter, FPGA_DEVICE) != FPGA_OK) {
      fpgaDestroyProperties(&filter);
      return nullptr;
    }

    return filter;
  }

  virtual fpga_properties accelerator_filter(fpga_token parent) const
  {
      fpga_properties filter = nullptr;

      if (GetProperties(nullptr, &filter) != FPGA_OK)
        return nullptr;

      if ((fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR) != FPGA_OK) ||
          (fpgaPropertiesSetParent(filter, parent) != FPGA_OK)) {
        fpgaDestroyProperties(&filter);
        return nullptr;
      }

      return filter;
  }

  virtual int open_flags() const
  {
    return 0;
  }

 protected:
  fpga_result num_tokens_for(fpga_properties filter, uint32_t &num_tokens) const
  {
    if (!filter)
      return Enumerate(nullptr, 0, NULL, 0, &num_tokens);
    return Enumerate(&filter, 1, NULL, 0, &num_tokens);
  }

  void destroy_tokens(std::vector<fpga_token> &tokens) const
  {
    for (auto &t : tokens) {
      if (t) {
        ASSERT_EQ(DestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    tokens.clear();
  }

  void enumerate_device_tokens()
  {
    if (state_ & got_device_tokens)
      return;

    state_ |= got_device_tokens;

    fpga_properties filter = device_filter();
    ASSERT_NE(filter, nullptr);

    uint32_t num_tokens = 0;
    ASSERT_EQ(num_tokens_for(filter, num_tokens), FPGA_OK);

    if (!num_tokens) {
      ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
      return;
    }

    device_tokens_.resize(num_tokens, nullptr);
    ASSERT_EQ(Enumerate(&filter, 1,
                        device_tokens_.data(), device_tokens_.size(),
                        &num_tokens), FPGA_OK);

    ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);

    std::sort(device_tokens_.begin(),
              device_tokens_.end(),
              compare(fn_prefix_, get_dl_handle(plugin_hint_)));
  }

  void cleanup_device_tokens()
  {
    if (state_ & got_device_tokens) {
      destroy_tokens(device_tokens_);
      state_ &= ~got_device_tokens;
    }
  }

  void enumerate_accelerator_tokens()
  {
    enumerate_device_tokens();

    if (state_ & got_accelerator_tokens)
      return;

    state_ |= got_accelerator_tokens;

    for (auto parent : device_tokens_) {
      std::vector<fpga_token> accel_tokens;

      fpga_properties filter = accelerator_filter(parent);
      ASSERT_NE(filter, nullptr);

      uint32_t num_tokens = 0;
      ASSERT_EQ(num_tokens_for(filter, num_tokens), FPGA_OK);

      if (!num_tokens) {
        ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
        device_to_accelerator_map_.insert(std::make_pair(parent, accel_tokens));
        continue;
      }

      accel_tokens.resize(num_tokens, nullptr);
      ASSERT_EQ(Enumerate(&filter, 1,
                          accel_tokens.data(), accel_tokens.size(),
                          &num_tokens), FPGA_OK);

      ASSERT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);

      std::sort(accel_tokens.begin(),
                accel_tokens.end(),
                compare(fn_prefix_, get_dl_handle(plugin_hint_)));
      device_to_accelerator_map_.insert(std::make_pair(parent, accel_tokens));

      for (auto child : accel_tokens) {
        accelerator_to_parent_map_.insert(std::make_pair(child, parent));
      }
    }
  }

  void cleanup_accelerator_tokens()
  {
    if (state_ & got_accelerator_tokens) {
      std::map<fpga_token, std::vector<fpga_token>>::iterator it;

      for (it = device_to_accelerator_map_.begin() ;
             it != device_to_accelerator_map_.end() ;
               ++it) {
        destroy_tokens(it->second);
      }
      device_to_accelerator_map_.clear();
      accelerator_to_parent_map_.clear();

      state_ &= ~got_accelerator_tokens;
    }
  }

  uint32_t state_;
  std::string fn_prefix_;
  std::string plugin_hint_;
  test_system *system_;
  test_platform platform_;
  test_device invalid_device_;
  
  std::vector<fpga_token> device_tokens_;

  typedef std::map<fpga_token, std::vector<fpga_token>> token_map_t;
  token_map_t device_to_accelerator_map_;
  std::map<fpga_token, fpga_token> accelerator_to_parent_map_;
};

template <const char *_P = none_>
class opae_p : public opae_base_p<_P> {
 public:

  opae_p() : opae_base_p<_P>(),
    device_token_(nullptr),
    accel_token_(nullptr),
    accel_(nullptr)
  {}

  virtual void SetUp() override
  {
    opae_base_p<_P>::SetUp();
    device_token_ = opae_base_p<_P>::get_device_token(0);
    ASSERT_NE(device_token_, nullptr);
    accel_token_ = opae_base_p<_P>::get_accelerator_token(device_token_, 0);
    ASSERT_NE(accel_token_, nullptr);

    ASSERT_EQ(opae_base_p<_P>::Open(accel_token_, &accel_,
                                    opae_base_p<_P>::open_flags()), FPGA_OK);
  }

  virtual void TearDown() override
  {
    ASSERT_EQ(opae_base_p<_P>::Close(accel_), FPGA_OK);
    accel_ = nullptr;
    opae_base_p<_P>::TearDown();
  }

 protected:
  fpga_token device_token_;
  fpga_token accel_token_;
  fpga_handle accel_;
};

template <const char *_P = none_>
class opae_device_p : public opae_base_p<_P> {
 public:

  opae_device_p() : opae_base_p<_P>(),
    device_token_(nullptr),
    accel_token_(nullptr),
    device_(nullptr)
  {}

  virtual void SetUp() override
  {
    opae_base_p<_P>::SetUp();
    device_token_ = opae_base_p<_P>::get_device_token(0);
    ASSERT_NE(device_token_, nullptr);
    accel_token_ = opae_base_p<_P>::get_accelerator_token(device_token_, 0);
    ASSERT_NE(accel_token_, nullptr);

    ASSERT_EQ(opae_base_p<_P>::Open(device_token_, &device_,
                                    opae_base_p<_P>::open_flags()), FPGA_OK);
  }

  virtual void TearDown() override
  {
    ASSERT_EQ(opae_base_p<_P>::Close(device_), FPGA_OK);
    device_ = nullptr;
    opae_base_p<_P>::TearDown();
  }

 protected:
  fpga_token device_token_;
  fpga_token accel_token_;
  fpga_handle device_;
};

} // end of namespace testing
} // end of namespace opae
