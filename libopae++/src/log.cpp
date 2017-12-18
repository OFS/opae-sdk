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
#include <safe_string/safe_string.h>

#include <unistd.h>
#include <map>

#include "opaec++/log.h"

extern "C" {
  void fpga_print(int, char*, ...);
}

namespace opae {
namespace fpga {
namespace internal {

static int s_pid = ::getpid();

static std::map<logger::level, std::string> s_level_map =
{
    {logger::level::none,       "NONE"},
    {logger::level::debug,      "DEBUG"},
    {logger::level::info,       "INFO"},
    {logger::level::warn,       "WARN"},
    {logger::level::error,      "ERROR"},
    {logger::level::exception,  "EXCEPTION"},
    {logger::level::fatal,      "FATAL"}
};

wrapped_stream::wrapped_stream(int level, bool new_line)
    : sstream_()
    , level_(level)
    , fmt_{"%s\n"}
{
  if (!new_line){
    fmt_[2] = 0;
  }
}

wrapped_stream::wrapped_stream(const wrapped_stream & other)
    : sstream_(other.sstream_.str())
{
}

wrapped_stream & wrapped_stream::operator=(const wrapped_stream & other) {
  if (this != &other) {
      sstream_.str(other.sstream_.str());
  }
  return *this;
}

wrapped_stream::~wrapped_stream() {
  fpga_print(level_, fmt_, sstream_.str().c_str());
  sstream_.str(std::string());
}


wrapped_stream& wrapped_stream::operator<<(std::ostream& (*manip)(std::ostream&)) {
  sstream_ << manip;
  return *this;
}

logger::logger(const std::string & name)
  : name_(name)
{
}

wrapped_stream logger::log(logger::level l, std::string str)
{
  // convert C++ log levels to C log
  int lvl = static_cast<int>(l)/20;
  wrapped_stream stream(lvl);
  stream << std::dec << "[" << s_pid << "][" << s_level_map[l] << "]";
  if (name_ != "") stream << "[" << name_ << "]";
  if (str != ""){
      stream << "[" << str << "]";
  }
  stream << std::boolalpha << " ";

  return stream;
}

wrapped_stream logger::debug(std::string str)
{
  return log(level::debug, str);
}

wrapped_stream logger::info(std::string str)
{
  return log(level::info, str);
}

wrapped_stream logger::warn(std::string str)
{
  return log(level::warn, str);
}

wrapped_stream logger::error(std::string str)
{
  return log(level::error, str);
}

wrapped_stream logger::exception(std::string str)
{
  return log(level::exception, str);
}

wrapped_stream logger::fatal(std::string str)
{
  return log(level::fatal, str);
}

}  // end of namespace internal
}  // end of namespace fpga
}  // end of namespace opae
