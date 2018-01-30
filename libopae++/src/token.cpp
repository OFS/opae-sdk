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
#include <algorithm>
#include <opae/utils.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/except.h>

namespace opae {
namespace fpga {
namespace types {

std::vector<token::ptr_t> token::enumerate(
    const std::vector<properties>& props) {
  std::vector<token::ptr_t> tokens;
  std::vector<fpga_properties> c_props(props.size());
  std::transform(props.begin(), props.end(), c_props.begin(),
                 [](const properties& p) { return p.get(); });
  uint32_t matches = 0;
  opae::fpga::internal::logger log("token::enumerate()");
  auto res =
      fpgaEnumerate(c_props.data(), c_props.size(), nullptr, 0, &matches);
  if (res == FPGA_OK && matches > 0) {
    std::vector<fpga_token> c_tokens(matches);
    tokens.resize(matches);
    res = fpgaEnumerate(c_props.data(), c_props.size(), c_tokens.data(),
                        c_tokens.size(), &matches);

    // create a new c++ token object for each c token struct
    std::transform(c_tokens.begin(), c_tokens.end(), tokens.begin(),
                   [](fpga_token t) { return token::ptr_t(new token(t)); });

    // discard our c struct token objects
    std::for_each(c_tokens.begin(), c_tokens.end(),
                  [&log](fpga_token t) {
                    auto res = fpgaDestroyToken(&t);
                    if (res != FPGA_OK){
                      log.error() << "fpgaDestroyToken() failed with (" << res
                                  << ") " << fpgaErrStr(res);
                      throw except(res, OPAECXX_HERE);
                    }
                  });
  }
  return tokens;
}

token::~token() {
  auto res = fpgaDestroyToken(&token_);
  if (res != FPGA_OK){
    log_.error() << "fpgaDestroyToken() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }
}

token::token(fpga_token tok) : log_("token") {
  auto res = fpgaCloneToken(tok, &token_);
  if (res != FPGA_OK){
    log_.error() << "fpgaCloneToken() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
