#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "opaec++/token.h"
#include "opaec++/handle.h"
#include "opaec++/properties.h"

using namespace opae::fpga::types;


/**
 * @test cxx_enum_01
 * Given an environment with at least one accelerator<br>
 * When I call accelerator::enumerate with a filter of only FPGA_ACCELERATOR<br>
 * And I call handle::open with a token<br>
 * Then I get a non-null handle<br>
 * And no exceptions are thrown
 */
TEST(CxxOpen, open_01){
    auto tokens = token::enumerate({ FPGA_ACCELERATOR });
    EXPECT_TRUE(tokens.size() > 0);
    handle::ptr_t h = handle::open(tokens[0], FPGA_OPEN_SHARED);
    ASSERT_NE(nullptr, h.get());
    h.reset();
    ASSERT_NO_THROW(tokens.clear());
}
