// Copyright(c) 2021, Intel Corporation
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
#include "hssi_100g_cmd.h"

class hssi_pkt_filt_100g_cmd : public hssi_100g_cmd
{
public:
  hssi_pkt_filt_100g_cmd()
    : dfl_dev_("none")
  {}

  virtual const char *name() const override
  {
    return "pkt_filt_100g";
  }

  virtual const char *description() const override
  {
    return "100G Packet Filter test\n";
  }

  virtual void add_options(CLI::App *app) override
  {
    auto opt = app->add_option("--dfl-dev", dfl_dev_,
                               "dfl device");
    opt->default_str(dfl_dev_);

    hssi_100g_cmd::add_options(app);
  }

  virtual int run(test_afu *afu, CLI::App *app) override
  {
    uint64_t bin_dest_addr = mac_bits_for(dest_addr_);
    if (bin_dest_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << dest_addr_ << std::endl;
      return test_afu::error;
    }

    std::string dfl_dev = dfl_dev_;
    if (dfl_dev == "none") {
      std::cout << "--dfl-dev is missing." << std::endl
                << "Skipping Packet Filter." << std::endl
                << std::endl;
    } else {
      std::cout << "Packet Filter" << std::endl
                << "  dfl_dev: " << dfl_dev << std::endl
                << std::endl;

      int res = set_pkt_filt_dest(dfl_dev, bin_dest_addr);
      if (res != test_afu::success)
        return res;
    }

    return hssi_100g_cmd::run(afu, app);
  }

protected:
  std::string dfl_dev_;
};
