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

extern client_event_registry *event_registry_list;

struct client_event_registry *register_event(int conn_socket, int fd,
	                                     fpga_event_type e, const char *device);

void unregister_event(int conn_socket, fpga_event_type e, const char *device);

// FPGA_EVENT_INTERRUPT
// FPGA_EVENT_ERROR
// FPGA_EVENT_POWER_THERMAL


void unregister_all_events(void);
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_srv_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_srv_c_p()
      : tmpfpgad_log_("tmpfpgad-XXXXXX.log") {}

  virtual void SetUp() override {
    tmpfpgad_log_ = mkstemp(const_cast<char *>(tmpfpgad_log_.c_str()));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    open_log(tmpfpgad_log_.c_str());

  }

  virtual void TearDown() override {
    unregister_all_events();

    close_log();
    system_->finalize();
  }

  std::string tmpfpgad_log_;
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
  EXPECT_EQ(nullptr, register_event(0, 0, FPGA_EVENT_INTERRUPT, "dev"));
}

/**
 * @test       unregister01
 * @brief      Test: unregister_event
 * @details    When event_registry_list is empty,<br>
 *             unregister_event returns immediately.<br>
 */
TEST_P(fpgad_srv_c_p, unregister01) {
  ASSERT_EQ(nullptr, event_registry_list);
  unregister_event(0, FPGA_EVENT_INTERRUPT, "dev");
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
  int conn_sockets[3] = { 1, 2, 3 };
  const char *devs[3] = { "deva", "devb", "devc" }; 

  ASSERT_EQ(nullptr, event_registry_list);
  EXPECT_NE(nullptr, register_event(conn_sockets[2], 0, FPGA_EVENT_INTERRUPT, devs[2]));
  EXPECT_NE(nullptr, register_event(conn_sockets[1], 0, FPGA_EVENT_INTERRUPT, devs[1]));
  EXPECT_NE(nullptr, register_event(conn_sockets[0], 0, FPGA_EVENT_INTERRUPT, devs[0]));

  client_event_registry *r = event_registry_list;
  ASSERT_NE(nullptr, r);
  // New entries are placed onto the front of the list, so we should have 1, 2, 3..
  EXPECT_EQ(1, r->conn_socket);
  r = r->next;
  EXPECT_EQ(2, r->conn_socket);
  r = r->next;
  EXPECT_EQ(3, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);

  // Remove the middle entry from the list.
  unregister_event(conn_sockets[1], FPGA_EVENT_INTERRUPT, devs[1]);
  r = event_registry_list;
  ASSERT_NE(nullptr, r);
  EXPECT_EQ(1, r->conn_socket);
  r = r->next;
  EXPECT_EQ(3, r->conn_socket);
  EXPECT_EQ(nullptr, r->next);
 


}



INSTANTIATE_TEST_CASE_P(fpgad_srv_c, fpgad_srv_c_p,
                        ::testing::ValuesIn(test_platform::keys()));
