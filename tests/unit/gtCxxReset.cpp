#include "gtest/gtest.h"

#include <opae/cxx/accelerator.h>

using namespace opae::fpga::types;
using namespace opae::fpga::resource;

/**
 * @test handle_reset_01
 * Given a handle that has been opened<br>
 * When I call handle::reset<br>
 * Then no exceptions are thrown<br>
 */
TEST(CxxReset, handle_reset_01) {
  auto tokens = token::enumerate({FPGA_ACCELERATOR});
  ASSERT_TRUE(tokens.size() > 0);
  handle::ptr_t h = handle::open(tokens[0], FPGA_OPEN_SHARED);
  ASSERT_NE(nullptr, h.get());
  h->reset();
  // reset the pointer
  h.reset();
  ASSERT_NO_THROW(tokens.clear());
}

/**
 * @test accelerator_reset_01
 * Given an accelerator that has been opened<br>
 * When I call accelerator::reset<br>
 * Then no exceptions are thrown<br>
 */
TEST(CxxReset, accelerator_reset_01) {
  auto accelerators = accelerator::enumerate({});
  ASSERT_TRUE(accelerators.size() > 0);
  ASSERT_NO_THROW(accelerators[0]->open(FPGA_OPEN_SHARED));
  ASSERT_NO_THROW(accelerators[0]->reset());
  ASSERT_NO_THROW(accelerators.clear());
}
