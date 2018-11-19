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
#include "fpgad/config_int.h"
#include "fpgad/log.h"
#include "fpgad/srv.h"
#include "fpgad/errtable.h"
}

#include <chrono>
#include <thread>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include <opae/cxx/core/events.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>

#include "intel-fpga.h"

using namespace opae::testing;
using namespace opae::fpga::types;

class events_cxx_core : public ::testing::TestWithParam<std::string> {
 protected:
  events_cxx_core() : handle_(nullptr) {}

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

    ASSERT_EQ(fpgaInitialize(nullptr), FPGA_OK);

    properties::ptr_t props = properties::get(FPGA_ACCELERATOR);
    props->device_id = platform_.devices[0].device_id;

    tokens_ = token::enumerate({props});
    ASSERT_TRUE(tokens_.size() > 0);

    handle_= handle::open(tokens_[0], 0);
    ASSERT_NE(nullptr, handle_.get());

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
    strcpy(config_.pidfile, tmpfpgad_pid_);

    open_log(tmpfpgad_log_);
    fpgad_ = std::thread(server_thread, &config_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  virtual void TearDown() override {
    config_.running = false;
    fpgad_.join();
    handle_.reset();
    ASSERT_NO_THROW(tokens_.clear());
    close_log();
    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
      unlink(tmpfpgad_pid_);
    }
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t handle_;
  char tmpfpgad_log_[20];
  char tmpfpgad_pid_[20];
  struct config config_;
  test_platform platform_;
  std::thread fpgad_;
  test_system *system_;
};

/**
 * @test register_event_01
 * Given an open accelerator handle object<br>
 * When I call event::register_event() with nullptr handle<br>
 * And event type of FPGA_EVENT_ERROR
 * Then exception is thrown<br>
 * And I get a std invalid_argument<br>
 */
TEST_P(events_cxx_core, register_event_01) {
  event::ptr_t ev;
  ASSERT_THROW(ev = event::register_event(nullptr, FPGA_EVENT_ERROR), std::invalid_argument);
  ASSERT_EQ(nullptr, ev.get());
}

/**
 * @test register_event_02
 * Given an open handleerator handle object<br>
 * When I call event::register_event() with that handle<br>
 * And event type of event::type_t::error
 * Then no exception is thrown<br>
 * And I get a non-null event shared pointer<br>
 */
TEST_P(events_cxx_core, register_event_02) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(handle_, event::type_t::error));
  ASSERT_NE(nullptr, ev.get());
}

/**
 * @test register_event_03
 * Given an open handleerator handle object<br>
 * When I call event::register_event() with that handle<br>
 * And event type of FPGA_EVENT_ERROR
 * Then no exception is thrown<br>
 * And I get a non-null event shared pointer<br>
 */
TEST_P(events_cxx_core, register_event_03) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(handle_, FPGA_EVENT_ERROR));
  ASSERT_NE(nullptr, ev.get());
}

/**
 * @test get_os_object
 * Given an open handleerator handle object<br>
 * And an event object created with event::register_event()<br>
 * When I call event::os_object using the event object<br>
 * Then I get a valid file descriptor for polling on the event<br>
 */
TEST_P(events_cxx_core, get_os_object) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(handle_, FPGA_EVENT_ERROR));
  ASSERT_NE(nullptr, ev.get());
  auto fd = ev->os_object();
  auto res = fcntl(fd, F_GETFL);
  ASSERT_NE(res, -1);
}

INSTANTIATE_TEST_CASE_P(events, events_cxx_core, ::testing::ValuesIn(test_platform::keys(true)));
