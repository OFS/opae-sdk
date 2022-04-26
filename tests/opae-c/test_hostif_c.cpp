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

class hostif_c_p : public opae_p<> {};

/**
 * @test       assign_to_ifc
 * @brief      Test: fpgaAssignToInterface
 * @details    fpgaAssignToInterface is currently unsupported,<br>
 *             and returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(hostif_c_p, assign_to_ifc) {
  EXPECT_EQ(fpgaAssignToInterface(accel_, accel_token_,
                                  0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       release_from_ifc
 * @brief      Test: fpgaReleaseFromInterface
 * @details    fpgaReleaseFromInterface is currently unsupported,<br>
 *             and returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(hostif_c_p, release_from_ifc) {
  EXPECT_EQ(fpgaReleaseFromInterface(accel_, accel_token_),
            FPGA_NOT_SUPPORTED);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hostif_c_p);
INSTANTIATE_TEST_SUITE_P(hostif_c, hostif_c_p, 
                         ::testing::ValuesIn(test_platform::platforms({})));

class hostif_c_mock_p : public hostif_c_p {};

/**
 * @test       assign_port
 * @brief      Test: fpgaAssignPortToInterface
 * @details    When fpgaAssignPortToInterface is called with valid params,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(hostif_c_mock_p, assign_port) {
  EXPECT_EQ(fpgaAssignPortToInterface(accel_, 0, 0, 0), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(hostif_c_mock_p);
INSTANTIATE_TEST_SUITE_P(hostif_c, hostif_c_mock_p, 
                         ::testing::ValuesIn(test_platform::mock_platforms({})));
