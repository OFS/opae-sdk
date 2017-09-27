#pragma once

#include "hssi_przone.h"
#include "mmio.h"

namespace intel {
namespace fpga {
namespace hssi {

class pll {
 public:
  typedef std::shared_ptr<pll> ptr_t;

  pll(hssi_przone::ptr_t przone);
  bool read(uint32_t info_sel, uint32_t& value);

 private:
  hssi_przone::ptr_t przone_;
  mmio::ptr_t mmio_;
};
}
}
}
