// Copyright(c) 2018-2020, Intel Corporation
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
#include <sstream>

#include <opae/utils.h>

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
  std::stringstream ss;
  if (msg_) {
    ss << msg_;
  } else {
    ss << "failed with error " << fpgaErrStr(res_);
  }
  ss << " at: " << loc_.file() << ":" << loc_.fn() << "():" << loc_.line();
  strncpy(buf_, ss.str().c_str(), sizeof(buf_));
  buf_[ss.str().length()] = '\0';

  return const_cast<const char *>(buf_);
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
