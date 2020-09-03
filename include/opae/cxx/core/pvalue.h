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
#pragma once
#include <opae/cxx/core/except.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include <uuid/uuid.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace opae {
namespace fpga {
namespace types {

/** Representation of the guid member of a properties object.
 */
struct guid_t {
  /** Construct the guid_t given its containing fpga_properties.
   */
  guid_t(fpga_properties *p) : props_(p), is_set_(false) {}

  /** Update the local cached copy of the guid.
   */
  void update() {
    fpga_result res = fpgaPropertiesGetGUID(
        *props_, reinterpret_cast<fpga_guid *>(data_.data()));
    ASSERT_FPGA_OK(res);
    is_set_ = true;
  }

  /** Return a raw pointer to the guid.
   * @retval nullptr if the guid could not be queried.
   */
  operator uint8_t *() {
    update();
    return data_.data();
  }

  /** Return a raw pointer to the guid.
   */
  const uint8_t *c_type() const { return data_.data(); }

  /** Assign from fpga_guid
   * Sets the guid field of the associated properties
   * object using the OPAE properties API.
   * @param[in] g The given fpga_guid.
   * @return a reference to this guid_t.
   */
  guid_t &operator=(fpga_guid g) {
    is_set_ = false;
    ASSERT_FPGA_OK(fpgaPropertiesSetGUID(*props_, g));
    is_set_ = true;
    uint8_t *begin = &g[0];
    uint8_t *end = begin + sizeof(fpga_guid);
    std::copy(begin, end, data_.begin());
    return *this;
  }

  /** Compare contents with an fpga_guid.
   * @retval The result of memcmp of the two objects.
   */
  bool operator==(const fpga_guid &g) {
    return is_set() && (0 == std::memcmp(data_.data(), g, sizeof(fpga_guid)));
  }

  /** Convert a string representation of a guid to binary.
   * @param[in] str The guid string.
   */
  void parse(const char *str) {
    is_set_ = false;
    if (0 != uuid_parse(str, data_.data())) {
      throw except(OPAECXX_HERE);
    }
    ASSERT_FPGA_OK(fpgaPropertiesSetGUID(*props_, data_.data()));
    is_set_ = true;
  }

  /** Send the string representation of the guid_t to the given stream.
   */
  friend std::ostream &operator<<(std::ostream &ostr, const guid_t &g) {
    fpga_properties props = *g.props_;
    fpga_guid guid_value;
    fpga_result res;
    if ((res = fpgaPropertiesGetGUID(props, &guid_value)) == FPGA_OK) {
      char guid_str[84];
      uuid_unparse(guid_value, guid_str);
      ostr << guid_str;
    } else if (FPGA_NOT_FOUND == res) {
      std::cerr << "[guid_t::<<] GUID property not set\n";
    } else {
      ASSERT_FPGA_OK(res);
    }
    return ostr;
  }

  /** Tracks whether the cached local copy of the guid is valid.
   */
  bool is_set() const { return is_set_; }

  /** Invalidate the cached local copy of the guid.
   */
  void invalidate() { is_set_ = false; }

 private:
  fpga_properties *props_;
  bool is_set_;
  std::array<uint8_t, 16> data_;
};

/**
 * @brief Wraps OPAE properties defined in the OPAE C API
 *        by associating an `fpga_properties` reference
 *        with the getters and setters defined for a property
 *
 * @tparam T The type of the property value being wrapped
 */
template <typename T>
struct pvalue {
  /**
   * @brief Define getter function as getter_t
   * For `char*` types, do not use T* as the second argument
   * but instead use T
   */
  typedef typename std::conditional<
      std::is_same<T, char *>::value, fpga_result (*)(fpga_properties, T),
      fpga_result (*)(fpga_properties, T *)>::type getter_t;

  /**
   * @brief Define the setter function as setter_t
   *
   */
  typedef fpga_result (*setter_t)(fpga_properties, T);

  /**
   * @brief Define the type of our copy variable
   * For `char*` types use std::string as the copy
   */
  typedef typename std::conditional<std::is_same<T, char *>::value,
                                    typename std::string, T>::type copy_t;

  pvalue() : props_(0), is_set_(false), get_(nullptr), set_(nullptr) {}

  /**
   * @brief pvalue contructor that takes in a reference to fpga_properties
   *        and corresponding accessor methods for a property
   *
   * @param p A reference to an fpga_properties
   * @param g The getter function
   * @param s The setter function
   */
  pvalue(fpga_properties *p, getter_t g, setter_t s)
      : props_(p), is_set_(false), get_(g), set_(s) {}

  /**
   * @brief Overload of `=` operator that calls the wrapped setter
   *
   * @param v The value to set
   *
   * @return A reference to itself
   */
  pvalue<T> &operator=(const T &v) {
    is_set_ = false;
    ASSERT_FPGA_OK(set_(*props_, v));
    is_set_ = true;
    copy_ = v;
    return *this;
  }

  /**
   * @brief Compare a property for equality with a value
   *
   * @param other The value being compared to
   *
   * @return Whether or not the property is equal to the value
   */
  bool operator==(const T &other) { return is_set() && (copy_ == other); }

  void update() {
    ASSERT_FPGA_OK(get_(*props_, &copy_));
    is_set_ = true;
  }

  /**
   * @brief Implicit converter operator - calls the wrapped getter
   *
   * @return The property value after calling the getter or a default
   *         value of the value type
   */
  operator copy_t() {
    update();
    return copy_;
  }

  // TODO: Remove this once all properties are tested
  fpga_result get_value(T &value) const { return get_(*props_, &value); }

  /**
   * @brief Stream overalod operator
   *
   * @param ostr The output stream
   * @param p A reference to a pvalue<T> object
   *
   * @return The stream operator after streaming the property value
   */
  friend std::ostream &operator<<(std::ostream &ostr, const pvalue<T> &p) {
    T value;
    fpga_properties props = *p.props_;
    fpga_result res;
    if ((res = p.get_(props, &value)) == FPGA_OK) {
      ostr << +(value);
    } else if (FPGA_NOT_FOUND == res) {
      std::cerr << "property getter returned (" << res << ") "
                << fpgaErrStr(res);
    } else {
      ASSERT_FPGA_OK(res);
    }
    return ostr;
  }

  /** Tracks whether the cached local copy of the pvalue is valid.
   */
  bool is_set() const { return is_set_; }

  /** Invalidate the cached local copy of the pvalue.
   */
  void invalidate() { is_set_ = false; }

 private:
  fpga_properties *props_;
  bool is_set_;
  copy_t copy_;
  getter_t get_;
  setter_t set_;
};

/**
 * @brief Template specialization of `char*` type property updater
 *
 * @return The result of the property getter function.
 */
template <>
inline void pvalue<char *>::update() {
  char buf[256];
  ASSERT_FPGA_OK(get_(*props_, buf));
  copy_.assign(buf);
  is_set_ = true;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
