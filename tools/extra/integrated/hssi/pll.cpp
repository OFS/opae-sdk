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
#include "pll.h"
#include "hssi_msg.h"

namespace intel {
namespace fpga {
namespace hssi {

pll::pll(hssi_przone::ptr_t przone)
    : przone_{przone}, mmio_{przone->get_mmio()} {}

bool pll::read(uint32_t info_sel, uint32_t& value) {
  controller::hssi_ctrl msg;

  // 3. Read data from local bus to aux bus
  // Write recfg_cmd_rddata to aux_bus::local_cmd
  // recfg_cmd_rddata (from recfg_cmd_addr), output goes into local_dout
  msg.clear();
  msg.set_address(controller::aux_bus::local_cmd);
  if (info_sel == 0)
    msg.set_bus_command(controller::bus_cmd::local_read,
                        controller::local_bus::pll_rst_control);
  else
    msg.set_bus_command(controller::bus_cmd::local_read,
                        controller::local_bus::pll_locked_status);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 4. Read data from aux bus to hssi_stat
  // Read local_dout
  msg.clear();
  msg.set_address(controller::aux_bus::local_dout);
  msg.set_command(controller::hssi_cmd::aux_read);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  uint64_t hssi_value;
  if (!mmio_->read_mmio64(przone_->get_stat(), hssi_value))
  {
    return false;
  }

  value = static_cast<uint32_t>(hssi_value & 0x00000000FFFFFFFF);
  return true;
}
}
}
}
