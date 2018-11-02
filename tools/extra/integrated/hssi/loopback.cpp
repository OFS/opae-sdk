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
#include "loopback.h"
#include "accelerator_przone.h"

using namespace intel::utils;

namespace intel
{
namespace fpga
{
namespace hssi
{

std::array<uint32_t, 4> mac_offsets = { { 0xE0, 0xE8, 0xF0, 0xF8 } };

loopback::loopback()
: accelerator_app()
, afu_id_("")
, accelerator_(0)
, continuous_(false)
, packet_count_(0)
, packet_length_(0)
, packet_delay_(0)
, random_count_(false)
, random_length_(false)
, random_delay_(false)
, config_("hssi.json")
, mode_("auto")
{

}

loopback::loopback(const std::string & name)
: accelerator_app(name)
, afu_id_("")
, accelerator_(0)
, continuous_(false)
, packet_count_(0)
, packet_length_(0)
, packet_delay_(0)
, random_count_(false)
, random_length_(false)
, random_delay_(false)
, config_("hssi.json")
, mode_("auto")
{

}

loopback::~loopback()
{
    if (accelerator_ /* && accelerator_->ready() */)
        accelerator_->close();
}

void loopback::assign(opae::fpga::types::handle::ptr_t accelerator)
{
    accelerator_ = accelerator;
    przone_.reset(new accelerator_przone(accelerator_));
    i2c_.reset(new i2c(przone_));
}

bool loopback::initialize()
{
    uint64_t value = accelerator_->read_csr64(afu_init);
    if ((value & 0x2) == 0x2)
    {
        return true;
    }
    value = 0x1;
    using hrc = std::chrono::high_resolution_clock;

    accelerator_->write_csr64(afu_init, value);

    auto begin = hrc::now();
    while((hrc::now() - begin) < std::chrono::seconds(1))
    {
        value = accelerator_->read_csr64(afu_init);
        if ((value & 0x2) == 0x2)
        {
            return przone_->write(0x1, 0x0);
        }
    }

    return false;
}

bool loopback::setup()
{
    return true;
}

bool loopback::run()
{
    return false;
}

void loopback::show_help(std::ostream &os)
{
    UNUSED_PARAM(os);
}


void loopback::clear_status()
{
}

void loopback::internal_loopback(uint32_t port)
{
    UNUSED_PARAM(port);
}

void loopback::external_loopback(uint32_t source, uint32_t destination)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(destination);
}

void loopback::stop(uint32_t instance, packet_flow flow)
{
    UNUSED_PARAM(instance);
    UNUSED_PARAM(flow);
}

mac_report loopback::gen_report(uint32_t instace)
{
    UNUSED_PARAM(instace);
    return mac_report();
}

std::vector<mac_address_t> loopback::get_mac_addresses()
{
    return std::vector<mac_address_t>();
}

mac_address_t loopback::read_mac_address(uint32_t port)
{
    mac_address_t mac;
    uint8_t *hi = reinterpret_cast<uint8_t*>(&mac.hi);
    uint8_t *lo = reinterpret_cast<uint8_t*>(&mac.lo);

    i2c_->read(1, eeprom, mac_offsets[port],   hi+1, 1);
    i2c_->read(1, eeprom, mac_offsets[port]+1, hi,   1);
    i2c_->read(1, eeprom, mac_offsets[port]+2, lo+3, 1);
    i2c_->read(1, eeprom, mac_offsets[port]+3, lo+2, 1);
    i2c_->read(1, eeprom, mac_offsets[port]+4, lo+1, 1);
    i2c_->read(1, eeprom, mac_offsets[port]+5, lo,   1);
    return mac;
}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel
