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
#include <deque>
#include <atomic>
#include <mutex>
#include "nlb.h"
#include "option_map.h"
#include "fpga_app/accelerator_app.h"
#include "accelerator.h"
#include "dma_buffer.h"
#include "csr.h"
#include "log.h"
#include "perf_counters.h"
#include "nlb_stats.h"

namespace intel
{
namespace fpga
{
namespace diag
{

class cmdq3 : public intel::fpga::accelerator_app
{
public:
    cmdq3();
    ~cmdq3();

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
    std::string mode_;
    uint32_t allocations_;
    uint32_t cachelines_;
    bool cont_;
    uint32_t timeout_usec_;
    uint32_t timeout_msec_;
    uint32_t timeout_sec_;
    uint32_t timeout_min_;
    uint32_t timeout_hour_;
    uint32_t freq_;
    uint32_t multi_cl_;
    std::string read_vc_;
    std::string write_vc_;
    std::string wrfence_vc_;
    bool cool_fpga_cache_;
    bool warm_fpga_cache_;
    std::string guid_;

    uint32_t dsm_size_;
    double timeout_;

    csr_t<uint32_t> cfg_;

    intel::utils::logger log_;
    intel::utils::option_map options_;

    accelerator::ptr_t accelerator_;
    dma_buffer::ptr_t wkspc_;
    dma_buffer::ptr_t dsm_;
    intel::fpga::nlb::dsm_tuple dsm_tuple_;

    typedef std::pair< dma_buffer::ptr_t, dma_buffer::ptr_t > cmdq_entry_t;
    typedef std::deque< cmdq_entry_t > cmdq_t;

    cmdq_t fifo1_;
    uint64_t poll_sleep_ns_;
    std::atomic_bool cancel_;
    std::atomic<uint32_t> swvalid_next_;
    std::atomic<uint32_t> hwvalid_next_;
    volatile uint64_t *src_address_;
    volatile uint64_t *dst_address_;
    volatile uint32_t *sw_valid_;
    uint32_t assignments_;
    uint32_t iterations_;

    int errors_;
    bool suppress_header_;
    bool csv_format_;
    bool batch_mode_;

    std::mutex print_lock_;

    fpga_cache_counters start_cache_ctrs_;
    fpga_fabric_counters start_fabric_ctrs_;
    fpga_cache_counters end_cache_ctrs_;
    fpga_fabric_counters end_fabric_ctrs_;

    inline bool apply(const cmdq_entry_t &entry);
    inline bool apply_ptr(const cmdq_entry_t &entry);


    void swvalid_thr();
    void hwvalid_thr();
    void cont_swvalid_thr();
    void cont_hwvalid_thr();

    bool wait_for_done(uint32_t allocations);

};

} // end of namespace diag
} // end of namespace fpga
} // end of namespace intel

