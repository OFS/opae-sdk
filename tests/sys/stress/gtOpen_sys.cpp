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

#include <opae/access.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>

#include "common_sys.h"
#include "common_utils.h"
#include "gtest/gtest.h"

using namespace common_utils;
using namespace std;

class StressLibopaecOpenFCommonHW : public BaseFixture,
                                    public ::testing::Test {};

/**
 * @test       03
 *
 * @brief      This test covers 4 different test conditions:
 *
 *             1. Use gtest process in exclusive mode to test the
 *             following conditions: a. open device in exclusive mode,
 *             step shall fail. b. open device in shared mode, step
 *             shall fail.
 *
 *             2. Use gtest process in shared mode to test the following
 *             conditions: a. open device in exclusive mode, step shall
 *             fail. b. open device in shared mode, step shall PASS.
 */
TEST_F(StressLibopaecOpenFCommonHW, 01) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;
    fpga_properties props = NULL;
    uint8_t obus = 0;

    // open exclusive in gtest process
    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(fpgaGetProperties(tokens[index], &props),
                       LINE(__LINE__));

      checkReturnCodes(fpgaPropertiesGetBus(props, &obus), LINE(__LINE__));

      ASSERT_LT(
          0,
          tryOpen(
              false,
              obus));  // open exclusive (shared == false) in external process
                       // expect fail
      ASSERT_LT(
          0,
          tryOpen(true,
                  obus));  // open shared (shared == true) in external process
                           // expect fail

      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    // open shared in gtest process
    if (checkReturnCodes(fpgaOpen(tokens[index], &h, FPGA_OPEN_SHARED),
                         LINE(__LINE__))) {
      checkReturnCodes(fpgaGetProperties(tokens[index], &props),
                       LINE(__LINE__));

      checkReturnCodes(fpgaPropertiesGetBus(props, &obus), LINE(__LINE__));

      ASSERT_LT(
          0,
          tryOpen(
              false,
              obus));  // open exclusive (shared == false) in external process
                       // expect fail
      ASSERT_EQ(
          FPGA_OK,
          tryOpen(true,
                  obus));  // open shared (shared == true) in external process
                           // expect PASS

      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }
  };

  for (int i = 0; i < 100; i++) {
    TestAllFPGA(FPGA_ACCELERATOR,  // object type
                true,              // reconfig default NLB0
                functor);          // test code
  }
}
