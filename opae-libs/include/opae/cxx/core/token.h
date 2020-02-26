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
#include <memory>
#include <vector>

#include <opae/access.h>
#include <opae/cxx/core/properties.h>
#include <opae/enum.h>
#include <opae/types.h>

namespace opae {
namespace fpga {
namespace types {

/** Wraps the OPAE fpga_token primitive.
 * token's are created from an enumeration operation
 * that uses properties describing an accelerator resource
 * as search criteria.
 */
class token {
 public:
  typedef std::shared_ptr<token> ptr_t;

  /** Obtain a vector of token smart pointers
   * for given search criteria.
   * @param[in] props The search criteria.
   * @return A set of known tokens that match the search.
   */
  static std::vector<token::ptr_t> enumerate(
      const std::vector<properties::ptr_t>& props);

  ~token();

  /** Retrieve the underlying fpga_token primitive.
   */
  fpga_token c_type() const { return token_; }

  /** Retrieve the underlying fpga_token primitive.
   */
  operator fpga_token() const { return token_; }

 private:
  token(fpga_token tok);

  fpga_token token_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
