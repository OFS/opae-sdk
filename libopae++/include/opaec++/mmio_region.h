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
#include <cstdint>
#include "mmio.h"
#include "handle.h"

namespace opae
{
namespace fpga
{
namespace io
{

class mmio_region : public mmio
{
public:
    enum class id_t : int32_t
    {
        AFU,
        STP
    };

    static mmio::ptr_t region(opae::fpga::types::handle::ptr_t h, id_t id);

    ~mmio_region();

    virtual bool write_mmio32(uint32_t offset, uint32_t value) override;

    virtual bool write_mmio64(uint32_t offset, uint64_t value) override;

    virtual bool read_mmio32(uint32_t offset, uint32_t & value) const override;

    virtual bool read_mmio64(uint32_t offset, uint64_t & value) const override;

    virtual volatile uint8_t * mmio_pointer(uint32_t offset) const override
    { return const_cast<volatile uint8_t *>(mmio_base_); }

protected:
    opae::fpga::types::handle::ptr_t owner_;
    uint32_t id_;
    uint8_t *mmio_base_;

private:
    mmio_region(opae::fpga::types::handle::ptr_t h, id_t id);
};

} // end of namespace io
} // end of namespace fpga
} // end of namespace opae
