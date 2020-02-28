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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "opae_int.h"

}

#include <opae/fpga.h>
#include "intel-fpga.h"
#include <linux/ioctl.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class hostif_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  hostif_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle accel_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       assign_to_ifc
 * @brief      Test: fpgaAssignToInterface
 * @details    fpgaAssignToInterface is currently unsupported,<br>
 *             and returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(hostif_c_p, assign_to_ifc) {
  EXPECT_EQ(fpgaAssignToInterface(accel_, tokens_[0],
		    0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       release_from_ifc
 * @brief      Test: fpgaReleaseFromInterface
 * @details    fpgaReleaseFromInterface is currently unsupported,<br>
 *             and returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(hostif_c_p, release_from_ifc) {
  EXPECT_EQ(fpgaReleaseFromInterface(accel_, tokens_[0]),
		     FPGA_NOT_SUPPORTED);
}

INSTANTIATE_TEST_CASE_P(hostif_c, hostif_c_p, 
                        ::testing::ValuesIn(test_platform::platforms({})));

class hostif_c_mock_p : public hostif_c_p{
  protected:
    hostif_c_mock_p() {};
};

/**
 * @test       assign_port
 * @brief      Test: fpgaAssignPortToInterface
 * @details    When fpgaAssignPortToInterface is called with valid params,<br>
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(hostif_c_mock_p, assign_port) {
  EXPECT_EQ(fpgaAssignPortToInterface(accel_, 0,
			  0, 0), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(hostif_c, hostif_c_mock_p, 
                        ::testing::ValuesIn(test_platform::mock_platforms({})));

