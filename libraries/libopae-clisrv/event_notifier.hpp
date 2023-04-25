// Copyright(c) 2023, Intel Corporation
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
#pragma once

#include <poll.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "grpc_client.hpp"
#include "map_helper.hpp"

class IEventNotify {
 public:
  virtual void operator()(int event_fd) = 0;
  virtual const fpga_remote_id &event_id() const = 0;
  virtual ~IEventNotify() {}
};

class EventFDNotify : public IEventNotify {
 public:
  EventFDNotify(std::mutex *lock, OPAEEventsClient *events_client,
                const fpga_remote_id &event_id)
      : lock_(lock), events_client_(events_client), event_id_(event_id) {}

  virtual void operator()(int server_event_fd) override {
    // Consume the server-side event
    uint64_t event_count = 0;
    ssize_t slen = ::read(server_event_fd, &event_count, sizeof(uint64_t));

    if (slen == sizeof(uint64_t)) {
      // Send event notification
      std::lock_guard<std::mutex> g(*lock_);
      events_client_->fpgaSignalRemoteEvent(event_id_);
    }
  }

  virtual const fpga_remote_id &event_id() const override { return event_id_; }

 private:
  std::mutex *lock_;
  OPAEEventsClient *events_client_;
  fpga_remote_id event_id_;
};

class EventNotifier {
 public:
  EventNotifier() : running_(false), thread_(nullptr), poll_timeout_(100) {}

  void add(int event_fd, IEventNotify *n);
  void remove(int event_fd);
  IEventNotify *find(int event_fd);

  size_t num_event_fds();

  void start();
  void stop();
  bool is_running() const { return running_; }

  void reset();

 private:
  typedef std::mutex mutex_t;

  static void run(EventNotifier *n);
  void notify(size_t index);

  std::atomic<bool> running_;
  std::thread *thread_;
  int poll_timeout_;
  std::vector<IEventNotify *> actions_;
  std::vector<pollfd> poll_fds_;
  mutex_t lock_;
  std::queue<std::pair<int, IEventNotify *>> to_add_;
  std::queue<int> to_remove_;
};
