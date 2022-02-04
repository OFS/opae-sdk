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
#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <sstream>

namespace intel
{
namespace utils
{

class wrapped_stream
{
public:
    wrapped_stream(std::ostream * stream, bool new_line = false);

    wrapped_stream(const wrapped_stream & other);

    wrapped_stream & operator=(const wrapped_stream & other);

    ~wrapped_stream();

    template<typename T>
    wrapped_stream& operator<<(const T& v)
    {
        if (stream_) *sstream_ << v;
        return *this;
    }

    void lock();
    wrapped_stream& operator<<(std::ostream& (*manip)(std::ostream&));
private:
    std::ostream * stream_;
    std::shared_ptr<std::stringstream> sstream_;
    bool locked_;
    bool have_stream_;
    std::unique_lock<std::timed_mutex> lock_;
    bool append_new_line_;
};

class logger
{
public :
    enum level
    {
        level_none        =  0,
        level_debug       = 10,
        level_info        = 20,
        level_warn        = 30,
        level_error       = 40,
        level_exception   = 50,
        level_fatal       = 60
    };

    logger();
    logger(const std::string & name);
    logger(const std::string & name, const std::string & filename);

    virtual ~logger()
    {
    }


    wrapped_stream log(level l, std::string str = "");
    wrapped_stream debug(std::string str = "");
    wrapped_stream info(std::string str = "");
    wrapped_stream warn(std::string str = "");
    wrapped_stream error(std::string str= "");
    wrapped_stream exception(std::string str = "");
    wrapped_stream fatal(std::string str = "");

    void set_level(level l);

    void open(const std::string & filename);

    void flush() { sink_->flush(); }

    level get_level();

    std::string get_level_name();

    std::string level_name(level l);

private:
    std::string name_;
    std::ofstream filestream_;
    std::ostream  *sink_;
    int pid_;
    level level_;
};

} // end of namespace utils
} // end of namespace intel
