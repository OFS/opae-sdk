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
#include <iostream>
#include <fstream>
#include <string>
#include <map>

namespace intel
{
namespace utils
{

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

    logger(const std::string & filename);
    logger(std::ostream & stream = std::cout);

    virtual ~logger()
    {
    }


    std::ostream & log(level l, std::string str = "");
    std::ostream & debug(std::string str = "");
    std::ostream & info(std::string str = "");
    std::ostream & warn(std::string str = "");
    std::ostream & error(std::string str= "");
    std::ostream & exception(std::string str = "");
    std::ostream & fatal(std::string str = "");

    void set_level(level l);

    level get_level();

    std::string get_level_name();

    std::string level_name(level l);

private:
    std::ostream *log_;
    std::ostream null_stream_;
    std::ofstream filestream_;
    int pid_;
    level level_;
};

} // end of namespace utils
} // end of namespace intel
