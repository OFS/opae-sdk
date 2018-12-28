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
#include "fpgad/srv.h"
#include "opae_int.h"
}

#include <linux/ioctl.h>
#include <opae/fpga.h>
#include "intel-fpga.h"

#include <unistd.h>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class event_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  event_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    strcpy(tmpfpgad_pid_, "tmpfpgad-XXXXXX.pid");
    close(mkstemps(tmpfpgad_log_, 4));
    close(mkstemps(tmpfpgad_pid_, 4));
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);

    event_handle_ = nullptr;
    EXPECT_EQ(fpgaCreateEventHandle(&event_handle_), FPGA_OK);

    config_ = {
        .verbosity = 0,
        .poll_interval_usec = 100 * 1000,
        .daemon = 0,
        .directory =
            {
                0,
            },
        .logfile =
            {
                0,
            },
        .pidfile =
            {
                0,
            },
        .filemode = 0,
        .running = true,
        .socket = "/tmp/fpga_event_socket",
        .null_gbs = {0},
        .num_null_gbs = 0,
    };
    strcpy(config_.logfile, tmpfpgad_log_);
    strcpy(config_.pidfile, tmpfpgad_pid_);

    open_log(tmpfpgad_log_);
    fpgad_ = std::thread(server_thread, &config_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  virtual void TearDown() override {
    config_.running = false;
    EXPECT_EQ(fpgaDestroyEventHandle(&event_handle_), FPGA_OK);
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
    fpgad_.join();
    close_log();
    fpgaFinalize();
    system_->finalize();
    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
      unlink(tmpfpgad_pid_);
    }
  }

  std::array<fpga_token, 2> tokens_;
  char tmpfpgad_log_[20];
  char tmpfpgad_pid_[20];
  struct config config_;
  std::thread fpgad_;
  fpga_properties filter_;
  fpga_handle accel_;
  fpga_event_handle event_handle_;
  test_platform platform_;
  uint32_t num_matches_;
  test_system *system_;
};

/**
 * @test       get_obj_err01
 * @brief      Test: fpgaGetOSObjectFromEventHandle
 * @details    When fpgaGetOSObjectFromEventHandle is called prior<br>
 *             to registering an event type,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, get_obj_err01) {
  int fd = -1;
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd),
            FPGA_INVALID_PARAM);
}

/**
 * @test       get_obj_err02
 * @brief      Test: fpgaGetOSObjectFromEventHandle
 * @details    When fpgaGetOSObjectFromEventHandle is called with a wrapped<br>
 *             event handle that has a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, get_obj_err02) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_, 0),
            FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
      opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  int fd = -1;
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd),
            FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_OK);
}

/**
 * @test       get_obj_success
 * @brief      Test: fpgaRegisterEvent, fpgaUnregisterEvent,
 * fpgaGetOSObjectFromEventHandle
 * @details    When fpgaGetOSObjectFromEventHandle is called after<br>
 *             registering an event type,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(event_c_p, get_obj_success) {
  int fd = -1;
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_, 0),
            FPGA_OK);
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd), FPGA_OK);

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_OK);
}

/**
 * @test       unreg_err01
 * @brief      Test: fpgaUnregisterEvent
 * @details    When fpgaUnregisterEvent is called before<br>
 *             registering an event type,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, unreg_err01) {
  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_INVALID_PARAM);
}

/**
 * @test       unreg_err02
 * @brief      Test: fpgaUnregisterEvent
 * @details    When fpgaUnregisterEvent is called on a wrapped<br>
 *             event handle object with a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, unreg_err02) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_, 0),
            FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
      opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_OK);
}

/**
 * @test       destroy_err
 * @brief      Test: fpgaDestroyEventHandle
 * @details    When fpgaDestroyEventHandle is called on a wrapped<br>
 *             event handle object with a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, destroy_err) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_, 0),
            FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
      opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  EXPECT_EQ(fpgaDestroyEventHandle(&event_handle_), FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR, event_handle_),
            FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(event_c, event_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));
