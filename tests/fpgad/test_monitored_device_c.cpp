// Copyright(c) 2019, Intel Corporation
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

#include <signal.h>

extern "C" {

#include "fpgad/api/logging.h"
#include "fpgad/monitored_device.h"
#include "fpgad/monitor_thread.h"

fpgad_monitored_device *
allocate_monitored_device(struct fpgad_config *config,
                          fpgad_supported_device *supported,
                          fpga_token token,
                          uint64_t object_id,
                          fpga_objtype object_type,
                          opae_bitstream_info *bitstr);

}

#include <config.h>
#include <opae/fpga.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class fpgad_monitored_device_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_monitored_device_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    fpgaFinalize();
    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
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

INSTANTIATE_TEST_CASE_P(fpgad_monitored_device_c, fpgad_monitored_device_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));

class mock_fpgad_monitored_device_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  mock_fpgad_monitored_device_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

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

INSTANTIATE_TEST_CASE_P(mock_fpgad_monitored_device_c, mock_fpgad_monitored_device_c_p,
  ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","skx-p-dfl0" })));
