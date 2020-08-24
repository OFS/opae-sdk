// Copyright(c) 2020, Intel Corporation
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
#include "hssi_cmd.h"

#define CSR_SCRATCH     0x1000
#define CSR_FEATURES    0x1002
#define CSR_CTRL0       0x1009
#define CSR_CTRL1       0x1010
#define CSR_DST_ADDR_LO 0x1011
#define CSR_DST_ADDR_HI 0x1012
#define CSR_SRC_ADDR_LO 0x1013
#define CSR_SRC_ADDR_HI 0x1014
#define CSR_MLB_RST     0x1016

using namespace opae::app;

class hssi_100g_cmd : public hssi_cmd
{
public:
  hssi_100g_cmd()
    : port_(0)
    , eth_loopback_("on")
    , num_packets_(1)
    , gap_("none")
    , pattern_("random")
    , src_addr_("11:22:33:44:55:66")
    , dest_addr_("77:88:99:aa:bb:cc")
  {}

  virtual const char *name() const override
  {
    return "hssi_100g";
  }

  virtual const char *description() const override
  {
    return "hssi 100G test\n";
  }

  virtual void add_options(CLI::App *app) override
  {
    auto opt = app->add_option("--port", port_,
                               "QSFP port");
    opt->check(CLI::Range(0, 7))->default_val(port_);

    opt = app->add_option("--eth-loopback", eth_loopback_,
                    "whether to enable loopback on the eth interface");
    opt->check(CLI::IsMember({"on", "off"}))->default_val(eth_loopback_);

    opt = app->add_option("--num-packets", num_packets_,
                          "number of packets");
    opt->default_val(num_packets_);

    opt = app->add_option("--gap", gap_,
                          "inter-packet gap");
    opt->check(CLI::IsMember({"random", "none"}))->default_val(gap_);

    opt = app->add_option("--pattern", pattern_,
                          "pattern mode");
    opt->check(CLI::IsMember({"random", "fixed", "increment"}))->default_val(pattern_);

    opt = app->add_option("--src-addr", src_addr_,
                          "source MAC address");
    opt->default_val(src_addr_);

    opt = app->add_option("--dest-addr", dest_addr_,
                          "destination MAC address");
    opt->default_val(dest_addr_);
  }

  virtual int run(test_afu *afu, CLI::App *app) override
  {
    (void)app;

    hssi_afu *hafu = dynamic_cast<hssi_afu *>(afu);

    uint64_t bin_src_addr = mac_bits_for(src_addr_);
    if (bin_src_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << src_addr_ << std::endl;
      return test_afu::error;
    }

    uint64_t bin_dest_addr = mac_bits_for(dest_addr_);
    if (bin_dest_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << dest_addr_ << std::endl;
      return test_afu::error;
    }

    std::string eth_ifc = hafu->ethernet_interface();

    std::cout << "100G loopback test" << std::endl
              << "  port: " << port_ << std::endl
              << "  eth_loopback: " << eth_loopback_ << std::endl
              << "  num_packets: " << num_packets_ << std::endl
              << "  gap: " << gap_ << std::endl
              << "  src_address: " << src_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_src_addr << std::endl
              << "  dest_address: " << dest_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_dest_addr << std::endl
              << "  pattern: " << pattern_ << std::endl
              << "  eth: " << eth_ifc << std::endl
              << std::endl;

    if (eth_loopback_ == "on")
      enable_eth_loopback(eth_ifc, true);

    hafu->write64(TRAFFIC_CTRL_PORT_SEL, port_);

    uint32_t reg = num_packets_;
    if (reg)
      reg |= 0x80000000;
    hafu->mbox_write(CSR_CTRL0, reg);

    hafu->mbox_write(CSR_SRC_ADDR_LO, static_cast<uint32_t>(bin_src_addr));
    hafu->mbox_write(CSR_SRC_ADDR_HI, static_cast<uint32_t>(bin_src_addr >> 32));

    hafu->mbox_write(CSR_DST_ADDR_LO, static_cast<uint32_t>(bin_dest_addr));
    hafu->mbox_write(CSR_DST_ADDR_HI, static_cast<uint32_t>(bin_dest_addr >> 32));

    reg = 0;
    if (gap_ == "random")
      reg |= 1 << 6;

    if (pattern_ == "fixed")
      reg |= 1 << 4;
    else if (pattern_ == "increment")
      reg |= 2 << 4;

    if (eth_loopback_ == "on")
      reg |= 1 << 3;

    hafu->mbox_write(CSR_CTRL1, reg);

    print_registers(std::cout, hafu);

    const uint64_t interval = 100ULL;
    do
    {
      reg = hafu->mbox_read(CSR_CTRL1);

      if (!running_) {


        return test_afu::error;
      }

      std::this_thread::sleep_for(std::chrono::microseconds(interval));
    } while(!(reg & 2));

    std::cout << std::endl;
    show_eth_stats(eth_ifc);

    if (eth_loopback_ == "on")
      enable_eth_loopback(eth_ifc, false);

    return test_afu::success;
  }

  virtual const char *afu_id() const override
  {
    return "823c334c-98bf-11ea-bb37-0242ac130002";
  }

  std::ostream & print_registers(std::ostream &os, hssi_afu *hafu) const
  {
    os << "0x40000 " << std::setw(21) << "ETH_AFU_DFH" << ": " <<
      int_to_hex(hafu->read64(ETH_AFU_DFH)) << std::endl;
    os << "0x40008 " << std::setw(21) << "ETH_AFU_ID_L" << ": " <<
      int_to_hex(hafu->read64(ETH_AFU_ID_L)) << std::endl;
    os << "0x40010 " << std::setw(21) << "ETH_AFU_ID_H" << ": " <<
      int_to_hex(hafu->read64(ETH_AFU_ID_H)) << std::endl;
    os << "0x40030 " << std::setw(21) << "TRAFFIC_CTRL_CMD" << ": " <<
      int_to_hex(hafu->read64(TRAFFIC_CTRL_CMD)) << std::endl;
    os << "0x40038 " << std::setw(21) << "TRAFFIC_CTRL_DATA" << ": " <<
      int_to_hex(hafu->read64(TRAFFIC_CTRL_DATA)) << std::endl;
    os << "0x40040 " << std::setw(21) << "TRAFFIC_CTRL_PORT_SEL" << ": " <<
      int_to_hex(hafu->read64(TRAFFIC_CTRL_PORT_SEL)) << std::endl;
    os << "0x40048 " << std::setw(21) << "AFU_SCRATCHPAD" << ": " <<
      int_to_hex(hafu->read64(AFU_SCRATCHPAD)) << std::endl;
    
    os << std::endl;

    os << "0x1000 " << std::setw(22) << "scratch" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_SCRATCH)) << std::endl;
    os << "0x1002 " << std::setw(22) << "features" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_FEATURES)) << std::endl;
    os << "0x1009 " << std::setw(22) << "ctrl0" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_CTRL0)) << std::endl;
    os << "0x1010 " << std::setw(22) << "ctrl1" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_CTRL1)) << std::endl;
    os << "0x1011 " << std::setw(22) << "dst_addr_lo" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_DST_ADDR_LO)) << std::endl;
    os << "0x1012 " << std::setw(22) << "dst_addr_hi" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_DST_ADDR_HI)) << std::endl;
    os << "0x1013 " << std::setw(22) << "src_addr_lo" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_SRC_ADDR_LO)) << std::endl;
    os << "0x1014 " << std::setw(22) << "src_addr_hi" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_SRC_ADDR_HI)) << std::endl;
    os << "0x1016 " << std::setw(22) << "mlb_rst" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_MLB_RST)) << std::endl;

    return os;
  }

protected:
  uint32_t port_;
  std::string eth_loopback_;
  uint32_t num_packets_;
  std::string gap_;
  std::string pattern_;
  std::string src_addr_;
  std::string dest_addr_;
};
