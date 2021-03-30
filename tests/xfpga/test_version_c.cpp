// Copyright(c) 2017-2020, Intel Corporation
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

#include <opae/fpga.h>
#include "gtest/gtest.h"
#include "xfpga.h"
#include "mock/test_system.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#ifdef __cplusplus
}
#endif

using namespace opae::testing;
/**
 * @test       version_01
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECVersion using a NULL pointer as the return buffer,
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(version_c, version_01) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetOPAECVersion(NULL));
}

/**
 * @test       version_02
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECVersion, the returned values match the constants
 *             defined in config_int.h (and the function returns FPGA_OK)
 */
TEST(version_c, version_02) {
  fpga_version version = { 0xFF, 0xFF, 0xFFFF };
  
  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetOPAECVersion(&version));
  EXPECT_EQ(OPAE_VERSION_MAJOR, version.major);
  EXPECT_EQ(OPAE_VERSION_MINOR, version.minor);
  EXPECT_EQ(OPAE_VERSION_REVISION, version.patch);
}

/**
 * @test       version_03
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECVersionString using a NULL pointer as the return
 *             buffer, the function returns FPGA_INVALID_PARAM.
 */
TEST(version_c, version_03) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetOPAECVersionString(NULL, 42));
}

/**
 * @test       version_04
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECVersionString using a size shorter than the minimum
 *             possible (which is 6) the function returns FPGA_INVALID_PARAM.
 */
TEST(version_c, version_04) {
  char str[80];

  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetOPAECVersionString(str, 5));
}

/**
 * @test       version_05
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECVersionString, the returned string represents
 *             a string comprised of the constants defined in config_int.h
 *             (and the function returns FPGA_OK)
 */
TEST(version_c, version_05) {
  char want[80];
  char have[80];
  
  snprintf(want, 80, "%d.%d.%d",
  		OPAE_VERSION_MAJOR,
  		OPAE_VERSION_MINOR,
  		OPAE_VERSION_REVISION
  	);
  
  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetOPAECVersionString(have, 80));
  EXPECT_STREQ(want, have);
}

/**
 * @test       version_06
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECBuildString using a NULL pointer as the return
 *             buffer, the function returns FPGA_INVALID_PARAM.
 */
TEST(version_c, version_06) {
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetOPAECBuildString(NULL, 42));
}

/**
 * @test       version_07
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECBuildString using a size of 0
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(version_c, version_07) {
  char str[80];
 
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetOPAECBuildString(str, 0));
}

/**
 * @test       version_08
 *
 * @brief      When I retrieve fpga_version information using
 *             xfpga_fpgaGetOPAECBuildString, the returned string equals
 *             the constant defined in config_int.h (and the
 *             function returns FPGA_OK)
 */
TEST(version_c, version_08) {
  char want[] = OPAE_GIT_COMMIT_HASH;
  char have[80];
  
  EXPECT_EQ(FPGA_OK, xfpga_fpgaGetOPAECBuildString(have, 80));
  EXPECT_STREQ(want, have);
}

