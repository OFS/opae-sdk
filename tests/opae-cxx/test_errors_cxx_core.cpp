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

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/errors.h>

using namespace opae::testing;
using namespace opae::fpga::types;

class errors_cxx_core : public opae_base_p<> {
 protected:
  errors_cxx_core() :
    handle_(nullptr)
  {}

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_TRUE(tokens_.size() > 0);
  }

  virtual void TearDown() override {
    tokens_.clear();

    if (handle_) {
      handle_->close();
      handle_.reset();
    }

    opae_base_p<>::TearDown();
  }

  handle::ptr_t handle_;
  std::vector<token::ptr_t> tokens_;
};

/**
 * @test get_errors
 * Given an OPAE resource token<br>
 * When I call error::get() with that token<br>
 * Then I get a non-null
 * And I am able to read information about the error
 */
TEST_P(errors_cxx_core, get_error) {
  for (auto &t : tokens_) {
    auto props = properties::get(t);
    ASSERT_NE(props, nullptr);
    if (props->interface != FPGA_IFC_DFL) // otherwise, no sysfs
	    continue;
    for (int i = 0; i < static_cast<uint32_t>(props->num_errors); ++i) {
      auto err = error::get(t, i);
      std::cout << "Error [" << err->name() << "]: " << err->read_value() << "\n";
    }
  }
}

/**
 * @test get_errors
 * Given an OPAE resource token<br>
 * When I call error::get() with nullptr <br>
 * Then I get a invalid_argument
 */
TEST_P(errors_cxx_core, throw_error) {
  ASSERT_THROW(error::get(nullptr, 0), std::invalid_argument);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(errors_cxx_core);
INSTANTIATE_TEST_SUITE_P(error, errors_cxx_core,
                         ::testing::ValuesIn(test_platform::platforms({})));
