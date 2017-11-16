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
#include <string>
#include <cstdint>
#include <ostream>
#include "mmio.h"

namespace intel
{
namespace fpga
{

class mmio_resource : public mmio
{
public:
    typedef std::shared_ptr<mmio_resource> ptr_t;

    mmio_resource(const std::string & resource, bool read_only);

    virtual ~mmio_resource();
    bool is_open()
    {
        return fd_ != -1 && mmio_ != nullptr;
    }

    virtual bool read_mmio64(uint32_t offset, uint64_t & value)
    {
        if (base_ptr() && offset < mmap_size_)
        {
            volatile uint8_t* ptr = base_ptr() + offset;
            value = *reinterpret_cast<volatile uint64_t*>(ptr);
            return true;
        }
        return false;
    }

    virtual bool write_mmio64(uint32_t offset, uint64_t value)
    {
        if (base_ptr() && offset < mmap_size_)
        {
            volatile uint8_t* ptr = base_ptr() + offset;
            *reinterpret_cast<volatile uint64_t*>(ptr) = value;
            return true;
        }
        return false;
    }

    virtual bool read_mmio32(uint32_t offset, uint32_t & value)
    {
        if (base_ptr() && offset < mmap_size_)
        {
            volatile uint8_t* ptr = base_ptr() + offset;
            value = *reinterpret_cast<volatile uint32_t*>(ptr);
            return true;
        }
        return false;
    }

    virtual bool write_mmio32(uint32_t offset, uint32_t value)
    {
        if (base_ptr() && offset < mmap_size_)
        {
            volatile uint8_t* ptr = base_ptr() + offset;
            *reinterpret_cast<volatile uint32_t*>(ptr) = value;
            return true;
        }
        return false;
    }

    virtual uint8_t* mmio_pointer(uint32_t offset)
    {
        return reinterpret_cast<uint8_t*>(mmio_) + offset;
    }

    volatile uint8_t* base_ptr()
    {
        return reinterpret_cast<volatile uint8_t*>(mmio_);
    }

    mmio_resource & ref()
    {
        return *this;
    }

    const mmio_resource & const_ref() const
    {
        return *this;
    }

    volatile uint64_t & operator[](uint32_t offset);

    static ptr_t open(std::string resource, bool read_only = false);

private:
    mmio_resource()
    : mmio_(nullptr)
    , resource_("")
    , fd_(-1)
    , mmap_size_(0)
    {
    }

    void * mmio_;
    std::string resource_;
    int fd_;
    uint32_t mmap_size_;
};

} // end of namespace fpga
} // end of namespace intel
