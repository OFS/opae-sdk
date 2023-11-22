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
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <netinet/ether.h>
#include "afu_test.h"
#include <opae/uio.h>

using test_command = opae::afu_test::command;
namespace fpga = opae::fpga::types;

#define PKT_FILT_CSR_DEST_ADDR 0x0040

#define INVALID_MAC 0xffffffffffffffffULL

#define INVALID_CLOCK_FREQ    0.0

#define USER_CLKFREQ_S10      156.25  // MHz
#define USER_CLKFREQ_N6000    402.83203125  // MHz
#define BITSVER_MAJOR_S10     4
#define BITSVER_MAJOR_N6000   5

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)

class hssi_cmd : public test_command
{
public:
  hssi_cmd() {}

  double clock_freq_for(test_afu *afu) const
  {
    auto afu_props = afu->afu_properties();
    auto filter = fpga::properties::get();

    // To get the FME matching our accelerator, we filter
    // for the ssss:bb:dd and for FPGA_DEVICE object type.
    filter->segment = afu_props->segment;
    filter->bus = afu_props->bus;
    filter->device = afu_props->device;
    filter->type = FPGA_DEVICE;

    std::vector<fpga::token::ptr_t> toks =
	    fpga::token::enumerate({filter});

    if (toks.empty()) {
      std::cerr << "Couldn't find FME for AFU!" << std::endl;
      return INVALID_CLOCK_FREQ;
    }

    auto fme_props = fpga::properties::get(toks[0]);

    uint32_t major = FPGA_BBS_VER_MAJOR(fme_props->bbs_id);

    if (major == BITSVER_MAJOR_S10)
      return USER_CLKFREQ_S10;
    else if (major == BITSVER_MAJOR_N6000)
      return USER_CLKFREQ_N6000;

    return INVALID_CLOCK_FREQ;
  }

  int set_pkt_filt_dest(const std::string &dfl_dev,
                        uint64_t bin_dest_addr) const
  {
    struct opae_uio uio;

    if (opae_uio_open(&uio, dfl_dev.c_str())) {
      std::cerr << "Failed to open: " << dfl_dev << std::endl;
      return test_afu::error;
    }

    uint8_t *mmio = nullptr;

    if (opae_uio_region_get(&uio, 0, &mmio, nullptr)) {
      std::cerr << "Failed to get region 0 from: " << dfl_dev << std::endl;
      opae_uio_close(&uio);
      return test_afu::error;
    }

    *(volatile uint64_t *)(mmio + PKT_FILT_CSR_DEST_ADDR) = bin_dest_addr;
    uint64_t dest_addr = *(volatile uint64_t *)(mmio + PKT_FILT_CSR_DEST_ADDR);

    if (bin_dest_addr != dest_addr) {
      std::cerr << "Packet Filter dest address mismatch! wrote: " << int_to_hex(bin_dest_addr)
                << " read: " << int_to_hex(dest_addr) << std::endl;
      opae_uio_close(&uio);
      return test_afu::error;
    }

    opae_uio_close(&uio);

    return test_afu::success;
  }

  uint64_t mac_bits_for(std::string addr) const
  {
    uint64_t res = INVALID_MAC;
    struct ether_addr *eth = ether_aton(addr.c_str());
  
    if (eth) {
      res = 0ULL;
      memcpy(&res, eth->ether_addr_octet, sizeof(eth->ether_addr_octet));
    }

    return res;
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
    usleep(1000000);
  }

  template <typename X>
  std::string int_to_hex(X x) const
  {
    std::stringstream ss;
    ss << "0x" <<
      std::setfill('0') <<
      std::setw(2 * sizeof(X)) <<
      std::hex << x;
    return ss.str();
  }

};
