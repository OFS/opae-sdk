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
#include <iostream>

namespace intel
{
namespace utils
{

std::timed_mutex s_mutex;

wrapped_stream::wrapped_stream(std::ostream * stream, bool new_line)
: stream_(stream)
, sstream_(new std::stringstream())
, locked_(false)
, have_stream_(false)
, lock_(s_mutex, std::defer_lock)
, append_new_line_(new_line)
{
}

wrapped_stream::wrapped_stream(const wrapped_stream & other)
: stream_(other.stream_)
, sstream_(other.sstream_)
, locked_(false)
, have_stream_(other.have_stream_)
, lock_(s_mutex, std::defer_lock)
, append_new_line_(other.append_new_line_)
{
}

wrapped_stream & wrapped_stream::operator=(const wrapped_stream & other)
{
    if (this != &other)
    {
        stream_ = other.stream_;
        sstream_ = other.sstream_;
        locked_ = other.locked_;
        have_stream_ = other.have_stream_;
        if (other.lock_ && !lock_)
        {
            lock_.lock();
        }
        else if (!other.lock_ && lock_)
        {
            lock_.release();
        }
        append_new_line_ = other.append_new_line_;
    }
    return *this;
}

wrapped_stream::~wrapped_stream()
{
    lock();
    if (append_new_line_)
    {
        *stream_ << "\n";
    }
    if (stream_) *stream_  << sstream_->str();
    sstream_->str(std::string());
}

void wrapped_stream::lock()
{
    if (!locked_)
    {
        lock_.try_lock_for(std::chrono::seconds(1));
        locked_ = true;
    }
}

wrapped_stream& wrapped_stream::operator<<(std::ostream& (*manip)(std::ostream&))
{
    if (stream_) *sstream_ << manip;
    return *this;
}

logger::logger()
: level_(level::level_none)
, name_()
{
    pid_ = ::getpid();
    sink_ = &std::cout;
}

logger::logger(const std::string & name)
: level_(level::level_none)
, name_(name)
{
    pid_ = ::getpid();
    sink_ = &std::cout;
}

logger::logger(const std::string & name, const std::string & filename)
: level_(level::level_none)
, name_(name)
{
    pid_ = ::getpid();
    filestream_.open(filename);
    sink_ = &filestream_;
}

void logger::open(const std::string & filename)
{
    filestream_.open(filename);
    sink_ = &filestream_;
}

wrapped_stream logger::log(level l, std::string str)
{
    wrapped_stream stream((l >= level_  ? sink_ : 0));
    stream << std::dec << "[" << pid_ << "][" << level_name(l) << "]";
    if (name_ != "") stream << "[" << name_ << "]";
    if (str != "")
    {
        stream << "[" << str << "]";
    }
    stream << std::boolalpha << " ";

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


wrapped_stream logger::debug(std::string str)
{
    return log(level::level_debug, str);
}

wrapped_stream logger::info(std::string str)
{
    return log(level::level_info, str);
}

wrapped_stream logger::warn(std::string str)
{
    return log(level::level_warn, str);
}

wrapped_stream logger::error(std::string str)
{
    return log(level::level_error, str);
}
wrapped_stream logger::exception(std::string str)
{
    return log(level::level_exception, str);
}
wrapped_stream logger::fatal(std::string str)
{
    return log(level::level_fatal, str);
}

} // end of namespace utils
} // end of namespace intel
