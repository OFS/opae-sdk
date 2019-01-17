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
#include "option_map.h"
#include "fpga_app/accelerator_app.h"
#include "csr.h"
#include "log.h"
#include <chrono>

#define AFU_DFH_EOL_OFFSET 40
#define NEXT_DFH_OFFSET(dfh) ((dfh >> 16) & 0xffffff)
#define DFH_EOL(dfh) ((((dfh >> AFU_DFH_EOL_OFFSET) & 1) == 1) || \
                        (NEXT_DFH_OFFSET(dfh)==0))
#define swap64(type, ptr) (((type)(*(ptr))<<56)  | \
                          ((type)(*(ptr+1))<<48) | \
                          ((type)(*(ptr+2))<<40) | \
                          ((type)(*(ptr+3))<<32) | \
                          ((type)(*(ptr+4))<<24) | \
                          ((type)(*(ptr+5))<<16) | \
                          ((type)(*(ptr+6))<<8)  | \
                          ((type)(*(ptr+7))))

namespace intel
{
namespace fpga
{
namespace diag
{

class nlb0 : public intel::fpga::accelerator_app
{
public:
    nlb0();
    nlb0(const std::string & name)
    : accelerator_app(name)
    {
        nlb0();
    }
    ~nlb0();

    virtual intel::utils::option_map & get_options()          override { return options_; }
    virtual void                       assign(opae::fpga::types::handle::ptr_t accelerator) override { accelerator_ = accelerator;      }
    virtual const std::string &        afu_id()               override { return afu_id_;  }
    virtual const std::string &        name()                 override { return name_;    }
    virtual bool                       setup()                override;
    virtual bool                       run()                  override;
    virtual opae::fpga::types::shared_buffer::ptr_t          dsm()            const override { return dsm_; }
    virtual uint64_t                   cachelines()     const override { return cachelines_; }

    void show_help(std::ostream &os);


private:
    std::string name_;
    std::string config_;
    std::string target_;
    std::string afu_id_;
    std::string nlb0_id_;

    std::size_t dsm_size_;
    uint32_t step_;
    uint32_t begin_;
    uint32_t end_;
    uint32_t frequency_;
    bool cont_;

    intel::utils::logger log_;
    intel::utils::option_map options_;

    opae::fpga::types::handle::ptr_t accelerator_;
    opae::fpga::types::shared_buffer::ptr_t dsm_;
    csr_t<uint32_t> cfg_;

    std::chrono::duration<double> cont_timeout_;
    std::chrono::microseconds     dsm_timeout_;

    bool suppress_header_;
    bool csv_format_;
    bool suppress_stats_;
    uint64_t cachelines_;
    uint32_t offset_;
};

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

