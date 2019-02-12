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
#include "hssi_przone.h"
#include "hssi_msg.h"
#include <chrono>
#include <thread>

namespace intel
{
namespace fpga
{
namespace hssi
{

using namespace intel::fpga;
using namespace intel::fpga::hssi::controller;
using namespace std;
using namespace std::chrono;

hssi_przone::hssi_przone(mmio::ptr_t mmio, uint32_t ctrl, uint32_t stat)
: mmio_(mmio)
, ctrl_(ctrl)
, stat_(stat)
{
}

bool hssi_przone::read(uint32_t address, uint32_t & value)
{
    hssi_ctrl msg;

    msg.clear();
    msg.set_address(aux_bus::prmgmt_cmd);
    msg.set_data(address);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(static_cast<uint32_t>(ctrl_), msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    msg.clear();
    msg.set_address(aux_bus::prmgmt_dout);
    msg.set_bus_command(bus_cmd::prmgmt_write, address);
    msg.set_command(hssi_cmd::aux_read);
    mmio_->write_mmio64(static_cast<uint32_t>(ctrl_), msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    uint64_t hssi_value;
    if (!mmio_->read_mmio64(static_cast<uint32_t>(stat_), hssi_value))
    {
        return false;
    }

    value = static_cast<uint32_t>(hssi_value & 0x00000000FFFFFFFF);

    return true;
}

bool hssi_przone::write(uint32_t address, uint32_t value)
{
    hssi_ctrl msg;

    msg.clear();
    msg.set_address(aux_bus::prmgmt_din);
    msg.set_data(static_cast<uint32_t>(value));
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(static_cast<uint32_t>(ctrl_), msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    msg.clear();
    msg.set_address(aux_bus::prmgmt_cmd);
    msg.set_bus_command(bus_cmd::prmgmt_write, address);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(static_cast<uint32_t>(ctrl_), msg.data());

    if (!hssi_ack())
    {
        return false;
    }

    return true;
}

bool hssi_przone::wait_for_ack(ack_t response, uint32_t timeout_usec, uint32_t * duration)
{
    uint64_t value = response == ack_t::ack ? 0 : 0xFFFF;
    // write a little lambda to check the value based on response type we are
    // waiting on
    auto check_ack = [response](uint64_t v) -> bool
    {
        return response == ack_t::ack ?  (v & (1UL << ack_bit)) : (~v & (1UL << ack_bit));
    };

    auto begin  = high_resolution_clock::now();
    auto delta = high_resolution_clock::now() - begin;
    while (delta < microseconds(timeout_usec))
    {
        if (mmio_->read_mmio64(static_cast<uint32_t>(stat_), value) && check_ack(value))
        {
            if (duration)
            {
                *duration = duration_cast<microseconds>(delta).count();
            }

            return true;
        }

        std::this_thread::sleep_for(microseconds(10));
        delta = high_resolution_clock::now() - begin;
    }
    return false;
}

bool hssi_przone::hssi_ack(uint32_t timeout_usec, uint32_t * duration)
{
    if (!wait_for_ack(ack_t::ack, timeout_usec, duration))
    {
        return false;
    }
    mmio_->write_mmio64(static_cast<uint32_t>(ctrl_), 0UL);
    if (!wait_for_ack(ack_t::nack, timeout_usec, duration))
    {
        return false;
    }
    return true;
}

uint32_t hssi_przone::get_ctrl() const { return ctrl_; }
uint32_t hssi_przone::get_stat() const { return stat_; }

mmio::ptr_t hssi_przone::get_mmio() const { return mmio_; }

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel
