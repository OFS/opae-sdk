#pragma once

#include "hssi_przone.h"
#include "mmio.h"

namespace intel {
namespace fpga {
namespace hssi {

class xcvr {
 public:
  typedef std::shared_ptr<xcvr> ptr_t;

  xcvr(hssi_przone::ptr_t przone);

  bool write(uint32_t lane, uint32_t reg_addr, uint32_t value);
  bool read(uint32_t lane, uint32_t reg_addr, uint32_t& value);

 private:
  hssi_przone::ptr_t przone_;
  mmio::ptr_t mmio_;
};
}
}
}
