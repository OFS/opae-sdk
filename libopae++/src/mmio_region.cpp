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
#include <opae/mmio.h>
#include "opaec++/mmio_region.h"

namespace opae
{
namespace fpga
{
namespace io
{

mmio::ptr_t mmio_region::region(opae::fpga::types::handle::ptr_t h, mmio_region::id_t id)
{
    mmio_region *rptr = new mmio_region(h, id);
    mmio::ptr_t p;

    if (!rptr->mmio_base_) {
        // TODO throw fpga_error?
        delete rptr;
        return p;
    }

    p.reset(rptr);

    return p;
}

mmio_region::mmio_region(opae::fpga::types::handle::ptr_t h, id_t id)
: owner_(h)
, id_(id_t::AFU == id ? 0 : 1)
, mmio_base_(nullptr)
{
    fpga_result res = fpgaMapMMIO(owner_->get(),
                                  id_,
                                  reinterpret_cast<uint64_t **>(&mmio_base_));
    if (res != FPGA_OK) {
        // TODO log appropriate error message
    }
}

mmio_region::~mmio_region()
{
    if (mmio_base_) {
        fpga_result res = fpgaUnmapMMIO(owner_->get(), id_);
        if (res != FPGA_OK) {
            // TODO log appropriate error message
        }
    }
}

bool mmio_region::write_mmio32(uint32_t offset, uint32_t value)
{
    return FPGA_OK == fpgaWriteMMIO32(owner_->get(), id_, offset, value);
}

bool mmio_region::write_mmio64(uint32_t offset, uint64_t value)
{
    return FPGA_OK == fpgaWriteMMIO64(owner_->get(), id_, offset, value);
}

bool mmio_region::read_mmio32(uint32_t offset, uint32_t & value) const
{
    return FPGA_OK == fpgaReadMMIO32(owner_->get(), id_, offset, &value);
}

bool mmio_region::read_mmio64(uint32_t offset, uint64_t & value) const
{
    return FPGA_OK == fpgaReadMMIO64(owner_->get(), id_, offset, &value);
}

} // end of namespace io
} // end of namespace fpga
} // end of namespace opae

