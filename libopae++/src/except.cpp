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
#include <cerrno>
#include <cstring>

#include <opae/utils.h>
#include <safe_string/safe_string.h>

#include "opaec++/except.h"
#include "opaec++/log.h"

namespace opae {
namespace fpga {
namespace types {

src_location::src_location(const char *file, const char *fn, int line) noexcept
    : file_(file), fn_(fn), line_(line) {}

const char *src_location::file() const noexcept {
  // return a pointer to the file name component.
  const char *p = file_;

  while (*p++) {
  }
  while ((p > file_) && (*p != '\\') && (*p != '/')) --p;
  if (('\\' == *p) || ('/' == *p)) ++p;

  return p;
}

except::except(src_location loc) noexcept : res_(FPGA_EXCEPTION), loc_(loc) {}

except::except(fpga_result res, src_location loc) noexcept
    : res_(res), loc_(loc) {}

const char *except::what() const noexcept {
  errno_t err;

  err = strncpy_s(buf_, 32, loc_.file(), 32);
  if (err)
    goto log_err;

  err = strcat_s(buf_, 34, ":");
  if (err)
    goto log_err;

  err = strcat_s(buf_, 50, loc_.fn());
  if (err)
    goto log_err;

  err = strcat_s(buf_, 53, "():");
  if (err)
    goto log_err;

  snprintf_s_i(buf_ + strlen(buf_), 64, "%d:", loc_.line());

  err = strcat_s(buf_, sizeof(buf_), fpgaErrStr(res_));
  if (err)
    goto log_err;

  return const_cast<const char *>(buf_);

log_err:
  opae::fpga::internal::logger log("except::what()");
  log.error() << "safestr error " << err;

  buf_[sizeof(buf_)-1] = '\0';
  return const_cast<const char *>(buf_);
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
