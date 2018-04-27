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

#include <opae/access.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>

#include "common_sys.h"
#include "common_utils.h"
#include "gtest/gtest.h"


#define FLAGS 0

using namespace common_utils;
using namespace std;

class StressLibopaecTPFCommonHW : public BaseFixture, public ::testing::Test {};

/*
 * @test       01
 *
 * @brief      Load to be used for the AP6 portion of the maunal RAS
 *             test.
 */

TEST_F(StressLibopaecTPFCommonHW, 01) {
    auto functor = [=]() -> void {

      sayHello(tokens[index]);

    };

  while (true) {
    // pass test code to enumerator
    TestAllFPGA(FPGA_ACCELERATOR,  // object type
                false,             // reconfig default NLB0
                functor);          // test code
  }
}
