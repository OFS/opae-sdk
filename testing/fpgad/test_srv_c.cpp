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

#include "fpgad/log.h"
#include "fpgad/srv.h"
#include "fpgad/errtable.h"

extern client_event_registry *event_registry_list;

struct client_event_registry *register_event(int conn_socket, int fd,
	                                     fpga_event_type e, uint64_t object_id);

void unregister_event(int conn_socket, fpga_event_type e, uint64_t object_id);

void unregister_all_events_for(int conn_socket);

void for_each_registered_event(void (*cb)(struct client_event_registry *,
					  uint64_t object_id,
					  const struct fpga_err *),
			       uint64_t object_id,
			       const struct fpga_err *e);

void unregister_all_events(void);

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_srv_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_srv_c_p() {}

  virtual void SetUp() override {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    close(mkstemps(tmpfpgad_log_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    open_log(tmpfpgad_log_);

  }

  virtual void TearDown() override {
    unregister_all_events();

    close_log();
    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
    }
  }

  char tmpfpgad_log_[20];
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       register01
 * @brief      Test: register_event
 * @details    When malloc fails, register_event returns NULL.<br>
 */
TEST_P(fpgad_srv_c_p, register01) {
  system_->invalidate_malloc(0, "register_event");
  EXPECT_EQ(nullptr, register_event(0, 0, FPGA_EVENT_INTERRUPT, 0));
}

/**
 * @test       unregister01
 * @brief      Test: unregister_event
 * @details    When event_registry_list is empty,<br>
 *             unregister_event returns immediately.<br>
 */
TEST_P(fpgad_srv_c_p, unregister01) {
  ASSERT_EQ(nullptr, event_registry_list);
  unregister_event(0, FPGA_EVENT_INTERRUPT, 0);
  EXPECT_EQ(nullptr, event_registry_list);
}

/**
 * @test       unregister02
 * @brief      Test: unregister_event
 * @details    When event_registry_list has more than one entry,<br>
 *             unregister_event removes the correct entry and leaves<br>
 *             list in a valid state.<br>
 */
TEST_P(fpgad_srv_c_p, unregister02) {
  int conn_sockets[3] = { 0, 1, 2 };
  uint64_t obj_ids[3] = { 0, 1, 2 };

  ASSERT_EQ(nullptr, event_registry_list);
  EXPECT_NE(nullptr, register_event(conn_sockets[2], 0, FPGA_EVENT_INTERRUPT, obj_ids[2]));
  EXPECT_NE(nullptr, register_event(conn_sockets[1], 0, FPGA_EVENT_INTERRUPT, obj_ids[1]));
  EXPECT_NE(nullptr, register_event(conn_sockets[0], 0, FPGA_EVENT_INTERRUPT, obj_ids[0]));

  client_event_registry *r = event_registry_list;
  ASSERT_NE(nullptr, r);
  // New entries are placed onto the front of the list, so we should have 0, 1, 2..
  EXPECT_EQ(0, r->conn_socket);
  r = r->next;
  EXPECT_EQ(1, r->conn_socket);
  r = r->next;
  EXPECT_EQ(2, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);

  // Remove the middle entry from the list.
  unregister_event(conn_sockets[1], FPGA_EVENT_INTERRUPT, obj_ids[1]);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(0, r->conn_socket);
  r = r->next;
  EXPECT_EQ(2, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);
 
  // Remove the last entry from the list.
  unregister_event(conn_sockets[2], FPGA_EVENT_INTERRUPT, obj_ids[2]);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(0, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);

  // Try to remove a non-existent entry.
  unregister_event(99, FPGA_EVENT_INTERRUPT, 3);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(0, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);

  EXPECT_NE(nullptr, register_event(conn_sockets[1], 0, FPGA_EVENT_INTERRUPT, obj_ids[1]));
  EXPECT_NE(nullptr, register_event(conn_sockets[2], 0, FPGA_EVENT_INTERRUPT, obj_ids[2]));

  // The list now has 2, 1, 0. Remove the end (0).
  unregister_event(conn_sockets[0], FPGA_EVENT_INTERRUPT, obj_ids[0]);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(2, r->conn_socket);
  r = r->next; 
  EXPECT_EQ(1, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);
}

/**
 * @test       unregister_all_for
 * @brief      Test: unregister_all_events_for
 * @details    unregister_all_events_for removes all event entries<br>
 *             matching the given socket from the event_registry_list.<br>
 */
TEST_P(fpgad_srv_c_p, unregister_all_for) {
  int conn_sockets[3] = { 0, 1, 2 };
  uint64_t obj_ids[3] = { 0, 1, 2 };

  ASSERT_EQ(nullptr, event_registry_list);
  EXPECT_NE(nullptr, register_event(conn_sockets[2], 0, FPGA_EVENT_INTERRUPT, obj_ids[2]));
  EXPECT_NE(nullptr, register_event(conn_sockets[1], 0, FPGA_EVENT_INTERRUPT, obj_ids[1]));
  EXPECT_NE(nullptr, register_event(conn_sockets[0], 0, FPGA_EVENT_INTERRUPT, obj_ids[0]));
  // The list contains 0, 1, 2.
 
  unregister_all_events_for(conn_sockets[2]);

  client_event_registry *r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(0, r->conn_socket);
  r = r->next;
  EXPECT_EQ(1, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);

  unregister_all_events_for(conn_sockets[0]);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(1, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);
}

static uint64_t callback_obj_ids[3] = { 98, 99, 100 };

extern "C" {

void test_event_registry_callback(struct client_event_registry *r,
				  uint64_t object_id,
				  const struct fpga_err *e)
{
  UNUSED_PARAM(object_id);
  EXPECT_STREQ(e->sysfsfile, "sysfsfile");
  callback_obj_ids[r->conn_socket] = r->object_id;
}

}

/**
 * @test       for_each
 * @brief      Test: for_each_registered_event
 * @details    for_each_registered_event invokes the given callback for<br>
 *             each client_event_registry in event_registry_list, passing
 *             the given fpga_err.<tbr>
 */
TEST_P(fpgad_srv_c_p, for_each) {
  int conn_sockets[3] = { 0, 1, 2 };
  uint64_t obj_ids[3] = { 0, 1, 2 };

  ASSERT_EQ(nullptr, event_registry_list);
  EXPECT_NE(nullptr, register_event(conn_sockets[2], 0, FPGA_EVENT_INTERRUPT, obj_ids[2]));
  EXPECT_NE(nullptr, register_event(conn_sockets[1], 0, FPGA_EVENT_INTERRUPT, obj_ids[1]));
  EXPECT_NE(nullptr, register_event(conn_sockets[0], 0, FPGA_EVENT_INTERRUPT, obj_ids[0]));
  // The list contains 0, 1, 2.
 
  struct fpga_err err = {
    .sysfsfile = "sysfsfile",
    .reg_field = "reg_field",
    .lowbit = 0,
    .highbit = 1,
    .callback = nullptr
  };

  for_each_registered_event(test_event_registry_callback, 101, &err);

  EXPECT_EQ(callback_obj_ids[0], obj_ids[0]);
  EXPECT_EQ(callback_obj_ids[1], obj_ids[1]);
  EXPECT_EQ(callback_obj_ids[2], obj_ids[2]);

  callback_obj_ids[0] = 98;
  callback_obj_ids[1] = 99;
  callback_obj_ids[2] = 100;
}

INSTANTIATE_TEST_CASE_P(fpgad_srv_c, fpgad_srv_c_p,
                        ::testing::Values(std::string("skx-p")));
