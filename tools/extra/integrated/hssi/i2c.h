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

enum i2c_cmd
{
    i2c_cmd_write = 1,
    i2c_cmd_read  = 2
};


enum i2c_bus
{
    i2c_bus_ctrl     =  9,
    i2c_bus_wr_data  = 10,
    i2c_bus_rd_data  = 11
};

enum i2c_reg
{
    i2c_ctrl_reg    = 0,
    i2c_status_reg  = 1,
    i2c_wr_data_reg = 2,
    i2c_rd_data_reg = 3,
    i2c_reg_ctrl_wrdata = 0x8,
    i2c_reg_stat_rddata = 0x9
};

enum i2c_ctrl
{
    i2c_ctrl_instance   = 16,
    i2c_ctrl_trigger    = 1 <<  8,
    i2c_ctrl_transmit   = 1 <<  9,
    i2c_ctrl_send_start = 1 << 10,
    i2c_ctrl_send_ack   = 1 << 11,
    i2c_ctrl_send_stop  = 1 << 12
};

enum i2c_stat
{
    i2c_stat_tx    = 1 << 8,
    i2c_stat_error = 1 << 9
};

class i2c
{
public:
    typedef std::shared_ptr<i2c> ptr_t;
    i2c(przone_interface::ptr_t przone, size_t byte_addr_size = 1);
    ~i2c(){}
    bool read(uint32_t instance, uint32_t device_addr, uint32_t byte_addr, uint8_t bytes[], std::size_t read_bytes);
    bool write(uint32_t instance, uint32_t device_addr, uint32_t byte_addr, uint8_t bytes[], std::size_t read_bytes);
    bool wait_for_i2c_tx(uint32_t timeout_usec = 400);
private:
    przone_interface::ptr_t przone_;
    intel::utils::logger log_;
    size_t byte_addr_size_;

    bool send_byte_address(uint32_t instance, uint32_t byte_addr);
};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

