// Copyright(c) 2017, Intel Corporation
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
#include "types_int.h"

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
TEST(LibopaecCppEnumCommonMOCKHW, NoFilters01) {
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
TEST(LibopaecCppEnumCommonMOCKHW, BusFilter02) {
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
