// Copyright(c) 2022, Intel Corporation
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

class self_test_base_p : public opae_base_p<none_> {};

TEST_P(self_test_base_p, device_tokens) {
  std::cout << GetParam() << std::endl;

  size_t i = 0;
  while (true) {
    fpga_token token = get_device_token(i++);

    if (!token)
      break;

    std::cout << '\t' << get_pcie_address(token).to_string() << std::endl;
  }
}

TEST_P(self_test_base_p, accelerator_tokens) {
  std::cout << GetParam() << std::endl;

  size_t i = 0;
  while (true) {
    fpga_token parent = get_device_token(i++);

    if (!parent)
      break;

    std::cout << '\t' << get_pcie_address(parent).to_string()
                      << ' '
                      << get_token_type_str(parent)
                      << " : ";

    size_t j = 0;
    while (true) {
      fpga_token child = get_accelerator_token(parent, j++);

      if (!child)
        break;

      fpga_token p = get_parent_token(child);

      ASSERT_EQ(parent, p);

      std::cout << get_pcie_address(child).to_string()
                << ' '
                << get_token_type_str(child)
                << " (parent: "
                << get_pcie_address(p).to_string()
                << ' '
                << get_token_type_str(p)
                << ") ";
    }

    std::cout << std::endl;
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(self_test_base_p);
INSTANTIATE_TEST_SUITE_P(self_test_base, self_test_base_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1"
                                                                      })));


class self_test_base_xfpga_p : public opae_base_p<xfpga_> {};

TEST_P(self_test_base_xfpga_p, device_tokens_xfpga) {
  std::cout << GetParam() << std::endl;

  size_t i = 0;
  while (true) {
    fpga_token token = get_device_token(i++);

    if (!token)
      break;

    std::cout << '\t' << get_pcie_address(token).to_string() << std::endl;
  }
}

TEST_P(self_test_base_xfpga_p, accelerator_tokens_xfpga) {
  std::cout << GetParam() << std::endl;

  size_t i = 0;
  while (true) {
    fpga_token parent = get_device_token(i++);

    if (!parent)
      break;

    std::cout << '\t' << get_pcie_address(parent).to_string()
                      << ' '
                      << get_token_type_str(parent)
                      << " : ";

    size_t j = 0;
    while (true) {
      fpga_token child = get_accelerator_token(parent, j++);

      if (!child)
        break;

      fpga_token p = get_parent_token(child);

      ASSERT_EQ(parent, p);

      std::cout << get_pcie_address(child).to_string()
                << ' '
                << get_token_type_str(child)
                << " (parent: "
                << get_pcie_address(p).to_string()
                << ' '
                << get_token_type_str(p)
                << ") ";
    }

    std::cout << std::endl;
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(self_test_base_xfpga_p);
INSTANTIATE_TEST_SUITE_P(self_test_base_xfpga, self_test_base_xfpga_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1"
                                                                      })));


class self_test_p : public opae_p<none_> {};

TEST_P(self_test_p, device_token) {
  std::cout << GetParam() << std::endl;
  std::cout << '\t' << get_pcie_address(device_token_).to_string()
            << ' '
            << get_token_type_str(device_token_)
            << std::endl;
}

TEST_P(self_test_p, accelerator_token) {
  std::cout << GetParam() << std::endl;
  std::cout << '\t' << get_pcie_address(accel_token_).to_string()
            << ' '
            << get_token_type_str(accel_token_)
            << std::endl;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(self_test_p);
INSTANTIATE_TEST_SUITE_P(self_test, self_test_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1"
                                                                      })));
