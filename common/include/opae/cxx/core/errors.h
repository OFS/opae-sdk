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

#include <opae/cxx/core/token.h>
#include <opae/types_enum.h>
#include <memory>

namespace opae {
namespace fpga {
namespace types {

/**
 * @brief An error object represents an error register for a resource.
 * This is used to read out the raw value in the register. No parsing is
 * done by this class.
 */
class error {
 public:
  typedef std::shared_ptr<error> ptr_t;

  error(const error &e) = delete;

  error &operator=(const error &e) = delete;

  /**
   * @brief Factory function for creating an error object.
   *
   * @param tok The token object representing a resource.
   * @param num The index of the error register. This must be lower than the
   * num_errors property of the resource.
   *
   * @return A shared_ptr containing the error object
   */
  static error::ptr_t get(token::ptr_t tok, uint32_t num);

  /**
   * @brief Get the error register name.
   *
   * @return A std::string object set to the error name.
   */
  std::string name() { return error_info_.name; }

  /**
   * @brief Indicates whether an error register can be cleared.
   *
   * @return A boolean value indicating if the error register can be cleared.
   */
  bool can_clear() { return error_info_.can_clear; }

  /**
   * @brief Read the raw value contained in the associated error register.
   *
   * @return A 64-bit value (unparsed) read from the error register
   */
  uint64_t read_value();

  ~error() {}

  /**
   * @brief Get the C data structure
   *
   * @return The fpga_error_info that contains the name and the can_clear
   * boolean.
   */
  fpga_error_info c_type() const { return error_info_; }

 private:
  error(token::ptr_t token, uint32_t num);
  token::ptr_t token_;
  fpga_error_info error_info_;
  uint32_t error_num_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
