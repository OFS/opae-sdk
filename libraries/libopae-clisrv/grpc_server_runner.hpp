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

#include <atomic>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "grpc_server.hpp"

template <typename I>
class gRPCServerRunner {
 public:
  gRPCServerRunner(const std::string &listen_ip, uint16_t listen_port,
                   bool debug)
      : listen_ip_(listen_ip),
        listen_port_(listen_port),
        debug_(debug),
        thread_(nullptr),
        running_(false) {}

  void start() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!thread_) {
      running_ = false;
      thread_ = new std::thread(gRPCServerRunner::run, this);
    }
  }

  void stop() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (thread_) server_->Shutdown();
  }

  void join() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (thread_) {
      thread_->join();
      delete thread_;
      thread_ = nullptr;
      server_.reset(nullptr);
      running_ = false;
    }
  }

  bool is_running() const { return running_; }

 private:
  std::string listen_ip_;
  uint16_t listen_port_;
  bool debug_;
  std::thread *thread_;
  std::atomic<bool> running_;

  std::recursive_mutex lock_;

  std::unique_ptr<grpc::Server> server_;

  static void run(gRPCServerRunner *runner) {
    I service(runner->debug_);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    std::ostringstream oss;
    oss << runner->listen_ip_ << ':' << runner->listen_port_;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(oss.str(), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    runner->server_ = builder.BuildAndStart();
    runner->running_ = true;
    runner->server_->Wait();
  }
};
