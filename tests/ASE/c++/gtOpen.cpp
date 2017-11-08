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
 * @test fx_open_01
 * Given an environment with at least one accelerator<br>
 * And accelerator::enumerate returns at least one accelerator<br>
 * When I call accelerator::open with one accelerator<br>
 * Then the result is true<br>
 */
TEST(Open, fx_open_01)
{
    auto accelerator_list = accelerator::enumerate({});
    ASSERT_GT(accelerator_list.size() , 0);
    EXPECT_TRUE(accelerator_list[0]->open());
    ASSERT_NO_THROW(accelerator_list.clear());
}
