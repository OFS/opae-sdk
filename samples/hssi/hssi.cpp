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
#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <thread>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <regex.h>
#include <glob.h>
#include <netinet/ether.h>
#include <CLI/CLI.hpp>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/sysobject.h>
#include "hssi_mbox.h"

using namespace opae::fpga::types;

#define AFU_ID "823c334c-98bf-11ea-bb37-0242ac130002"

#define DEFAULT_ETH_LOOPBACK std::string("on")
#define DEFAULT_PACKETS 1
#define DEFAULT_PACKET_LENGTH 64
#define DEFAULT_QSFP_PORT 0
#define DEFAULT_RANDOM_LENGTH std::string("fixed")
#define DEFAULT_RANDOM_PAYLOAD std::string("incremental")
#define DEFAULT_RND_SEED0 0x5eed0000
#define DEFAULT_RND_SEED1 0x5eed0001
#define DEFAULT_RND_SEED2 0x00025eed
#define DEFAULT_TIMEOUT 5

#define ETH_AFU_DFH           0x0000
#define ETH_AFU_ID_L          0x0008
#define ETH_AFU_ID_H          0x0010
#define TRAFFIC_CTRL_CMD      0x0030
#define TRAFFIC_CTRL_DATA     0x0038
#define TRAFFIC_CTRL_PORT_SEL 0x0040
#define AFU_SCRATCHPAD        0x0048

#define CSR_NUM_PACKETS     0x3c00
#define CSR_RANDOM_LENGTH   0x3c01
#define CSR_RANDOM_PAYLOAD  0x3c02
#define CSR_START           0x3c03
#define CSR_STOP            0x3c04
#define CSR_SRC_ADDR0       0x3c05
#define CSR_SRC_ADDR1       0x3c06
#define CSR_DEST_ADDR0      0x3c07
#define CSR_DEST_ADDR1      0x3c08
#define CSR_PACKET_TX_COUNT 0x3c09
#define CSR_RND_SEED0       0x3c0a
#define CSR_RND_SEED1       0x3c0b
#define CSR_RND_SEED2       0x3c0c
#define CSR_PACKET_LENGTH   0x3c0d

#define CSR_NUM_PKT         0x3d00
#define CSR_PKT_GOOD        0x3d01
#define CSR_PKT_BAD         0x3d02
#define CSR_AVST_RX_ERR     0x3d07

#define CSR_MAC_LOOP        0x3e00

#define MBOX_TIMEOUT 1000

bool parse_filter_address(properties::ptr_t filter, const char *sbdf)
{
  regex_t re;
  regmatch_t matches[5];

  memset(&matches, 0, sizeof(matches));

  std::string sbdf_expr("([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-7])");

  regcomp(&re, sbdf_expr.c_str(), REG_EXTENDED|REG_ICASE);
  bool match = regexec(&re, sbdf, 5, matches, 0) == 0;

  if (match) {
    std::string segment(sbdf + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
    filter->segment = static_cast<uint16_t>(std::stoul(segment, nullptr, 16));

    std::string bus(sbdf + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
    filter->bus = static_cast<uint8_t>(std::stoul(bus, nullptr, 16));

    std::string device(sbdf + matches[3].rm_so, matches[3].rm_eo - matches[3].rm_so);
    filter->device = static_cast<uint8_t>(std::stoul(device, nullptr, 16));

    std::string fn(sbdf + matches[4].rm_so, matches[4].rm_eo - matches[4].rm_so);
    filter->function = static_cast<uint8_t>(std::stoul(fn, nullptr, 16));

    regfree(&re);
    return true;
  }

  regfree(&re);
  memset(&matches, 0, sizeof(matches));

  std::string bdf_expr("([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-7])");

  regcomp(&re, bdf_expr.c_str(), REG_EXTENDED|REG_ICASE);
  match = regexec(&re, sbdf, 5, matches, 0) == 0;

  if (match) {
    filter->segment = 0;

    std::string bus(sbdf + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
    filter->bus = static_cast<uint8_t>(std::stoul(bus, nullptr, 16));

    std::string device(sbdf + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
    filter->device = static_cast<uint8_t>(std::stoul(device, nullptr, 16));

    std::string fn(sbdf + matches[3].rm_so, matches[3].rm_eo - matches[3].rm_so);
    filter->function = static_cast<uint8_t>(std::stoul(fn, nullptr, 16));
  }

  regfree(&re);
  return match;
}

handle::ptr_t open_accelerator(const char *guid, const char *sbdf)
{
  handle::ptr_t accel;

  auto filter = properties::get();
  filter->type = FPGA_ACCELERATOR;
  filter->guid.parse(guid);

  if (sbdf)
    if (!parse_filter_address(filter, sbdf)) {
      std::cerr << "Invalid PCIe address: " << sbdf << std::endl;
      return accel;
    }

  auto tokens = token::enumerate({filter});
  if (tokens.size() < 1) {
    std::cerr << "Accelerator not found." << std::endl;
    return accel;
  }

  if (tokens.size() > 1) {
    std::cerr << "warning: More than one accelerator found." << std::endl;
  }

  return handle::open(tokens[0], 0);
}

#define INVALID_MAC 0xffffffffffffffffULL
uint64_t mac_string_to_bits(const std::string &str)
{
  uint64_t res = INVALID_MAC;
  struct ether_addr *eth = ether_aton(str.c_str());

  if (eth) {
    res = 0ULL;
    memcpy(&res, eth->ether_addr_octet, sizeof(eth->ether_addr_octet));
  }

  return res;
}

class hssi_exception : public std::runtime_error
{
public:
  hssi_exception(const char *msg) :
    std::runtime_error(msg)
  {}
};

class hssi_accessor
{
public:
  hssi_accessor(uint8_t *mmio_base, uint64_t max_ticks=MBOX_TIMEOUT) :
      mmio_base_(mmio_base),
      max_ticks_(max_ticks)
  {}

  void write(uint16_t offset, uint32_t data)
  {
    if (mbox_write(mmio_base_, offset, data, max_ticks_)) {
      throw hssi_exception("mbox_write() timeout");
    }
  }

  uint32_t read(uint16_t offset)
  {
    uint32_t data = 0;
    if (mbox_read(mmio_base_, offset, &data, max_ticks_)) {
      throw hssi_exception("mbox_read() timeout");
    }
    return data;
  }

protected:
  uint8_t *mmio_base_;
  uint64_t max_ticks_;
};

class base_test
{
public:
  base_test(handle::ptr_t h, CLI::App *top, CLI::App *app) :
    running_(true),
    handle_(h),
    top_(top),
    app_(app)
  {}
  virtual ~base_test() {}

  virtual int run() = 0;
  void stop() { running_ = false; }

  std::string ethernet_interface()
  {
    auto props = properties::get(handle_);

    std::ostringstream oss;
    oss << "/sys/bus/pci/devices/" <<
        std::setw(4) << std::setfill('0') << std::hex << props->segment << ":" <<
        std::setw(2) << std::setfill('0') << std::hex << props->bus << ":" <<
        std::setw(2) << std::setfill('0') << std::hex << props->device << "." <<
        std::setw(1) << std::setfill('0') << std::hex << props->function <<
        "/fpga_region/region*/dfl-fme.*/dfl-fme.*.*/net/*";

    glob_t gl;
    if (glob(oss.str().c_str(), 0, nullptr, &gl)) {
      if (gl.gl_pathv)
        globfree(&gl);
      return std::string("");
    }

    if (gl.gl_pathc > 1)
      std::cerr << "Warning: more than one ethernet interface found." << std::endl;

    std::string ifc;
    if (gl.gl_pathc) {
      ifc = gl.gl_pathv[0];
      size_t pos = ifc.rfind("/") + 1;
      ifc = ifc.substr(pos);
    }

    if (gl.gl_pathv)
      globfree(&gl);

    return ifc;
  }

  void run_process(const std::string &proc)
  {
    FILE *fp = popen(proc.c_str(), "r");
    if (fp) {
      char buf[256];
      while (fgets(buf, sizeof(buf), fp)) {
        std::cout << buf;
      }
      pclose(fp);
    }
  }

  void show_eth_stats(const std::string &eth)
  {
    std::string cmd = std::string("ethtool --statistics ") + eth;
    run_process(cmd);
  }

  void enable_eth_loopback(const std::string &eth, bool enable)
  {
    std::string cmd = std::string("ethtool --features ") + eth;
    if (enable)
      cmd += std::string(" loopback on");
    else
      cmd += std::string(" loopback off");
    run_process(cmd);
  }

protected:
  bool running_;
  handle::ptr_t handle_;
  CLI::App *top_;
  CLI::App *app_;
};

base_test *the_test = nullptr;

void print_regs(std::ostream &os, handle::ptr_t h)
{
  os << "0x40000 " << std::setw(21) << "ETH_AFU_DFH" << ": " <<
    int_to_hex(h->read_csr64(ETH_AFU_DFH)) << std::endl;
  os << "0x40008 " << std::setw(21) << "ETH_AFU_ID_L" << ": " <<
    int_to_hex(h->read_csr64(ETH_AFU_ID_L)) << std::endl;
  os << "0x40010 " << std::setw(21) << "ETH_AFU_ID_H" << ": " <<
    int_to_hex(h->read_csr64(ETH_AFU_ID_H)) << std::endl;
  os << "0x40030 " << std::setw(21) << "TRAFFIC_CTRL_CMD" << ": " <<
    int_to_hex(h->read_csr64(TRAFFIC_CTRL_CMD)) << std::endl;
  os << "0x40038 " << std::setw(21) << "TRAFFIC_CTRL_DATA" << ": " <<
    int_to_hex(h->read_csr64(TRAFFIC_CTRL_DATA)) << std::endl;
  os << "0x40040 " << std::setw(21) << "TRAFFIC_CTRL_PORT_SEL" << ": " <<
    int_to_hex(h->read_csr64(TRAFFIC_CTRL_PORT_SEL)) << std::endl;
  os << "0x40048 " << std::setw(21) << "AFU_SCRATCHPAD" << ": " <<
    int_to_hex(h->read_csr64(AFU_SCRATCHPAD)) << std::endl;

  os << std::endl;

  hssi_accessor a(h->mmio_ptr(0));

  os << "0x3c00 " << std::setw(22) << "number_packets" << ": " <<
    int_to_hex(a.read(CSR_NUM_PACKETS)) << std::endl;
  os << "0x3c01 " << std::setw(22) << "random_length" << ": " <<
    int_to_hex(a.read(CSR_RANDOM_LENGTH)) << std::endl;
  os << "0x3c02 " << std::setw(22) << "random_payload" << ": " <<
    int_to_hex(a.read(CSR_RANDOM_PAYLOAD)) << std::endl;
  os << "0x3c03 " << std::setw(22) << "start" << ": " <<
    int_to_hex(a.read(CSR_START)) << std::endl;
  os << "0x3c04 " << std::setw(22) << "stop" << ": " <<
    int_to_hex(a.read(CSR_STOP)) << std::endl;
  os << "0x3c05 " << std::setw(22) << "source_addr0" << ": " <<
    int_to_hex(a.read(CSR_SRC_ADDR0)) << std::endl;
  os << "0x3c06 " << std::setw(22) << "source_addr1" << ": " <<
    int_to_hex(a.read(CSR_SRC_ADDR1)) << std::endl;
  os << "0x3c07 " << std::setw(22) << "dest_addr0" << ": " <<
    int_to_hex(a.read(CSR_DEST_ADDR0)) << std::endl;
  os << "0x3c08 " << std::setw(22) << "dest_addr1" << ": " <<
    int_to_hex(a.read(CSR_DEST_ADDR1)) << std::endl;
  os << "0x3c09 " << std::setw(22) << "packet_tx_count" << ": " <<
    int_to_hex(a.read(CSR_PACKET_TX_COUNT)) << std::endl;
  os << "0x3c0a " << std::setw(22) << "rnd_seed0" << ": " <<
    int_to_hex(a.read(CSR_RND_SEED0)) << std::endl;
  os << "0x3c0b " << std::setw(22) << "rnd_seed1" << ": " <<
    int_to_hex(a.read(CSR_RND_SEED1)) << std::endl;
  os << "0x3c0c " << std::setw(22) << "rnd_seed2" << ": " <<
    int_to_hex(a.read(CSR_RND_SEED2)) << std::endl;
  os << "0x3c0d " << std::setw(22) << "pkt_length" << ": " <<
    int_to_hex(a.read(CSR_PACKET_LENGTH)) << std::endl;

  os << "0x3d00 " << std::setw(22) << "num_pkt" << ": " <<
    int_to_hex(a.read(CSR_NUM_PKT)) << std::endl;
  os << "0x3d01 " << std::setw(22) << "pkt_good" << ": " <<
    int_to_hex(a.read(CSR_PKT_GOOD)) << std::endl;
  os << "0x3d02 " << std::setw(22) << "pkt_bad" << ": " <<
    int_to_hex(a.read(CSR_PKT_BAD)) << std::endl;
  os << "0x3d07 " << std::setw(22) << "avst_rx_err" << ": " <<
    int_to_hex(a.read(CSR_AVST_RX_ERR)) << std::endl;

  os << "0x3e00 " << std::setw(22) << "mac_loop" << ": " <<
    int_to_hex(a.read(CSR_MAC_LOOP)) << std::endl;
}

template <class X>
base_test * create_test(handle::ptr_t h, CLI::App *top, CLI::App *app)
{
  return new X(h, top, app);
}

class scratchpad_test : public base_test
{
public:
  scratchpad_test(handle::ptr_t h, CLI::App *top, CLI::App *app) :
    base_test(h, top, app)
  {}

  virtual int run() override
  {
    std::cout << "Ctrl+C to exit" << std::endl;
    while(running_) {
      std::cout << "scratchpad: " <<
        int_to_hex(handle_->read_csr64(AFU_SCRATCHPAD)) << std::endl;
  
      handle_->write_csr64(AFU_SCRATCHPAD, 0xc0cac01a);
      std::cout << "scratchpad: " << 
        int_to_hex(handle_->read_csr64(AFU_SCRATCHPAD)) << std::endl;

      usleep(500 * 1000);
    }
    print_regs(std::cout, handle_);
    return 0;
  }
};

class lpbk_test : public base_test
{
public:
  lpbk_test(handle::ptr_t h, CLI::App *top, CLI::App *app) :
    base_test(h, top, app)
  {}

  virtual int run() override
  {
    std::cout << "lpbk test" << std::endl;

    auto port_opt = top_->get_option("--port");
    unsigned port = port_opt->as<unsigned>();
    std::cout << "  port: " << port << std::endl;

    auto eth_loopback_opt = app_->get_option("--eth-loopback");
    std::string eth_loopback = eth_loopback_opt->as<std::string>();
    std::cout << "  eth_loopback: " << eth_loopback << std::endl;

    auto num_packets_opt = app_->get_option("--num-packets");
    unsigned num_packets = num_packets_opt->as<unsigned>();
    std::cout << "  num_packets: " << num_packets << std::endl;
  
    auto packet_length_opt = app_->get_option("--packet-length");
    unsigned packet_length = packet_length_opt->as<unsigned>();
    std::cout << "  packet_length: " << packet_length << std::endl;
  
    auto src_addr_opt = app_->get_option("--src-addr");
    std::string src_addr = src_addr_opt->as<std::string>();
    std::cout << "  src address: " << src_addr << std::endl;
    uint64_t bin_src_addr = mac_string_to_bits(src_addr);
    std::cout << "   (bits): 0x" << std::hex << bin_src_addr << std::endl;
  
    if (bin_src_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << src_addr << std::endl;
      return 1;
    }
  
    auto dest_addr_opt = app_->get_option("--dest-addr");
    std::string dest_addr = dest_addr_opt->as<std::string>();
    std::cout << "  dest address: " << dest_addr << std::endl;
    uint64_t bin_dest_addr = mac_string_to_bits(dest_addr);
    std::cout << "   (bits): 0x" << std::hex << bin_dest_addr << std::endl;
    
    if (bin_dest_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << dest_addr << std::endl;
      return 1;
    }

    auto random_length_opt = app_->get_option("--random-length");
    std::string random_length = random_length_opt->as<std::string>();
    std::cout << "  random length: " << random_length << std::endl;

    auto random_payload_opt = app_->get_option("--random-payload");
    std::string random_payload = random_payload_opt->as<std::string>();
    std::cout << "  random payload: " << random_payload << std::endl;

    auto rnd_seed0_opt = app_->get_option("--rnd-seed0");
    uint32_t rnd_seed0 = rnd_seed0_opt->as<uint32_t>();
    std::cout << "  rnd seed0: " << int_to_hex(rnd_seed0) << std::endl;

    auto rnd_seed1_opt = app_->get_option("--rnd-seed1");
    uint32_t rnd_seed1 = rnd_seed1_opt->as<uint32_t>();
    std::cout << "  rnd seed1: " << int_to_hex(rnd_seed1) << std::endl;

    auto rnd_seed2_opt = app_->get_option("--rnd-seed2");
    uint32_t rnd_seed2 = rnd_seed2_opt->as<uint32_t>();
    std::cout << "  rnd seed2: " << int_to_hex(rnd_seed2) << std::endl;

    auto timeout_opt = app_->get_option("--timeout");
    uint32_t timeout_sec = timeout_opt->as<uint32_t>();
    std::cout << "  timeout: " << timeout_sec << " seconds" << std::endl;
    uint64_t timeout_usec = static_cast<uint64_t>(timeout_sec) * 1000000;
    uint64_t timer = 0;
    uint64_t interval = 100; // usec

    std::string eth_ifc = ethernet_interface();
    std::cout << "  eth: " << eth_ifc << std::endl;

    std::cout << std::endl;

    if (eth_loopback == "on")
      enable_eth_loopback(eth_ifc, true);

    hssi_accessor a(handle_->mmio_ptr(0));

    a.write(CSR_STOP, 0);
  
    // 1. External Loopback Test: In this test, traffic will be
    //    generated by AFU and loopback will be done with QSFP
    //    loopback connector. 

    handle_->write_csr64(TRAFFIC_CTRL_PORT_SEL, static_cast<uint64_t>(port));
    a.write(CSR_MAC_LOOP, 0);

    // a. Clear MAC IP statistics registers (May be some OPAE API
    //    to do it, as these are in FIM space)
    //
    // b. Program number_of_packets  register of Traffic
    //    generator CSR space with number of packets to be
    //    transmitted.
  
    a.write(CSR_NUM_PACKETS, num_packets);
  
    // c. Program pkt_length register of Traffic generator CSR
    //    space with length of each packet to be transmitted.
  
    a.write(CSR_PACKET_LENGTH, packet_length);

    // d. Program source_addr0 and source_addr1 registers of
    //    Traffic generator CSR space with Source MAC address.
  
    a.write(CSR_SRC_ADDR0, static_cast<uint32_t>(bin_src_addr));
    a.write(CSR_SRC_ADDR1, static_cast<uint32_t>(bin_src_addr >> 32));
  
    // e. Program destination_addr0 and destination_addr1
    //    registers of Traffic generator CSR space with
    //    Destination MAC address.
  
    a.write(CSR_DEST_ADDR0, static_cast<uint32_t>(bin_dest_addr));
    a.write(CSR_DEST_ADDR1, static_cast<uint32_t>(bin_dest_addr >> 32));
  
    // f. Write 1 to start register of Traffic generator
    //    CSR space.

    a.write(CSR_RANDOM_LENGTH, (random_length == "fixed") ? 0 : 1);
    a.write(CSR_RANDOM_PAYLOAD, (random_payload == "incremental") ? 0 : 1);
    a.write(CSR_RND_SEED0, rnd_seed0);
    a.write(CSR_RND_SEED1, rnd_seed1);
    a.write(CSR_RND_SEED2, rnd_seed2);

    a.write(CSR_START, 1);

    print_regs(std::cout, handle_);

    uint32_t count;
    do
    {
      count = a.read(CSR_PACKET_TX_COUNT);

      if (!running_) {
          a.write(CSR_STOP, 1);
          return 1;
      }

      std::this_thread::sleep_for(std::chrono::microseconds(interval));
      timer += interval;
      if (timer >= timeout_usec) {
        a.write(CSR_STOP, 1);
        std::cerr << "Error: timed out" << std::endl;
        return 1;
      }

    } while(count < num_packets);
  
    // g. Print MAC IP statistics registers (May be some OPAE
    //    API to do it, as these are in FIM space)

    std::cout << std::endl;
    show_eth_stats(eth_ifc);

    if (eth_loopback == "on")
      enable_eth_loopback(eth_ifc, false);

    return 0;
  }
};

class regs_test : public base_test
{
public:
  regs_test(handle::ptr_t h, CLI::App *top, CLI::App *app) :
    base_test(h, top, app)
  {}

  virtual int run() override
  {
    print_regs(std::cout, handle_);
    return 0;
  }
};

void sig_handler(int signum)
{
  switch (signum) {
  case SIGINT:
      std::cerr << "caught SIGINT" << std::endl;
      if (the_test)
        the_test->stop();
      break;
  }
}

int main(int argc, char *argv[])
{
  CLI::App app("hssi");
  std::string guid, bdf;
  auto guid_opt = app.add_option("-g,--guid", guid, "GUID");
  auto bdf_opt = app.add_option("-b,--bdf", bdf, "<segment>:<bus>:<device>.<function>");
  auto port_opt = app.add_option("-p,--port", "QSFP port");
  port_opt->default_val(DEFAULT_QSFP_PORT);

  std::map<CLI::App *, base_test * (*)(handle::ptr_t , CLI::App * , CLI::App * )> tests;

  auto scratch_cmd = app.add_subcommand("scratch", "run scratchpad test");
  tests[scratch_cmd] = create_test<scratchpad_test>;

  auto lpbk_cmd = app.add_subcommand("lpbk", "run lpbk test");
  lpbk_cmd->add_option("--eth-loopback",
                       "on or off, whether to enable loopback on the eth interface")->
                       default_val(DEFAULT_ETH_LOOPBACK);
  lpbk_cmd->add_option("--num-packets", "number of packets")->default_val(DEFAULT_PACKETS);
  lpbk_cmd->add_option("--random-length", "fixed or random")->default_val(DEFAULT_RANDOM_LENGTH);
  lpbk_cmd->add_option("--random-payload", "incremental or random")->default_val(DEFAULT_RANDOM_PAYLOAD);
  lpbk_cmd->add_option("--packet-length", "packet length")->default_val(DEFAULT_PACKET_LENGTH);
  lpbk_cmd->add_option("--src-addr", "source MAC address");
  lpbk_cmd->add_option("--dest-addr", "destination MAC address");
  lpbk_cmd->add_option("--rnd-seed0", "prbs generator [31:0]")->default_val(DEFAULT_RND_SEED0);
  lpbk_cmd->add_option("--rnd-seed1", "prbs generator [47:32]")->default_val(DEFAULT_RND_SEED1);
  lpbk_cmd->add_option("--rnd-seed2", "prbs generator [91:64]")->default_val(DEFAULT_RND_SEED2);
  lpbk_cmd->add_option("--timeout", "timeout in seconds")->default_val(DEFAULT_TIMEOUT);
  tests[lpbk_cmd] = create_test<lpbk_test>;

  auto regs_cmd = app.add_subcommand("regs", "display HSSI registers");
  tests[regs_cmd] = create_test<regs_test>;

  CLI11_PARSE(app, argc, argv);

  signal(SIGINT, sig_handler);

  auto afu_id = *guid_opt ? guid.c_str() : AFU_ID;
  auto addr = *bdf_opt ? bdf.c_str() : nullptr;
  auto h = open_accelerator(afu_id, addr);
  if (!h)
      return 1;

  int res = 1;
  for (auto kv : tests) {
    if (*kv.first) {
      the_test = kv.second(h, &app, kv.first);
      try {
        res = the_test->run();
      } catch(hssi_exception &e) {
        std::cerr << "exception: " << e.what() << std::endl;
      }
      break;
    }
  }

  if (the_test) 
    delete the_test;

  return res;
}
