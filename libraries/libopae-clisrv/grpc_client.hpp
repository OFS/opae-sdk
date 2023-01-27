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

#include <grpcpp/grpcpp.h>
#include <opae/fpga.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "convert.hpp"
#include "opae.grpc.pb.h"
#include "opae.pb.h"
#include "remote.h"

class pthread_lock_guard {
 public:
  pthread_lock_guard(pthread_mutex_t *lock) : lock_(lock) {
    int res = pthread_mutex_lock(lock_);
    if (res) OPAE_ERR("pthread_mutex_lock() failed");
  }

  virtual ~pthread_lock_guard() {
    int res = pthread_mutex_unlock(lock_);
    if (res) OPAE_ERR("pthread_mutex_unlock() failed");
  }

 private:
  pthread_mutex_t *lock_;
};

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using opaegrpc::OPAEService;

class OPAEClient final {
 public:
  typedef std::map<fpga_remote_id, _remote_token *> token_map_t;

  OPAEClient(std::shared_ptr<Channel> channel)
      : stub_(OPAEService::NewStub(channel)) {}

  fpga_result fpgaEnumerate(const std::vector<fpga_properties> &filters,
                            uint32_t num_filters, uint32_t max_tokens,
                            uint32_t &num_matches,
                            std::vector<fpga_token_header> &tokens);

  fpga_result fpgaDestroyToken(const fpga_remote_id &token_id);

  fpga_result fpgaCloneToken(const fpga_remote_id &src_token_id,
                             fpga_token_header &dest_token_hdr);

  _remote_token *find_token(const fpga_remote_id &rid) const {
    token_map_t::const_iterator it = token_map_.find(rid);
    return (it == token_map_.end()) ? nullptr : it->second;
  }

  bool add_token(const fpga_remote_id &rid, _remote_token *tok) {
    std::pair<token_map_t::iterator, bool> res =
        token_map_.insert(std::make_pair(rid, tok));
    return res.second;
  }

  bool remove_token(const fpga_remote_id &rid) {
    token_map_t::iterator it = token_map_.find(rid);

    if (it == token_map_.end()) return false;

    token_map_.erase(it);

    return true;
  }

 private:
  std::unique_ptr<OPAEService::Stub> stub_;

  token_map_t token_map_;
};
