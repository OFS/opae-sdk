#pragma once
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;

class lpbk_test : public test_command
{
public:
  lpbk_test(){}
  virtual ~lpbk_test(){}
  virtual const char *name() const
  {
    return "lpbk";
  }

  virtual const char *description() const
  {
    return "run simple loopback test";
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)app;
    auto done = afu->register_interrupt();
    auto source = afu->allocate(64);
    afu->fill(source);
    auto destination = afu->allocate(64);
    afu->write64(dummy_afu::MEM_TEST_SRC_ADDR, source->io_address());
    afu->write64(dummy_afu::MEM_TEST_DST_ADDR, destination->io_address());
    afu->write64(dummy_afu::MEM_TEST_CTRL, 0x0);
    afu->write64(dummy_afu::MEM_TEST_CTRL, 0b1);
    afu->interrupt_wait(done, 1000);
    afu->compare(source, destination);
    return 0;
  }

private:

};