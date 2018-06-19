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
#include "pyerrors.h"
namespace py = pybind11;
using opae::fpga::types::error;
using opae::fpga::types::token;
using opae::fpga::types::properties;

const char *error_doc() {
  return R"opaedoc(
    error object is used to represent an error register in an FPGA resource.
    It holds two read-only properties, `name` and `can_clear` and it can also
    be used to read the raw register value from its corresponding error register.
  )opaedoc";
}

const char *error_doc_name() {
  return R"opaedoc(
    Error register name - read-only property
  )opaedoc";
}


const char *error_doc_can_clear() {
  return R"opaedoc(
    Indicates if the error register can be cleared - read-only property
  )opaedoc";
}

const char *error_doc_read_value() {
  return R"opaedoc(
    Read the raw value from the error register.
  )opaedoc";
}

const char *error_doc_errors() {
  return R"opaedoc(
    Get a list of error objects in an FPGA resource.
    Each error object represents an error register contained in the resource.

    Args:
      tok(token): Token representing an FPGA resource.
  )opaedoc";
}

std::vector<error::ptr_t> error_errors(token::ptr_t tok) {
    auto props = properties::get(tok);
    std::vector<error::ptr_t> errors(props->num_errors);
    for (uint32_t i = 0; i < props->num_errors; ++i) {
      errors[i] = error::get(tok, i);
    }
    return errors;
}

