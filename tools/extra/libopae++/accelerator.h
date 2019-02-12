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
#include <atomic>
#include <cstdint>
#include <opae/fpga.h>
#include "option_map.h"
#include "fpga_resource.h"
#include "log.h"
#include "perf_counters.h"
#include <opae/cxx/core/shared_buffer.h>

namespace intel
{
namespace fpga
{

class accelerator : public fpga_resource
{
public:
    virtual ~accelerator() { close(); }

    enum status_t
    {
        unknown = 0,
        opened,
        released,
        closed
    };

    fpga_resource::type_t type();

    virtual bool open(bool shared);

    virtual bool close();

    typedef std::shared_ptr<accelerator> ptr_t;

    static std::vector<ptr_t> enumerate(std::vector<intel::utils::option_map::ptr_t> options);

    virtual bool write_mmio32(uint32_t offset, uint32_t value);

    virtual bool write_mmio64(uint32_t offset, uint64_t value);

    virtual bool read_mmio32(uint32_t offset, uint32_t & value);

    virtual bool read_mmio64(uint32_t offset, uint64_t & value);

    virtual uint8_t* mmio_pointer(uint32_t offset);

    virtual bool reset();

    virtual bool ready();

    virtual void release();

    virtual opae::fpga::types::shared_buffer::ptr_t allocate_buffer(std::size_t size);

    virtual uint64_t umsg_num();

    virtual bool umsg_set_mask(uint64_t mask);

    virtual uint64_t * umsg_get_ptr();

    fpga_cache_counters cache_counters();

    fpga_fabric_counters fabric_counters();

    uint64_t port_errors() const { return port_errors_; }

    void throw_errors(bool value) { throw_errors_ = value; }
    void error_assert(bool update = false);
protected:
    accelerator(const accelerator & other);
    accelerator & operator=(const accelerator & other);

private:

    accelerator(opae::fpga::types::token::ptr_t token, opae::fpga::types::properties::ptr_t props, opae::fpga::types::token::ptr_t parent);

    status_t status_;
    uint8_t * mmio_base_;
    opae::fpga::types::event::ptr_t error_event_;
    std::atomic<uint64_t> port_errors_;
    bool throw_errors_;
};

} // end of namespace fpga
} // end of namespace intel
