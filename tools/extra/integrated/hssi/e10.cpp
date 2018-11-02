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

#include "e10.h"
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;
using namespace intel::utils;

namespace intel
{
namespace fpga
{
namespace hssi
{

e10::e10()
: loopback()
, interactive_(false)
, config_("e10.json")
, afu_id_("05189fe4-0676-dd24-b74f-291af34e1783")
, gen_ctrl_(0)
, mon_ctrl_(0)
{
}

e10::e10(const std::string & name)
: loopback(name)
, interactive_(false)
, config_("e10.json")
, afu_id_("05189FE4-0676-DD24-B74F-291AF34E1783")
, gen_ctrl_(0)
, mon_ctrl_(0)
{
}

e10::~e10()
{

}

void e10::assign(opae::fpga::types::handle::ptr_t h)
{
    loopback::assign(h);
    eth_.reset(new eth_ctrl(przone_, eth_ctrl::gbs_version::e10));
}

void e10::clear_status()
{
    for (uint32_t i = 0; i < num_ports(); ++i)
    {
        eth_->write(stats::tx_stats_clr, i, 1);
        eth_->write(stats::rx_stats_clr, i, 1);
    }
}

void e10::internal_loopback(uint32_t instance)
{
    // turn on serial loopback for all ports
    przone_->write(static_cast<uint32_t>(eth_ctrl_reg::eth_ser_lpbk), 0xf);
    auto mac0 = read_mac_address(instance);

    gen_ctrl_ |= static_cast<uint32_t>(gen_ctrl::continuous);
    eth_->write(eth_ctrl_reg::gen_src_addr_l, instance, mac0.lo);
    eth_->write(eth_ctrl_reg::gen_src_addr_h, instance, mac0.hi);
    eth_->write(eth_ctrl_reg::mon_src_addr_l, instance, mac0.lo);
    eth_->write(eth_ctrl_reg::mon_src_addr_h, instance, mac0.hi);
    eth_->write(eth_ctrl_reg::gen_dst_addr_l, instance, mac0.lo);
    eth_->write(eth_ctrl_reg::gen_dst_addr_h, instance, mac0.hi);
    eth_->write(eth_ctrl_reg::mon_dst_addr_l, instance, mac0.lo);
    eth_->write(eth_ctrl_reg::mon_dst_addr_h, instance, mac0.hi);

    eth_->write(eth_ctrl_reg::mon_pkt_ctrl,   instance, mon_ctrl_ | static_cast<uint32_t>(mon_ctrl::start));
    eth_->write(eth_ctrl_reg::gen_pkt_ctrl,   instance, gen_ctrl_ | static_cast<uint32_t>(gen_ctrl::start));
}

void e10::external_loopback(uint32_t source_port, uint32_t destination_port)
{
    auto mac0 = read_mac_address(source_port);
    auto mac1 = read_mac_address(destination_port);
    gen_ctrl_ = 0;
    mon_ctrl_ = 0;

    if (random_length_)
    {
        gen_ctrl_ |= static_cast<uint32_t>(gen_ctrl::random_packet_length);
    }

    if (random_count_)
    {
        gen_ctrl_ |= static_cast<uint32_t>(gen_ctrl::random_packet_count);
        mon_ctrl_ |= static_cast<uint32_t>(mon_ctrl::continuous);
    }
    else if (continuous_)
    {
        gen_ctrl_ |= static_cast<uint32_t>(gen_ctrl::continuous);
        mon_ctrl_ |= static_cast<uint32_t>(mon_ctrl::continuous);
    }
    else
    {
        if (packet_count_ == 0)
        {
            gen_ctrl_ |= static_cast<uint32_t>(gen_ctrl::random_packet_count);
        }

        if (packet_count_ > 0)
        {
            eth_->write(eth_ctrl_reg::gen_pkt_number, source_port,  packet_count_);
            eth_->write(eth_ctrl_reg::mon_pkt_number, destination_port,  packet_count_);
        }
        if (packet_length_ > 0) eth_->write(eth_ctrl_reg::gen_pkt_length, source_port, packet_length_);

    }

    // turn off serial loopback for all ports
    przone_->write(static_cast<uint32_t>(eth_ctrl_reg::eth_ser_lpbk), 0);


    // PORT 0 GEN SRC -> MAC0
    eth_->write(eth_ctrl_reg::gen_src_addr_l, source_port, mac0.lo);
    eth_->write(eth_ctrl_reg::gen_src_addr_h, source_port, mac0.hi);
    // PORT 0 GEN DST -> MAC1
    eth_->write(eth_ctrl_reg::gen_dst_addr_l, source_port, mac1.lo);
    eth_->write(eth_ctrl_reg::gen_dst_addr_h, source_port, mac1.hi);
    // PORT 0 MON SRC -> MAC1
    eth_->write(eth_ctrl_reg::mon_src_addr_l, source_port, mac1.lo);
    eth_->write(eth_ctrl_reg::mon_src_addr_h, source_port, mac1.hi);
    // PORT 0 MON DST -> MAC0
    eth_->write(eth_ctrl_reg::mon_dst_addr_l, source_port, mac0.lo);
    eth_->write(eth_ctrl_reg::mon_dst_addr_h, source_port, mac0.hi);

    // PORT 1 GEN SRC -> MAC1
    eth_->write(eth_ctrl_reg::gen_src_addr_l, destination_port, mac1.lo);
    eth_->write(eth_ctrl_reg::gen_src_addr_h, destination_port, mac1.hi);
    // PORT 1 GEN SRC -> MAC0
    eth_->write(eth_ctrl_reg::gen_dst_addr_l, destination_port, mac0.lo);
    eth_->write(eth_ctrl_reg::gen_dst_addr_h, destination_port, mac0.hi);
    // PORT 1 MON SRC -> MAC0
    eth_->write(eth_ctrl_reg::mon_src_addr_l, destination_port, mac0.lo);
    eth_->write(eth_ctrl_reg::mon_src_addr_h, destination_port, mac0.hi);
    // PORT 1 MON DST -> MAC1
    eth_->write(eth_ctrl_reg::mon_dst_addr_l, destination_port, mac1.lo);
    eth_->write(eth_ctrl_reg::mon_dst_addr_h, destination_port, mac1.hi);

    eth_->write(eth_ctrl_reg::mon_pkt_ctrl, destination_port, mon_ctrl_ | static_cast<uint32_t>(mon_ctrl::start));
    eth_->write(eth_ctrl_reg::gen_pkt_ctrl, source_port, gen_ctrl_ | static_cast<uint32_t>(gen_ctrl::start));
}


void e10::stop(uint32_t instance, loopback::packet_flow flow)
{
    switch(flow)
    {
        case loopback::packet_flow::monitor:
            eth_->write(eth_ctrl_reg::mon_pkt_ctrl,   instance, static_cast<uint32_t>(mon_ctrl::stop));
            break;
        case loopback::packet_flow::generator:
            eth_->write(eth_ctrl_reg::gen_pkt_ctrl,   instance, static_cast<uint32_t>(gen_ctrl::stop));
            break;
    }
}

mac_report e10::gen_report(uint32_t port)
{
    mac_report report;
    report.port = port;
    uint32_t lo, hi;
    if (eth_->read(stats::tx_stats_frames_ok_lo, port, lo) &&
        eth_->read(stats::tx_stats_frames_ok_hi, port, hi))
    {
        report.cntr_tx_stat = hi;
        report.cntr_tx_stat <<= 32;
        report.cntr_tx_stat |= static_cast<uint64_t>(lo);

    }
    if (eth_->read(stats::rx_stats_frames_ok_lo, port, lo) &&
        eth_->read(stats::rx_stats_frames_ok_hi, port, hi))
    {
        report.cntr_rx_stat = hi;
        report.cntr_rx_stat <<= 32;
        report.cntr_rx_stat |= static_cast<uint64_t>(lo);

    }
    if (eth_->read(stats::rx_stats_pause_mac_ctrl_lo, port, lo) &&
        eth_->read(stats::rx_stats_pause_mac_ctrl_hi, port, hi))
    {
        report.cntr_rx_pause = hi;
        report.cntr_rx_pause <<= 32;
        report.cntr_rx_pause |= static_cast<uint64_t>(lo);

    }
    if (eth_->read(stats::rx_stats_frames_crcerr_lo, port, lo) &&
        eth_->read(stats::rx_stats_frames_crcerr_hi, port, hi))
    {
        report.cntr_rx_crcerr = hi;
        report.cntr_rx_crcerr <<= 32;
        report.cntr_rx_crcerr |= static_cast<uint64_t>(lo);
    }

    uint32_t status;
    if (eth_->read(eth_ctrl_reg::gen_pkt_stat, port, status))
    {
        report.gen_complete = status & static_cast<uint32_t>(gen_stat::complete);
    }

    if (eth_->read(eth_ctrl_reg::mon_pkt_stat, port, status))
    {
        report.mon_complete = status & static_cast<uint32_t>(mon_stat::complete);
        report.mon_dest_error   = status & static_cast<uint32_t>(mon_stat::destination_error);
        report.mon_src_error   = status & static_cast<uint32_t>(mon_stat::source_error);
        report.mon_pkt_length_error   = status & static_cast<uint32_t>(mon_stat::packet_length_error);
    }
    return report;

}

std::vector<mac_address_t> e10::get_mac_addresses()
{
    int begin = 0, end = 4;
    std::vector<mac_address_t> addresses;
    for (int i = begin; i < end; ++i)
    {
        mac_address_t mac = read_mac_address(i);
        addresses.push_back(mac);
    }

    return addresses;
}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel





