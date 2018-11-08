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

#include <thread>
#include <chrono>
#include "fpga_app/fpga_common.h"
#include "nlb_cache_prime.h"
#include "buffer_utils.h"

using namespace opae::fpga::types;

namespace intel
{
namespace fpga
{
namespace nlb
{

nlb_cache_cool::nlb_cache_cool(const std::string & target,
                               handle::ptr_t accelerator,
                               shared_buffer::ptr_t dsm,
                               shared_buffer::ptr_t cool_buf,
                               bool cmdq)
: target_(target)
, accelerator_(accelerator)
, dsm_(dsm)
, cool_buf_(cool_buf)
, cmdq_(cmdq)
{
    const uint32_t cool_data = 0xc001c001;

    volatile uint32_t *pCool    = (volatile uint32_t *) cool_buf_->c_type();
    volatile uint32_t *pEndCool = pCool + (cool_buf_->size() / sizeof(uint32_t));

    for ( ; pCool < pEndCool ; ++pCool)
        *pCool = cool_data;
}

bool nlb_cache_cool::cool()
{
    bool res = true;

    // set dsm base, high then low
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->io_address()));
    // assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
    // clear the DSM
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 1);
    // set input workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::src_addr), CACHELINE_ALIGNED_ADDR(cool_buf_->io_address()));
    // set number of cache lines for test
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::num_lines), cool_buf_->size() / CL(1));

    const uint32_t test_mode = static_cast<uint32_t>(nlb0_ctl::read) |
                               static_cast<uint32_t>(nlb0_ctl::rdi)  |
                               static_cast<uint32_t>(nlb0_ctl::read_vl0);

    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cfg), test_mode);

    // start the test
    if (cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), 1);
    }
    else
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 3);
    }

    // wait for test complete
    std::chrono::microseconds dsm_timeout = (target_ == "ase") ? ASE_DSM_TIMEOUT : FPGA_DSM_TIMEOUT;
    if (!buffer_wait<uint32_t>(dsm_,
                              static_cast<size_t>(nlb0_dsm::test_complete),
                              std::chrono::microseconds(10),
                              dsm_timeout,
                              static_cast<uint32_t>(0x1),
                              static_cast<uint32_t>(1)))
    {
        res = false;
    }

    // stop the device
    if (!cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
    }

    return res;
}


nlb_read_cache_warm::nlb_read_cache_warm(const std::string & target,
                                         handle::ptr_t accelerator,
                                         shared_buffer::ptr_t dsm,
                                         shared_buffer::ptr_t src_buf,
                                         shared_buffer::ptr_t dst_buf,
                                         bool cmdq)
: target_(target)
, accelerator_(accelerator)
, dsm_(dsm)
, src_buf_(src_buf)
, dst_buf_(dst_buf)
, cmdq_(cmdq)
{}

bool nlb_read_cache_warm::warm()
{
    bool res = true;

    // set dsm base, high then low
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->io_address()));
    // assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
    // clear the DSM
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 1);
    // set input workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::src_addr), CACHELINE_ALIGNED_ADDR(src_buf_->io_address()));
    // set output workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::dst_addr), CACHELINE_ALIGNED_ADDR(dst_buf_->io_address()));
    // set number of cache lines for test
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::num_lines), src_buf_->size() / CL(1));

    const uint32_t test_mode = static_cast<uint32_t>(nlb0_ctl::read) |
                               static_cast<uint32_t>(nlb0_ctl::read_vl0);
 
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cfg), test_mode);

    // start the test
    if (cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), 1);
    }
    else
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 3);
    }

    // wait for test complete
    std::chrono::microseconds dsm_timeout = (target_ == "ase") ? ASE_DSM_TIMEOUT : FPGA_DSM_TIMEOUT;
    if (!buffer_wait<uint32_t>(dsm_,
                              static_cast<size_t>(nlb0_dsm::test_complete),
                              std::chrono::microseconds(10),
                              dsm_timeout,
                              static_cast<uint32_t>(0x1),
                              static_cast<uint32_t>(1)))
    {
        res = false;
    }

    // stop the device
    if (!cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
    }

    return res;
}


nlb_write_cache_warm::nlb_write_cache_warm(const std::string & target,
                                           handle::ptr_t accelerator,
                                           shared_buffer::ptr_t dsm,
                                           shared_buffer::ptr_t src_buf,
                                           shared_buffer::ptr_t dst_buf,
                                           bool cmdq)
: target_(target)
, accelerator_(accelerator)
, dsm_(dsm)
, src_buf_(src_buf)
, dst_buf_(dst_buf)
, cmdq_(cmdq)
{}

bool nlb_write_cache_warm::warm()
{
    bool res = true;

    // set dsm base, high then low
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_dsm::basel), reinterpret_cast<uint64_t>(dsm_->io_address()));
    // assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 0);
    // clear the DSM
    dsm_->fill(0);
    // de-assert afu reset
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 1);
    // set input workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::src_addr), CACHELINE_ALIGNED_ADDR(src_buf_->io_address()));
    // set output workspace address
    accelerator_->write_csr64(static_cast<uint32_t>(nlb0_csr::dst_addr), CACHELINE_ALIGNED_ADDR(dst_buf_->io_address()));
    // set number of cache lines for test
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::num_lines), src_buf_->size() / CL(1));

    const uint32_t test_mode = static_cast<uint32_t>(nlb0_ctl::write) |
                               static_cast<uint32_t>(nlb0_ctl::write_vl0);
 
    accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cfg), test_mode);

    // start the test
    if (cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::cmdq_sw), 1);
    }
    else
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 3);
    }

    // wait for test complete
    std::chrono::microseconds dsm_timeout = (target_ == "ase") ? ASE_DSM_TIMEOUT : FPGA_DSM_TIMEOUT;
    if (!buffer_wait<uint32_t>(dsm_,
                              static_cast<size_t>(nlb0_dsm::test_complete),
                              std::chrono::microseconds(10),
                              dsm_timeout,
                              static_cast<uint32_t>(0x1),
                              static_cast<uint32_t>(1)))
    {
        res = false;
    }

    // stop the device
    if (!cmdq_)
    {
        accelerator_->write_csr32(static_cast<uint32_t>(nlb0_csr::ctl), 7);
    }

    return res;
}

} // end of namespace nlb
} // end of namespace fpga
} // end of namespace intel

