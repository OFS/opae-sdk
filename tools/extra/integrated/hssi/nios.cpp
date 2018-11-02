// Copyright(c) 2017-2018, Intel Corporation
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
#include "nios.h"
#include "hssi_msg.h"

namespace intel {
namespace fpga {
namespace hssi {

nios::nios(hssi_przone::ptr_t przone)
    : przone_{przone}, mmio_{przone->get_mmio()} {}

bool nios::write(uint32_t nios_func, std::vector<uint32_t> args) {
  uint32_t junk;
  return write(nios_func, args, junk);
}

bool nios::write(uint32_t nios_func, std::vector<uint32_t> args,
                 uint32_t& value_out) {
  if (args.size() > 4) {
    return false;
  }

  controller::hssi_ctrl msg;
  for (size_t i = 0; i < args.size(); ++i) {
    msg.clear();
    msg.set_command(controller::hssi_cmd::sw_write);
    msg.set_address(i + 2);
    msg.set_data(args[i]);
    mmio_->write_mmio64(przone_->get_ctrl(), msg.data());
    if (!przone_->hssi_ack(100000)) {
      return false;
    }
  }

  msg.set_command(controller::hssi_cmd::sw_write);
  msg.set_address(1);
  msg.set_data(nios_func);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());
  if (!przone_->hssi_ack(100000)) {
    return false;
  }

  switch (nios_func) {
    case controller::nios_cmd::tx_eq_read:
    case controller::nios_cmd::hssi_init:
    case controller::nios_cmd::hssi_init_done:
    case controller::nios_cmd::fatal_err:
    case controller::nios_cmd::get_hssi_enable:
    case controller::nios_cmd::get_hssi_mode: {
      msg.set_command(controller::hssi_cmd::sw_read);
      msg.set_address(6);
      msg.set_data(nios_func);
      mmio_->write_mmio64(przone_->get_ctrl(), msg.data());
      if (!przone_->hssi_ack(100000)) {
        return false;
      }

      uint64_t value64;
      if (!mmio_->read_mmio64(przone_->get_stat(), value64))
      {
        return false;
      }

      value_out = static_cast<uint32_t>(value64);
    } break;
    default:
      break;
  }

  return true;
}
}
}
}
