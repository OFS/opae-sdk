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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

#include "fpgad/api/opae_events_api.h"

extern api_client_event_registry *event_registry_list;

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_opae_events_api_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_opae_events_api_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
};

/**
 * @test       events01
 * @brief      Test: opae_api_register_event
 * @details    When malloc fails, the fn returns a on-zero value.<br>
 */
TEST_P(fpgad_opae_events_api_c_p, events01) {
  system_->invalidate_malloc(0, "opae_api_register_event");
  EXPECT_NE(opae_api_register_event(0, 0, FPGA_EVENT_ERROR, 0), 0);
}

/**
 * @test       events02
 * @brief      Test: opae_api_unregister_event, opae_api_register_event
 * @details    Verifies the fn's ability to correctly remove<br>
 *             items from event_registry_list.<br>
 */
TEST_P(fpgad_opae_events_api_c_p, events02) {
  const int num = 4;
  int i;
  api_client_event_registry registries[] = {
    { 0, -1, 0, FPGA_EVENT_ERROR, 0, NULL },
    { 1, -1, 0, FPGA_EVENT_ERROR, 0, NULL },
    { 2, -1, 0, FPGA_EVENT_ERROR, 0, NULL },
    { 3, -1, 0, FPGA_EVENT_ERROR, 0, NULL },
  };
  api_client_event_registry *l;

  ASSERT_EQ(event_registry_list, (void *)NULL);
  EXPECT_NE(opae_api_unregister_event(0,
                                      FPGA_EVENT_ERROR,
                                      0), 0);

  // 3 -> 2 -> 1 -> 0
  for (i = 0 ; i < num ; ++i) {
    api_client_event_registry *r = &registries[i];
    EXPECT_EQ(opae_api_register_event(r->conn_socket,
                                      r->fd,
                                      r->event,
				      r->object_id), 0);
  }

  // Try removing a registry that isn't there.
  EXPECT_NE(opae_api_unregister_event(4,
                                      FPGA_EVENT_ERROR,
                                      0), 0);

  // remove 2
  EXPECT_EQ(opae_api_unregister_event(2,
                                      FPGA_EVENT_ERROR,
                                      0), 0);
  // 3 -> 1 -> 0
  l = event_registry_list;
  EXPECT_EQ(l->conn_socket, 3);
  EXPECT_EQ(l->next->conn_socket, 1);
  EXPECT_EQ(l->next->next->conn_socket, 0);
  EXPECT_EQ(l->next->next->next, (void *)NULL);

  // remove 3
  EXPECT_EQ(opae_api_unregister_event(3,
                                      FPGA_EVENT_ERROR,
                                      0), 0);
   // 1 -> 0
  l = event_registry_list;
  EXPECT_EQ(l->conn_socket, 1);
  EXPECT_EQ(l->next->conn_socket, 0);
  EXPECT_EQ(l->next->next, (void *)NULL);

  // remove 0
  EXPECT_EQ(opae_api_unregister_event(0,
                                      FPGA_EVENT_ERROR,
                                      0), 0);
  // 1 
  l = event_registry_list;
  EXPECT_EQ(l->conn_socket, 1);
  EXPECT_EQ(l->next, (void *)NULL);

  // remove 1
  EXPECT_EQ(opae_api_unregister_event(1,
                                      FPGA_EVENT_ERROR,
                                      0), 0);
  EXPECT_EQ(event_registry_list, (void *)NULL);
}

/**
 * @test       events03
 * @brief      Test: opae_api_send_EVENT_ERROR, check_and_send_EVENT_ERROR
 * @details    Verifies the fn's ability to correctly signal<br>
 *             an FPGA_EVENT_ERROR.<br>
 */
TEST_P(fpgad_opae_events_api_c_p, events03) {
  fpgad_monitored_device d;
  memset_s(&d, sizeof(d), 0);
  d.object_id = 43;

  ASSERT_EQ(event_registry_list, (void *)NULL);

  ASSERT_EQ(opae_api_register_event(0,
                                    -1,
                                    FPGA_EVENT_ERROR,
				    43), 0);

  opae_api_send_EVENT_ERROR(&d);

  EXPECT_EQ(event_registry_list->data, 2);

  EXPECT_EQ(opae_api_unregister_event(0,
                                      FPGA_EVENT_ERROR,
                                      43), 0);
  EXPECT_EQ(event_registry_list, (void *)NULL);
}

INSTANTIATE_TEST_CASE_P(fpgad_c, fpgad_opae_events_api_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p"/*,"skx-p-dfl0"*/ })));
