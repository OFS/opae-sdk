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

#include <poll.h>
#include "mock/opae_fpgad_fixtures.h"

using namespace opae::testing;

class event_c_p : public opae_fpgad_p<> {
 protected:
  event_c_p() :
    event_handle_(nullptr)
  {}

  virtual void SetUp() override {
    opae_fpgad_p<>::SetUp();
    event_handle_ = nullptr;
    EXPECT_EQ(fpgaCreateEventHandle(&event_handle_), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyEventHandle(&event_handle_), FPGA_OK);
    opae_fpgad_p<>::TearDown();
  }

  fpga_event_handle event_handle_;
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
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd), FPGA_INVALID_PARAM);
}

/**
 * @test       get_obj_err02
 * @brief      Test: fpgaGetOSObjectFromEventHandle
 * @details    When fpgaGetOSObjectFromEventHandle is called with a wrapped<br>
 *             event handle that has a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, get_obj_err02) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR,
                              event_handle_, 0), FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
	  opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  int fd = -1;
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd), FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
			  event_handle_), FPGA_OK);
}

/**
 * @test       fpgaRegisterEvent
 * @brief      Test: fpgaRegisterEvent
 * @details    When fpgaRegisterEvent is called with a NULL<br>
 *             the fpgaRegisterEvent returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, get_obj_err03) {
  EXPECT_EQ(fpgaRegisterEvent(nullptr, FPGA_EVENT_ERROR,
                              event_handle_, 0), FPGA_INVALID_PARAM);
}

/**
 * @test       fpgaRegisterEvent
 * @brief      Test: fpgaRegisterEvent
 * @details    When fpgaRegisterEvent is called with a NULL<br>
 *             the fpgaRegisterEvent returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, get_obj_err04) {
  EXPECT_EQ(fpgaUnregisterEvent(nullptr, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_INVALID_PARAM);
}


/**
 * @test       get_obj_success
 * @brief      Test: fpgaRegisterEvent, fpgaUnregisterEvent, fpgaGetOSObjectFromEventHandle
 * @details    When fpgaGetOSObjectFromEventHandle is called after<br>
 *             registering an event type,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(event_c_p, get_obj_success) {
  int fd = -1;
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR,
                              event_handle_, 0), FPGA_OK);
  EXPECT_EQ(fpgaGetOSObjectFromEventHandle(event_handle_, &fd), FPGA_OK);

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_OK);
}

/**
 * @test       unreg_err01
 * @brief      Test: fpgaUnregisterEvent
 * @details    When fpgaUnregisterEvent is called before<br>
 *             registering an event type,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, unreg_err01) {
  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_INVALID_PARAM);
}

/**
 * @test       unreg_err02
 * @brief      Test: fpgaUnregisterEvent
 * @details    When fpgaUnregisterEvent is called on a wrapped<br>
 *             event handle object with a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, unreg_err02) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR,
                              event_handle_, 0), FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
    opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_OK);
}

/**
 * @test       destroy_err
 * @brief      Test: fpgaDestroyEventHandle
 * @details    When fpgaDestroyEventHandle is called on a wrapped<br>
 *             event handle object with a NULL opae_event_handle,<br>
 *             the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(event_c_p, destroy_err) {
  EXPECT_EQ(fpgaRegisterEvent(accel_, FPGA_EVENT_ERROR,
                              event_handle_, 0), FPGA_OK);

  opae_wrapped_event_handle *wrapped_evt_handle =
    opae_validate_wrapped_event_handle(event_handle_);
  ASSERT_NE(wrapped_evt_handle, nullptr);

  fpga_event_handle eh = wrapped_evt_handle->opae_event_handle;
  wrapped_evt_handle->opae_event_handle = nullptr;

  EXPECT_EQ(fpgaDestroyEventHandle(&event_handle_), FPGA_INVALID_PARAM);

  wrapped_evt_handle->opae_event_handle = eh;

  EXPECT_EQ(fpgaUnregisterEvent(accel_, FPGA_EVENT_ERROR,
                                event_handle_), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(event_c_p);
INSTANTIATE_TEST_SUITE_P(event_c, event_c_p, 
                         ::testing::ValuesIn(test_platform::platforms({})));

class events_handle_p : public opae_fpgad_p<> {
 protected:
  events_handle_p() :
    event_handle_(nullptr)
  {}

  virtual void SetUp() override {
    opae_fpgad_p<>::SetUp();

    event_handle_ = nullptr;
    EXPECT_EQ(fpgaCreateEventHandle(&event_handle_), FPGA_OK);

    fpgad_watch(accel_token_);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyEventHandle(&event_handle_), FPGA_OK);

    /*
    ** Don't destroy the ACCELERATOR token, because fpgad's
    ** monitor_thread() will destroy it by calling mon_destroy()
    ** when it is exiting.
    */
    state_ &= ~got_accelerator_tokens;
    opae_fpgad_p<>::TearDown();
  }

  fpga_event_handle event_handle_;
};

/**
 * @test       manual_ap6
 *
 * @brief      Given valid event handle and event type, this tests for
 *             triggering the FPGA_EVENT_POWER_THERMAL event by manually
 *             writing Ap6Event to errors.
 *
 */
TEST_P(events_handle_p, manual_ap6) {
  std::string error_csr("0x0004000000000000"); //Ap6Event
  std::string zero_csr("0x0000000000000000");
  int res;
  int fd = -1;
  struct pollfd poll_fd;
  int maxpolls = 20;

  ASSERT_EQ(FPGA_OK, fpgaRegisterEvent(accel_, FPGA_EVENT_POWER_THERMAL, event_handle_, 0));
  EXPECT_EQ(FPGA_OK, fpgaGetOSObjectFromEventHandle(event_handle_, &fd));
  EXPECT_GE(fd, 0);

  // Write to error file
  std::string tmpsysfs = system_->get_root();
  std::string sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";
  std::string path = tmpsysfs + sysfs_port + "/errors/errors";

  // Write to the mock sysfs node to generate the event.
  std::ofstream f;
  f.open(path.c_str(), std::ios::out);
  f << error_csr;
  f.close();

  poll_fd.fd = fd;
  poll_fd.events = POLLIN | POLLPRI;
  poll_fd.revents = 0;

  do
  {
    res = poll(&poll_fd, 1, 1000);
    ASSERT_GE(res, 0);
    --maxpolls;
    ASSERT_GT(maxpolls, 0);
  } while(res == 0);

  EXPECT_EQ(res, 1);
  EXPECT_NE(poll_fd.revents, 0);

  f.open(path.c_str(), std::ios::out);
  f << zero_csr;
  f.close();

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(accel_, FPGA_EVENT_POWER_THERMAL, event_handle_));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(events_handle_p);
INSTANTIATE_TEST_SUITE_P(events, events_handle_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));
