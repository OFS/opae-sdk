// Copyright(c) 2019-2022, Intel Corporation
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

extern "C" {
#include "fpgad/api/device_monitoring.h"
}

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class fpgad_device_monitoring_c_p : public opae_base_p<> {};

/**
 * @test       mon01
 * @brief      Test: mon_has_error_occurred, mon_add_device_error
 * @details    mon_add_device_error associates the given void * with<br>
 *             the fpgad_monitored_device, and mon_has_error_occurred
 *             correctly reports the added errors.<br>
 */
TEST_P(fpgad_device_monitoring_c_p, mon01) {
  fpgad_monitored_device d;
  d.num_error_occurrences = 0;

  int err0, err1;

  EXPECT_FALSE(mon_has_error_occurred(&d, &err0));
  ASSERT_TRUE(mon_add_device_error(&d, &err0));
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_TRUE(mon_has_error_occurred(&d, &err0));

  EXPECT_FALSE(mon_has_error_occurred(&d, &err1));
  ASSERT_TRUE(mon_add_device_error(&d, &err1));
  EXPECT_EQ(d.num_error_occurrences, 2);
  EXPECT_TRUE(mon_has_error_occurred(&d, &err1));

  // Verify overflow checks
  d.num_error_occurrences = MAX_DEV_ERROR_OCCURRENCES;
  EXPECT_FALSE(mon_add_device_error(&d, &err1));
}

/**
 * @test       mon02
 * @brief      Test: mon_remove_device_error
 * @details    mon_remove_device_error disassociates the given void * from<br>
 *             the fpgad_monitored_device.<br>
 */
TEST_P(fpgad_device_monitoring_c_p, mon02) {
  fpgad_monitored_device d;
  d.num_error_occurrences = 0;

  int err0, err1, err2, err3;

  ASSERT_TRUE(mon_add_device_error(&d, &err0));
  ASSERT_TRUE(mon_add_device_error(&d, &err1));
  ASSERT_TRUE(mon_add_device_error(&d, &err2));
  ASSERT_TRUE(mon_add_device_error(&d, &err3));
  EXPECT_EQ(d.num_error_occurrences, 4);
  EXPECT_EQ(d.error_occurrences[0], &err0);
  EXPECT_EQ(d.error_occurrences[1], &err1);
  EXPECT_EQ(d.error_occurrences[2], &err2);
  EXPECT_EQ(d.error_occurrences[3], &err3);

  mon_remove_device_error(&d, &err1);
  EXPECT_EQ(d.num_error_occurrences, 3);
  EXPECT_EQ(d.error_occurrences[0], &err0);
  EXPECT_EQ(d.error_occurrences[1], &err2);
  EXPECT_EQ(d.error_occurrences[2], &err3);

  mon_remove_device_error(&d, &err0);
  EXPECT_EQ(d.num_error_occurrences, 2);
  EXPECT_EQ(d.error_occurrences[0], &err2);
  EXPECT_EQ(d.error_occurrences[1], &err3);

  mon_remove_device_error(&d, &err3);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &err2);

  mon_remove_device_error(&d, &err2);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_device_monitoring_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_c, fpgad_device_monitoring_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
