// Copyright(c) 2019-2022, Intel Corporation
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

extern "C" {
#include "fpgad/api/logging.h"
#include "fpgad/monitored_device.h"
#include "fpgad/monitor_thread.h"

fpgad_monitored_device *
allocate_monitored_device(struct fpgad_config *config,
                          fpgad_config_data *supported,
                          fpga_token token,
                          uint64_t object_id,
                          fpga_objtype object_type,
                          opae_bitstream_info *bitstr);
}

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class fpgad_monitored_device_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    opae_base_p<>::TearDown();
  }

};

/**
 * @test       alloc_err
 * @brief      Test: allocate_monitored_device
 * @details    When calloc fails,<br>
 *             allocate_monitored_device returns NULL.<br>
 */
TEST_P(fpgad_monitored_device_c_p, alloc_err) {
  system_->invalidate_calloc(0, "allocate_monitored_device");
  EXPECT_EQ(allocate_monitored_device(nullptr, nullptr, nullptr,
                                      0, FPGA_DEVICE, nullptr), nullptr);
}

/**
 * @test       enum_err
 * @brief      Test: mon_enumerate
 * @details    When calloc fails,<br>
 *             mon_enumerate returns non-zero.<br>
 */
TEST_P(fpgad_monitored_device_c_p, enum_err) {
  system_->invalidate_calloc(0, "mon_enumerate");
  struct fpgad_config cfg;
  EXPECT_NE(mon_enumerate(&cfg), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_monitored_device_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_monitored_device_c, fpgad_monitored_device_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));

class mock_fpgad_monitored_device_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    /* no call to fpgaInitialize() */

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
};

/**
 * @test       enum_err0
 * @brief      Test: mon_enumerate
 * @details    When fpgaEnumerate returns no matches,<br>
 *             mon_enumerate returns non-zero.<br>
 */
TEST_P(mock_fpgad_monitored_device_c_p, enum_err0) {
  struct fpgad_config cfg;
  // num_matches will be 0, because we have not called fpgaInitialize.
  EXPECT_NE(mon_enumerate(&cfg), 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(mock_fpgad_monitored_device_c_p);
INSTANTIATE_TEST_SUITE_P(mock_fpgad_monitored_device_c, mock_fpgad_monitored_device_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
