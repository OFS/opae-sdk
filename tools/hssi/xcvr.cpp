#include "xcvr.h"

#include "hssi_msg.h"

namespace intel {
namespace fpga {
namespace hssi {

xcvr::xcvr(hssi_przone::ptr_t przone)
    : przone_{przone}, mmio_{przone->get_mmio()} {}

bool xcvr::write(uint32_t lane, uint32_t reg_addr, uint32_t value) {
  // 1. Provide the data to be written into reconf register to the AUX bus
  // Write value to local_din
  uint32_t reconfig_addr = reg_addr + controller::hssi_xcvr_lane_offset * lane;
  controller::hssi_ctrl msg;

  msg.clear();
  msg.set_address(controller::aux_bus::local_din);
  msg.set_data(static_cast<uint32_t>(value));
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 2. Send request to AUX bus handshake registers, to write data into RCFG
  // bus
  // Write from local_din to local_dout
  msg.clear();
  msg.set_address(controller::aux_bus::local_cmd);
  msg.set_bus_command(controller::bus_cmd::local_write,
                      controller::aux_bus::local_dout);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 3. Provide ID of reconf register to AUX bus handshake register
  // Write local_dout to channel+reconfig_addr
  msg.clear();
  msg.set_address(controller::aux_bus::local_din);
  msg.set_bus_command(controller::bus_cmd::local_write, reconfig_addr);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 4. Send the request to the AUX bus handshake registers, write ID
  // information from step 3
  msg.clear();
  msg.set_address(controller::aux_bus::local_cmd);
  msg.set_bus_command(controller::bus_cmd::local_write,
                      controller::aux_bus::local_din);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  return true;
}

bool xcvr::read(uint32_t lane, uint32_t reg_addr, uint32_t& value) {
  uint32_t reconfig_addr = reg_addr + controller::hssi_xcvr_lane_offset * lane;
  // HSSI_CTRL cmd(63-48), addr(47-32), data(31-0)

  // 1. Provide ID information of local register to AUX bus handshake
  // registers
  // Write read_cmd, channel, reconfig_addr to aux_bus::local_din
  // read_cmd, channel, reconfig_addr -> local_din
  controller::hssi_ctrl msg;

  msg.clear();
  msg.set_address(controller::aux_bus::local_din);
  msg.set_bus_command(controller::bus_cmd::rcfg_read, reconfig_addr);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 2. Send request to AUX bus handshake, write ID information from step 1
  // into local bus
  // Write write_cmd, recfg_cmd_addr to aux_bus::local_cmd
  // write_cmd from local_din (read_cmd, channel, reconf_addr) ->
  // recfg_cmd_addr
  msg.clear();
  msg.set_address(controller::aux_bus::local_cmd);
  msg.set_bus_command(controller::bus_cmd::local_write,
                      controller::local_bus::recfg_cmd_addr);
  msg.set_command(controller::hssi_cmd::aux_write);
  mmio_->write_mmio64(przone_->get_ctrl(), msg.data());

  if (!przone_->hssi_ack()) {
    return false;
  }

  // 3. Read data from local bus to aux bus
  // Write recfg_cmd_rddata to aux_bus::local_cmd
  // recfg_cmd_rddata (from recfg_cmd_addr), output goes into local_dout
  msg.clear();
  msg.set_address(controller::aux_bus::local_cmd);
  msg.set_bus_command(controller::bus_cmd::local_read,
                      controller::local_bus::recfg_cmd_rddata);
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
  if (!mmio_->read_mmio64(przone_->get_stat(), hssi_value)) {
    return false;
  }

  value = static_cast<uint32_t>(hssi_value & 0x00000000FFFFFFFF);
  return true;
}
}
}
}
