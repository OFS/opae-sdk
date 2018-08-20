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

#include <opae/cxx/core/version.h>
#include "common_test.h"
#include "config_int.h"
#include "gtest/gtest.h"

using namespace common_test;
using namespace std;
using namespace opae::fpga::types;

/**
 * @test       as_struct
 *
 * @brief      When I retrieve fpga_version information using
 *             version::as_struct() then the struct values match the
 *             oonstants defined in config_int.h
 */
TEST(LibopaecppVersionCommonALL, as_struct) {
  auto v = version::as_struct();
  EXPECT_EQ(v.major, INTEL_FPGA_API_VER_MAJOR);
  EXPECT_EQ(v.minor, INTEL_FPGA_API_VER_MINOR);
  EXPECT_EQ(v.patch, INTEL_FPGA_API_VER_REV);
}

/**
 * @test       as_string
 *
 * @brief      When I retrieve version information using
 *             version::as_string() then the value returned matches
 *             the string oonstant defined in config_int.h
 */
TEST(LibopaecppVersionCommonALL, as_string) {
  auto v = version::as_string();
  EXPECT_STREQ(v.c_str(), INTEL_FPGA_API_VERSION);
}

/**
 * @test       build
 *
 * @brief      When I retrieve version information using
 *             version::build() then the value returned matches
 *             the string oonstant defined in config_int.h
 */
TEST(LibopaecppVersionCommonALL, build) {
  auto v = version::build();
  EXPECT_STREQ(v.c_str(), INTEL_FPGA_API_HASH);
}
