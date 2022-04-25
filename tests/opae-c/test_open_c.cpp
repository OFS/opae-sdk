// Copyright(c) 2018-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "mock/opae_fixtures.h"

using namespace opae::testing;

class open_c_p : public opae_base_p<> {
 protected:
  open_c_p() :
    accel_token_(nullptr)
  {}

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    fpga_token device_token = opae_base_p<>::get_device_token(0);
    ASSERT_NE(device_token, nullptr);

    accel_token_ = opae_base_p<>::get_accelerator_token(device_token, 0);
    ASSERT_NE(accel_token_, nullptr);
  }

  fpga_token accel_token_;
};

TEST_P(open_c_p, mallocfails) {
  fpga_handle accel_handle = nullptr;

  // Invalidate the allocation of the wrapped handle.
  system_->invalidate_malloc(0, "opae_allocate_wrapped_handle");

  ASSERT_EQ(fpgaOpen(accel_token_, &accel_handle, 0), FPGA_NO_MEMORY);
  EXPECT_EQ(accel_handle, nullptr);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(open_c_p);
INSTANTIATE_TEST_SUITE_P(open_c, open_c_p, 
                         ::testing::ValuesIn(test_platform::mock_platforms({})));
