/*++

INTEL CONFIDENTIAL
Copyright 2016 - 2017 Intel Corporation

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

#ifdef __cplusplus
extern "C" {
#endif
#include "opae/fpga.h"

#ifdef __cplusplus
}
#endif

#include "accelerator.h"
#include "gtest/gtest.h"
#include "option_map.h"
#ifdef BUILD_ASE
#include "ase/api/src/types_int.h"
#else
#include "types_int.h"
#endif

using namespace intel::fpga;
using namespace intel::utils;

/**
 * @test       NoFilters01
 *
 * @brief      Given an environment with at least one accelerator<br>
 *             When I call accelerator::enumerate with no filters<br>
 *             Then I get at least one accelerator object in the return
 *             list<br> And no exceptions are thrown
 *
 */
TEST(LibopaecCppEnumerateCommonMOCK, NoFilters01) {
  auto accelerator_list = accelerator::enumerate({});
  EXPECT_TRUE(accelerator_list.size() > 0);
  ASSERT_NO_THROW(accelerator_list.clear());
}

/**
 * @test       BusFilter02
 *
 * @brief      Given an environment with at least one accelerator<br>
 *             When I call accelerator::enumerate with the bus number as
 *             the filter<br> Then I get at exactly one accelerator
 *             object in the return list<br> And no exceptions are
 *             thrown
 *
 */
TEST(LibopaecCppEnumerateCommonMOCK, BusFilter02) {
  option_map::ptr_t opts(new option_map());
  // add an option with a default
  opts->add_option<uint8_t>("bus-number", option::with_argument, "bus number",
                            0x0);
  // set the options to a value (make sure tye types match)
  *(*opts)["bus-number"] = static_cast<uint8_t>(0x5e);
  auto accelerator_list = accelerator::enumerate({opts});
  EXPECT_EQ(accelerator_list.size(), 1);
  ASSERT_NO_THROW(accelerator_list.clear());
}
