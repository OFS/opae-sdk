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
#include <mutex>
#include "fpga_common.h"
#include <opae/cxx/core/shared_buffer.h>
#include "buffer_utils.h"

namespace intel
{
namespace fpga
{

class buffer_pool
{
public:
    typedef std::shared_ptr<buffer_pool> ptr_t;

    buffer_pool(opae::fpga::types::shared_buffer::ptr_t buffer)
    : buffer_(buffer)
    , addr_offset_(0)
    , io_address_offset_(0)
    {
        if (buffer)
        {
            addr_offset_ = const_cast<uint8_t*>(buffer->c_type());
            io_address_offset_ = buffer->io_address();
        }
    }


    opae::fpga::types::shared_buffer::ptr_t allocate_buffer(std::size_t size)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        opae::fpga::types::shared_buffer::ptr_t buffer(0);
        // round up to next multiple of 4MB
        auto next_size = size;
        if (next_size % MB(4) != 0)
        {
            next_size = (size/MB(4) + 1)*MB(4);
        }

        if (!buffer_ || addr_offset_ + next_size > buffer_->c_type() + buffer_->size())
        {
            // TODO: Log some sort of error or throw an exception?
            // but for now return a null buffer
            return buffer;
        }
        buffer.reset(new split_buffer(buffer_, next_size, addr_offset_, buffer_->wsid(), io_address_offset_));
        addr_offset_ += next_size;
        io_address_offset_ += next_size;
        return buffer;
    }

private:
    opae::fpga::types::shared_buffer::ptr_t buffer_;
    uint8_t          *addr_offset_;
    uint64_t          io_address_offset_;
    size_t size_;
    std::mutex        mutex_;

};

} // end of namespace fpga
} // end of namespace intel

