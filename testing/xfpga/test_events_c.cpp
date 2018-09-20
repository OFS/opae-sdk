// Copyright(c) 2017-2018, Intel Corporation
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
#include <opae/fpga.h>

#ifdef __cplusplus

extern "C" {
#endif
#include "fpgad/config_int.h"
#include "fpgad/log.h"
#include "fpgad/srv.h"
#include "xfpga.h"
#ifdef __cplusplus
}
#endif
#include <stdlib.h>
#include <chrono>
#include <thread>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class events_p : public ::testing::TestWithParam<std::string> {
 protected:
  events_p()
      : tmpsysfs_("mocksys-XXXXXX"),
        tmpfpgad_log_("tmpfpgad-XXXXXX.log"),
        tmpfpgad_pid_("tmpfpgad-XXXXXX.pid"),
        handle_(nullptr) {}

  virtual void SetUp() override {
    tmpfpgad_log_ = mkstemp(const_cast<char *>(tmpfpgad_log_.c_str()));
    tmpfpgad_pid_ = mkstemp(const_cast<char *>(tmpfpgad_pid_.c_str()));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaCreateEventHandle(&eh_), FPGA_OK);
    config_ = {
        .verbosity = 0,
        .poll_interval_usec = 100 * 1000,
        .daemon = 0,
        .directory = ".",
        .logfile = tmpfpgad_log_.c_str(),
        .pidfile = tmpfpgad_pid_.c_str(),
        .filemode = 0,
        .running = true,
        .socket = "/tmp/fpga_event_socket",
        .null_gbs = {0},
        .num_null_gbs = 0,
    };
    open_log(tmpfpgad_log_.c_str());
    fpgad_ = std::thread(server_thread, &config_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  virtual void TearDown() override {
    config_.running = false;
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    EXPECT_EQ(xfpga_fpgaDestroyEventHandle(&eh_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
    fpgad_.join();
  }

  std::string tmpsysfs_;
  std::string tmpfpgad_log_;
  std::string tmpfpgad_pid_;
  struct config config_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::thread fpgad_;
  fpga_event_handle eh_;
};

TEST_P(events_p, register_event) {
  fpga_result res;
  ASSERT_EQ(res = xfpga_fpgaRegisterEvent(handle_, FPGA_EVENT_ERROR, eh_, 0), FPGA_OK)
      << "\tEVENT TYPE: ERROR, RESULT: " << fpgaErrStr(res);
  EXPECT_EQ(res = xfpga_fpgaUnregisterEvent(handle_, FPGA_EVENT_ERROR, eh_), FPGA_OK)
      << "\tRESULT: " << fpgaErrStr(res);

  ASSERT_EQ(res = xfpga_fpgaRegisterEvent(handle_, FPGA_EVENT_POWER_THERMAL, eh_, 0),
            FPGA_OK)
      << "\tEVENT TYPE: ERROR, RESULT: " << fpgaErrStr(res);
  EXPECT_EQ(res = xfpga_fpgaUnregisterEvent(handle_, FPGA_EVENT_POWER_THERMAL, eh_),
            FPGA_OK)
      << "\tRESULT: " << fpgaErrStr(res);
}

INSTANTIATE_TEST_CASE_P(events, events_p,
                        ::testing::ValuesIn(test_platform::keys()));
