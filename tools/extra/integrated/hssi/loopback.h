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
#include "accelerator_app.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <array>
#include "przone.h"
#include "i2c.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

struct mac_report
{
    uint16_t port;
    uint64_t cntr_tx_stat;
    uint64_t cntr_rx_stat;
    uint64_t cntr_rx_pause;
    uint64_t cntr_rx_frag;
    uint64_t cntr_rx_crcerr;
    bool gen_complete;
    bool mon_complete;
    bool mon_dest_error;
    bool mon_src_error;
    bool mon_pkt_length_error;
    mac_report():
        port(0),
        cntr_tx_stat(0),
        cntr_rx_stat(0),
        cntr_rx_pause(0),
        cntr_rx_frag(0),
        cntr_rx_crcerr(0),
        gen_complete(false),
        mon_complete(false),
        mon_dest_error(false),
        mon_src_error(false),
        mon_pkt_length_error(false)
    {
    }

};


struct mac_address_t
{
    uint32_t lo;
    uint32_t hi;
};

enum
{
    eeprom = 0xAE
};

extern std::array<uint32_t, 4> mac_offsets;

class loopback : public accelerator_app
{
public:
    // FIXME: got to be a better way..
    static const size_t dsm_size_ = 2*1024*1024;

    typedef std::shared_ptr<loopback> ptr_t;

    typedef std::pair<uint32_t, uint32_t> pair_t;
    const uint32_t afu_init = 0x0018;
    enum class packet_flow : uint16_t
    {
        generator,
        monitor
    };

    loopback(const std::string & name);
    loopback();

    virtual ~loopback();

    virtual intel::utils::option_map & get_options()
    {
        return options_;
    }
    virtual void assign(opae::fpga::types::handle::ptr_t accelerator) override;
    virtual const std::string & afu_id() = 0;

    virtual bool setup();

    virtual bool run();

    virtual opae::fpga::types::shared_buffer::ptr_t dsm() const override
    { return opae::fpga::types::shared_buffer::allocate(accelerator_, loopback::dsm_size_); }

    virtual uint64_t cachelines() const override { throw std::logic_error("Not Implemented"); }

    void show_help(std::ostream &os);

    virtual void clear_status();
    virtual void internal_loopback(uint32_t port);
    virtual void external_loopback(uint32_t source, uint32_t destination);
    virtual void stop(uint32_t instance, packet_flow flow);
    virtual mac_report gen_report(uint32_t instace);
    virtual std::vector<mac_address_t> get_mac_addresses();
    mac_address_t read_mac_address(uint32_t port);


    virtual bool initialize();

    virtual uint32_t num_ports()
    {
        return 0;
    }

    virtual void continuous(bool value)
    {
        continuous_ = value;
    }

    virtual bool continuous()
    {
        return continuous_;
    }

    virtual void packet_count(uint32_t value)
    {
        packet_count_ = value;
    }

    virtual uint32_t packet_count()
    {
        return packet_count_;
    }

    virtual void packet_length(uint32_t value)
    {
        packet_length_ = value;
    }

    virtual uint32_t packet_length()
    {
        return packet_length_;
    }

    virtual void packet_delay(uint32_t value)
    {
        packet_delay_ = value;
    }

    virtual uint32_t packet_delay()
    {
        return packet_delay_;
    }

    virtual void random_count(bool value)
    {
        random_count_ = value;
    }

    virtual bool random_count()
    {
        return random_count_;
    }

    virtual void random_length(bool value)
    {
        random_length_ = value;
    }

    virtual bool random_length()
    {
        return random_length_;
    }

    virtual void random_delay(bool value)
    {
        random_delay_ = value;
    }

    virtual bool random_delay()
    {
        return random_delay_;
    }

protected:
    przone_interface::ptr_t przone_;
    i2c::ptr_t i2c_;
    std::string afu_id_;
    opae::fpga::types::handle::ptr_t accelerator_;
    bool continuous_;
    uint32_t packet_count_;
    uint32_t packet_length_;
    uint32_t packet_delay_;
    bool random_count_;
    bool random_length_;
    bool random_delay_;

private:
    std::string config_;
    std::string mode_;
    intel::utils::logger log_;
    intel::utils::option_map options_;

};

#define UNUSED_PARAM(x) (void)x

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel


