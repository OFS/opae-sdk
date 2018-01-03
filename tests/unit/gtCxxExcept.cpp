#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "opae/cxx/except.h"

using namespace opae::fpga::types;

/**
 * @test except_01
 * Given a src_location object<br>
 * When the object is constructed with OPAECXX_HERE<br>
 * Then it represents the current location in the source file.<br>
 */
TEST(CxxExcept, except_01) {
  src_location loc(OPAECXX_HERE);

  EXPECT_STREQ("gtCxxExcept.cpp", loc.file());
  EXPECT_STREQ("TestBody", loc.fn());
  EXPECT_EQ(21, loc.line());
}

/**
 * @test except_02
 * Given an except object<br>
 * When the object is constructed with a src_location only<br>
 * Then then the fpga_result value is FPGA_EXCEPTION.<br>
 */
TEST(CxxExcept, except_02) {
  except e(OPAECXX_HERE);

  EXPECT_EQ(FPGA_EXCEPTION, e);
}

/**
 * @test except_03
 * Given an except object<br>
 * When the object is constructed with an fpga_result and src_location<br>
 * Then then the fpga_result value matches the value passed to the
 * constructor<br> And the string returned by what() represents the fpga_result
 * and src_location.<br>
 */
TEST(CxxExcept, except_03) {
  except e(FPGA_INVALID_PARAM, OPAECXX_HERE);

  EXPECT_EQ(FPGA_INVALID_PARAM, e);

  EXPECT_STREQ("gtCxxExcept.cpp:TestBody():49:invalid parameter", e.what());
}
