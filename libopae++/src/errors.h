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

#define MAX_ERROR_MSG 256

#include <map>
#include <opae/types_enum.h>

#define STRINGIFY(x) #x
#define NUMSTR(x) STRINGIFY(x)
#define OPAECXX_AT  __FILE__ ":" NUMSTR(__LINE__)

#include <safe_string/safe_string.h>

#include <opae/cxx/errors/busy.h>
#include <opae/cxx/errors/exception.h>
#include <opae/cxx/errors/invalid_param.h>
#include <opae/cxx/errors/no_access.h>
#include <opae/cxx/errors/no_daemon.h>
#include <opae/cxx/errors/no_driver.h>
#include <opae/cxx/errors/no_memory.h>
#include <opae/cxx/errors/not_found.h>
#include <opae/cxx/errors/not_supported.h>
#include <opae/cxx/errors/reconf_error.h>

namespace opae {
namespace fpga {
namespace detail {

typedef bool (*exception_fn)(fpga_result, const char*, const char*);

template<typename T>
constexpr bool is_ok(fpga_result result, const char* func, const char* loc) {
  return result == FPGA_OK ? true : throw T(func, loc);
}


static exception_fn opae_exceptions[12] = { is_ok<opae::fpga::types::invalid_param>,
                                            is_ok<opae::fpga::types::busy>,
                                            is_ok<opae::fpga::types::exception>,
                                            is_ok<opae::fpga::types::not_found>,
                                            is_ok<opae::fpga::types::no_memory>,
                                            is_ok<opae::fpga::types::not_supported>,
                                            is_ok<opae::fpga::types::no_driver>,
                                            is_ok<opae::fpga::types::no_daemon>,
                                            is_ok<opae::fpga::types::no_access>,
                                            is_ok<opae::fpga::types::reconf_error> };


static inline void assert_fpga_ok(fpga_result result,
                                  const char* func,
                                  const char* loc) {
  if (result >= FPGA_OK && result <= FPGA_RECONF_ERROR)
    opae_exceptions[result](result, func, loc);
}

#define ASSERT_FPGA_OK(r) \
  opae::fpga::detail::assert_fpga_ok(r, __func__, OPAECXX_AT);


}  // end of namespace detail
}  // end of namespace fpga
}  // end of namespace opae
