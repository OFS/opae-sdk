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

#include "event_notifier.hpp"

void EventNotifier::add(int event_fd, IEventNotify *en) {
  std::lock_guard<mutex_t> g(lock_);
  to_add_.push(std::make_pair(event_fd, en));
}

void EventNotifier::remove(int event_fd) {
  std::lock_guard<mutex_t> g(lock_);
  to_remove_.push(event_fd);
}

IEventNotify *EventNotifier::find(int event_fd) {
  std::lock_guard<mutex_t> g(lock_);

  size_t i;
  for (i = 0; i < poll_fds_.size(); ++i)
    if (poll_fds_[i].fd == event_fd) return actions_[i];

  return nullptr;
}

size_t EventNotifier::num_event_fds() {
  std::lock_guard<mutex_t> g(lock_);
  return poll_fds_.size();
}

void EventNotifier::start() {
  std::lock_guard<mutex_t> g(lock_);
  if (!running_) {
    running_ = true;
    thread_ = new std::thread(EventNotifier::run, this);
  }
}

void EventNotifier::stop() {
  lock_.lock();
  if (running_) {
    running_ = false;
    lock_.unlock();
    thread_->join();
    delete thread_;
    thread_ = nullptr;
  } else
    lock_.unlock();
}

void EventNotifier::reset() {
  stop();

  {
    std::lock_guard<mutex_t> g(lock_);

    std::vector<IEventNotify *>::iterator it;
    for (it = actions_.begin(); it != actions_.end(); ++it) delete *it;
    actions_.clear();

    poll_fds_.clear();

    while (!to_add_.empty()) {
      std::pair<int, IEventNotify *> p = to_add_.front();
      delete p.second;
      to_add_.pop();
    }

    while (!to_remove_.empty()) to_remove_.pop();
  }
}

void EventNotifier::run(EventNotifier *n) {
  int res;
  bool check_events;

  while (n->is_running()) {
    res = 0;
    if (n->poll_fds_.size() > 0)
      res = ::poll(n->poll_fds_.data(), n->poll_fds_.size(), n->poll_timeout_);

    check_events = false;
    if (!res || ((nfds_t)res > n->poll_fds_.size())) {  // timeout / weird error
      std::this_thread::yield();
    } else {
      check_events = true;
    }

    {
      std::lock_guard<mutex_t> g(n->lock_);

      if (check_events) {
        std::vector<pollfd>::const_iterator it;
        for (it = n->poll_fds_.cbegin(); it != n->poll_fds_.cend(); ++it) {
          if (it->revents) {
            n->notify(it - n->poll_fds_.cbegin());
          }
        }
      }

      while (!n->to_add_.empty()) {
        std::pair<int, IEventNotify *> item(n->to_add_.front());
        n->to_add_.pop();

        pollfd pfd{.fd = item.first, .events = POLLIN | POLLPRI, .revents = 0};

        n->poll_fds_.push_back(pfd);
        n->actions_.push_back(item.second);
      }

      while (!n->to_remove_.empty()) {
        int event_fd = n->to_remove_.front();
        n->to_remove_.pop();

        size_t i;
        for (i = 0; i < n->poll_fds_.size(); ++i)
          if (n->poll_fds_[i].fd == event_fd) break;

        if (i < n->poll_fds_.size()) {
          n->poll_fds_.erase(n->poll_fds_.cbegin() + i);
          IEventNotify *en = n->actions_[i];
          delete en;
          n->actions_.erase(n->actions_.cbegin() + i);
        }
      }
    }
  }
}

void EventNotifier::notify(size_t index) {
  IEventNotify *en = actions_[index];
  if (en) (*en)(poll_fds_[index].fd);
}
