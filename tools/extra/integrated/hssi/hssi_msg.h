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
#include <cstdint>
namespace intel
{
namespace fpga
{
namespace hssi
{
namespace controller
{

enum i2c_instance
{
    i2c_instance_qsfp = 0,
    i2c_instance_retimer = 1
};

enum
{
    hssi_xcvr_lane_offset = 0x400
};


enum bus_cmd
{
    rcfg_write   = 1,
    rcfg_read    = 2,
    local_read   = 0,
    local_write  = 1,
    prmgmt_write = 1,
    prmgmt_read  = 2,
    i2c_cmd_write    = 1,
    i2c_cmd_read     = 2
};


enum aux_bus
{
    led_r        = 0,
    prmgmt_cmd   = 4,
    prmgmt_din   = 5,
    prmgmt_dout  = 6,
    local_cmd    = 7,
    local_din    = 8,
    local_dout   = 9
};

enum local_bus
{
    tsd              =  0,
    pll_rst_control  =  1,
    pll_locked_status=  2,
    prmgmt_ram_ena   =  3,
    clk_mon_sel      =  4,
    clk_mon_out      =  5,
    recfg_cmd_addr   =  8,
    recfg_cmd_wrdata =  9,
    recfg_cmd_rddata = 10,
    pll_cmd_addr     = 11,
    pll_write        = 12,
    pll_t_read       = 13,
    pll_r_read       = 14
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

enum hssi_cmd
{
    none      = 0x00,
    sw_read   = 0x08,
    sw_write  = 0x10,
    aux_read  = 0x40,
    aux_write = 0x80
};

enum nios_cmd
{
    change_hssi_mode = 0x08,
    tx_eq_write      = 0x09,
    tx_eq_read       = 0x0A,
    hssi_init        = 0x0B,
    hssi_init_done   = 0x0C,
    fatal_err        = 0x0D,
    set_hssi_enable  = 0x0E,
    get_hssi_enable  = 0x0F,
    get_hssi_mode    = 0x10,
    gbs_service_ena  = 0x11,
    firmware_version = 0xFF
};

struct hssi_ctrl_t
{
    uint32_t data;
    uint16_t address;
    uint16_t command;
};

union hssi_ctrl_u
{
    uint64_t d;
    hssi_ctrl_t s;
};

class hssi_ctrl
{
public:
    hssi_ctrl()
    {
        clear();
    }

    void clear()
    {
        data_.d = 0UL;
    }

    template<typename T>
    void set_command(T cmd)
    {
        data_.s.command = static_cast<uint16_t>(cmd);
    }

    template<typename T>
    void set_address(T addr)
    {
        data_.s.address = static_cast<uint16_t>(addr);
    }

    template<typename T>
    void set_data(T data)
    {
        data_.s.data = static_cast<uint32_t>(data);
    }

    template<typename T>
    void set_bus_command(bus_cmd cmd, T addr)
    {
        set_data(static_cast<uint32_t>(cmd) << 16 | static_cast<uint32_t>(addr));
    }

    uint64_t data()
    {
        return data_.d;
    }
private:
    hssi_ctrl_u data_;
};

} // end of namespace controller
} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel
