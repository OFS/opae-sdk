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

#include "mtnlb8.h"
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

const std::string AFUID = "2D9C88FE-2BE5-4105-8A1A-EEEC94364467";

mtnlb8::mtnlb8(const std::string & name)
: mtnlb(AFUID, "mtnlb8")
{
    mode_ = "mt8";
    config_ = "mtnlb8.json";
}

mtnlb8::mtnlb8()
: mtnlb(AFUID, "mtnlb8")
{
    mode_ = "mt8";
    config_ = "mtnlb8.json";
}

mtnlb8::~mtnlb8()
{
}

void mtnlb8::work(uint64_t thread_id, uint64_t iterations, uint64_t stride)
{
    // wait for the signal to start
    while (!ready_);

    uint64_t offset = 0;

    volatile uint64_t* out_ptr = reinterpret_cast<volatile uint64_t*>(out_->address() + thread_id*stride*cacheline_size);
    uint64_t cpuCount = 2, fpgaCount = 1;
    while (!cancel_ && fpgaCount <= mtnlb_max_count)
    {
        auto begin = system_clock::now();
        while (!cancel_ && *out_ptr != fpgaCount)
        {
            if (system_clock::now() - begin >= seconds(1))
            {
                log_.debug(mode_) << "thread [" << thread_id << "]: timeout waiting for value" << std::endl;
                if (!cancel_)
                {
                    // cancel threads
                    cancel_ = true;
                    // stop the test
                    stop_ = true;
                    log_.warn(mode_)  << "thread [" << thread_id << "]: Timed out waiting for expected value from FPGA" << std::endl;
                    log_.warn(mode_)  << "thread [" << thread_id << "]: Expected: " << fpgaCount << ". Got: " << *out_ptr << std::endl;
                    show_pending(thread_id);
                    return;
                }
            }
            else
            {
                std::this_thread::sleep_for(microseconds(10));
            }
        }

        if (!cancel_)
        {
            //log_.info() << "thread [" << thread_id << "]: fpgaCount ="<< *out_ptr << std::endl;
            log_.debug(mode_) << "thread[" << thread_id << "] [FPGA] count is: " << *out_ptr << std::endl;
            fpgaCount += 2;
            if (cpuCount <= mtnlb_max_count)
            {
                *out_ptr = cpuCount;
            }
            log_.debug(mode_) << "thread[" << thread_id << "] [CPU] count is: " << *out_ptr << std::endl;
            //log_.info() << "thread [" << thread_id << "]: cpuCount  ="<< *out_ptr << std::endl;

            if (cpuCount == mtnlb_max_count || fpgaCount > mtnlb_max_count )
            {
                // stop the test
                if (!stop_)
                {
                    stop_ = true;
                }
                return;
            }
            cpuCount += 2;
        }
    }

    log_.debug() << "thread[" << thread_id << "]: Normal exit" << std::endl;
}

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel
