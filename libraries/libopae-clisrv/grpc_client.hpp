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
  OPAEClient(std::shared_ptr<Channel> channel)
      : stub_(OPAEService::NewStub(channel)), token_map_(nullptr) {}

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

 private:
  std::unique_ptr<OPAEService::Stub> stub_;

 public:
  typedef opae_map_helper<fpga_remote_id, _remote_token *> token_map_t;
  token_map_t token_map_;
};
