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
#include <cstdint>
#include <exception>
#include <iostream>
#include <opae/types_enum.h>
#include <opae/cxx/core/errors.h>

namespace intel
{
namespace fpga
{

class port_error : public std::exception
{
public:
    port_error(uint64_t err);
    port_error(const port_error & other)
    : err_(other.err_)
    {
    }
    port_error & operator=(const port_error & other)
    {
        if(this != &other) err_ = other.err_;
        return *this;
    }
    static uint64_t read(opae::fpga::types::token::ptr_t tok);
    static uint64_t clear(opae::fpga::types::token::ptr_t tok, uint64_t errs);

    uint64_t value() const { return err_; }
    friend std::ostream & operator<<(std::ostream & str, const port_error &err);
private:
    uint64_t err_;
    opae::fpga::types::error::ptr_t port_err;
};

class fpga_error : public std::exception
{
public:
    fpga_error(fpga_result res);
    virtual const char* what() const noexcept;
    friend std::ostream & operator<<(std::ostream & str, const fpga_error &err);
private:
    fpga_result result_;
};

} // end of namespace fpga
} // end of namespace intel

