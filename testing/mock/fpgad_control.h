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
#ifndef _FPGAD_CONTROL_H
#define _FPGAD_CONTROL_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include <thread>
#include <chrono>

extern "C" {

#include "opae_int.h"
#include "fpgad/api/logging.h"
#include "fpgad/event_dispatcher_thread.h"
#include "fpgad/monitor_thread.h"
#include "fpgad/events_api_thread.h"

bool events_api_is_ready(void);
bool monitor_is_ready(void);
bool mon_consider_device(struct fpgad_config *c,
                         fpga_token token);

extern fpgad_supported_device default_supported_devices_table[];

}

namespace opae {
namespace testing {

class fpgad_control {
 public:

  void fpgad_start()
  {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    strcpy(tmpfpgad_pid_, "tmpfpgad-XXXXXX.pid");
    close(mkstemps(tmpfpgad_log_, 4));
    close(mkstemps(tmpfpgad_pid_, 4));

    memset_s(&fpgad_config_, sizeof(fpgad_config_), 0);
    fpgad_config_.poll_interval_usec = 100 * 1000;
    fpgad_config_.running = true;
    fpgad_config_.api_socket = "/tmp/fpga_event_socket";
    strcpy(fpgad_config_.logfile, tmpfpgad_log_);
    strcpy(fpgad_config_.pidfile, tmpfpgad_pid_);
    fpgad_config_.supported_devices = default_supported_devices_table;

    log_open(tmpfpgad_log_);

    dispatcher_config_.global = &fpgad_config_;
    dispatcher_config_.sched_policy = SCHED_RR;
    dispatcher_config_.sched_priority = 30;

    dispatcher_thr_ = std::thread(event_dispatcher_thread,
                                  &dispatcher_config_);
    while (!evt_dispatcher_is_ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    monitor_config_.global = &fpgad_config_;
    monitor_config_.sched_policy = SCHED_RR;
    monitor_config_.sched_priority = 20;

    monitor_thr_ = std::thread(monitor_thread, &monitor_config_);
    while (!monitor_is_ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    events_config_.global = &fpgad_config_;
    events_config_.sched_policy = SCHED_RR;
    events_config_.sched_priority = 10;

    events_thr_ = std::thread(events_api_thread, &events_config_);
    while (!events_api_is_ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void fpgad_stop()
  {
    fpgad_config_.running = false;

    events_thr_.join();
    monitor_thr_.join();
    dispatcher_thr_.join();

    log_close();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
      unlink(tmpfpgad_pid_);
    }
  }

  bool fpgad_watch()
  {
    return mon_enumerate(&fpgad_config_) == 0;
  }

  bool fpgad_watch(fpga_token token)
  {
    return mon_consider_device(&fpgad_config_, token);
  }

  char tmpfpgad_log_[20];
  char tmpfpgad_pid_[20];

 private:
  struct fpgad_config fpgad_config_;
  event_dispatcher_thread_config dispatcher_config_;
  std::thread dispatcher_thr_;
  monitor_thread_config monitor_config_;
  std::thread monitor_thr_;
  events_api_thread_config events_config_;
  std::thread events_thr_;
};

}  // end of namespace testing
}  // end of namespace opae

#endif /* !_FPGAD_CONTROL_H */
