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
#include "i2c.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace intel
{
namespace fpga
{
namespace hssi
{

using namespace std;
using namespace std::chrono;

i2c::i2c(przone_interface::ptr_t przone, size_t byte_addr_size)
: przone_(przone)
, byte_addr_size_(byte_addr_size)
{
}

bool i2c::send_byte_address(uint32_t instance, uint32_t byte_addr)
{
    uint32_t ctrl = i2c_ctrl_trigger | i2c_ctrl_transmit | i2c_ctrl_send_start;
    // support multi-byte byte addresses in big endian format
    uint8_t *base_ptr = reinterpret_cast<uint8_t*>(&byte_addr);
    // start with the upper byte (base address of the byte_addr + byte address size)
    uint8_t* ptr = base_ptr + byte_addr_size_;
    // make sure we decrement the ptr before we start using it to dereference the byte
    while (ptr-- > base_ptr){
        ctrl = i2c_ctrl_trigger | i2c_ctrl_transmit;
        przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl | *ptr);
        wait_for_i2c_tx();
    }
    return true;
}

bool i2c::read(uint32_t instance, uint32_t device_addr, uint32_t byte_addr, uint8_t bytes[], size_t read_bytes)
{
    // 1. Set the device address and control bits (07) into I2C_CTRL_WDATA
    uint32_t ctrl = i2c_ctrl_trigger  | i2c_ctrl_transmit | i2c_ctrl_send_start;
    przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl | device_addr);
    wait_for_i2c_tx();

    // 2. Set the byte address and control bits (03) into I2C_CTRL_WDATA
    send_byte_address(instance, byte_addr);

    // 3. Set the device address (as read) and control bits (07) into I2C_CTRL_WDATA
    ctrl = i2c_ctrl_trigger | i2c_ctrl_transmit | i2c_ctrl_send_start;
    przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl | device_addr | 1);
    wait_for_i2c_tx();

    // 4. Trigger the I2C byte read and ACK, writing control bits (09) into I2C_CTRL_WDATA
    ctrl = i2c_ctrl_trigger | i2c_ctrl_send_ack;
    uint32_t value;
    for(size_t i = 0; i < read_bytes; ++i)
    {
        // Last byte:
        // Trigger the I2C byte read, NACK and STOP, writing into the I2C_CONTROL
        if (i == read_bytes - 1)
        {
            ctrl = i2c_ctrl_trigger | i2c_ctrl_send_stop;
        }

        przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl);
        wait_for_i2c_tx();

        if(przone_->read(i2c_reg_stat_rddata, value))
        {
            bytes[i] = value;
        }
        else
        {
            std::cerr << "WARNING: Could not complete I2C read" << std::endl;
            break;
        }
    }
    return true;
}

bool i2c::write(uint32_t instance, uint32_t device_addr, uint32_t byte_addr, uint8_t bytes[], size_t write_bytes)
{
    uint32_t ctrl = i2c_ctrl_trigger  | i2c_ctrl_transmit | i2c_ctrl_send_start;
    // 1. Set the Device Address (30) and the control bits (07) into I2C_CTRL_WDATA
    przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl | device_addr);
    wait_for_i2c_tx();

    // 2. Set the byte address and control bits (03) into I2C_CTRL_WDATA
    send_byte_address(instance, byte_addr);

    // 3. Set the Byte0 Value (bb) and the control bits (13) into I2C_CTRL_WDATA
    ctrl = i2c_ctrl_trigger | i2c_ctrl_transmit;
    for (size_t i = 0; i < write_bytes; ++i)
    {
        // Last byte? If yes, include send_stop in ctrl message
        if (i == write_bytes-1)
        {
            ctrl |= i2c_ctrl_send_stop;
        }

        przone_->write(i2c_reg_ctrl_wrdata, (instance << i2c_ctrl_instance) | ctrl | bytes[i]);
        wait_for_i2c_tx();
    }
    return true;
}

bool i2c::wait_for_i2c_tx(uint32_t timeout_usec)
{
    auto begin  = high_resolution_clock::now();
    uint32_t stat;
    while (true)
    {
        auto delta = high_resolution_clock::now() - begin;
        if (delta >= microseconds(timeout_usec))
        {
            log_.warn() << "Timed out waiting for I2C TX to stop" << std::endl;
            return false;
        }

        if (!przone_->read(i2c_reg_stat_rddata, stat))
        {
            std::cerr << "ERROR: Waiting for i2c ready" << std::endl;
            return false;
        }

        if (~stat & i2c_stat_tx)
        {
            return true;
        }
        else
        {
            std::this_thread::sleep_for(microseconds(10));
        }
    }
}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

