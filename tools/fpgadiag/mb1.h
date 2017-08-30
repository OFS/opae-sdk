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
#include <iostream>
#include <chrono>
#include "nlb.h"
#include "option_map.h"
#include "fpga_app/accelerator_app.h"
#include "accelerator.h"
#include "dma_buffer.h"
#include "csr.h"
#include "log.h"

namespace intel
{
namespace fpga
{
namespace diag
{

class mb1 : public intel::fpga::accelerator_app
{
public:
    mb1();
    ~mb1();

    virtual intel::utils::option_map & get_options()          override { return options_; }
    virtual void                       assign(accelerator::ptr_t accelerator) override { accelerator_ = accelerator;      }
    virtual const std::string &        afu_id()               override { return guid_;    }
    virtual const std::string &        name()                 override { return name_;    }
    virtual bool                       setup()                override;
    virtual bool                       run()                  override;

    void show_help(std::ostream &os);

private:
    std::string name_;
    std::string config_;
    std::string target_;
    uint32_t cachelines_;
    uint32_t num_sw_repeat_;
    std::string read_vc_;
    std::string cache_hint_;
    bool cool_cpu_cache_;
    std::string guid_;
    uint32_t frequency_;

    std::size_t wkspc_size_;
    std::size_t dsm_size_;
    std::size_t inp_size_;
    std::size_t out_size_;
    std::size_t rd_lines_size_;
    std::size_t cool_cache_size_;
    bool suppress_header_;
    bool csv_format_;

    intel::utils::logger log_;
    intel::utils::option_map options_;

    accelerator::ptr_t accelerator_;
    csr_t<uint32_t> cfg_;
    dma_buffer::ptr_t wkspc_;
};

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

