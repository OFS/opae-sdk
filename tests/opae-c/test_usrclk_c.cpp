// Copyright(c) 2018-2022, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "mock/opae_fixtures.h"

using namespace opae::testing;

class usrclk_c_p : public opae_p<> {};

/**
 * @test       get
 * @brief      Test: fpgaGetUserClock
 * @details    When fpgaGetUserClock is called with valid parameters,<br>
 *             then it retrieves the user clock values,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(usrclk_c_p, get) {
  uint64_t low = 999;
  uint64_t high = 999;
  EXPECT_NE(fpgaGetUserClock(accel_, &high, &low, 0), FPGA_OK);
}

// TODO: Fix user clock test for DCP
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(usrclk_c_p);
INSTANTIATE_TEST_SUITE_P(usrclk_c, usrclk_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000"
                                                                      })));

class usrclk_c_hw_p : public usrclk_c_p {};

/**
 * @test       set
 * @brief      Test: fpgaSetUserClock
 * @details    When fpgaSetUserClock is called with valid parameters,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(usrclk_c_hw_p, set) {
  uint64_t low = 25;
  uint64_t high = 600;
  EXPECT_EQ(fpgaSetUserClock(accel_, high, low, 0), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(usrclk_c_hw_p);
INSTANTIATE_TEST_SUITE_P(usrclk_c, usrclk_c_hw_p,
                         ::testing::ValuesIn(test_platform::hw_platforms({
                                                                           "dfl-d5005",
                                                                           "dfl-n3000"
                                                                         })));
