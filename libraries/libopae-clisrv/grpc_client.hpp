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
#include "map_helper.hpp"
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
  OPAEClient(std::shared_ptr<Channel> channel, bool debug)
      : stub_(OPAEService::NewStub(channel)),
        token_map_(nullptr),
        debug_(debug) {}

  fpga_result fpgaEnumerate(const std::vector<fpga_properties> &filters,
                            uint32_t num_filters, uint32_t max_tokens,
                            uint32_t &num_matches,
                            std::vector<fpga_token_header> &tokens);

  fpga_result fpgaDestroyToken(const fpga_remote_id &token_id);

  fpga_result fpgaCloneToken(const fpga_remote_id &src_token_id,
                             fpga_token_header &dest_token_hdr);

  fpga_result fpgaGetProperties(const fpga_remote_id &token_id,
                                fpga_properties &properties);

  fpga_result fpgaUpdateProperties(const fpga_remote_id &token_id,
                                   fpga_properties &properties);

  fpga_result fpgaOpen(const fpga_remote_id &token_id, int flags,
                       fpga_handle_header &header);

  fpga_result fpgaClose(const fpga_remote_id &handle_id);

  fpga_result fpgaReset(const fpga_remote_id &handle_id);

  fpga_result fpgaGetPropertiesFromHandle(const fpga_remote_id &handle_id,
                                          fpga_properties &properties);

  fpga_result fpgaMapMMIO(const fpga_remote_id &handle_id, uint32_t mmio_num,
                          fpga_remote_id &mmio_id);

  fpga_result fpgaUnmapMMIO(const fpga_remote_id &handle_id,
                            const fpga_remote_id &mmio_id, uint32_t mmio_num);

  fpga_result fpgaReadMMIO32(const fpga_remote_id &handle_id, uint32_t mmio_num,
                             uint64_t offset, uint32_t &value);

  fpga_result fpgaWriteMMIO32(const fpga_remote_id &handle_id,
                              uint32_t mmio_num, uint64_t offset,
                              uint32_t value);

  fpga_result fpgaReadMMIO64(const fpga_remote_id &handle_id, uint32_t mmio_num,
                             uint64_t offset, uint64_t &value);

  fpga_result fpgaWriteMMIO64(const fpga_remote_id &handle_id,
                              uint32_t mmio_num, uint64_t offset,
                              uint64_t value);

  fpga_result fpgaWriteMMIO512(const fpga_remote_id &handle_id,
                               uint32_t mmio_num, uint64_t offset,
                               const void *value);

  fpga_result fpgaPrepareBuffer(const fpga_remote_id &handle_id,
                                uint64_t length, void **buf_addr, int flags,
                                fpga_remote_id &buf_id);

  fpga_result fpgaReleaseBuffer(const fpga_remote_id &handle_id,
                                const fpga_remote_id &buf_id);

  fpga_result fpgaGetIOAddress(const fpga_remote_id &handle_id,
                               const fpga_remote_id &buf_id, uint64_t &ioaddr);

  fpga_result fpgaReadError(const fpga_remote_id &token_id, uint32_t error_num,
                            uint64_t &value);

  fpga_result fpgaGetErrorInfo(const fpga_remote_id &token_id,
                               uint32_t error_num, fpga_error_info &error_info);

  fpga_result fpgaClearError(const fpga_remote_id &token_id,
                             uint32_t error_num);

  fpga_result fpgaClearAllErrors(const fpga_remote_id &token_id);

  fpga_result fpgaTokenGetObject(const fpga_remote_id &token_id,
                                 const char *name, int flags,
                                 fpga_remote_id &object_id);

  fpga_result fpgaDestroyObject(const fpga_remote_id &object_id);

  fpga_result fpgaObjectGetType(const fpga_remote_id &object_id,
                                fpga_sysobject_type &type);

  fpga_result fpgaObjectGetName(const fpga_remote_id &object_id, char *name,
                                size_t max_len);

  fpga_result fpgaObjectGetSize(const fpga_remote_id &object_id, int flags,
                                uint32_t &value);

  fpga_result fpgaObjectRead(const fpga_remote_id &object_id, uint8_t *buffer,
                             size_t offset, size_t length, int flags);

  fpga_result fpgaObjectRead64(const fpga_remote_id &object_id, int flags,
                               uint64_t &value);

  fpga_result fpgaObjectWrite64(const fpga_remote_id &object_id, uint64_t value,
                                int flags);

  fpga_result fpgaHandleGetObject(const fpga_remote_id &handle_id,
                                  const char *name, int flags,
                                  fpga_remote_id &object_id);

  fpga_result fpgaObjectGetObject(const fpga_remote_id &parent_id,
                                  const char *name, int flags,
                                  fpga_remote_id &child_id);

  fpga_result fpgaObjectGetObjectAt(const fpga_remote_id &parent_id,
                                    size_t index, fpga_remote_id &child_id);

  fpga_result fpgaSetUserClock(const fpga_remote_id &handle_id,
                               uint64_t high_clk, uint64_t low_clk, int flags);

  fpga_result fpgaGetUserClock(const fpga_remote_id &handle_id, int flags,
                               uint64_t &high_clk, uint64_t &low_clk);

  fpga_result fpgaGetNumMetrics(const fpga_remote_id &handle_id,
                                uint64_t &num_metrics);

  fpga_result fpgaGetMetricsInfo(const fpga_remote_id &handle_id,
                                 uint64_t &num_metrics,
                                 std::vector<fpga_metric_info> &info);

  fpga_result fpgaGetMetricsByIndex(const fpga_remote_id &handle_id,
                                    const std::vector<uint64_t> &metric_num,
                                    uint64_t num_metric_indexes,
                                    std::vector<fpga_metric> &metrics);

  fpga_result fpgaGetMetricsByName(
      const fpga_remote_id &handle_id,
      const std::vector<std::string> &metrics_names, uint64_t num_metric_names,
      std::vector<fpga_metric> &metrics);

  fpga_result fpgaGetMetricsThresholdInfo(
      const fpga_remote_id &handle_id, uint32_t &num_thresholds,
      std::vector<metric_threshold> &metric_threshold);

  fpga_result fpgaReconfigureSlotByName(const fpga_remote_id &handle_id,
                                        uint32_t slot, const std::string &path,
                                        int flags);

  fpga_result fpgaBufMemSet(const fpga_remote_id &handle_id,
                            const fpga_remote_id &buf_id, size_t offset, int c,
                            size_t n);

  fpga_result fpgaBufMemCpyToRemote(const fpga_remote_id &handle_id,
                                    const fpga_remote_id &dest_buf_id,
                                    size_t dest_offset, const void *src,
                                    size_t n);

  fpga_result fpgaBufPoll(const fpga_remote_id &handle_id,
                          const fpga_remote_id &buf_id, uint64_t offset,
                          int width, uint64_t mask, uint64_t expected_value,
                          uint64_t sleep_interval, uint64_t loops_timeout);

  fpga_result fpgaBufMemCmp(const fpga_remote_id &handle_id,
                            const fpga_remote_id &bufa_id, size_t bufa_offset,
                            const fpga_remote_id &bufb_id, size_t bufb_offset,
                            size_t n, int &cmp_result);

  fpga_result fpgaBufWritePattern(const fpga_remote_id &handle_id,
                                  const fpga_remote_id &buf_id,
                                  const char *pattern_name);

  fpga_result fpgaCreateEventHandle(fpga_remote_id &eh_id);

  fpga_result fpgaRegisterEvent(const fpga_remote_id &handle_id,
                                fpga_event_type event_type,
                                const fpga_remote_id &eh_id, uint32_t flags,
                                uint32_t events_port, int &client_event_fd);

  fpga_result fpgaUnregisterEvent(const fpga_remote_id &handle_id,
                                  fpga_event_type event_type,
                                  const fpga_remote_id &eh_id);

  fpga_result fpgaDestroyEventHandle(const fpga_remote_id &eh_id);

  fpga_result ServerReset();

 private:
  std::unique_ptr<OPAEService::Stub> stub_;

 public:
  typedef opae_map_helper<fpga_remote_id, _remote_token *> token_map_t;
  token_map_t token_map_;

 private:
  bool debug_;
};

////////////////////////////////////////////////////////////////////////////////

using opaegrpc::OPAEEventsService;

class OPAEEventsClient final {
 public:
  OPAEEventsClient(std::shared_ptr<Channel> channel, bool debug)
      : stub_(OPAEEventsService::NewStub(channel)), debug_(debug) {
    (void)debug_;
  }

  fpga_result fpgaGetRemoteEventID(fpga_remote_id &event_id,
                                   int &client_event_fd);

  fpga_result fpgaSignalRemoteEvent(const fpga_remote_id &event_id);

  fpga_result fpgaReleaseRemoteEvent(const fpga_remote_id &event_id);

 private:
  std::unique_ptr<OPAEEventsService::Stub> stub_;
  bool debug_;
};
