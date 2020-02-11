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

#include <signal.h>

extern "C" {

#include "fpgad/api/logging.h"
#include "fpgad/monitored_device.h"
#include "fpgad/monitor_thread.h"
#include "fpgad/event_dispatcher_thread.h"

#define EVENT_DISPATCH_QUEUE_DEPTH 512

typedef struct _evt_dispatch_queue {
  event_dispatch_queue_item q[EVENT_DISPATCH_QUEUE_DEPTH];
  unsigned head;
  unsigned tail;
  pthread_mutex_t lock;
} evt_dispatch_queue;

extern evt_dispatch_queue normal_queue;
extern evt_dispatch_queue high_priority_queue;

void mon_queue_response(fpgad_detection_status status,
                        fpgad_respond_event_t response,
                        fpgad_monitored_device *d,
                        void *response_context);

void mon_monitor(fpgad_monitored_device *d);

}

#include <config.h>
#include <opae/fpga.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class fpgad_monitor_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_monitor_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
};

static void test_evt_response(fpgad_monitored_device *dev,
                              void *context)
{
  UNUSED_PARAM(dev);
  UNUSED_PARAM(context);
}

/**
 * @test       high_q_full
 * @brief      Test: mon_queue_response
 * @details    When high_priority_queue is full,<br>
 *             calls to the function log an error and drop the request.<br>
 */
TEST_P(fpgad_monitor_c_p, high_q_full) {

  high_priority_queue.head = 1;
  high_priority_queue.tail = 0;

  fpgad_monitored_device d;
  mon_queue_response(FPGAD_STATUS_DETECTED_HIGH,
                     test_evt_response,
                     &d,
                     NULL);
  EXPECT_EQ(high_priority_queue.head, 1);
  EXPECT_EQ(high_priority_queue.tail, 0);

  high_priority_queue.head = 0;
  high_priority_queue.tail = 0;
}

/**
 * @test       normal_q_full
 * @brief      Test: mon_queue_response
 * @details    When normal_queue is full,<br>
 *             calls to the function log an error and drop the request.<br>
 */
TEST_P(fpgad_monitor_c_p, normal_q_full) {

  normal_queue.head = 1;
  normal_queue.tail = 0;

  fpgad_monitored_device d;
  mon_queue_response(FPGAD_STATUS_DETECTED,
                     test_evt_response,
                     &d,
                     NULL);
  EXPECT_EQ(normal_queue.head, 1);
  EXPECT_EQ(normal_queue.tail, 0);

  normal_queue.head = 0;
  normal_queue.tail = 0;
}

/**
 * @test       null_detections0
 * @brief      Test: mon_monitor
 * @details    When the given fpgad_monitored_device has no detections,<br>
 *             the fn returns immediately.<br>
 */
TEST_P(fpgad_monitor_c_p, null_detections0) {
  fpgad_monitored_device d;
  memset_s(&d, sizeof(d), 0);
  mon_monitor(&d);
}

static fpgad_detection_status
positive_detection(fpgad_monitored_device *dev,
                 void *context)
{
  UNUSED_PARAM(dev);
  UNUSED_PARAM(context);
  return FPGAD_STATUS_DETECTED;
}

/**
 * @test       null_response0
 * @brief      Test: mon_monitor
 * @details    When the given fpgad_monitored_device has detections,<br>
 *             but the corresponding response entry is NULL,<br>
 *             then no response is queued.<br>
 */
TEST_P(fpgad_monitor_c_p, null_response0) {
  fpgad_monitored_device d;
  memset_s(&d, sizeof(d), 0);

  fpgad_detect_event_t detections[] = {
    positive_detection,
    nullptr,
  };

  fpgad_respond_event_t responses[] = {
    nullptr,
    nullptr
  };

  d.detections = detections;
  d.responses = responses;

  normal_queue.head = 0;
  normal_queue.tail = 0;

  mon_monitor(&d);
  EXPECT_EQ(normal_queue.head, 0);
  EXPECT_EQ(normal_queue.tail, 0);

  normal_queue.head = 0;
  normal_queue.tail = 0;
}

INSTANTIATE_TEST_CASE_P(fpgad_monitor_c, fpgad_monitor_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));
