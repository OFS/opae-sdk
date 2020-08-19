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
#include "test_afu.h"

#define ETH_AFU_DFH           0x0000
#define ETH_AFU_ID_L          0x0008
#define ETH_AFU_ID_H          0x0010
#define TRAFFIC_CTRL_CMD      0x0030
#define TRAFFIC_CTRL_DATA     0x0038
#define TRAFFIC_CTRL_PORT_SEL 0x0040
#define AFU_SCRATCHPAD        0x0048

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

#define CSR_NUM_PKT           0x3d00
#define CSR_PKT_GOOD          0x3d01
#define CSR_PKT_BAD           0x3d02
#define CSR_AVST_RX_ERR       0x3d07

#define CSR_MAC_LOOP          0x3e00

using namespace opae::app;

class hssi_10g_cmd : public test_command
{
public:
  hssi_10g_cmd()
    : eth_loopback_("on")
    , num_packets_(1)
    , random_length_("fixed")
    , random_payload_("incremental")
    , packet_length_(64)
    , src_addr_("11:22:33:44:55:66")
    , dest_addr_("77:88:99:aa:bb:cc")
    , rnd_seed0_(0x5eed0000)
    , rnd_seed1_(0x5eed0001)
    , rnd_seed2_(0x00025eed)
  {

  }
  virtual ~hssi_10g_cmd() {}

  virtual const char *name() const
  {
    return "hssi_10g";
  }

  virtual const char *description() const
  {
    return "hssi 10G test\n";
  }

  virtual void add_options(CLI::App *app)
  {
    auto opt = app->add_option("--eth-loopback", eth_loopback_,
                    "whether to enable loopback on the eth interface");
    opt->check(CLI::IsMember({"on", "off"}))->default_val(eth_loopback_);

    opt = app->add_option("--num-packets", num_packets_,
                          "number of packets");
    opt->default_val(num_packets_);

    opt = app->add_option("--random-length", random_length_,
                          "packet length randomization");
    opt->check(CLI::IsMember({"fixed", "random"}))->default_val(random_length_);

    opt = app->add_option("--random-payload", random_payload_,
                          "payload randomization");
    opt->check(CLI::IsMember({"incremental", "random"}))->default_val(random_payload_);

    opt = app->add_option("--packet-length", packet_length_,
                          "packet length");
    opt->default_val(packet_length_);

    opt = app->add_option("--src-addr", src_addr_,
                          "source MAC address");
    opt->default_val(src_addr_);

    opt = app->add_option("--dest-addr", dest_addr_,
                          "destination MAC address");
    opt->default_val(dest_addr_);

    opt = app->add_option("--rnd-seed0", rnd_seed0_,
                          "prbs generator [31:0]");
    opt->default_val(rnd_seed0_);

    opt = app->add_option("--rnd-seed1", rnd_seed1_,
                          "prbs generator [47:32]");
    opt->default_val(rnd_seed1_);

    opt = app->add_option("--rnd-seed2", rnd_seed2_,
                          "prbs generator [91:64]");
    opt->default_val(rnd_seed2_);
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)afu;
    (void)app;
    return test_afu::success;
  }

protected:
  std::string eth_loopback_;
  uint32_t num_packets_;
  std::string random_length_;
  std::string random_payload_;
  uint32_t packet_length_;
  std::string src_addr_;
  std::string dest_addr_;
  uint32_t rnd_seed0_;
  uint32_t rnd_seed1_;
  uint32_t rnd_seed2_;
};
