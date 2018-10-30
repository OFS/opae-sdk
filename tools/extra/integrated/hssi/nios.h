#pragma once

#include "fme.h"
#include "hssi_przone.h"
#include <opae/cxx/core/handle.h>

#include <vector>

namespace intel {
namespace fpga {
namespace hssi {

class nios {
 public:
  typedef std::shared_ptr<nios> ptr_t;

  nios(hssi_przone::ptr_t przone);
  bool write(uint32_t nios_func, std::vector<uint32_t> args);
  bool write(uint32_t nios_func, std::vector<uint32_t> args,
             uint32_t& value_out);

 private:
  hssi_przone::ptr_t przone_;
  opae::fpga::types::handle::ptr_t handle_;
};

}  // end of namespace hssi
}  // end of namespace fpga
}  // end of namespace intel
