#pragma once
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>

#include "dummy_afu.h"
#include "afu.h"



int run_mmio(opae::fpga::types::handle::ptr_t h)
{
  dummy_afu::dummy_afu afu(h);
  uint64_t dummy_value = 0xc0c0cafe;
  afu.write64(dummy_afu::SCRATCHPAD, dummy_value);
  auto value = afu.read64(dummy_afu::SCRATCHPAD);
  if (value != dummy_value){
    throw std::logic_error("mmio write failed");
  }

  for (uint32_t i = 0; i < 8; ++i) {
    afu.write64(dummy_afu::MMIO_TEST_SCRATCHPAD, i, dummy_value+i);
    auto v1 = afu.read64(dummy_afu::MMIO_TEST_SCRATCHPAD, i);
    if (v1 != dummy_value+i) {
      throw std::logic_error("mmio write failed");
    }
  }
  return 0;
}