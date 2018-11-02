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
#include "option_map.h"
#include "log.h"
#include <chrono>
#include "loopback.h"
#include "eth_ctrl.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

class e10 : public loopback
{
public:
    enum class stats : uint32_t
    {
        tx_stats_clr               = 0x1c00,
        tx_stats_frames_ok_lo      = 0x1c02,
        tx_stats_frames_ok_hi      = 0x1c03,
        rx_stats_clr               = 0x0c00,
        rx_stats_frames_ok_lo      = 0x0c02,
        rx_stats_frames_ok_hi      = 0x0c03,
        rx_stats_frames_crcerr_lo  = 0x0c06,
        rx_stats_frames_crcerr_hi  = 0x0c07,
        rx_stats_pause_mac_ctrl_lo = 0x0c0a,
        rx_stats_pause_mac_ctrl_hi = 0x0c0b
    };

    enum class eth_ctrl_reg : uint32_t
    {
        eth_inst_sel   = 0x0005,
        eth_ser_lpbk   = 0x0006,
        gen_dst_addr_l = 0x3000,
        gen_dst_addr_h = 0x3001,
        gen_src_addr_l = 0x3002,
        gen_src_addr_h = 0x3003,
        gen_pkt_number = 0x3004,
        gen_pkt_length = 0x3005,
        gen_pkt_delay  = 0x3006,
        gen_pkt_ctrl   = 0x3007,
        gen_pkt_stat   = 0x3008,
        mon_dst_addr_l = 0x3100,
        mon_dst_addr_h = 0x3101,
        mon_src_addr_l = 0x3102,
        mon_src_addr_h = 0x3103,
        mon_pkt_number = 0x3104,
        mon_pkt_ctrl   = 0x3105,
        mon_pkt_stat   = 0x3106
    };

    enum class gen_ctrl : uint32_t
    {
        start                = 1 << 0,
        stop                 = 1 << 1,
        continuous           = 1 << 2,
        random_packet_count  = 1 << 3,
        random_packet_length = 1 << 4,
    };

    enum class gen_stat : uint32_t
    {
        complete             = 1 << 0
    };

    enum class mon_ctrl : uint32_t
    {
        start                = 1 << 0,
        stop                 = 1 << 1,
        continuous           = 1 << 2
    };

    enum class mon_stat : uint32_t
    {
        complete             = 1 << 0,
        destination_error    = 1 << 1,
        source_error         = 1 << 2,
        packet_length_error  = 1 << 3
    };

    e10();
    e10(const std::string & name);
    ~e10();

    virtual void assign(opae::fpga::types::handle::ptr_t accelerator_ptr);

    virtual const std::string & afu_id()
    {
        return afu_id_;
    }

    virtual uint32_t num_ports()  override { return 4; };
    virtual void clear_status();
    virtual void internal_loopback(uint32_t port);
    virtual void external_loopback(uint32_t source_port, uint32_t destination_port);
    virtual void stop(uint32_t instance, loopback::packet_flow flow);
    virtual mac_report gen_report(uint32_t instance);
    virtual std::vector<mac_address_t> get_mac_addresses();
private:
    eth_ctrl::ptr_t eth_;
    bool interactive_;
    std::string config_;
    std::string afu_id_;
    uint32_t gen_ctrl_;
    uint32_t mon_ctrl_;
};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

