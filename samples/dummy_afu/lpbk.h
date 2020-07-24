#pragma once
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>

#include "dummy_afu.h"
#include "afu.h"



int run_lpbk(opae::fpga::types::handle::ptr_t h)
{
  dummy_afu::dummy_afu afu(h);
  auto done = afu.register_interrupt();
  auto source = afu.allocate(64);
  afu.fill(source);
  auto destination = afu.allocate(64);
  afu.write64(dummy_afu::MEM_TEST_SRC_ADDR, source->io_address() >> 2);
  afu.write64(dummy_afu::MEM_TEST_DST_ADDR, destination->io_address() >> 2);
  afu.write64(dummy_afu::MEM_TEST_CTRL, 0x0);
  afu.write64(dummy_afu::MEM_TEST_CTRL, 0b1);
  afu.interrupt_wait(done, 1000);
  afu.compare(source, destination);
  return 0;
}