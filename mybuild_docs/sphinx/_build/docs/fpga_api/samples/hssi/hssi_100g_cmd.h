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
#include "hssi_cmd.h"
#include <cmath>

#define CSR_SCRATCH               0x1000
#define CSR_BLOCK_ID              0x1001
#define CSR_PKT_SIZE              0x1008
#define CSR_CTRL0                 0x1009
#define CSR_CTRL1                 0x1010
#define CSR_DST_ADDR_LO           0x1011
#define CSR_DST_ADDR_HI           0x1012
#define CSR_SRC_ADDR_LO           0x1013
#define CSR_SRC_ADDR_HI           0x1014
#define CSR_RX_COUNT              0x1015
#define CSR_MLB_RST               0x1016
#define CSR_TX_COUNT              0x1017
#define CSR_STATS_CTRL            0x1018
#define CSR_STATS_TX_CNT_LO       0x1019
#define CSR_STATS_TX_CNT_HI       0x101A
#define CSR_STATS_RX_CNT_LO       0x101B
#define CSR_STATS_RX_CNT_HI       0x101C
#define CSR_STATS_RX_GD_CNT_LO    0x101D
#define CSR_STATS_RX_GD_CNT_HI    0x101E
#define CSR_TX_START_TIMESTAMP_LO 0x101F
#define CSR_TX_START_TIMESTAMP_HI 0x1020
#define CSR_TX_END_TIMESTAMP_LO   0x1021
#define CSR_TX_END_TIMESTAMP_HI   0x1022
#define CSR_RX_START_TIMESTAMP_LO 0x1023
#define CSR_RX_START_TIMESTAMP_HI 0x1024
#define CSR_RX_END_TIMESTAMP_LO   0x1025
#define CSR_RX_END_TIMESTAMP_HI   0x1026

#define STOP_BITS                 0xa
#define ZERO                      0x0
#define ONE                       0x1
#define CSR_SHIFT                 32
#define CONV_SEC                  402832031
#define DATA_CNF_PKT_NUM          0x80000000
#define DATA_CNF_CONTINUOUS_MODE  0x00000000
#define DATA_CNF_FIXED_MODE       0x80000000
#define CURSOR_UP                 "A"
#define CURSOR_DOWN               "B"
#define MAX_PORT                  2

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
    , eth_ifc_("none")
    , start_size_(64)
    , end_size_(9600)
    , end_select_("pkt_num")
    , continuous_("off")
    , contmonitor_(0)
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
    opt->check(CLI::Range(0, 7));

    opt = app->add_option("--eth-loopback", eth_loopback_,
                    "whether to enable loopback on the eth interface");
    opt->check(CLI::IsMember({"on", "off"}))->default_str(eth_loopback_);

    opt = app->add_option("--num-packets", num_packets_,
                          "number of packets");
    opt->default_str(std::to_string(num_packets_));

    opt = app->add_option("--gap", gap_,
                          "inter-packet gap");
    opt->check(CLI::IsMember({"random", "none"}))->default_str(gap_);

    opt = app->add_option("--pattern", pattern_,
                          "pattern mode");
    opt->check(CLI::IsMember({"random", "fixed", "increment"}))->default_str(pattern_);

    opt = app->add_option("--src-addr", src_addr_,
                          "source MAC address");
    opt->default_str(src_addr_);

    opt = app->add_option("--dest-addr", dest_addr_,
                          "destination MAC address");
    opt->default_str(dest_addr_);

    opt = app->add_option("--eth-ifc", eth_ifc_,
                          "ethernet interface name");
    opt->default_str(eth_ifc_);

    opt = app->add_option("--start-size", start_size_,
                          "packet size in bytes (lower limit for incr mode)");
    opt->default_str(std::to_string(start_size_));

    opt = app->add_option("--end-size", end_size_,
                          "upper limit of packet size in bytes");
    opt->default_str(std::to_string(end_size_));

    opt = app->add_option("--end-select", end_select_,
                          "end of packet generation control");
    opt->check(CLI::IsMember({"pkt_num", "gen_idle"}))->default_str(end_select_);

    opt = app->add_option("--continuous", continuous_,
                          "continuous mode");
    opt->check(CLI::IsMember({"on","off"}))->default_str(continuous_);

    opt = app->add_option("--contmonitor", contmonitor_,
                          "time period(in seconds) for performance monitor");
    opt->default_str(std::to_string(contmonitor_));
  }

  uint64_t timestamp_in_seconds(uint64_t x) const
  {
    return round(x/CONV_SEC);
  }

  uint64_t total_per_second(uint64_t y, uint64_t z) const
  {
    if (timestamp_in_seconds(z) != 0)
      return round(y/timestamp_in_seconds(z));
    return 0;
  }

  typedef struct DFH {
    union {
      uint64_t csr;
      struct {
        uint64_t id : 12;
        uint64_t major_rev : 4;
        uint64_t next : 24;
        uint64_t eol : 1;
        uint64_t reserved41 : 7;
        uint64_t minor_rev : 4;
        uint64_t version : 8;
        uint64_t type : 4;
      };
    };
  } DFH;

  typedef struct ctrl_config{
    uint32_t pkt_size_data;
    uint32_t ctrl0_data;
    uint32_t ctrl1_data;
  } ctrl_config;

  typedef struct perf_data{
    volatile uint64_t tx_count;
    volatile uint64_t rx_count;
    volatile uint64_t rx_good_packet_count;
    volatile uint64_t rx_pkt_sec;
    uint64_t bw;
  } perf_data;

  uint64_t data_64(uint32_t lowerbytes, uint32_t higherbytes) const
  {
    return (uint64_t)higherbytes << CSR_SHIFT | lowerbytes;
  }

  void read_performance(perf_data *perf, hssi_afu *hafu) const
  {
    perf->tx_count = data_64(hafu->mbox_read(CSR_STATS_TX_CNT_LO), hafu->mbox_read(CSR_STATS_TX_CNT_HI));
    perf->rx_count = data_64(hafu->mbox_read(CSR_STATS_RX_CNT_LO), hafu->mbox_read(CSR_STATS_RX_CNT_HI));
    perf->rx_good_packet_count = data_64(hafu->mbox_read(CSR_STATS_RX_GD_CNT_LO), hafu->mbox_read(CSR_STATS_RX_GD_CNT_HI));
    perf->rx_pkt_sec = data_64(hafu->mbox_read(CSR_RX_END_TIMESTAMP_LO), hafu->mbox_read(CSR_RX_END_TIMESTAMP_HI));
  }

  void calc_performance(perf_data *old_perf, perf_data *new_perf, perf_data *perf, uint64_t size) const
  {
    perf->tx_count = new_perf->tx_count;
    perf->rx_count = new_perf->rx_count;
    perf->rx_good_packet_count = new_perf->rx_good_packet_count;
    perf->rx_pkt_sec = total_per_second((new_perf->rx_count - old_perf->rx_count), (new_perf->rx_pkt_sec - old_perf->rx_pkt_sec));
    perf->bw = (perf->rx_pkt_sec * size) >> 27;
  }

  uint8_t stall_enable(hssi_afu *hafu) const
  {
    volatile uint32_t reg;
    reg = hafu->mbox_read(CSR_STATS_CTRL);
    reg |= 1 << 1;
    hafu->mbox_write(CSR_STATS_CTRL, reg);
    return 1;
  }

  uint8_t stall_disable(hssi_afu *hafu) const
  {
    volatile uint32_t reg;
    reg = hafu->mbox_read(CSR_STATS_CTRL);
    reg &= ~(1 << 1);
    hafu->mbox_write(CSR_STATS_CTRL, reg);
    return 1;
  }

  void write_ctrl_config(hssi_afu *hafu, ctrl_config config_data) const
  {
    hafu->mbox_write(CSR_PKT_SIZE, config_data.pkt_size_data);
    hafu->mbox_write(CSR_CTRL0, config_data.ctrl0_data);
    hafu->mbox_write(CSR_CTRL1, config_data.ctrl1_data);
  }

  void write_csr_addr(hssi_afu *hafu, uint64_t bin_src_addr, uint64_t bin_dest_addr) const
  {
    hafu->mbox_write(CSR_SRC_ADDR_LO, static_cast<uint32_t>(bin_src_addr));
    hafu->mbox_write(CSR_SRC_ADDR_HI, static_cast<uint32_t>(bin_src_addr >> 32));

    hafu->mbox_write(CSR_DST_ADDR_LO, static_cast<uint32_t>(bin_dest_addr));
    hafu->mbox_write(CSR_DST_ADDR_HI, static_cast<uint32_t>(bin_dest_addr >> 32));
  }

  std::ostream & print_monitor_headers(std::ostream &os, uint32_t max_timer) const
  {
    os<< std::dec << "\r\n"<<"Monitor mode for "<< max_timer <<" sec, Press 'q' to quit and 'r' to reset"<<"\r";
    os<< "\r\n___________________________________________________________________________________________________\r\n";

    os<< "\r\n"
    << std::left
    << "| "
    << std::setw(5)
    << "Port" << " | " << std::setw(7)
    << "Time(sec)" << " | " << std::setw(15)
    << "Tx count" << " | " << std::setw(15)
    << "Rx count" << " | " << std::setw(15)
    << "Rx good"  << " | " << std::setw(10)
    << "Rx pkt/s" << " | " << std::setw(8)
    << "BW" << " | "
    << "\r";

    os<< "\n---------------------------------------------------------------------------------------------------\r\n";
    return os;
  }

  std::ostream & print_monitor_data(std::ostream &os, perf_data *data, int port, uint32_t timer) const
  {
    os << std::dec
    << std::left
    << "| "
    << std::setw(5)
    << port << " | " << std::setw(6)
    << timer<< "   " << " | " << std::setw(15)
    << data->tx_count << " | " << std::setw(15)
    << data->rx_count << " | " << std::setw(15)
    << data->rx_good_packet_count<< " | " << std::setw(10)
    << data->rx_pkt_sec << " | " << std::setw(4)
    << data->bw << "Gbps" << " | "
    << "\r";
    return os;
  }

  void select_port(int port, ctrl_config config_data, hssi_afu *hafu) const
  {
    /* Selects the port before performing read/write to traffic controller reg space */
    hafu->write64(TRAFFIC_CTRL_PORT_SEL, port);
    write_ctrl_config(hafu, config_data);
    write_csr_addr(hafu, mac_bits_for(src_addr_), mac_bits_for(dest_addr_));
  }

  void capture_perf(perf_data *old_perf_data, hssi_afu *hafu) const
  {
    memset(old_perf_data, 0, sizeof(perf_data));
    if (stall_enable(hafu))
    {
      read_performance(old_perf_data, hafu);
      stall_disable(hafu);
    }
  }

  std::ostream & run_monitor(std::ostream &os, perf_data *old_perf_data, int port, hssi_afu *hafu, uint32_t timer)
  {
    perf_data *cur_perf_data = (perf_data*)malloc(sizeof(perf_data));
    perf_data *new_perf_data = (perf_data*)malloc(sizeof(perf_data));

    memset(cur_perf_data, 0, sizeof(perf_data));

    capture_perf(new_perf_data, hafu);
    calc_performance(old_perf_data, new_perf_data, cur_perf_data, start_size_);

    print_monitor_data(os, cur_perf_data, port, timer);
    memcpy(old_perf_data, new_perf_data, sizeof(perf_data));

    return os;
  }

  char get_user_input(char key) const
  {
    /* Waits for user input for 1 second */
    key = '\0';      // initialized to null
    std::thread t1([&](){
      std::cin>>key;
    });
    t1.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return key;
  }

  void quit_monitor(uint8_t size, std::string move, hssi_afu *hafu) const
  {
    /* Quit the monitor*/
    move_cursor(size, move);
    hafu->mbox_write(CSR_CTRL0, DATA_CNF_PKT_NUM);
    hafu->mbox_write(CSR_STATS_CTRL, ZERO);
  }

  void reset_monitor(std::vector<int> port_, std::string move, ctrl_config config_data, hssi_afu *hafu) const
  {
    /* Reset the monitor ports traffic data*/
    uint8_t size = (uint8_t)port_.size();
    for(uint8_t port=0; port < size; port++)
      {
        select_port(port_[port], config_data, hafu);
        hafu->mbox_write(CSR_STATS_CTRL, ONE);
        hafu->mbox_write(CSR_STATS_CTRL, ZERO);
      }
     move_cursor(1, move);
  }

  void move_cursor(uint8_t size, std::string move) const
  {
    /* Moves the cursor position (up/down) based on port size*/
    std::ostringstream oss;

    oss << "\x1b[" << (int)size << move;

    std::cout << oss.str().c_str();
  }

  virtual int run(test_afu *afu, CLI::App *app) override
  {
    (void)app;

    if (port_.size() > MAX_PORT)
    {
      std::cerr<< "more than " << MAX_PORT << " ports are not supported"<<std::endl;
      return test_afu::error;
    }

    if (port_.empty())
        port_.push_back(0);

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
    if (eth_ifc == "none")
      eth_ifc = hafu->ethernet_interface();

    std::cout << "100G loopback test" << std::endl
              << "  port: " << port_[0] << std::endl
              << "  eth_loopback: " << eth_loopback_ << std::endl
              << "  num_packets: " << num_packets_ << std::endl
              << "  gap: " << gap_ << std::endl
              << "  src_address: " << src_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_src_addr << std::endl
              << "  dest_address: " << dest_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_dest_addr << std::endl
              << "  pattern: " << pattern_ << std::endl
              << "  start size: " << start_size_ << std::endl
              << "  end size: " << end_size_ << std::endl
              << "  end select: " << end_select_ << std::endl
              << "  eth: " << eth_ifc << std::endl
              << "  continuous mode: " << continuous_ << std::endl
              << "  monitor duration: " << std::dec << contmonitor_ << " sec" << std::endl
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
    ctrl_config config_data;

    DFH dfh;
    dfh.csr = hafu->read64(ETH_AFU_DFH);

    hafu->write64(TRAFFIC_CTRL_PORT_SEL, port_[0]);
    hafu->mbox_write(CSR_CTRL1, STOP_BITS);

    uint32_t reg;

    reg = start_size_ | (end_size_ << 16);
    hafu->mbox_write(CSR_PKT_SIZE, reg);
    config_data.pkt_size_data = reg;

    reg = num_packets_;
    if (dfh.major_rev < 2){
        reg |= DATA_CNF_FIXED_MODE;
    }
    else {
      if(continuous_ == "on")
        reg = DATA_CNF_CONTINUOUS_MODE;
      if(continuous_ == "off")
        reg |= DATA_CNF_FIXED_MODE;
    }

    hafu->mbox_write(CSR_CTRL0, reg);
    config_data.ctrl0_data = reg;

    hafu->mbox_write(CSR_SRC_ADDR_LO, static_cast<uint32_t>(bin_src_addr));
    hafu->mbox_write(CSR_SRC_ADDR_HI, static_cast<uint32_t>(bin_src_addr >> 32));

    hafu->mbox_write(CSR_DST_ADDR_LO, static_cast<uint32_t>(bin_dest_addr));
    hafu->mbox_write(CSR_DST_ADDR_HI, static_cast<uint32_t>(bin_dest_addr >> 32));

    reg = 0;
    if (dfh.major_rev < 2){
      if (gap_ == "random")
        reg |= 1 << 7;

      if (end_select_ == "pkt_num")
        reg |= 1 << 6;
    }
    else{
      if (gap_ == "random")
        reg |= 1 << 6;
    }

    if (pattern_ == "fixed")
      reg |= 1 << 4;
    else if (pattern_ == "increment")
      reg |= 2 << 4;

    if (eth_loopback_ == "off")
      reg |= 1 << 3;

    hafu->mbox_write(CSR_CTRL1, reg);
    config_data.ctrl1_data = reg;

    if(port_.size() > 1) // Supporting for 2nd port register setting
      select_port(port_[1], config_data, hafu);

    volatile uint32_t count;
    const uint64_t interval = 100ULL;
    do
    {
      count = hafu->mbox_read(CSR_TX_COUNT);
      if (count == 0)  // Added to eliminate infinite loop occured when CSR_TX_COUNT is 0
        break;

      if (!running()) {
        hafu->mbox_write(CSR_CTRL1, STOP_BITS);
        return test_afu::error;
      }

      std::this_thread::sleep_for(std::chrono::microseconds(interval));
    } while(count < num_packets_);

    print_registers(std::cout, hafu);

    std::cout << std::endl;

    if (eth_ifc == "") {
      std::cout << "No eth interface, so not "
                   "showing stats." << std::endl;
    } else {
      show_eth_stats(eth_ifc);

      if (eth_loopback_ == "on")
        enable_eth_loopback(eth_ifc, false);
    }

    if ((dfh.major_rev < 2) && (continuous_ == "on" || (contmonitor_ > 0)))
    {
        std::cout << "\nHSSI doesn't support continuous mode\n" << std::endl;
        return test_afu::error;
    }

    /* Monitor Implementation */

    if (continuous_ != "off" && contmonitor_ != 0)
    {
      uint32_t timer = 0;
      uint32_t max_timer = contmonitor_;
      char key;

      uint8_t port_size = (uint8_t)port_.size();
      sort(port_.begin(), port_.end());
      std::vector<perf_data*> old_perf_data(port_size);    // old performance

      /* Initialise performance data and initiate monitor */
      print_monitor_headers(std::cout, max_timer);
      for(uint8_t port=0; port < port_size; port++)
      {
        old_perf_data[port] = (perf_data*)malloc(sizeof(perf_data));
        select_port(port_[port], config_data, hafu);
        capture_perf(old_perf_data[port], hafu);           // captures the monitor data for first time
      }

      /* Calculate performance and run monitor */
      while(timer < max_timer)
      {
        for(uint8_t port=0; port < port_size; port++)
        {
          select_port(port_[port], config_data, hafu);
          run_monitor(std::cout, old_perf_data[port], port_[port], hafu, timer+1) << std::endl;
        }
        move_cursor(port_size, CURSOR_UP);
        timer+=1;

        key = get_user_input(key);
        if(key=='q')
        {
          quit_monitor(port_size, CURSOR_DOWN, hafu);
          return test_afu::success;
        }
        if(key=='r')
          reset_monitor(port_, CURSOR_UP, config_data, hafu);

        //Handling CTL+C and CTL+Z
        if (!running()) {
          quit_monitor(port_size, CURSOR_DOWN, hafu);
          return test_afu::error;
        }
      }
      quit_monitor(port_size, CURSOR_DOWN, hafu);
    }
    else if (contmonitor_ > 0)
    {
      std::cout << "Please use --continuous and --contmonitor together" << std::endl;
      return test_afu::error;
    }
    return test_afu::success;
  }

  virtual const char *afu_id() const override
  {
    return "43425ee6-92b2-4742-b03a-bd8d4a533812";
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
    os << "0x1001 " << std::setw(22) << "block_ID" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_BLOCK_ID)) << std::endl;
    os << "0x1008 " << std::setw(22) << "pkt_size" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_PKT_SIZE)) << std::endl;
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
    os << "0x1015 " << std::setw(22) << "rx_count" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_RX_COUNT)) << std::endl;
    os << "0x1016 " << std::setw(22) << "mlb_rst" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_MLB_RST)) << std::endl;
    os << "0x1017 " << std::setw(22) << "tx_count" << ": " <<
      int_to_hex(hafu->mbox_read(CSR_TX_COUNT)) << std::endl;

    return os;
  }

protected:
  std::vector<int> port_;
  std::string eth_loopback_;
  uint32_t num_packets_;
  std::string gap_;
  std::string pattern_;
  std::string src_addr_;
  std::string dest_addr_;
  std::string eth_ifc_;
  uint32_t start_size_;
  uint32_t end_size_;
  std::string end_select_;
  std::string continuous_;
  uint32_t contmonitor_;
};
