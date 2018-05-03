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
