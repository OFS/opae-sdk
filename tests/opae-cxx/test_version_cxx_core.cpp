// Copyright(c) 2018, Intel Corporation
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

#include "test_system.h"
#include "gtest/gtest.h"
#include <opae/cxx/core/version.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#ifdef __cplusplus
}
#endif

using namespace opae::testing;
using namespace opae::fpga::types;

/**
 * @test       as_struct
 *
 * @brief      When I retrieve fpga_version information using
 *             version::as_struct() then the struct values match the
 *             constants defined in config.h
 */
TEST(version_cxx_core, as_struct) {
  auto v = version::as_struct();
  EXPECT_EQ(v.major, OPAE_VERSION_MAJOR);
  EXPECT_EQ(v.minor, OPAE_VERSION_MINOR);
  EXPECT_EQ(v.patch, OPAE_VERSION_REVISION);
}

/**
 * @test       as_string
 *
 * @brief      When I retrieve version information using
 *             version::as_string() then the value returned matches
 *             the string constant defined in config.h
 */
TEST(version_cxx_core, as_string) {
  auto v = version::as_string();
  EXPECT_STREQ(v.c_str(), OPAE_VERSION);
}

/**
 * @test       build
 *
 * @brief      When I retrieve version information using
 *             version::build() then the value returned matches
 *             the string constant defined in config.h
 */
TEST(version_cxx_core, build) {
  auto v = version::build();
  EXPECT_STREQ(v.c_str(), INTEL_FPGA_API_HASH);
}
