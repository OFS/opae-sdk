// Copyright(c) 2019-2022, Intel Corporation
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

extern "C" {
#include "fpgad/api/logging.h"
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

bool evt_queue_is_full(evt_dispatch_queue *q);
}

#include <thread>
#include <chrono>

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class fpgad_evt_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    opae_base_p<>::TearDown();
  }

};

/**
 * @test       q_full0
 * @brief      Test: evt_queue_is_full
 * @details    When the fn is called on a full queue,<br>
 *             it returns true.<br>
 */
TEST_P(fpgad_evt_c_p, q_full0) {
  evt_dispatch_queue q;

  // case 0
  q.head = 0;
  q.tail = EVENT_DISPATCH_QUEUE_DEPTH - 1;
  EXPECT_TRUE(evt_queue_is_full(&q));

  // case 1
  q.head = 1;
  q.tail = 0;
  EXPECT_TRUE(evt_queue_is_full(&q));
}

static void test_evt_response(fpgad_monitored_device *dev,
                              void *context)
{
  UNUSED_PARAM(dev);
  UNUSED_PARAM(context);
}

/**
 * @test       q_full1
 * @brief      Test: evt_queue_response
 * @details    When normal_queue is full,<br>
 *             the function logs an error and returns false.<br>
 */
TEST_P(fpgad_evt_c_p, q_full1) {
  fpgad_monitored_device d;

  normal_queue.head = 1;
  normal_queue.tail = 0;
  EXPECT_FALSE(evt_queue_response(test_evt_response,
                                  &d,
                                  NULL));
  normal_queue.head = 0;
  normal_queue.tail = 0;
}

static void stop_running_response(fpgad_monitored_device *dev,
                                  void *context)
{
  UNUSED_PARAM(dev);
  UNUSED_PARAM(context);
  event_dispatcher_config.global->running = false;
}

/**
 * @test       normal_dispatch
 * @brief      Test: evt_queue_response, event_dispatcher_thread
 * @details    Test event_dispatcher_thread's ability to process,<br>
 *             items from normal_queue.<br>
 */
TEST_P(fpgad_evt_c_p, normal_dispatch) {
  event_dispatcher_config.global->running = true;

  std::thread dispatch_thr = std::thread(event_dispatcher_thread,
                                         &event_dispatcher_config);
  while (!evt_dispatcher_is_ready())
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

  fpgad_monitored_device d;
  d.object_id = platform_.devices[0].fme_object_id;
  EXPECT_TRUE(evt_queue_response(stop_running_response,
                                 &d,
                                 NULL));
  dispatch_thr.join();
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_evt_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_evt_c, fpgad_evt_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
