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

#include "mtnlb7.h"
#include "fpga_app/fpga_common.h"
#include <chrono>
#include <thread>

using namespace std::chrono;
using namespace intel::utils;
using namespace intel::fpga::nlb;

namespace intel
{
namespace fpga
{
namespace diag
{

const std::string AFUID = "B6B76CDF-4012-4671-AA2B-3612B792E59A";

mtnlb7::mtnlb7()
: mtnlb(AFUID, "mtnlb7")
{
    mode_ = "mt7";
    config_ = "mtnlb7.json";
}

mtnlb7::~mtnlb7()
{
}

void mtnlb7::work(uint64_t thread_id, uint64_t iterations, uint64_t stride)
{
    // wait for the signal to start
    while (!ready_);
    uint64_t iteration = 1;
    volatile uint64_t* out_ptr = reinterpret_cast<volatile uint64_t*>(out_->address() + thread_id*stride*cacheline_size);
    volatile uint64_t* inp_ptr = reinterpret_cast<volatile uint64_t*>(inp_->address() + thread_id*stride*cacheline_size);

    while (!cancel_ && iteration < iterations+1)
    {
        auto begin = system_clock::now();
        while (!cancel_ && *out_ptr != iteration)
        {
            if (system_clock::now() - begin >= seconds(1))
            {
                log_.error(mode_) << "thread [" << thread_id << "]: timeout waiting for value: " << iteration << std::endl;
                log_.error(mode_) << "thread [" << thread_id << "]: Expected: " << iteration << ". Got: " << *out_ptr << std::endl;
                if (!cancel_)
                {
                    show_pending(thread_id);
                    cancel_ = true;
                    break;
                }
            }
            else
            {
                std::this_thread::sleep_for(microseconds(10));
            }
        }

        if (!cancel_)
        {
            log_.debug(mode_) << "thread [" << thread_id << "]: iteration="<< iteration << std::endl;
            *inp_ptr = iteration++;
        }
    }

    if (!stop_)
    {
        stop_ = true;
    }

    log_.debug(mode_) << "thread[" << thread_id << "]: Normal exit" << std::endl;
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel
