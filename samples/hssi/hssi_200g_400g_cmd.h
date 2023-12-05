// Copyright(c) 2023, Intel Corporation
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
#include <unistd.h>
#include <cmath>
#include "hssi_cmd.h"

// The intention of this utility is to demonstrate simple use of the 200G/400G
// Ethernet subsystem so that a user may add their own complexity on top.

// Code structure:
// This class inherits hssi_cmd (which in turn inherits 'command') as part of
// the afu-test framework. It implements a specific 'command' (which is
// essentially like a test / sequence of operations) to exercise the 200G-400G
// F-Tile HSSI Sub-System.

// HSSI = "High Speed Serial Interface" which is better referred to as
// "Ethernet". It expects a specific AFU (accelerator function unit) to be
// present in the FPGA (specified by afu_id()). This corresponding AFU
// instantiates a traffic generator (TG) to transmit/receive packet data to the
// HSSI-SS.

// The run() function talks to the TG to create some test traffic flow and
// measure throughput.

// The TG has very limited control -- you can only control the number of packets
// sent. The following terms are synonymous: traffic generator, traffic
// controller, packet client. The TG has its own control/status register (CSR)
// map that is abstracted behind a "Mailbox". The Mailbox presents a small set
// of command registers that accept address, data, control. Through these, the
// user can read/write the abstracted register space. The Mailbox CSR address
// space is defined with the hssi_afu() class. The Mailbox may have multiple
// ports (also called 'channels') where each port provides a window into a
// distinct abstracted address space. In other words, the AFU may instantiate
// multiple TGs where each TG is accessed through a separate Mailbox port.

// It is assumed that a 200G AFU uses two Mailbox channels (i.e. two traffic
// generators) and a 400G AFU uses one channel.

// Registers in the AFU CSR space. Other registers, in this space, are defined
// in hssi_afu.h
#define CSR_AFU_400G_TG_EN 0x0058

// 200G/400G traffic generator registers. This address space is accessed
// indirectly through the Mailbox The CSR map is defined in a spreadsheet,
// located here at the time of this writing:
// https://github.com/OFS/ofs-fim-common/tree/release/ofs-2023.2/src/common/he_hssi/HE_HSSI_TC_200_400G.xls

#define CSR_HW_PC_CTRL 0x0000
#define CSR_HW_TEST_LOOP_CNT 0x0004
#define CSR_HW_TEST_ROM_ADDR 0x0008
#define CSR_STAT_TX_SOP_CNT_LSB 0x0020
#define CSR_STAT_TX_SOP_CNT_MSB 0x0024
#define CSR_STAT_TX_EOP_CNT_LSB 0x0028
#define CSR_STAT_TX_EOP_CNT_MSB 0x002C
#define CSR_STAT_TX_ERR_CNT_LSB 0x0030
#define CSR_STAT_TX_ERR_CNT_MSB 0x0034
#define CSR_STAT_RX_SOP_CNT_LSB 0x0038
#define CSR_STAT_RX_SOP_CNT_MSB 0x003C
#define CSR_STAT_RX_EOP_CNT_LSB 0x0040
#define CSR_STAT_RX_EOP_CNT_MSB 0x0044
#define CSR_STAT_RX_ERR_CNT_LSB 0x0048
#define CSR_STAT_RX_ERR_CNT_MSB 0x004C
#define CSR_STAT_TIMESTAMP_TG_START_LSB 0x0050
#define CSR_STAT_TIMESTAMP_TG_START_MSB 0x0054
#define CSR_STAT_TIMESTAMP_TG_END_LSB 0x0058
#define CSR_STAT_TIMESTAMP_TG_END_MSB 0x005C

#define USER_CLKFREQ_N6001 \
  470.00  // MHz. The HE-HSSI AFU is clocked by sys_pll|iopll_0_clk_sys

class hssi_200g_400g_cmd : public hssi_cmd {
 public:
  hssi_200g_400g_cmd() : num_packets_(32) {}

  virtual const char *name() const override { return "hssi_200g_400g"; }

  virtual const char *description() const override {
    return "hssi 200G_400G test\n";
  }

  virtual void add_options(CLI::App *app) override {
    auto opt =
        app->add_option("--num-packets", num_packets_, "number of packets");
    opt->default_str(std::to_string(num_packets_));
  }

  virtual int run(test_afu *afu, CLI::App *app) override {
    (void)app;

    hssi_afu *hafu = dynamic_cast<hssi_afu *>(afu);

    // Check if this AFU is for 200G or 400G by reading a status reg in the AFU
    // CSR region.
    bool tg_200n_400;
    tg_200n_400 = bool(
        hafu->read64(CSR_AFU_400G_TG_EN));  // Bit-0 = 0 for 200g, 1 for 400g
    std::cout << "Detected " << (tg_200n_400 ? "400G" : "200G") << std::setw(21)
              << " HE-HSSI AFU." << std::endl
              << std::endl;

    // For 200G there are two Ethernet ports, #8 and #12.
    // For 400G there is one Ethernet port, #8.
    // These are connected to Mailbox channels 0 and 1, respectively.
    int num_ports = tg_200n_400 ? 1 : 2;
    for (int i = 0; i < num_ports; i++) {
      // Select the appropriate port on the Mailbox
      std::cout << "Setting traffic control/mailbox channel-select to " << i
                << std::endl;
      hafu->write64(TRAFFIC_CTRL_PORT_SEL, i);

      // Set ROM start/end address.
      // The TG reads and transmits packet data from a 1024-word ROM. The ROM
      // contents are fixed at FPGA compile-time. It contains packets of size
      // 1482-bytes. Each loop through the ROM transmits 32 packets. The
      // end-address (i.e. the final address to read from in each loop through
      // the ROM) is different in 200G vs 400G. CSR_HW_TEST_ROM_ADDR[15:0]   =
      // ROM start address CSR_HW_TEST_ROM_ADDR[31:16]  = ROM end address
      std::cout << "Setting start/end ROM address" << std::endl;
      uint32_t reg;
      reg = 0;
      reg |= tg_200n_400 ? (0x0191 << 16) : (0x02FF << 16);
      hafu->mbox_write(CSR_HW_TEST_ROM_ADDR, reg);

      uint32_t num_packets_per_rom_loop = 32;
      if ((num_packets_ % num_packets_per_rom_loop != 0) ||
          (num_packets_ < num_packets_per_rom_loop)) {
        std::cout << "--num_packets <num> must be >="
                  << num_packets_per_rom_loop << std::setw(21)
                  << " and a multiple of " << num_packets_per_rom_loop
                  << std::setw(21)
                  << " since the traffic generator only sends this multiple."
                  << std::endl;
        return test_afu::error;
      }

      uint32_t rom_loop_count = num_packets_ / num_packets_per_rom_loop;
      // The ROM loopcount register in the TG is 32-bits. Since num_packets_ is
      // itself only 32 bits, it's guaranteed we will not overflow
      // rom_loop_count.

      reg = rom_loop_count;
      std::cout << "num_packets = " << num_packets_
                << ". Setting ROM loop count to " << reg << std::endl;
      std::cout << "Packet length is fixed at 1482-bytes. 32 packets are sent "
                   "for every ROM loop."
                << std::endl;
      hafu->mbox_write(CSR_HW_TEST_LOOP_CNT, reg);

      std::cout << "Resetting traffic-generator counters" << std::endl;
      reg = 0x180;  // Set bit-7 = 1 (clear status regs), bit8 = 1 (clear
                    // counters themselves).
      hafu->mbox_write(CSR_HW_PC_CTRL, reg);
      reg = 0x0;  // bit-7 must be manually cleared. bit-8 is self-clearing.
      hafu->mbox_write(CSR_HW_PC_CTRL, reg);

      std::cout << "Starting the traffic-generator." << std::endl;
      // Start the TG
      reg = 0x1;
      hafu->mbox_write(CSR_HW_PC_CTRL, reg);

      std::cout << "Waiting for all packets to be sent." << std::endl;
      // Loop until the TX SOP Count matches the expected num_packets.
      uint64_t tx_sop_count = 0;
      const uint64_t interval = 100ULL;
      while (tx_sop_count < num_packets_) {
        tx_sop_count =
            ((uint64_t)hafu->mbox_read(CSR_STAT_TX_SOP_CNT_MSB) << 32) |
            (uint64_t)hafu->mbox_read(CSR_STAT_TX_SOP_CNT_LSB);
        if (!running()) {
          reg = 0x00;  // Stop the TG
          hafu->mbox_write(CSR_HW_PC_CTRL, reg);
          return test_afu::error;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(interval));
      }

      std::cout << "Stopping the traffic-generator" << std::endl;
      reg = 0x00;  // Stop the TG (bit-0=0) and take snapshot (bit-6=1)
      hafu->mbox_write(CSR_HW_PC_CTRL, reg);

      std::cout << "Short sleep to allow packets to propagate" << std::endl;
      sleep(1);
      std::cout << "Taking snapshot of counters" << std::endl;

      reg = 0x40;  // Take snapshot (bit-6=1)
      hafu->mbox_write(CSR_HW_PC_CTRL, reg);

      std::cout << std::endl << std::endl;
      print_registers(std::cout, hafu);
      std::cout << std::endl << std::endl;

      // Throughput calculation
      double sample_period_ns = 1000 / USER_CLKFREQ_N6001;
      uint64_t timestamp_start, timestamp_end, timestamp_duration_cycles;
      double timestamp_duration_ns;
      timestamp_start =
          ((uint64_t)hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_START_MSB) << 32) |
          (uint64_t)hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_START_LSB);
      timestamp_end =
          ((uint64_t)hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_END_MSB) << 32) |
          (uint64_t)hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_END_LSB);
      assert(timestamp_end > timestamp_start);
      timestamp_duration_cycles = timestamp_end - timestamp_start;
      timestamp_duration_ns = timestamp_duration_cycles * sample_period_ns;
      uint64_t payload_bytes_per_packet = 1464;
      uint64_t total_bytes_per_packet = 1482;
      uint64_t total_num_packets = num_packets_per_rom_loop * rom_loop_count;
      uint64_t total_payload_size_bytes =
          payload_bytes_per_packet * total_num_packets;
      uint64_t total_size_bytes = total_bytes_per_packet * total_num_packets;
      double throughput_payload_bytes_gbps =
          total_payload_size_bytes / timestamp_duration_ns;
      double throughput_total_bytes_gbps =
          total_size_bytes / timestamp_duration_ns;

      std::cout << "AFU clock frequency : " << USER_CLKFREQ_N6001 << " MHz"
                << std::endl
                << "Total # packets transmitted: " << total_num_packets
                << std::endl
                << "Total payload transmitted: " << total_payload_size_bytes
                << " bytes" << std::endl
                << "Total data transmitted: " << total_size_bytes << " bytes"
                << std::endl
                << "Total duration ns: " << timestamp_duration_ns << " ns."
                << std::endl
                << "Throughput on payload data: "
                << throughput_payload_bytes_gbps
                << " Gbytes/sec = " << throughput_payload_bytes_gbps * 8
                << " Gbits/sec" << std::endl
                << "Throughput on total data: " << throughput_total_bytes_gbps
                << " Gbytes/sec = " << throughput_total_bytes_gbps * 8
                << " Gbits/sec" << std::endl
                << std::endl
                << std::endl
                << std::endl;
    }

    return test_afu::success;
  }

  virtual const char *afu_id() const override {
    // From 200g_400g AFU register map spreadsheet
    // REGISTER NAME    ADDRESS     DEFAULT             DESCRIPTION
    // AFU_ID_L         0x0008      0xB18A51879087C674  AFU ID (Lower 64-bit)
    // AFU_ID_H         0x0010      0x71F59769AD6049ED  AFU ID (Upper 64-bit)

    return "71f59769-ad60-49ed-b18a-51879087c674";
  }

  std::ostream &print_registers(std::ostream &os, hssi_afu *hafu) const {
    os << "Printing CSRs from base AFU region:" << std::endl;

    os << int_to_hex(ETH_AFU_DFH) << std::setw(21)
       << " ETH_AFU_DFH: " << int_to_hex(hafu->read64(ETH_AFU_DFH))
       << std::endl;
    os << int_to_hex(ETH_AFU_ID_L) << std::setw(21)
       << " ETH_AFU_ID_L: " << int_to_hex(hafu->read64(ETH_AFU_ID_L))
       << std::endl;
    os << int_to_hex(ETH_AFU_ID_H) << std::setw(21)
       << " ETH_AFU_ID_H: " << int_to_hex(hafu->read64(ETH_AFU_ID_H))
       << std::endl;
    os << int_to_hex(TRAFFIC_CTRL_CMD) << std::setw(21)
       << " TRAFFIC_CTRL_CMD: " << int_to_hex(hafu->read64(TRAFFIC_CTRL_CMD))
       << std::endl;
    os << int_to_hex(TRAFFIC_CTRL_DATA) << std::setw(21)
       << " TRAFFIC_CTRL_DATA: " << int_to_hex(hafu->read64(TRAFFIC_CTRL_DATA))
       << std::endl;
    os << int_to_hex(TRAFFIC_CTRL_PORT_SEL) << std::setw(21)
       << " TRAFFIC_CTRL_PORT_SEL: "
       << int_to_hex(hafu->read64(TRAFFIC_CTRL_PORT_SEL)) << std::endl;
    os << int_to_hex(AFU_SCRATCHPAD) << std::setw(21)
       << " AFU_SCRATCHPAD: " << int_to_hex(hafu->read64(AFU_SCRATCHPAD))
       << std::endl;
    os << int_to_hex(CSR_AFU_400G_TG_EN) << std::setw(21)
       << " CSR_AFU_400G_TG_EN: "
       << int_to_hex(hafu->read64(CSR_AFU_400G_TG_EN)) << std::endl;

    os << std::endl;
    os << "Printing CSRs from Traffic Generator IP (behind the mailbox):"
       << std::endl;

    os << int_to_hex(CSR_HW_PC_CTRL) << std::setw(21)
       << " CSR_HW_PC_CTRL: " << int_to_hex(hafu->mbox_read(CSR_HW_PC_CTRL))
       << std::endl;
    os << int_to_hex(CSR_HW_TEST_LOOP_CNT) << std::setw(21)
       << " CSR_HW_TEST_LOOP_CNT: "
       << int_to_hex(hafu->mbox_read(CSR_HW_TEST_LOOP_CNT)) << std::endl;
    os << int_to_hex(CSR_HW_TEST_ROM_ADDR) << std::setw(21)
       << " CSR_HW_TEST_ROM_ADDR: "
       << int_to_hex(hafu->mbox_read(CSR_HW_TEST_ROM_ADDR)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_SOP_CNT_LSB) << std::setw(21)
       << " CSR_STAT_TX_SOP_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_SOP_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_SOP_CNT_MSB) << std::setw(21)
       << " CSR_STAT_TX_SOP_CNT_MSB : "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_SOP_CNT_MSB)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_EOP_CNT_LSB) << std::setw(21)
       << " CSR_STAT_TX_EOP_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_EOP_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_EOP_CNT_MSB) << std::setw(21)
       << " CSR_STAT_TX_EOP_CNT_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_EOP_CNT_MSB)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_ERR_CNT_LSB) << std::setw(21)
       << " CSR_STAT_TX_ERR_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_ERR_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_TX_ERR_CNT_MSB) << std::setw(21)
       << " CSR_STAT_TX_ERR_CNT_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TX_ERR_CNT_MSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_SOP_CNT_LSB) << std::setw(21)
       << " CSR_STAT_RX_SOP_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_SOP_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_SOP_CNT_MSB) << std::setw(21)
       << " CSR_STAT_RX_SOP_CNT_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_SOP_CNT_MSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_EOP_CNT_LSB) << std::setw(21)
       << " CSR_STAT_RX_EOP_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_EOP_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_EOP_CNT_MSB) << std::setw(21)
       << " CSR_STAT_RX_EOP_CNT_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_EOP_CNT_MSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_ERR_CNT_LSB) << std::setw(21)
       << " CSR_STAT_RX_ERR_CNT_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_ERR_CNT_LSB)) << std::endl;
    os << int_to_hex(CSR_STAT_RX_ERR_CNT_MSB) << std::setw(21)
       << " CSR_STAT_RX_ERR_CNT_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_RX_ERR_CNT_MSB)) << std::endl;

    os << int_to_hex(CSR_STAT_TIMESTAMP_TG_START_LSB) << std::setw(21)
       << " CSR_STAT_TIMESTAMP_TG_START_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_START_LSB))
       << std::endl;
    os << int_to_hex(CSR_STAT_TIMESTAMP_TG_START_MSB) << std::setw(21)
       << " CSR_STAT_TIMESTAMP_TG_START_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_START_MSB))
       << std::endl;
    os << int_to_hex(CSR_STAT_TIMESTAMP_TG_END_LSB) << std::setw(21)
       << " CSR_STAT_TIMESTAMP_TG_END_LSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_END_LSB))
       << std::endl;
    os << int_to_hex(CSR_STAT_TIMESTAMP_TG_END_MSB) << std::setw(21)
       << " CSR_STAT_TIMESTAMP_TG_END_MSB: "
       << int_to_hex(hafu->mbox_read(CSR_STAT_TIMESTAMP_TG_END_MSB))
       << std::endl;

    return os;
  }

 protected:
  uint32_t num_packets_;
};
