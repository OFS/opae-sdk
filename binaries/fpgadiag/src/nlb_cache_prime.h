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

#pragma once
#include "nlb.h"
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>

namespace intel
{
namespace fpga
{
namespace nlb
{

class nlb_cache_cool
{
public:
static const uint32_t fpga_cache_cool_size = CL(1024);

    nlb_cache_cool(const std::string & target,
                   opae::fpga::types::handle::ptr_t accelerator,
                   opae::fpga::types::shared_buffer::ptr_t dsm,
                   opae::fpga::types::shared_buffer::ptr_t cool_buf,
                   bool cmdq=false);
    bool cool();

private:
    std::string target_;
    opae::fpga::types::handle::ptr_t accelerator_;
    opae::fpga::types::shared_buffer::ptr_t dsm_;
    opae::fpga::types::shared_buffer::ptr_t cool_buf_;
    bool cmdq_;
};

class nlb_read_cache_warm
{
public:
    nlb_read_cache_warm(const std::string & target,
                        opae::fpga::types::handle::ptr_t accelerator,
                        opae::fpga::types::shared_buffer::ptr_t dsm,
                        opae::fpga::types::shared_buffer::ptr_t src_buf,
                        opae::fpga::types::shared_buffer::ptr_t dst_buf,
                        bool cmdq=false);
    bool warm();

private:
    std::string target_;
    opae::fpga::types::handle::ptr_t accelerator_;
    opae::fpga::types::shared_buffer::ptr_t dsm_;
    opae::fpga::types::shared_buffer::ptr_t src_buf_;
    opae::fpga::types::shared_buffer::ptr_t dst_buf_;
    bool cmdq_;
};

class nlb_write_cache_warm
{
public:
    nlb_write_cache_warm(const std::string & target,
                         opae::fpga::types::handle::ptr_t accelerator,
                         opae::fpga::types::shared_buffer::ptr_t dsm,
                         opae::fpga::types::shared_buffer::ptr_t src_buf,
                         opae::fpga::types::shared_buffer::ptr_t dst_buf,
                         bool cmdq=false);
    bool warm();

private:
    std::string target_;
    opae::fpga::types::handle::ptr_t accelerator_;
    opae::fpga::types::shared_buffer::ptr_t dsm_;
    opae::fpga::types::shared_buffer::ptr_t src_buf_;
    opae::fpga::types::shared_buffer::ptr_t dst_buf_;
    bool cmdq_;
};

} // end of namespace nlb
} // end of namespace fpga
} // end of namespace intel

