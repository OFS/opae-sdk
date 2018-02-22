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
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <type_traits>

namespace intel {
namespace utils {

class any_value_cast_error : public std::exception {};

/**
 * @brief any_value is an opaque type and holder of any type
 * It allows assigning values of arbitrary types to a variable
 * of type any_value and later querying the value
 */
class any_value {
 public:
  /**
   * @brief alias of the underlying storage type of the template
   * argument
   *
   * @tparam T template argument to get the type of
   */
  template <typename T>
  using storage_type =
      typename std::decay<typename std::remove_reference<T>::type>::type;

  /**
   * @brief Create an any_value variable from an arbitrary variable.
   * This allows a variable to be moved if possible to the
   * private member variable (value_)
   *
   * @tparam U The type of the argument
   * @param v The variable to copy
   */
  template <typename U>
  any_value(U&& v)
      : value_(new any_typed<storage_type<U>>(std::forward<U>(v))) {}

  /**
   * @brief Cast the underlying variable to argument type
   *
   * @tparam U The type to cast to
   *
   * @return The underlying value if its type is compatible with U
   */
  template <typename U>
  U value() {
    auto v = dynamic_cast<any_typed<storage_type<U>>*>(value_);
    if (v == nullptr) {
      throw any_value_cast_error();
    }

    return v->value_;
  }

  /**
   * @brief Query if the underlying type is of type U
   *
   * @tparam U The type to check for
   *
   * @return true if the value can be casted to U, false otherwise
   */
  template <typename U>
  bool is_type() {
    return dynamic_cast<any_typed<storage_type<U>>*>(value_) != nullptr;
  }

  /**
   * @brief Create an empty any_value variable with no underlying value
   */
  any_value() : value_(nullptr) {}

  /**
   * @brief Create an any_value copy variable from another one
   * (of const type)
   *
   * @param other The other any_value variable to copy from
   */
  any_value(const any_value& other) : value_(other.clone()) {}

  /**
   * @brief Create an any_value copy variable from another one
   *
   * @param othe The other any_value variable to copy from
   */
  any_value(any_value& other) : value_(other.clone()) {}

  /**
   * @brief Create an any_value variable transfering ownership
   * to it. The source variable is invalidated
   *
   * @param other The other any_value variable to move from
   */
  any_value(any_value&& other) : value_(other.value_) {
    other.value_ = nullptr;
  }

  /**
   * @brief Assign the underlying value from another any_value variable
   *
   * @param other The other any_value to copy from
   *
   * @return A reference to any_value self
   */
  any_value& operator=(const any_value& other) {
    if (&other == this || value_ == other.value_) {
      return *this;
    }
    if (other.value_ != nullptr) {
      auto old_ptr = value_;
      value_ = other.value_->clone();
      if (old_ptr != nullptr) {
        delete old_ptr;
      }
    }
    return *this;
  }

  /**
   * @brief Assign the underlying value from another any_value variable
   * transfering ownership the any_value self. The other any_value
   * variable is invalidated
   *
   * @param other The other any_value to copy from
   *
   * @return A reference to any_value self
   */
  any_value& operator=(any_value&& other) {
    if (&other == this || value_ == other.value_) {
      return *this;
    }
    if (other.value_ != nullptr) {
      std::swap(value_, other.value_);
    }
    return *this;
  }

  /**
   * @brief Destroy the any_value variable and its resources
   */
  virtual ~any_value() {
    if (value_ != nullptr) {
      delete value_;
    }
  }

 private:
  struct any_base {
    virtual ~any_base() {}

    virtual any_base* clone() const = 0;
  };

  template <typename T>
  struct any_typed : public any_base {
    template <typename U>
    any_typed(U&& v) : value_(std::forward<U>(v)) {}
    virtual ~any_typed() {}

    virtual any_base* clone() const { return new any_typed<T>(value_); }
    T value_;
  };

  any_base* clone() const {
    if (value_ == nullptr) {
      return nullptr;
    }
    return value_->clone();
  }
  any_base* value_;
};

}  // end of namespace utils
}  // end of namespace intel
