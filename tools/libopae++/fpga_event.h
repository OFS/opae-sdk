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
#include <thread>
#include <atomic>
#include <vector>
#include <opae/event.h>

namespace intel
{
namespace fpga
{

class fpga_event
{
public:
    typedef std::shared_ptr<fpga_event> ptr_t;

    enum class event_type : uint32_t
    {
        interrupt = 0,
        error,
        power_thermal
    };

    enum class poll_result : uint32_t
    {
        unknown = 0,
        error,
        triggered,
        timeout,
        canceled
    };

    fpga_event(event_type etype, fpga_event_handle handle);

    virtual ~fpga_event();

    poll_result poll(int timeout_msec = -1);

    void notify(void (*callback)(void*, event_type), void* data = nullptr);

    void cancel();

private:
    event_type type_;
    std::atomic_bool cancel_;
    fpga_event_handle handle_;
    int epollfd_;
    std::vector<std::thread> callbacks_;
};

} // end of namespace fpga
} // end of namespace intel
