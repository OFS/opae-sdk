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
#include "fpga_event.h"
#include <sys/epoll.h>
#include <chrono>

using namespace std::chrono;

namespace intel
{
namespace fpga
{

fpga_event::fpga_event(fpga_event::event_type etype, fpga_event_handle handle)
: type_(etype)
, cancel_(false)
, handle_(handle)
{
    epollfd_ = epoll_create(1);
    if (epollfd_ > 0)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = handle_;
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, handle_, &ev) == -1)
        {
            epollfd_ = -1;
        }
    }
}

fpga_event::~fpga_event()
{
    cancel();
    for (auto & t : callbacks_)
    {
        t.join();
    }
    fpgaDestroyEventHandle(&handle_);
}

fpga_event::poll_result fpga_event::poll(int timeout_msec)
{
    struct epoll_event events[1];
    if (epollfd_ == -1)
    {
        return poll_result::error;
    }
    if (cancel_) return poll_result::canceled;
    int num_events = 0;
    auto begin = high_resolution_clock::now();
    bool timedout = false;
    while(!cancel_ && !timedout && num_events == 0)
    {
        num_events = epoll_wait(epollfd_, events, 1, 0);
        if (timeout_msec > 0)
        {
            timedout = high_resolution_clock::now() - begin > milliseconds(timeout_msec) &&
                       num_events == 0;
        }

        if (!timedout)
        {
            std::this_thread::sleep_for(microseconds(100));
        }
    }

    if (cancel_) return poll_result::canceled;

    if (timedout)
    {
        return poll_result::timeout;
    }

    if (num_events < 0)
    {
        return errno == EINTR ? poll_result::canceled : poll_result::error;
    }

    if (num_events == 1 && events[0].data.fd == handle_)
    {
        return poll_result::triggered;
    }

    return poll_result::unknown;
}

void fpga_event::cancel()
{
    cancel_ = true;
}

void fpga_event::notify(void (*callback)(void*, fpga_event::event_type), void* data)
{
    std::thread t([this, callback, data](){
        while(!cancel_)
        {
            if (poll(-1) == poll_result::triggered)
            {
                callback(data, type_);
            }
        }
    });
    callbacks_.push_back(std::move(t));

}




} // end of namespace fpga
} // end of namespace intel
