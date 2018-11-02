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
#include "przone.h"
#include "log.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

enum mdio_reg
{
    mdio_ctrl_reg    = 0x5,
    mdio_wr_data_reg = 0x6,
    mdio_rd_data_reg = 0x7,
    mdio_access_reg  = 0x20,
    mdio_address_reg = 0x21
};

enum mdio_ctrl
{
    mdio_write                  = 0x40,
    mdio_read                   = 0x80,
    mdio_device_address         = 0,
    mdio_port_addres            = 8,
    mdio_register_address       = 16,
    mdio_device_address_mask    = 0x1F,
    mdio_port_address_mask      = 0x1F00,
    mdio_register_address_mask  = 0xFFFF0000
};

class mdio
{
public:
    typedef std::shared_ptr<mdio> ptr_t;
    mdio(przone_interface::ptr_t przone);
    ~mdio(){}
    bool read(uint8_t device_addr, uint8_t port_addr, uint16_t reg_addr, uint32_t &value);
    bool write(uint8_t device_addr, uint8_t port_addr, uint16_t reg_addr, uint32_t value);
    bool wait_for_mdio_tx(uint32_t timeout_usec = 500);
private:
    przone_interface::ptr_t przone_;
    intel::utils::logger log_;
};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

