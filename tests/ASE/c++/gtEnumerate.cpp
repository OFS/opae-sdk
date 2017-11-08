#ifdef __cplusplus
extern "C" {
#endif
#include "opae/fpga.h"

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"
#include "accelerator.h"
using namespace intel::fpga;

/**
 * @test fx_enum_01
 * Given an environment with at least one accelerator<br>
 * When I call accelerator::enumerate with no filters<br>
 * Then I get at least one accelerator object in the return list<br>
 * And no exceptions are thrown
 */
TEST(Enum, fx_enum_01)
{
    auto accelerator_list = accelerator::enumerate({});
    EXPECT_TRUE(accelerator_list.size() > 0);
    ASSERT_NO_THROW(accelerator_list.clear());
}
