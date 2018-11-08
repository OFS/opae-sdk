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
#include <cmath>
#include <memory>
#include "accelerator.h"
#include "buffer_pool.h"

namespace intel
{
namespace fpga
{

class accelerator_mux : public accelerator
{
public:
    typedef std::shared_ptr<accelerator_mux> ptr_t;

    accelerator_mux(accelerator::ptr_t accelerator_ptr, uint32_t count, uint32_t mux_id, buffer_pool::ptr_t pool = buffer_pool::ptr_t(0))
    : accelerator(*accelerator_ptr)
    , count_(count)
    , mux_id_(mux_id)
    , pool_(pool)
    {
        double bits = std::ceil(std::log2(count));
        mask32_ = static_cast<uint32_t>(mux_id) << static_cast<uint32_t>(18-bits);
    }

    virtual bool write_mmio32(unsigned int offset, uint32_t value)
    {
        return accelerator::write_mmio32(mask32_ | offset, value);
    }

    virtual bool write_mmio64(unsigned int offset, uint64_t value)
    {
        return accelerator::write_mmio64(mask32_ | offset, value);
    }

    virtual bool read_mmio32(unsigned int offset, unsigned int & value)
    {
        return accelerator::read_mmio32(mask32_ | offset, value);
    }

    virtual bool read_mmio64(unsigned int offset, uint64_t & value)
    {
        return accelerator::read_mmio64(mask32_ | offset, value);
    }

    virtual opae::fpga::types::shared_buffer::ptr_t allocate_buffer(std::size_t size)
    {
        if (pool_)
        {
            return pool_->allocate_buffer(size);
        }
        return accelerator::allocate_buffer(size);
    }

    virtual bool reset()
    {
        return true;
    }

private:
    uint32_t           count_;
    uint32_t           mux_id_;
    uint32_t           mask32_;
    buffer_pool::ptr_t pool_;

};

} // end of namespace fpga
} // end of namespace intel
