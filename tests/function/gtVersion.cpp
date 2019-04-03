// Copyright(c) 2017-2018, Intel Corporation
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

#include "common_test.h"
#include "gtest/gtest.h"
#include "config.h"

using namespace common_test;
using namespace std;

/**
 * @test       version_01
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECVersion using a NULL pointer as the return buffer,
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecVersionCommonALL, version_01) {
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECVersion(NULL));
}

/**
 * @test       version_02
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECVersion, the returned values match the constants
 *             defined in config_int.h (and the function returns FPGA_OK)
 */
TEST(LibopaecVersionCommonALL, version_02) {
	fpga_version version = { 0xFF, 0xFF, 0xFFFF };

	EXPECT_EQ(FPGA_OK, fpgaGetOPAECVersion(&version));
	EXPECT_EQ(INTEL_FPGA_API_VER_MAJOR, version.major);
	EXPECT_EQ(INTEL_FPGA_API_VER_MINOR, version.minor);
	EXPECT_EQ(INTEL_FPGA_API_VER_REV, version.patch);
}

/**
 * @test       version_03
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECVersionString using a NULL pointer as the return
 *             buffer, the function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecVersionCommonALL, version_03) {
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECVersionString(NULL, 42));
}

/**
 * @test       version_04
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECVersionString using a size shorter than the minimum
 *             possible (which is 6) the function returns FPGA_EXCEPTION.
 */
TEST(LibopaecVersionCommonALL, version_04) {
	char str[80];

	EXPECT_EQ(FPGA_EXCEPTION, fpgaGetOPAECVersionString(str, 5));
}

/**
 * @test       version_05
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECVersionString, the returned string represents
 *             a string comprised of the constants defined in config_int.h
 *             (and the function returns FPGA_OK)
 */
TEST(LibopaecVersionCommonALL, version_05) {
	char want[80];
	char have[80];

	snprintf(want, 80, "%d.%d.%d",
			INTEL_FPGA_API_VER_MAJOR,
			INTEL_FPGA_API_VER_MINOR,
			INTEL_FPGA_API_VER_REV
		);

	EXPECT_EQ(FPGA_OK, fpgaGetOPAECVersionString(have, 80));
	EXPECT_STREQ(want, have);
}

/**
 * @test       version_06
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECBuildString using a NULL pointer as the return
 *             buffer, the function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecVersionCommonALL, version_06) {
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECBuildString(NULL, 42));
}

/**
 * @test       version_07
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECBuildString using a size of 0
 *             the function returns FPGA_EXCEPTION.
 */
TEST(LibopaecVersionCommonALL, version_07) {
	char str[80];

	EXPECT_EQ(FPGA_EXCEPTION, fpgaGetOPAECBuildString(str, 0));
}

/**
 * @test       version_08
 *
 * @brief      When I retrieve fpga_version information using
 *             fpgaGetOPAECBuildString, the returned string equals
 *             the constant defined in config_int.h (and the
 *             function returns FPGA_OK)
 */
TEST(LibopaecVersionCommonALL, version_08) {
	char want[] = INTEL_FPGA_API_HASH;
	char have[80];

	EXPECT_EQ(FPGA_OK, fpgaGetOPAECBuildString(have, 80));
	EXPECT_STREQ(want, have);
}
