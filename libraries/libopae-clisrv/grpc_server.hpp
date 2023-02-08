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

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <opae/fpga.h>

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "convert.hpp"
#include "opae.grpc.pb.h"
#include "opae.pb.h"

using grpc::ServerContext;
using grpc::Status;
using opaegrpc::CloneTokenReply;
using opaegrpc::CloneTokenRequest;
using opaegrpc::CloseReply;
using opaegrpc::CloseRequest;
using opaegrpc::DestroyTokenReply;
using opaegrpc::DestroyTokenRequest;
using opaegrpc::EnumerateReply;
using opaegrpc::EnumerateRequest;
using opaegrpc::GetPropertiesFromHandleReply;
using opaegrpc::GetPropertiesFromHandleRequest;
using opaegrpc::GetPropertiesReply;
using opaegrpc::GetPropertiesRequest;
using opaegrpc::MapMMIOReply;
using opaegrpc::MapMMIORequest;
using opaegrpc::OPAEService;
using opaegrpc::OpenReply;
using opaegrpc::OpenRequest;
using opaegrpc::ReadMMIO32Reply;
using opaegrpc::ReadMMIO32Request;
using opaegrpc::ReadMMIO64Reply;
using opaegrpc::ReadMMIO64Request;
using opaegrpc::ResetReply;
using opaegrpc::ResetRequest;
using opaegrpc::UnmapMMIOReply;
using opaegrpc::UnmapMMIORequest;
using opaegrpc::UpdatePropertiesReply;
using opaegrpc::UpdatePropertiesRequest;
using opaegrpc::WriteMMIO32Reply;
using opaegrpc::WriteMMIO32Request;
using opaegrpc::WriteMMIO512Reply;
using opaegrpc::WriteMMIO512Request;
using opaegrpc::WriteMMIO64Reply;
using opaegrpc::WriteMMIO64Request;

class OPAEServiceImpl final : public OPAEService::Service {
 public:
  typedef std::map<fpga_remote_id, fpga_token> token_map_t;
  typedef std::map<fpga_remote_id, fpga_handle> handle_map_t;
  typedef std::map<fpga_remote_id, uint64_t *> mmio_map_t;

  Status fpgaEnumerate(ServerContext *context, const EnumerateRequest *request,
                       EnumerateReply *reply) override;
  Status fpgaDestroyToken(ServerContext *context,
                          const DestroyTokenRequest *request,
                          DestroyTokenReply *reply) override;
  Status fpgaCloneToken(ServerContext *context,
                        const CloneTokenRequest *request,
                        CloneTokenReply *reply) override;

  Status fpgaGetProperties(ServerContext *context,
                           const GetPropertiesRequest *request,
                           GetPropertiesReply *reply) override;

  Status fpgaUpdateProperties(ServerContext *context,
                              const UpdatePropertiesRequest *request,
                              UpdatePropertiesReply *reply) override;

  Status fpgaOpen(ServerContext *context, const OpenRequest *request,
                  OpenReply *reply) override;

  Status fpgaClose(ServerContext *context, const CloseRequest *request,
                   CloseReply *reply) override;

  Status fpgaReset(ServerContext *context, const ResetRequest *request,
                   ResetReply *reply) override;

  Status fpgaGetPropertiesFromHandle(
      ServerContext *context, const GetPropertiesFromHandleRequest *request,
      GetPropertiesFromHandleReply *reply) override;

  Status fpgaMapMMIO(ServerContext *context, const MapMMIORequest *request,
                     MapMMIOReply *reply) override;

  Status fpgaUnmapMMIO(ServerContext *context, const UnmapMMIORequest *request,
                       UnmapMMIOReply *reply) override;

  Status fpgaReadMMIO32(ServerContext *context,
                        const ReadMMIO32Request *request,
                        ReadMMIO32Reply *reply) override;

  Status fpgaWriteMMIO32(ServerContext *context,
                         const WriteMMIO32Request *request,
                         WriteMMIO32Reply *reply) override;

  Status fpgaReadMMIO64(ServerContext *context,
                        const ReadMMIO64Request *request,
                        ReadMMIO64Reply *reply) override;

  Status fpgaWriteMMIO64(ServerContext *context,
                         const WriteMMIO64Request *request,
                         WriteMMIO64Reply *reply) override;

  Status fpgaWriteMMIO512(ServerContext *context,
                          const WriteMMIO512Request *request,
                          WriteMMIO512Reply *reply) override;

  fpga_token find_token(const fpga_remote_id &rid) const {
    token_map_t::const_iterator it = token_map_.find(rid);
    return (it == token_map_.end()) ? nullptr : it->second;
  }

  bool add_token(const fpga_remote_id &rid, fpga_token token) {
    std::pair<token_map_t::iterator, bool> res =
        token_map_.insert(std::make_pair(rid, token));
    return res.second;
  }

  bool remove_token(const fpga_remote_id &rid) {
    token_map_t::iterator it = token_map_.find(rid);
    if (it == token_map_.end()) return false;
    token_map_.erase(it);
    return true;
  }

  fpga_handle find_handle(const fpga_remote_id &rid) const {
    handle_map_t::const_iterator it = handle_map_.find(rid);
    return (it == handle_map_.end()) ? nullptr : it->second;
  }

  bool add_handle(const fpga_remote_id &rid, fpga_handle handle) {
    std::pair<handle_map_t::iterator, bool> res =
        handle_map_.insert(std::make_pair(rid, handle));
    return res.second;
  }

  bool remove_handle(const fpga_remote_id &rid) {
    handle_map_t::iterator it = handle_map_.find(rid);
    if (it == handle_map_.end()) return false;
    handle_map_.erase(it);
    return true;
  }

  uint64_t *find_mmio(const fpga_remote_id &rid) const {
    mmio_map_t::const_iterator it = mmio_map_.find(rid);
    return (it == mmio_map_.end()) ? nullptr : it->second;
  }

  bool add_mmio(const fpga_remote_id &rid, uint64_t *mmio_ptr) {
    std::pair<mmio_map_t::iterator, bool> res =
        mmio_map_.insert(std::make_pair(rid, mmio_ptr));
    return res.second;
  }

  bool remove_mmio(const fpga_remote_id &rid) {
    mmio_map_t::iterator it = mmio_map_.find(rid);
    if (it == mmio_map_.end()) return false;
    mmio_map_.erase(it);
    return true;
  }

 private:
  token_map_t token_map_;
  handle_map_t handle_map_;
  mmio_map_t mmio_map_;
};
