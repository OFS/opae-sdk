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

class error_c_p : public opae_p<> {};

/**
 * @test       read
 * @brief      Test: fpgaReadError
 * @details    When fpgaReadError is called with valid params,<br>
 *             it retrieves the value of the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_p, read) {
  uint64_t val = 0xdeadbeefdecafbad;
  EXPECT_EQ(fpgaReadError(device_token_, 0, &val), FPGA_OK);
  EXPECT_EQ(val, 0);

  test_device device = platform_.devices[0];
  if (device.has_afu) {
    val = 0xdeadbeefdecafbad;
    EXPECT_EQ(fpgaReadError(accel_token_, 0, &val), FPGA_OK);
    EXPECT_EQ(val, 0);
  }
}

/**
 * @test       get_info
 * @brief      Test: fpgaGetErrorInfo
 * @details    When fpgaGetErrorInfo is called with valid params,<br>
 *             it retrieves the info of the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_p, get_info) {
  fpga_properties props = nullptr;
  uint32_t num_errors = 0;

  test_device device = platform_.devices[0];

  if (device.has_afu) {

    ASSERT_EQ(fpgaGetProperties(accel_token_, &props), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesGetNumErrors(props, &num_errors), FPGA_OK);

    // this is a port, which only has three error registers
    ASSERT_EQ(num_errors, device.port_num_errors);
    std::map<std::string, bool> known_errors = { { "errors",              true  },
                                                 { "first_error",         false },
                                                 { "first_malformed_req", false } };
    std::vector<fpga_error_info> info_list(num_errors);

    for (int i = 0 ; i < num_errors ; ++i) {
      fpga_error_info &info = info_list[i];
      EXPECT_EQ(fpgaGetErrorInfo(accel_token_, i, &info), FPGA_OK);
      EXPECT_EQ(info.can_clear, known_errors[info.name]) << "name: " << info.name;
    }

    EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));

  }

  ASSERT_EQ(fpgaGetProperties(device_token_, &props), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesGetNumErrors(props, &num_errors), FPGA_OK);

  ASSERT_EQ(num_errors, device.fme_num_errors);
  std::map<std::string, bool> known_errors = { { "catfatal_errors", false },
                                               { "first_error",     false },
                                               { "fme_errors",      true  },
                                               { "inject_errors",   true  },
                                               { "next_error",      false },
                                               { "nonfatal_errors", false },
                                               { "pcie0_errors",    true  },
                                               { "pcie1_errors",    true  } };
  std::vector<fpga_error_info> info_list(num_errors);

  for (int i = 0 ; i < num_errors ; ++i) {
    fpga_error_info &info = info_list[i];
    EXPECT_EQ(fpgaGetErrorInfo(device_token_, i, &info), FPGA_OK);
    EXPECT_EQ(info.can_clear, known_errors[info.name]) << "name: " << info.name;
  }

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

/**
 * @test       clear
 * @brief      Test: fpgaClearError
 * @details    When fpgaClearError is called with valid params,<br>
 *             it clears the requested error,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_p, clear) {
  fpga_error_info info;
  uint32_t e = 0;
  bool cleared = false;

  test_device device = platform_.devices[0];
 
  if (device.has_afu) {

    while (fpgaGetErrorInfo(accel_token_, e, &info) == FPGA_OK) {
      if (info.can_clear) {
        EXPECT_EQ(fpgaClearError(accel_token_, e), FPGA_OK);
        cleared = true;
        break;
      }
      ++e;
    }

    EXPECT_EQ(cleared, true);

    e = 0;
    cleared = false;
  }

  while (fpgaGetErrorInfo(device_token_, e, &info) == FPGA_OK) {
    if (info.can_clear) {
      EXPECT_EQ(fpgaClearError(device_token_, e), FPGA_OK);
      cleared = true;
      break;
    }
    ++e;
  }

  EXPECT_EQ(cleared, true);
}

/**
 * @test       clear
 * @brief      Test: fpgaClearAllErrors
 * @details    When fpgaClearAllErrors is called with valid params,<br>
 *             it clears the requested errors,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(error_c_p, clear_all) {
  test_device device = platform_.devices[0];

  if (device.has_afu) {
    EXPECT_EQ(fpgaClearAllErrors(accel_token_), FPGA_OK);
  }

  EXPECT_EQ(fpgaClearAllErrors(device_token_), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(error_c_p);
INSTANTIATE_TEST_SUITE_P(error_c, error_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000"
                                                                      })));
