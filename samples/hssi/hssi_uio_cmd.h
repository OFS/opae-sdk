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
#include "hssi_cmd.h"
#include <opae/uio.h>

#define CSR_DEST_ADDR 0x0040

class hssi_uio_cmd : public hssi_cmd
{
public:
  hssi_uio_cmd()
    : dfl_dev_("none")
    , dest_addr_("77:88:99:aa:bb:cc")
  {}

  virtual const char *name() const override
  {
    return "uio";
  }

  virtual const char *description() const override
  {
    return "UIO Dest MAC Addr setter\n";
  }

  virtual void add_options(CLI::App *app) override
  {
    auto opt = app->add_option("--dfl-dev", dfl_dev_,
                               "dfl device");
    opt->default_str(dfl_dev_);

    opt = app->add_option("--dest-addr", dest_addr_,
                          "destination MAC address");
    opt->default_str(dest_addr_);
  }

  virtual int run(test_afu *afu, CLI::App *app) override
  {
    (void)afu;
    (void)app;

    std::string dfl_dev = dfl_dev_;
    if (dfl_dev == "none") {
      std::cerr << "please specify a --dfl-dev" << std::endl;
      return test_afu::error;
    }

    uint64_t bin_dest_addr = mac_bits_for(dest_addr_);
    if (bin_dest_addr == INVALID_MAC) {
      std::cerr << "invalid MAC address: " << dest_addr_ << std::endl;
      return test_afu::error;
    }

    std::cout << "UIO" << std::endl
              << "  dfl_dev: " << dfl_dev << std::endl
              << "  dest_address: " << dest_addr_ << std::endl
              << "    (bits): 0x" << std::hex << bin_dest_addr << std::endl
              << std::endl;

    struct opae_uio uio;

    if (opae_uio_open(&uio, dfl_dev.c_str())) {
      return test_afu::error;
    }

    uint8_t *mmio = nullptr;

    if (opae_uio_region_get(&uio, 0, &mmio, nullptr)) {
      opae_uio_close(&uio);
      return test_afu::error;
    }

    *(volatile uint64_t *)(mmio + CSR_DEST_ADDR) = bin_dest_addr;

    opae_uio_close(&uio);

    return test_afu::success;
  }

  virtual const char *afu_id() const override
  {
    return "823c334c-98bf-11ea-bb37-0242ac130002";
  }

protected:
  std::string dfl_dev_;
  std::string dest_addr_;
};
