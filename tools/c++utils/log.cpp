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
#include "log.h"
#include <fstream>
#include <unistd.h>

namespace intel
{
namespace utils
{

logger::logger(const std::string & filename)
    : log_(0)
    , null_stream_(0)
    , level_(level::level_none)
{
    filestream_.open(filename);
    if (filestream_.is_open())
    {
        log_ = &filestream_;
    }
    pid_ = ::getpid();
}

logger::logger(std::ostream & stream)
    : log_(&stream)
    , null_stream_(0)
    , level_(level::level_none)
{
    pid_ = ::getpid();
}


std::ostream & logger::log(level l, std::string str)
{
    auto & stream = l >= level_ && log_ ? *log_ : null_stream_;
    stream << "[" << pid_ << "][" << level_name(l) << "]";
    if (str != "")
    {
        stream << "[" << str << "]";
    }
    stream << std::boolalpha;

    return stream;
}

void logger::set_level(level l)
{
    level_ = l;
}

logger::level logger::get_level()
{
    return level_;
}

std::string logger::get_level_name()
{
    return level_name(get_level());
}

std::string logger::level_name(level l)
{
    static std::map<level, std::string> level_map =
    {
        {level::level_none,       "NONE"},
        {level::level_debug,      "DEBUG"},
        {level::level_info,       "INFO"},
        {level::level_warn,       "WARN"},
        {level::level_error,      "ERROR"},
        {level::level_exception,  "EXCEPTION"},
        {level::level_fatal,      "FATAL"}
    };
    return level_map[l];
}


std::ostream & logger::debug(std::string str)
{
    return log(level::level_debug, str);
}

std::ostream & logger::info(std::string str)
{
    return log(level::level_info, str);
}

std::ostream & logger::warn(std::string str)
{
    return log(level::level_warn, str);
}

std::ostream & logger::error(std::string str)
{
    return log(level::level_error, str);
}
std::ostream & logger::exception(std::string str)
{
    return log(level::level_exception, str);
}
std::ostream & logger::fatal(std::string str)
{
    return log(level::level_fatal, str);
}

} // end of namespace utils
} // end of namespace intel
