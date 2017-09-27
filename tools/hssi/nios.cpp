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
    case controller::nios_cmd::get_hssi_mode:
      msg.set_command(controller::hssi_cmd::sw_read);
      msg.set_address(6);
      msg.set_data(nios_func);
      mmio_->write_mmio64(przone_->get_ctrl(), msg.data());
      if (!przone_->hssi_ack(100000)) {
        return false;
      }
      uint64_t value64;

      if (!mmio_->read_mmio64(przone_->get_stat(), value64)) {
        return false;
      }
      value_out = static_cast<uint32_t>(value64);
      break;
    default:
      break;
  }

  return true;
}
}
}
}
