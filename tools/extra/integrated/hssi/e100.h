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

class e100 : public loopback
{
public:
    enum i2c_device
    {
        eeprom = 0xA2,
        qsfp28 = 0xA0,
        iox    = 0x82
    };


    enum class gen_ctrl
    {
        start                = 1 << 0,
        stop                 = 1 << 1,
        continuous           = 1 << 2,
        random_packet_count  = 1 << 3,
        random_packet_length = 1 << 4,
        random_packet_delay  = 1 << 5
    };

    enum class gen_stat
    {
        complete             = 1 << 0
    };

    enum class mon_ctrl
    {
        start                = 1 << 0,
        stop                 = 1 << 1,
        continuous           = 1 << 2
    };

    enum class mon_stat
    {
        complete             = 1 << 0,
        destination_error    = 1 << 1,
        source_error         = 1 << 2,
        packet_length_error  = 1 << 3
    };

    enum class eth_ctrl_reg
    {
        gen_dst_addr_l = 0x0,
        gen_dst_addr_h = 0x1,
        gen_src_addr_l = 0x2,
        gen_src_addr_h = 0x3,
        gen_pkt_number = 0x4,
        gen_pkt_length = 0x5,
        gen_pkt_delay  = 0x6,
        gen_pkt_ctrl   = 0x7,
        gen_pkt_stat   = 0x8,
        mon_dst_addr_l = 0x9,
        mon_dst_addr_h = 0xa,
        mon_src_addr_l = 0xb,
        mon_src_addr_h = 0xc,
        mon_pkt_number = 0xd,
        mon_pkt_ctrl   = 0xe,
        mon_pkt_stat   = 0xf
    };

    enum class prmgmt
    {
        scratch        = 0x0,
        eth_arst       = 0x1,
        mac0_ctrl      = 0x2,
        mac0_wrdata    = 0x3,
        mac0_rddata    = 0x4,
        mdio_ctrl      = 0x5,
        mdio_wrdata    = 0x6,
        mdio_rddata    = 0x7,
        i2c_ctrl       = 0x8,
        i2c_rddata     = 0x9,
        etch_ctrl_addr = 0xa,
        eth_wr_data    = 0xb,
        eth_rd_data    = 0xc
    };

    enum mac_ctrl_cmd
    {
        mac_ctrl_req_write = 1U << 16,
        mac_ctrl_req_read  = 1U << 17
    };

    enum class mac_reg : uint32_t
    {
        mac0_ctrl              = 0x002,
        mac0_wrdata            = 0x003,
        mac0_rddata            = 0x004,
        mac1_ctrl              = 0x005,
        mac1_wrdata            = 0x006,
        mac1_rddata            = 0x007,
        mac_srl_lpbk_ctrl      = 0x313,
        mac_cntr_tx_ctrl       = 0x845,
        mac_cntr_rx_ctrl       = 0x945,
        mac_cntr_tx_st_lo      = 0x836,
        mac_cntr_tx_st_hi      = 0x837,
        mac_cntr_rx_st_lo      = 0x936,
        mac_cntr_rx_st_hi      = 0x937,
        mac_cntr_rx_pause_lo   = 0x932,
        mac_cntr_rx_pause_hi   = 0x933,
        mac_cntr_rx_frag_lo    = 0x900,
        mac_cntr_rx_frag_hi    = 0x901,
        mac_cntr_rx_crcerr_lo  = 0x906,
        mac_cntr_rx_crcerr_hi  = 0x907
    };


    enum class eth_traff_cmd : uint32_t
    {
        write = 1U << 16,
        read  = 1U << 17
    };

    enum eth_instance
    {
        eth_instance_bit = 12
    };


    enum class mmio_reg : uint32_t
    {
        afu_ctrl    = 0x0020,
        afu_wr_data = 0x0028,
        afu_rd_data = 0x0030
    };

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

    enum class pr_zone_cmd : uint32_t
    {
        write = 1U << 16,
        read  = 1U << 17
    };

    e100();
    e100(const std::string & name);
    ~e100();

      virtual void assign(opae::fpga::types::handle::ptr_t accelerator_ptr) override;

    virtual const std::string & afu_id() override
    {
        return afu_id_;
    }

    virtual uint32_t num_ports()  override { return 1; }
    virtual void clear_status() override;
    virtual void internal_loopback(uint32_t port) override;
    virtual void external_loopback(uint32_t source_port, uint32_t destination_port) override;
    virtual void stop(uint32_t instance, loopback::packet_flow flow) override;
    virtual mac_report gen_report(uint32_t instance) override;
    virtual std::vector<mac_address_t> get_mac_addresses() override;
private:
    eth_ctrl::ptr_t eth_;
    std::string config_;
    std::string afu_id_;
    uint32_t gen_ctrl_;
    uint32_t mon_ctrl_;
    bool mac_read(mac_reg prctrl, mac_reg macreg, uint32_t & value);
    bool mac_write(mac_reg prctrl, mac_reg macreg, uint32_t value);
};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

