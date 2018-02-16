/*++

INTEL CONFIDENTIAL
Copyright 2016 - 2018 Intel Corporation

The source code contained or described  herein and all documents related to
the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
Corporation or  its suppliers  and licensors.  The Material  contains trade
secrets  and  proprietary  and  confidential  information  of Intel  or its
suppliers and licensors.  The Material is protected  by worldwide copyright
and trade secret laws and treaty provisions. No part of the Material may be
used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
transmitted,  distributed, or  disclosed in  any way  without Intel's prior
express written permission.

No license under any patent, copyright,  trade secret or other intellectual
property  right  is  granted to  or conferred  upon  you by  disclosure  or
delivery of the  Materials, either  expressly, by  implication, inducement,
estoppel or otherwise. Any license  under such intellectual property rights
must be express and approved by Intel in writing.

--*/

#include <opae/fpga.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "config_int.h"

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