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

#include "gtest/gtest.h"
#include "test_system.h"
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>

using namespace opae::testing;
using namespace opae::fpga::types;

class open_cxx_core : public ::testing::TestWithParam<std::string> {
 protected:
  open_cxx_core() : handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_TRUE(tokens_.size() > 0);
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test handle_open
 * Given an environment with at least one accelerator<br>
 * When I call token::enumerate with a filter of only FPGA_ACCELERATOR<br>
 * And I call handle::open with a token<br>
 * Then I get a non-null handle<br>
 * And no exceptions are thrown when I release the handle and tokens<br>
 */
TEST_P(open_cxx_core, handle_open) {
  handle_ = handle::open(tokens_[0], FPGA_OPEN_SHARED);
  ASSERT_NE(nullptr, handle_.get());
  handle_.reset();
  ASSERT_NO_THROW(tokens_.clear());
}

INSTANTIATE_TEST_CASE_P(open, open_cxx_core, ::testing::ValuesIn(test_platform::keys(true)));
