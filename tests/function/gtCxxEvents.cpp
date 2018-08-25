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
#include "gtest/gtest.h"

#include "opae/cxx/core/events.h"
#include "opae/cxx/core/handle.h"
#include "opae/cxx/core/token.h"

#include <fcntl.h>

using namespace opae::fpga::types;

class LibopaecppEventCommonALL_f1 : public ::testing::Test {
 protected:
  LibopaecppEventCommonALL_f1() {}

  virtual void SetUp() override {
    tokens_ = token::enumerate({properties::get(FPGA_ACCELERATOR)});
    ASSERT_GT(tokens_.size(), 0);
    accel_ = handle::open(tokens_[0], 0);
    ASSERT_NE(nullptr, accel_.get());
  }

  virtual void TearDown() override {
    accel_.reset();
    ASSERT_NO_THROW(tokens_.clear());
  }

  std::vector<token::ptr_t> tokens_;
  handle::ptr_t accel_;
};

/**
 * @test register_event_01
 * Given an open accelerator handle object<br>
 * When I call event::register_event() with that handle<br>
 * And event type of event::type_t::error
 * Then no exception is thrown<br>
 * And I get a non-null event shared pointer<br>
 */
TEST_F(LibopaecppEventCommonALL_f1, register_event_01) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(accel_, event::type_t::error));
  ASSERT_NE(nullptr, ev.get());
}

/**
 * @test register_event_02
 * Given an open accelerator handle object<br>
 * When I call event::register_event() with that handle<br>
 * And event type of FPGA_EVENT_ERROR
 * Then no exception is thrown<br>
 * And I get a non-null event shared pointer<br>
 */
TEST_F(LibopaecppEventCommonALL_f1, register_event_02) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(accel_, FPGA_EVENT_ERROR););
  ASSERT_NE(nullptr, ev.get());
}

/**
 * @test get_os_object
 * Given an open accelerator handle object<br>
 * And an event object created with event::register_event()<br>
 * When I call event::os_object using the event object<br>
 * Then I get a valid file descriptor for polling on the event<br>
 */
TEST_F(LibopaecppEventCommonALL_f1, get_os_object) {
  event::ptr_t ev;
  ASSERT_NO_THROW(ev = event::register_event(accel_, FPGA_EVENT_ERROR););
  ASSERT_NE(nullptr, ev.get());
  auto fd = ev->os_object();
  auto res = fcntl(fd, F_GETFL);
  ASSERT_NE(res, -1);
}
