#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <opae/cxx/core/token.h>
#include "gtest/gtest.h"
using namespace opae::fpga::types;

/**
 * @test enum_01
 * Given an environment with at least one accelerator<br>
 * When I call accelerator::enumerate with no filters<br>
 * Then I get at least one accelerator object in the return list<br>
 * And no exceptions are thrown
 */
TEST(LibopaecppEnumCommonALL, enum_01) {
  auto tokens = token::enumerate({});
  EXPECT_TRUE(tokens.size() > 0);
  ASSERT_NO_THROW(tokens.clear());
}
