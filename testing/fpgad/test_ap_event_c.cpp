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

#include "fpgad/config_int.h"
#include "fpgad/log.h"
#include "fpgad/ap_event.h"

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_ap_event_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_ap_event_c_p()
   : port0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0"),
     fme0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0") {}

  virtual void SetUp() override {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    close(mkstemps(tmpfpgad_log_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(FPGA_OK, fpgaInitialize(NULL));

    open_log(tmpfpgad_log_);
    save_fLog_ = fLog;

    config_ = {
      .verbosity = 0,
      .poll_interval_usec = 100 * 1000,
      .daemon = 0,
      .directory = { 0, },
      .logfile = { 0, },
      .pidfile = { 0, },
      .filemode = 0,
      .running = true,
      .socket = "/tmp/fpga_event_socket",
      .null_gbs = {0},
      .num_null_gbs = 0,
    };
    strcpy(config_.logfile, tmpfpgad_log_);

    ap_thread_ = std::thread(apevent_thread, &config_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void write_ap1_port0(bool set)
  {
    std::string fname = port0_ + "/ap1_event";
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "%s\n", set ? "1" : "0"), 0);
    fclose(fp);
  }

  void cause_ap1_port0() { write_ap1_port0(true);  }
  void clear_ap1_port0() { write_ap1_port0(false); }

  void write_ap2_port0(bool set)
  {
    std::string fname = port0_ + "/ap2_event";
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "%s\n", set ? "1" : "0"), 0);
    fclose(fp);
  }

  void cause_ap2_port0() { write_ap2_port0(true);  }
  void clear_ap2_port0() { write_ap2_port0(false); }

  enum fpga_power_state {
    NORMAL_PWR = 0,
    AP1_STATE,
    AP2_STATE,
    AP6_STATE
  };

  void write_power_state_port0(fpga_power_state state)
  {
    std::string fname = port0_ + "/power_state";
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "%d\n", state), 0);
    fclose(fp);
  }

  virtual void TearDown() override {
    config_.running = false;
    ap_thread_.join();

    fLog = save_fLog_;
    close_log();

    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
    }
  }

  std::string port0_;
  std::string fme0_;
  struct config config_;
  char tmpfpgad_log_[20];
  FILE *save_fLog_;
  std::thread ap_thread_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       ap1_event_log
 * @brief      Test: apevent_thread
 * @details    Verify apevent_thread's ability to log an AP1 event.<br>
 */
TEST_P(fpgad_ap_event_c_p, ap1_event_log) {
  fflush(stdout);
  fflush(fLog);
  fLog = stdout;

  testing::internal::CaptureStdout();

  cause_ap1_port0();

  // give apevent_thread enough time to respond.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  fflush(fLog);

  std::string captured = testing::internal::GetCapturedStdout();

  clear_ap1_port0();

  EXPECT_NE(nullptr, strstr(captured.c_str(), "AP1 Triggered"));
}

/**
 * @test       ap2_event_log
 * @brief      Test: apevent_thread
 * @details    Verify apevent_thread's ability to log an AP2 event.<br>
 */
TEST_P(fpgad_ap_event_c_p, ap2_event_log) {
  fflush(stdout);
  fflush(fLog);
  fLog = stdout;

  testing::internal::CaptureStdout();

  cause_ap2_port0();

  // give apevent_thread enough time to respond.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  fflush(fLog);

  std::string captured = testing::internal::GetCapturedStdout();

  clear_ap2_port0();

  EXPECT_NE(nullptr, strstr(captured.c_str(), "AP2 Triggered"));
}

/**
 * @test       ap1_power_state_log
 * @brief      Test: apevent_thread
 * @details    Verify apevent_thread's ability to log an AP1<br>
 *             power state change.<br>
 */
TEST_P(fpgad_ap_event_c_p, ap1_power_state_log) {
  fflush(stdout);
  fflush(fLog);
  fLog = stdout;

  testing::internal::CaptureStdout();

  write_power_state_port0(AP1_STATE);

  // give apevent_thread enough time to respond.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  fflush(fLog);

  std::string captured = testing::internal::GetCapturedStdout();

  write_power_state_port0(NORMAL_PWR);

  EXPECT_NE(nullptr, strstr(captured.c_str(), "Power state changed to AP1"));
}

/**
 * @test       ap2_power_state_log
 * @brief      Test: apevent_thread
 * @details    Verify apevent_thread's ability to log an AP2<br>
 *             power state change.<br>
 */
TEST_P(fpgad_ap_event_c_p, ap2_power_state_log) {
  fflush(stdout);
  fflush(fLog);
  fLog = stdout;

  testing::internal::CaptureStdout();

  write_power_state_port0(AP2_STATE);

  // give apevent_thread enough time to respond.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  fflush(fLog);

  std::string captured = testing::internal::GetCapturedStdout();

  write_power_state_port0(NORMAL_PWR);

  EXPECT_NE(nullptr, strstr(captured.c_str(), "Power state changed to AP2"));
}

/**
 * @test       ap6_power_state_log
 * @brief      Test: apevent_thread
 * @details    Verify apevent_thread's ability to log an AP6<br>
 *             power state change.<br>
 */
TEST_P(fpgad_ap_event_c_p, ap6_power_state_log) {
  fflush(stdout);
  fflush(fLog);
  fLog = stdout;

  testing::internal::CaptureStdout();

  write_power_state_port0(AP6_STATE);

  // give apevent_thread enough time to respond.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  fflush(fLog);

  std::string captured = testing::internal::GetCapturedStdout();

  write_power_state_port0(NORMAL_PWR);

  EXPECT_NE(nullptr, strstr(captured.c_str(), "Power state changed to AP6"));
}

INSTANTIATE_TEST_CASE_P(fpgad_ap_event_c, fpgad_ap_event_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));
