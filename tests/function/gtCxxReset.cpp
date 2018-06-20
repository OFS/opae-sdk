#include "gtest/gtest.h"
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>

using namespace opae::fpga::types;

/**
 * @test handle_reset_01
 * Given a handle that has been opened<br>
 * When I call handle::reset<br>
 * Then no exceptions are thrown<br>
 */
TEST(LibopaecppResetCommonALL, handle_reset_01) {
  auto tokens = token::enumerate({properties::get(FPGA_ACCELERATOR)});
  ASSERT_TRUE(tokens.size() > 0);
  handle::ptr_t h = handle::open(tokens[0], FPGA_OPEN_SHARED);
  ASSERT_NE(nullptr, h.get());
  h->reset();
  // reset the pointer
  h.reset();
  ASSERT_NO_THROW(tokens.clear());
}
