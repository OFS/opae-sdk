// Copyright(c) 2017, Intel Corporation
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
#include <type_traits>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <uuid/uuid.h>
#include <opae/properties.h>

namespace opae {
namespace fpga {
namespace types {

struct guid_t {
  guid_t(fpga_properties *p) : props_(p) {}

  operator uint8_t *() {
    if (fpgaPropertiesGetGUID(
            *props_, reinterpret_cast<fpga_guid *>(data_.data())) == FPGA_OK) {
      return data_.data();
    }
    return nullptr;
  }

  const uint8_t* get() const {
      return data_.data();
  }

  guid_t &operator=(fpga_guid g) {
    if (fpgaPropertiesSetGUID(*props_, g) == FPGA_OK) {
      uint8_t *begin = &g[0];
      uint8_t *end = begin + sizeof(fpga_guid);
      std::copy(begin, end, data_.begin());
    } else {
      // throw exception
    }
    return *this;
  }

  bool operator==(const fpga_guid &g) {
    return 0 == std::memcmp(data_.data(), g, sizeof(fpga_guid));
  }

  void parse(const char *str) {
    if (0 != uuid_parse(str, data_.data())) {
      // throw error
    }
    if (FPGA_OK != fpgaPropertiesSetGUID(*props_, data_.data())) {
      // throw error
    }
  }

  friend std::ostream &operator<<(std::ostream &ostr, const guid_t &g) {
    fpga_properties props = *g.props_;
    fpga_guid guid_value;
    if (fpgaPropertiesGetGUID(props, &guid_value) == FPGA_OK) {
      char guid_str[84];
      uuid_unparse(g.data_.data(), guid_str);
      ostr << guid_str;
    } else {
      // TODO: Log or throw
    }
    return ostr;
  }

 private:
  fpga_properties *props_;
  std::array<uint8_t, 16> data_;
};

template <typename T>
struct pvalue {
  typedef typename std::conditional<
      std::is_same<T, char *>::value, fpga_result (*)(fpga_properties, T),
      fpga_result (*)(fpga_properties, T *)>::type getter_t;
  typedef fpga_result (*setter_t)(fpga_properties, T);
  pvalue() : props_(0) {}

  pvalue(fpga_properties *p, getter_t g, setter_t s)
      : props_(p), get_(g), set_(s) {}

  pvalue<T> &operator=(const T &v) {
    auto res = set_(*props_, v);
    if (res == FPGA_OK) {
      copy_ = v;
    }
  }

  bool operator==(const T &other) { return copy_ == other; }

  operator T() {
    if (get_(*props_, &copy_) == FPGA_OK) {
      return copy_;
    }
    return T();
  }

  // TODO: Remove this once all properties are tested
  fpga_result get_value(T &value) const { return get_(*props_, &value); }

  friend std::ostream &operator<<(std::ostream &ostr, const pvalue<T> &p) {
    T value;
    fpga_properties props = *p.props_;
    if (p.get_(props, &value) == FPGA_OK) {
      ostr << +(value);
    } else {
      // TODO: Log or throw
    }
    return ostr;
  }

 private:
  fpga_properties *props_;
  T copy_;
  getter_t get_;
  setter_t set_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
