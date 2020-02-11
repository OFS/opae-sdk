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

#include "eth_ctrl.h"

namespace
{
    uint32_t prtable[3][5] =
    {
        // scratch, arst, ctrl,  wr,  rd
        {      0x0,  0x1,  0xa, 0xb, 0xc}, //  e40
        {      0x0,  0x1,  0x2, 0x3, 0x4},  //  e10
        {      0x0,  0x1,  0xa, 0xb, 0xc}  //  e100
    };
}

namespace intel
{
namespace fpga
{
namespace hssi
{

eth_ctrl::eth_ctrl(przone_interface::ptr_t przone, gbs_version version)
: przone_(przone)
, eth_ctrl_addr_(prtable[static_cast<uint8_t>(version)][2])
, eth_wr_data_  (prtable[static_cast<uint8_t>(version)][3])
, eth_rd_data_  (prtable[static_cast<uint8_t>(version)][4])
{

}


eth_ctrl::~eth_ctrl()
{

}


bool eth_ctrl::read(uint32_t address, uint32_t instance, uint32_t & value)
{
    uint32_t cmd = static_cast<uint32_t>(eth_traff_cmd::read) |
                   (instance << eth_instance_bit) | address;
    return przone_->write(eth_ctrl_addr_, cmd) &&
           przone_->read(eth_rd_data_, value);

}

bool eth_ctrl::write(uint32_t address, uint32_t instance, uint32_t value)
{
    uint32_t cmd = static_cast<uint32_t>(eth_traff_cmd::write) |
                   (instance << eth_instance_bit) | address;
    return przone_->write(eth_wr_data_, value) &&
           przone_->write(eth_ctrl_addr_, cmd);


}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

