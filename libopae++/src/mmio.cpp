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
#include "opaec++/mmio.h"
#include "opaec++/mmio_region.h"

namespace opae
{
namespace fpga
{
namespace io
{

mmio::mmio(mmio::region_t region, mmio::impl_t implementation)
: region_(region)
, implementation_(implementation)
{}

mmio::ptr_t mmio::map(opae::fpga::types::handle::ptr_t h,
                      mmio::region_t region,
                      mmio::impl_t implementation)
{
    mmio::ptr_t p;

    switch(implementation) {

        case impl_t::API : {
            uint32_t iid = 0;
            uint8_t *base = nullptr;

            switch (region) {
                case region_t::AFU : iid = 0; break;
                case region_t::STP : iid = 1; break;
                default:/* TODO throw exception? */ break;
            }

            fpga_result res = fpgaMapMMIO(h->get(),
                                          iid, 
                                          reinterpret_cast<uint64_t **>(&base));

            if (res == FPGA_OK) {
                p.reset(new mmio_region(h, iid, base, region));
            }
        } break;

        case impl_t::Direct : {

        // TODO

        } break;

    }

    return p;
}

} // end of namespace io
} // end of namespace fpga
} // end of namespace opae

