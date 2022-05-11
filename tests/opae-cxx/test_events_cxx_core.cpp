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
#include "mock/opae_fpgad_fixtures.h"

#include <unistd.h>
#include <fcntl.h>

#include <opae/cxx/core/events.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>

using namespace opae::testing;
using namespace opae::fpga::types;

class events_cxx_core : public opae_fpgad_p<> {
 protected:
  events_cxx_core() :
    handle_(nullptr)
  {}

  virtual void SetUp() override {
    opae_fpgad_p<>::SetUp();

    properties::ptr_t props = properties::get(FPGA_ACCELERATOR);

    auto device_id = platform_.devices[0].device_id;
    if (platform_.devices[0].num_vfs) {
      device_id++;
    }
    props->device_id = device_id;

    tokens_ = token::enumerate({props});
    ASSERT_TRUE(tokens_.size() > 0);

    handle_= handle::open(tokens_[0], 0);
    ASSERT_NE(nullptr, handle_.get());
  }

  virtual void TearDown() override {
    ASSERT_NO_THROW(tokens_.clear());

    if (handle_) {
      handle_->close();
      handle_.reset();
    }

    opae_fpgad_p<>::TearDown();
  }

  handle::ptr_t handle_;
  std::vector<token::ptr_t> tokens_;
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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(events_cxx_core);
INSTANTIATE_TEST_SUITE_P(events, events_cxx_core,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000"
                                                                      })));
