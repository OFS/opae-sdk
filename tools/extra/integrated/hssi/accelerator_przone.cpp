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

#include "accelerator_przone.h"
#include <thread>
#include <chrono>

namespace intel
{
namespace fpga
{
namespace hssi
{

using namespace intel::fpga;
using std::chrono::microseconds;

enum
{
    mmio_delay_usec = 1
};


accelerator_przone::accelerator_przone(opae::fpga::types::handle::ptr_t h)
: handle_(h)
{

}

bool accelerator_przone::read(uint32_t address, uint32_t & value)
{
    // add delay to account for timing differences in AFU logic and ETH logic
    std::this_thread::sleep_for(microseconds(mmio_delay_usec));
    uint32_t msg = static_cast<uint32_t>(przone_cmd::read) | address;
    handle_->write_csr32(static_cast<uint32_t>(mmio_reg::afu_ctrl), msg);
    value = handle_->read_csr32(static_cast<uint32_t>(mmio_reg::afu_rd_data));
    return true;
}

bool accelerator_przone::write(uint32_t address, uint32_t value)
{
    handle_->write_csr32(static_cast<uint32_t>(mmio_reg::afu_wr_data), value);
    uint32_t msg = static_cast<uint32_t>(przone_cmd::write) | address;
    handle_->write_csr32(static_cast<uint32_t>(mmio_reg::afu_ctrl), msg);
    return true;
}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel
