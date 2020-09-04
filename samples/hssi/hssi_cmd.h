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
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <netinet/ether.h>
#include "test_afu.h"

using namespace opae::app;

#define INVALID_MAC 0xffffffffffffffffULL

class hssi_cmd : public test_command
{
public:
  hssi_cmd()
  : running_(true)
  {}

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
    usleep(1000);
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

  void stop()
  {
    running_ = false;
  }

protected:
  bool running_;
};
