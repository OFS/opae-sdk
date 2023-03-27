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
#include <memory>
#include <string>

#include "convert.hpp"
#include "opae.grpc.pb.h"
#include "opae.pb.h"

using grpc::ServerContext;
using grpc::Status;
using opaegrpc::ClearAllErrorsReply;
using opaegrpc::ClearAllErrorsRequest;
using opaegrpc::ClearErrorReply;
using opaegrpc::ClearErrorRequest;
using opaegrpc::CloneTokenReply;
using opaegrpc::CloneTokenRequest;
using opaegrpc::CloseReply;
using opaegrpc::CloseRequest;
using opaegrpc::DestroyObjectReply;
using opaegrpc::DestroyObjectRequest;
using opaegrpc::DestroyTokenReply;
using opaegrpc::DestroyTokenRequest;
using opaegrpc::EnumerateReply;
using opaegrpc::EnumerateRequest;
using opaegrpc::GetErrorInfoReply;
using opaegrpc::GetErrorInfoRequest;
using opaegrpc::GetIOAddressReply;
using opaegrpc::GetIOAddressRequest;
using opaegrpc::GetMetricsByIndexReply;
using opaegrpc::GetMetricsByIndexRequest;
using opaegrpc::GetMetricsByNameReply;
using opaegrpc::GetMetricsByNameRequest;
using opaegrpc::GetMetricsInfoReply;
using opaegrpc::GetMetricsInfoRequest;
using opaegrpc::GetMetricsThresholdInfoReply;
using opaegrpc::GetMetricsThresholdInfoRequest;
using opaegrpc::GetNumMetricsReply;
using opaegrpc::GetNumMetricsRequest;
using opaegrpc::GetPropertiesFromHandleReply;
using opaegrpc::GetPropertiesFromHandleRequest;
using opaegrpc::GetPropertiesReply;
using opaegrpc::GetPropertiesRequest;
using opaegrpc::GetUserClockReply;
using opaegrpc::GetUserClockRequest;
using opaegrpc::HandleGetObjectReply;
using opaegrpc::HandleGetObjectRequest;
using opaegrpc::MapMMIOReply;
using opaegrpc::MapMMIORequest;
using opaegrpc::ObjectGetNameReply;
using opaegrpc::ObjectGetNameRequest;
using opaegrpc::ObjectGetObjectAtReply;
using opaegrpc::ObjectGetObjectAtRequest;
using opaegrpc::ObjectGetObjectReply;
using opaegrpc::ObjectGetObjectRequest;
using opaegrpc::ObjectGetSizeReply;
using opaegrpc::ObjectGetSizeRequest;
using opaegrpc::ObjectGetTypeReply;
using opaegrpc::ObjectGetTypeRequest;
using opaegrpc::ObjectRead64Reply;
using opaegrpc::ObjectRead64Request;
using opaegrpc::ObjectReadReply;
using opaegrpc::ObjectReadRequest;
using opaegrpc::ObjectWrite64Reply;
using opaegrpc::ObjectWrite64Request;
using opaegrpc::OPAEService;
using opaegrpc::OpenReply;
using opaegrpc::OpenRequest;
using opaegrpc::PrepareBufferReply;
using opaegrpc::PrepareBufferRequest;
using opaegrpc::ReadErrorReply;
using opaegrpc::ReadErrorRequest;
using opaegrpc::ReadMMIO32Reply;
using opaegrpc::ReadMMIO32Request;
using opaegrpc::ReadMMIO64Reply;
using opaegrpc::ReadMMIO64Request;
using opaegrpc::ReconfigureSlotByNameReply;
using opaegrpc::ReconfigureSlotByNameRequest;
using opaegrpc::ReleaseBufferReply;
using opaegrpc::ReleaseBufferRequest;
using opaegrpc::ResetReply;
using opaegrpc::ResetRequest;
using opaegrpc::SetUserClockReply;
using opaegrpc::SetUserClockRequest;
using opaegrpc::TokenGetObjectReply;
using opaegrpc::TokenGetObjectRequest;
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

struct OPAEBufferInfo {
  OPAEBufferInfo(const fpga_remote_id &handle_id, uint64_t length,
                 void *buf_addr, uint64_t wsid, int flags)
      : handle_id_(handle_id),
        length_(length),
        buf_addr_(buf_addr),
        wsid_(wsid),
        flags_(flags) {}

  fpga_remote_id handle_id_;
  uint64_t length_;
  void *buf_addr_;
  uint64_t wsid_;
  int flags_;
};

class OPAEServiceImpl final : public OPAEService::Service {
 public:
  OPAEServiceImpl(bool debug)
      : token_map_(nullptr),
        handle_map_(nullptr),
        mmio_map_(nullptr),
        binfo_map_(nullptr),
        sysobj_map_(nullptr),
        debug_(debug) {}

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

  Status fpgaPrepareBuffer(ServerContext *context,
                           const PrepareBufferRequest *request,
                           PrepareBufferReply *reply) override;

  Status fpgaReleaseBuffer(ServerContext *context,
                           const ReleaseBufferRequest *request,
                           ReleaseBufferReply *reply) override;

  Status fpgaGetIOAddress(ServerContext *context,
                          const GetIOAddressRequest *request,
                          GetIOAddressReply *reply) override;

  Status fpgaReadError(ServerContext *context, const ReadErrorRequest *request,
                       ReadErrorReply *reply) override;

  Status fpgaGetErrorInfo(ServerContext *context,
                          const GetErrorInfoRequest *request,
                          GetErrorInfoReply *reply) override;

  Status fpgaClearError(ServerContext *context,
                        const ClearErrorRequest *request,
                        ClearErrorReply *reply) override;

  Status fpgaClearAllErrors(ServerContext *context,
                            const ClearAllErrorsRequest *request,
                            ClearAllErrorsReply *reply) override;

  Status fpgaTokenGetObject(ServerContext *context,
                            const TokenGetObjectRequest *request,
                            TokenGetObjectReply *reply) override;

  Status fpgaDestroyObject(ServerContext *context,
                           const DestroyObjectRequest *request,
                           DestroyObjectReply *reply) override;

  Status fpgaObjectGetType(ServerContext *context,
                           const ObjectGetTypeRequest *request,
                           ObjectGetTypeReply *reply) override;

  Status fpgaObjectGetName(ServerContext *context,
                           const ObjectGetNameRequest *request,
                           ObjectGetNameReply *reply) override;

  Status fpgaObjectGetSize(ServerContext *context,
                           const ObjectGetSizeRequest *request,
                           ObjectGetSizeReply *reply) override;

  Status fpgaObjectRead(ServerContext *context,
                        const ObjectReadRequest *request,
                        ObjectReadReply *reply) override;

  Status fpgaObjectRead64(ServerContext *context,
                          const ObjectRead64Request *request,
                          ObjectRead64Reply *reply) override;

  Status fpgaObjectWrite64(ServerContext *context,
                           const ObjectWrite64Request *request,
                           ObjectWrite64Reply *reply) override;

  Status fpgaHandleGetObject(ServerContext *context,
                             const HandleGetObjectRequest *request,
                             HandleGetObjectReply *reply) override;

  Status fpgaObjectGetObject(ServerContext *context,
                             const ObjectGetObjectRequest *request,
                             ObjectGetObjectReply *reply) override;

  Status fpgaObjectGetObjectAt(ServerContext *context,
                               const ObjectGetObjectAtRequest *request,
                               ObjectGetObjectAtReply *reply) override;

  Status fpgaSetUserClock(ServerContext *context,
                          const SetUserClockRequest *request,
                          SetUserClockReply *reply) override;

  Status fpgaGetUserClock(ServerContext *context,
                          const GetUserClockRequest *request,
                          GetUserClockReply *reply) override;

  Status fpgaGetNumMetrics(ServerContext *context,
                           const GetNumMetricsRequest *request,
                           GetNumMetricsReply *reply) override;

  Status fpgaGetMetricsInfo(ServerContext *context,
                            const GetMetricsInfoRequest *request,
                            GetMetricsInfoReply *reply) override;

  Status fpgaGetMetricsByIndex(ServerContext *context,
                               const GetMetricsByIndexRequest *request,
                               GetMetricsByIndexReply *reply) override;

  Status fpgaGetMetricsByName(ServerContext *context,
                              const GetMetricsByNameRequest *request,
                              GetMetricsByNameReply *reply) override;

  Status fpgaGetMetricsThresholdInfo(
      ServerContext *context, const GetMetricsThresholdInfoRequest *request,
      GetMetricsThresholdInfoReply *reply) override;

  Status fpgaReconfigureSlotByName(ServerContext *context,
                                   const ReconfigureSlotByNameRequest *request,
                                   ReconfigureSlotByNameReply *reply) override;

 public:
  typedef opae_map_helper<fpga_remote_id, fpga_token> token_map_t;

  token_map_t token_map_;

 private:
  typedef opae_map_helper<fpga_remote_id, fpga_handle> handle_map_t;
  typedef opae_map_helper<fpga_remote_id, uint64_t *> mmio_map_t;
  typedef opae_map_helper<fpga_remote_id, OPAEBufferInfo *> binfo_map_t;
  typedef opae_map_helper<fpga_remote_id, fpga_object> sysobj_map_t;

  handle_map_t handle_map_;
  mmio_map_t mmio_map_;
  binfo_map_t binfo_map_;
  sysobj_map_t sysobj_map_;
  bool debug_;
};
