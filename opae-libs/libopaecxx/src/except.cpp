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
#include <cerrno>
#include <cstring>
#include <iostream>

#include <opae/utils.h>
#include <safe_string/safe_string.h>

#include <opae/cxx/core/except.h>

namespace opae {
namespace fpga {
namespace types {

src_location::src_location(const char *file, const char *fn, int line) noexcept
    : file_(file), fn_(fn), line_(line) {}

src_location::src_location(const src_location &other) noexcept
    : file_(other.file_), fn_(other.fn_), line_(other.line_) {}

src_location &src_location::operator=(const src_location &other) noexcept {
  if (&other != this) {
    file_ = other.file_;
    fn_ = other.fn_;
    line_ = other.line_;
  }
  return *this;
}

const char *src_location::file() const noexcept {
  // return a pointer to the file name component.
  const char *p = file_;

  while (*p++) {
  }
  while ((p > file_) && (*p != '\\') && (*p != '/')) --p;
  if (('\\' == *p) || ('/' == *p)) ++p;

  return p;
}

except::except(src_location loc) noexcept
    : res_(FPGA_EXCEPTION),
      msg_("failed with return code FPGA_EXCEPTION"),
      loc_(loc) {}

except::except(fpga_result res, const char *msg, src_location loc) noexcept
    : res_(res), msg_(msg), loc_(loc) {}

except::except(fpga_result res, src_location loc) noexcept
    : res_(res), msg_(0), loc_(loc) {}

const char *except::what() const noexcept {
  errno_t err;
  bool buf_ok = false;
  if (msg_) {
    err = strncpy_s(buf_, MAX_EXCEPT - 64, msg_, 64);
  } else {
    err = strncpy_s(buf_, MAX_EXCEPT - 64, "failed with error ", 64);
    if (err) goto log_err;
    err = strcat_s(buf_, MAX_EXCEPT - 64, fpgaErrStr(res_));
  }
  if (err) goto log_err;
  buf_ok = true;

  err = strcat_s(buf_, MAX_EXCEPT - 64, " at: ");
  if (err) goto log_err;

  err = strcat_s(buf_, MAX_EXCEPT - 64, loc_.file());
  if (err) goto log_err;

  err = strcat_s(buf_, MAX_EXCEPT - 64, ":");
  if (err) goto log_err;

  err = strcat_s(buf_, MAX_EXCEPT - 64, loc_.fn());
  if (err) goto log_err;

  err = strcat_s(buf_, MAX_EXCEPT - 64, "():");
  if (err) goto log_err;

  snprintf_s_i(buf_ + strlen(buf_), 64, "%d", loc_.line());

  return const_cast<const char *>(buf_);

log_err:
  std::cerr << "[except::what()] error with safestr operation: " << err << "\n";

  buf_[sizeof(buf_) - 1] = '\0';
  return buf_ok ? const_cast<const char *>(buf_) : msg_;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
