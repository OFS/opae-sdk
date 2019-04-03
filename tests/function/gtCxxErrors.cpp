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

#include "opae/cxx/core/errors.h"
#include "opae/cxx/core/handle.h"
#include "opae/cxx/core/token.h"


using namespace opae::fpga::types;

class LibopaecppErrorsCommonALL_f1 : public ::testing::Test {
 protected:
  LibopaecppErrorsCommonALL_f1() {}

  virtual void SetUp() override {
    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_GT(tokens_.size(), 0);
  }

  virtual void TearDown() override {
    ASSERT_NO_THROW(tokens_.clear());
  }

  std::vector<token::ptr_t> tokens_;
};

/**
 * @test get_errors
 * Given an OPAE resource token<br>
 * When I call error::get() with that token<br>
 * Then I get a non-null
 * And I am able to read information about the error
 */
TEST_F(LibopaecppErrorsCommonALL_f1, get_errors) {
  for (auto t : tokens_) {
    auto props = properties::get(t);
    for (int i = 0; i < static_cast<uint32_t>(props->num_errors); ++i) {
      auto err = error::get(t, i);
      std::cout << "Error [" << err->name() << "]: " << err->read_value() << "\n";
    }
  }
}

