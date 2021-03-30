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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTOR."AS ."
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

#include "pytoken.h"

#include "pyproperties.h"

namespace py = pybind11;
using opae::fpga::types::token;

const char *token_doc() {
  return R"opaedoc(
    Token for referencing an OPAE resource.

    A token object serves as a reference so a specific resource in the system.
    Holding a token does not constitute ownership of an OPAE resource.
    It is used to query information about a resource,
    or to acquire ownership by calling fpga.open module method.
  )opaedoc";
}

const char *token_doc_enumerate() {
  return R"opaedoc(
    Get a list of tokens for the given search criteria.

    Args:

      props(list): A list of properties objects that define the search criteria.
                   All OPAE properties in each properties object make up one filter.
                   All properties objects are combined in a union.
  )opaedoc";
}

const char *token_doc_enumerate_kwargs() {
  return R"opaedoc(
    Get a list of tokens from zero or one filters as defined by kwargs.
    If kwargs is empty, then no filter is used.
    If kwargs is not empty, then one properties object is created using the kwargs.
  )opaedoc";
}

std::vector<token::ptr_t> token_enumerate_kwargs(py::kwargs kwargs) {
  return token::enumerate({properties_get(kwargs)});
};
