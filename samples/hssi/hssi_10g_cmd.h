// Copyright(c) 2020-2023, Intel Corporation
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
#include <iostream>
#include <string>
#include "hssi_cmd.h"

#define CSR_NUM_PACKETS       0x3c00
#define CSR_RANDOM_LENGTH     0x3c01
#define CSR_RANDOM_PAYLOAD    0x3c02
#define CSR_START             0x3c03
#define CSR_STOP              0x3c04
#define CSR_SRC_ADDR0         0x3c05
#define CSR_SRC_ADDR1         0x3c06
#define CSR_DEST_ADDR0        0x3c07
#define CSR_DEST_ADDR1        0x3c08
#define CSR_PACKET_TX_COUNT   0x3c09
#define CSR_RND_SEED0         0x3c0a
#define CSR_RND_SEED1         0x3c0b
#define CSR_RND_SEED2         0x3c0c
#define CSR_PACKET_LENGTH     0x3c0d
#define CSR_TX_STA_TSTAMP     0x3cf4
#define CSR_TX_END_TSTAMP     0x3cf5

#define CSR_NUM_PKT           0x3d00
#define CSR_PKT_GOOD          0x3d01
#define CSR_PKT_BAD           0x3d02
#define CSR_AVST_RX_ERR       0x3d07
#define CSR_RX_STA_TSTAMP     0x3d0b
#define CSR_RX_END_TSTAMP     0x3d0c

#define CSR_MAC_LOOP          0x3e00

#define INVALID_PORT 99

class hssi_10g_cmd : public hssi_cmd
{
public:
  hssi_10g_cmd()
    : port_(INVALID_PORT)
    , dst_port_(0)
    , src_port_(0)
    , eth_loopback_("on")
    , he_loopback_("none")
    , num_packets_(1)
    , random_length_("fixed")
    , random_payload_("incremental")
    , packet_length_(64)
    , src_addr_("11:22:33:44:55:66")
    , dest_addr_("77:88:99:aa:bb:cc")
    , eth_ifc_("none")
    , rnd_seed0_(0x5eed0000)
    , rnd_seed1_(0x5eed0001)
    , rnd_seed2_(0x00025eed)
  {}

  virtual const char *name() const override
  {
    return "hssi_10g";
  }

  virtual const char *description() const override
  {
    return "hssi 10G test\n";
  }

  virtual void add_options(CLI::App *app) override
  {
    auto opt = app->add_option("--port", port_, "QSFP Tx/Rx ports");
    opt->check(CLI::Range(0, 7))->default_str(std::to_string(port_));

    opt = app->add_option("--dst-port", dst_port_,
                          "QSFP Rx port");
    opt->check(CLI::Range(0, 7))->default_str(std::to_string(dst_port_));

    opt = app->add_option("--src-port", src_port_,
                          "QSFP Tx port");
    opt->check(CLI::Range(0, 7))->default_str(std::to_string(src_port_));

    opt = app->add_option("--eth-loopback", eth_loopback_,
                    "whether to enable loopback on the eth interface");
    opt->check(CLI::IsMember({"on", "off"}))->default_str(eth_loopback_);

    opt = app->add_option("--he-loopback", he_loopback_,
                    "whether to enable loopback in the Hardware Exerciser (HE)");
    opt->check(CLI::IsMember({"on", "off"}))->default_str(he_loopback_);

    opt = app->add_option("--num-packets", num_packets_,
                          "number of packets");
    opt->default_str(std::to_string(num_packets_));

    opt = app->add_option("--random-length", random_length_,
                          "packet length randomization");
    opt->check(CLI::IsMember({"fixed", "random"}))->default_str(random_length_);

    opt = app->add_option("--random-payload", random_payload_,
                          "payload randomization");
    opt->check(CLI::IsMember({"incremental", "random"}))->default_str(random_payload_);

    opt = app->add_option("--packet-length", packet_length_,
                          "packet length");
    opt->default_str(std::to_string(packet_length_));

    opt = app->add_option("--src-addr", src_addr_,
                          "source MAC address");
    opt->default_str(src_addr_);

    opt = app->add_option("--dest-addr", dest_addr_,
                          "destination MAC address");
    opt->default_str(dest_addr_);

    opt = app->add_option("--eth-ifc", eth_ifc_,
                          "ethernet interface name");
    opt->default_str(eth_ifc_);

    opt = app->add_option("--rnd-seed0", rnd_seed0_,
                          "prbs generator [31:0]");
    opt->default_str(std::to_string(rnd_seed0_));

    opt = app->add_option("--rnd-seed1", rnd_seed1_,
                          "prbs generator [47:32]");
    opt->default_str(std::to_string(rnd_seed1_));

    opt = app->add_option("--rnd-seed2", rnd_seed2_,
                          "prbs generator [91:64]");
    opt->default_str(std::to_string(rnd_seed2_));
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

    std::string eth_ifc = eth_ifc_;
    if (eth_ifc_ == "none")
        eth_ifc = hafu->ethernet_interface();

    if (port_ != INVALID_PORT) {
        std::cout << "--port is overriding --src-port and --dst-port" << std::endl;
        src_port_ = dst_port_ = port_;
    }

    std::cout << "10G loopback test" << std::endl
              << "  Tx/Rx port: " << port_ << std::endl
              << "  Tx port: " << src_port_ << std::endl
              << "  Rx port: " << dst_port_ << std::endl
              << "  eth_loopback: " << eth_loopback_ << std::endl
              << "  he_loopback: " << he_loopback_ << std::endl
              << "  num_packets: " << num_packets_ << std::endl
              << "  packet_length: " << packet_length_ << std::endl
              << "  src_address: " << src_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_src_addr << std::endl
              << "  dest_address: " << dest_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_dest_addr << std::endl
              << "  random_length: " << random_length_ << std::endl
              << "  random_payload: " << random_payload_ << std::endl
              << "  rnd_seed0: " << rnd_seed0_ << std::endl
              << "  rnd_seed1: " << rnd_seed1_ << std::endl
              << "  rnd_seed2: " << rnd_seed2_ << std::endl
              << "  eth: " << eth_ifc << std::endl
              << std::endl;

    if (eth_ifc == "") {
        std::cout << "No eth interface, so not "
		     "honoring --eth-loopback." << std::endl;
    } else {
        if (eth_loopback_ == "on")
            enable_eth_loopback(eth_ifc, true);
        else
            enable_eth_loopback(eth_ifc, false);
    }

    hafu->mbox_write(src_port_, CSR_STOP, 0);

    if (he_loopback_ != "none") {
        hafu->mbox_write(src_port_, CSR_MAC_LOOP, (he_loopback_ == "on") ? 1 : 0);

        if (he_loopback_ != "on") // don't loop for "off"
            return test_afu::success;

        std::cout << "HE loopback enabled. Use Ctrl+C to exit." << std::endl;

        while (running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        return test_afu::success;
    }

    double clk_freq = clock_freq_for(hafu);

    hafu->mbox_write(src_port_, CSR_NUM_PACKETS, num_packets_);

    hafu->mbox_write(src_port_, CSR_PACKET_LENGTH, packet_length_);

    hafu->mbox_write(src_port_, CSR_SRC_ADDR0, static_cast<uint32_t>(bin_src_addr));
    hafu->mbox_write(src_port_, CSR_SRC_ADDR1, static_cast<uint32_t>(bin_src_addr >> 32));

    hafu->mbox_write(src_port_, CSR_DEST_ADDR0, static_cast<uint32_t>(bin_dest_addr));
    hafu->mbox_write(src_port_, CSR_DEST_ADDR1, static_cast<uint32_t>(bin_dest_addr >> 32));

    hafu->mbox_write(src_port_, CSR_RANDOM_LENGTH, (random_length_ == "fixed") ? 0 : 1);
    hafu->mbox_write(src_port_, CSR_RANDOM_PAYLOAD, (random_payload_ == "incremental") ? 0 : 1);
    hafu->mbox_write(src_port_, CSR_RND_SEED0, rnd_seed0_);
    hafu->mbox_write(src_port_, CSR_RND_SEED1, rnd_seed1_);
    hafu->mbox_write(src_port_, CSR_RND_SEED2, rnd_seed2_);

    hafu->mbox_write(src_port_, CSR_START, 1);

    print_registers(std::cout, hafu);

    uint32_t count;
    const uint64_t interval = 100ULL;
    do
    {
      count = hafu->mbox_read(src_port_, CSR_PACKET_TX_COUNT);
  
      if (!running()) {
        hafu->mbox_write(src_port_, CSR_STOP, 1);
        return test_afu::error;
      }
  
      std::this_thread::sleep_for(std::chrono::microseconds(interval));
    } while(count < num_packets_);

    std::cout << std::endl;

    if (clk_freq == INVALID_CLOCK_FREQ) {
      std::cerr << "Couldn't determine user clock freq." << std::endl
                << "Skipping performance display." << std::endl;
    } else {
      std::cout << "HSSI performance: " << std::endl;
      // Read traffic control Tx/Rx timestamp registers
      uint32_t tx_sta_tstamp = hafu->mbox_read(src_port_, CSR_TX_STA_TSTAMP);
      uint32_t tx_end_tstamp = hafu->mbox_read(src_port_, CSR_TX_END_TSTAMP);
      uint32_t rx_sta_tstamp = hafu->mbox_read(dst_port_, CSR_RX_STA_TSTAMP);
      uint32_t rx_end_tstamp = hafu->mbox_read(dst_port_, CSR_RX_END_TSTAMP);

      // Convert timestamp register from clock cycles to nanoseconds
      double sample_period_ns = 1000 / clk_freq;
      double tx_sta_tstamp_ns = tx_sta_tstamp * sample_period_ns;
      double tx_end_tstamp_ns = tx_end_tstamp * sample_period_ns;
      double rx_sta_tstamp_ns = rx_sta_tstamp * sample_period_ns;
      double rx_end_tstamp_ns = rx_end_tstamp * sample_period_ns;

      // Calculate latencies
      double latency_min_ns = rx_sta_tstamp_ns - tx_sta_tstamp_ns;
      double latency_max_ns = rx_end_tstamp_ns - tx_end_tstamp_ns;

      // Calculate Tx/Rx throughput achieved
      uint32_t data_pkt_size;
      if (clk_freq == USER_CLKFREQ_S10)
	      data_pkt_size = packet_length_ - 4;
      else
	      data_pkt_size = packet_length_ - 8;
      double total_tx_duration_ns = (tx_end_tstamp_ns - tx_sta_tstamp_ns);
      double total_rx_duration_ns = (rx_end_tstamp_ns - rx_sta_tstamp_ns);
      uint32_t total_data_size_bits = (num_packets_  * data_pkt_size * 8) ;
      double achieved_tx_tput_gbps = (total_data_size_bits / total_tx_duration_ns);
      double achieved_rx_tput_gbps = (total_data_size_bits / total_rx_duration_ns);

      std::cout << "\tSelected clock frequency : "<< clk_freq << " MHz" << std::endl
                << "\tLatency minimum : " << latency_min_ns << " ns" <<std::endl
                << "\tLatency maximum : " << latency_max_ns << " ns" <<std::endl
                << "\tAchieved Tx throughput : " << achieved_tx_tput_gbps << " GB/s" << std::endl;
      if (eth_loopback_ == "on") {
        std::cout << "\tAchieved Rx throughput : " << achieved_rx_tput_gbps << " GB/s" << std::endl;
      }
      std::cout << std::endl;
    }

    if (eth_ifc == "") {
        std::cout << "No eth interface, so not "
		     "showing stats." << std::endl;
    } else {
        show_eth_stats(eth_ifc);

        if (eth_loopback_ == "on")
            enable_eth_loopback(eth_ifc, false);
    }

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

    os << "0x3c00 " << std::setw(22) << "number_packets" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_NUM_PACKETS)) << std::endl;
    os << "0x3c01 " << std::setw(22) << "random_length" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_RANDOM_LENGTH)) << std::endl;
    os << "0x3c02 " << std::setw(22) << "random_payload" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_RANDOM_PAYLOAD)) << std::endl;
    os << "0x3c03 " << std::setw(22) << "start" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_START)) << std::endl;
    os << "0x3c04 " << std::setw(22) << "stop" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_STOP)) << std::endl;
    os << "0x3c05 " << std::setw(22) << "source_addr0" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_SRC_ADDR0)) << std::endl;
    os << "0x3c06 " << std::setw(22) << "source_addr1" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_SRC_ADDR1)) << std::endl;
    os << "0x3c07 " << std::setw(22) << "dest_addr0" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_DEST_ADDR0)) << std::endl;
    os << "0x3c08 " << std::setw(22) << "dest_addr1" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_DEST_ADDR1)) << std::endl;
    os << "0x3c09 " << std::setw(22) << "packet_tx_count" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_PACKET_TX_COUNT)) << std::endl;
    os << "0x3c0a " << std::setw(22) << "rnd_seed0" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_RND_SEED0)) << std::endl;
    os << "0x3c0b " << std::setw(22) << "rnd_seed1" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_RND_SEED1)) << std::endl;
    os << "0x3c0c " << std::setw(22) << "rnd_seed2" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_RND_SEED2)) << std::endl;
    os << "0x3c0d " << std::setw(22) << "pkt_length" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_PACKET_LENGTH)) << std::endl;
    os << "0x3cf4 " << std::setw(22) << "tx_sta_tstamp" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_TX_STA_TSTAMP)) << std::endl;
    os << "0x3cf5 " << std::setw(22) << "tx_end_tstamp" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_TX_END_TSTAMP)) << std::endl;
  
    os << "0x3d00 " << std::setw(22) << "num_pkt" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_NUM_PKT)) << std::endl;
    os << "0x3d01 " << std::setw(22) << "pkt_good" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_PKT_GOOD)) << std::endl;
    os << "0x3d02 " << std::setw(22) << "pkt_bad" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_PKT_BAD)) << std::endl;
    os << "0x3d07 " << std::setw(22) << "avst_rx_err" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_AVST_RX_ERR)) << std::endl;
    os << "0x3d0b " << std::setw(22) << "rx_sta_tstamp" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_RX_STA_TSTAMP)) << std::endl;
    os << "0x3d0c " << std::setw(22) << "rx_end_tstamp" << ": " <<
      int_to_hex(hafu->mbox_read(dst_port_, CSR_RX_END_TSTAMP)) << std::endl;
  
    os << "0x3e00 " << std::setw(22) << "mac_loop" << ": " <<
      int_to_hex(hafu->mbox_read(src_port_, CSR_MAC_LOOP)) << std::endl;

    return os;
  }

protected:
  uint32_t port_;
  uint32_t dst_port_;
  uint32_t src_port_;
  std::string eth_loopback_;
  std::string he_loopback_;
  uint32_t num_packets_;
  std::string random_length_;
  std::string random_payload_;
  uint32_t packet_length_;
  std::string src_addr_;
  std::string dest_addr_;
  std::string eth_ifc_;
  uint32_t rnd_seed0_;
  uint32_t rnd_seed1_;
  uint32_t rnd_seed2_;
};
