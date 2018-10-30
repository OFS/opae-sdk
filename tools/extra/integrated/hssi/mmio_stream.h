// Copyright(c) 2017-2018, Intel Corporation
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
#include "mmio.h"
#include "utils.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

class mmio_stream : public mmio
{
public:
    typedef std::shared_ptr<mmio_stream> ptr_t;

    mmio_stream(int indent = 1)
        : stream_(std::cout)
        , indent_(indent)
    {

    }

    mmio_stream(std::ostream & stream, int indent = 1)
        : stream_(stream)
        , indent_(indent)
    {

    }

    virtual ~mmio_stream()
    {
    }

    virtual bool write_mmio32(uint32_t offset, uint32_t value) override
    {
        UNUSED_PARAM(offset);
        for (int i = 0; i < indent_; ++i)
        {
            stream_ << "\t";
        }
        stream_  << intel::utils::print_hex<uint32_t>(value);
        return true;
    }

    virtual bool write_mmio64(uint32_t offset, uint64_t value) override
    {
        UNUSED_PARAM(offset);
        for (int i = 0; i < indent_; ++i)
        {
            stream_ << "\t";
        }
        stream_  << intel::utils::print_hex<uint64_t>(value);
        return true;
    }

    virtual bool read_mmio32(uint32_t offset, uint32_t & value) override
    {
        UNUSED_PARAM(offset);
        UNUSED_PARAM(value);
        for (int i = 0; i < indent_; ++i)
        {
            stream_ << "\t";
        }
        return true;
    }

    virtual bool read_mmio64(uint32_t offset, uint64_t & value) override
    {
        UNUSED_PARAM(offset);
	UNUSED_PARAM(value);
        for (int i = 0; i < indent_; ++i)
        {
            stream_ << "\t";
        }
        return true;
    }

    virtual uint8_t * mmio_pointer(uint32_t offset) override
    {
        UNUSED_PARAM(offset);
        return nullptr;
    }

private:
    std::ostream & stream_;
    int indent_;

};


} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel
