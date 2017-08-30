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
#include <string.h>
#include <vector>
#include <initializer_list>
#include <chrono>
#include <thread>
#include <opae/fpga.h>

namespace intel
{
namespace fpga
{

const std::chrono::microseconds FPGA_DSM_TIMEOUT{1000000};
const std::chrono::microseconds ASE_DSM_TIMEOUT{1000000L*100000};

class dma_buffer
{
public:
    typedef std::shared_ptr<dma_buffer> ptr_t;

    dma_buffer(fpga_handle handle, uint64_t wsid, uint8_t* virt, uint64_t iova, std::size_t size) :
        handle_(handle),
        wsid_(wsid),
        virtual_address_(virt),
        iova_(iova),
        size_(size)
    {
    }

    dma_buffer(ptr_t parent, uint8_t* virt, uint64_t iova, std::size_t size)
    : handle_(nullptr)
    , wsid_(0)
    , virtual_address_(virt)
    , iova_(iova)
    , size_(size)
    , parent_(parent)
    {
    }

    ~dma_buffer()
    {
        if (handle_ != nullptr)
        {
            fpgaReleaseBuffer(handle_, wsid_);
            handle_ = nullptr;
        }
    }

    template <typename T>
    static std::vector<ptr_t> split(ptr_t parent, std::initializer_list<T> sizes)
    {
        std::vector<ptr_t> v;
        std::size_t offset = 0;

        v.reserve(sizes.size());

        for (const auto &sz : sizes)
        {
            ptr_t p;
            p.reset(new dma_buffer(parent, parent->virtual_address_ + offset, parent->iova_ + offset, sz));
            v.push_back(p);
            offset += sz;
        }

        return v;
    }

    volatile uint8_t* address() const { return virtual_address_; }
    uint64_t iova() const { return iova_; }
    std::size_t size() const { return size_; }

    void fill(uint32_t value)
    {
        ::memset(virtual_address_, value, size_);
    }

    template<typename T>
    void write(const T& value, std::size_t offset = 0)
    {
        if ((offset < size_) && (virtual_address_ != nullptr))
        {
            *reinterpret_cast<T*>(virtual_address_ + offset) = value;
        }
    }

    template<typename T>
    T read(std::size_t offset = 0) const
    {
        if ((offset < size_) && (virtual_address_ != nullptr))
        {
            return *reinterpret_cast<T*>(virtual_address_ + offset);
        }
        return T();
    }

    bool equal(dma_buffer::ptr_t other, size_t size) const
    {
        return ::memcmp(virtual_address_, other.get()->virtual_address_, size) == 0;
    }

    typedef std::chrono::microseconds microseconds_t;

    template<typename T>
    bool poll(std::size_t offset, microseconds_t timeout, T mask, T value) const
    {
        time_point_t start = std::chrono::high_resolution_clock::now();
        microseconds_t elapsed;

        do
        {
            if ((read<T>(offset) & mask) == value)
                return true;

            elapsed = std::chrono::duration_cast<microseconds_t>(
                        std::chrono::high_resolution_clock::now() - start);

        }while(elapsed < timeout);

        return false;
    }

    template<typename T>
    bool wait(std::size_t offset, microseconds_t each, microseconds_t timeout, T mask, T value) const
    {
        time_point_t start = std::chrono::high_resolution_clock::now();
        microseconds_t elapsed;

        do
        {
            if ((read<T>(offset) & mask) == value)
                return true;

            std::this_thread::sleep_for(each);

            elapsed = std::chrono::duration_cast<microseconds_t>(
                        std::chrono::high_resolution_clock::now() - start);

        }while(elapsed < timeout);

        return false;
    }

private:
    typedef std::chrono::high_resolution_clock::time_point time_point_t;
    typedef std::chrono::duration<double> duration_t;


    fpga_handle handle_;
    uint64_t wsid_;
    uint8_t* virtual_address_;
    uint64_t iova_;
    std::size_t size_;
    ptr_t parent_;
};

} // end of namespace fpga
} // end of namespace intel
