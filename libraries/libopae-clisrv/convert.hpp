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

#include <opae/types.h>
#include <uuid/uuid.h>

#include <iostream>
#include <map>
#include <string>

#include "opae.grpc.pb.h"
#include "opae.pb.h"
#include "opae_int.h"
#include "props.h"
#include "remote.h"

inline bool operator<(const fpga_remote_id &lhs, const fpga_remote_id &rhs) {
  int res = strcmp(lhs.hostname, rhs.hostname);

  if (!res)  // same hostname
    return lhs.unique_id < rhs.unique_id;
  else if (res < 0)
    return true;
  else
    return false;
}

std::ostream &operator<<(std::ostream &os, const grpc::protobuf::Message &m);

std::string *to_string(const fpga_guid guid);
void from_string(const std::string &s, fpga_guid &guid);

extern const opaegrpc::fpga_result
    to_grpc_fpga_result[opaegrpc::fpga_result_ARRAYSIZE];
extern const fpga_result to_opae_fpga_result[opaegrpc::fpga_result_ARRAYSIZE];

extern const opaegrpc::fpga_event_type
    to_grpc_fpga_event_type[opaegrpc::fpga_event_type_ARRAYSIZE];
extern const fpga_event_type
    to_opae_fpga_event_type[opaegrpc::fpga_event_type_ARRAYSIZE];

extern const opaegrpc::fpga_accelerator_state
    to_grpc_fpga_accelerator_state[opaegrpc::fpga_accelerator_state_ARRAYSIZE];
extern const fpga_accelerator_state
    to_opae_fpga_accelerator_state[opaegrpc::fpga_accelerator_state_ARRAYSIZE];

extern const opaegrpc::fpga_objtype
    to_grpc_fpga_objtype[opaegrpc::fpga_objtype_ARRAYSIZE];
extern const fpga_objtype
    to_opae_fpga_objtype[opaegrpc::fpga_objtype_ARRAYSIZE];

extern const opaegrpc::fpga_interface
    to_grpc_fpga_interface[opaegrpc::fpga_interface_ARRAYSIZE];
extern const fpga_interface
    to_opae_fpga_interface[opaegrpc::fpga_interface_ARRAYSIZE];

extern const opaegrpc::fpga_sysobject_type
    to_grpc_fpga_sysobject_type[opaegrpc::fpga_sysobject_type_ARRAYSIZE];
extern const fpga_sysobject_type
    to_opae_fpga_sysobject_type[opaegrpc::fpga_sysobject_type_ARRAYSIZE];

extern const opaegrpc::fpga_metric_type
    to_grpc_fpga_metric_type[opaegrpc::fpga_metric_type_ARRAYSIZE];
extern const fpga_metric_type
    to_opae_fpga_metric_type[opaegrpc::fpga_metric_type_ARRAYSIZE];

extern const opaegrpc::fpga_metric_datatype
    to_grpc_fpga_metric_datatype[opaegrpc::fpga_metric_datatype_ARRAYSIZE];
extern const fpga_metric_datatype
    to_opae_fpga_metric_datatype[opaegrpc::fpga_metric_datatype_ARRAYSIZE];

opaegrpc::fpga_version *to_grpc_fpga_version(const fpga_version &ver);
fpga_version to_opae_fpga_version(const opaegrpc::fpga_version &gver);

opaegrpc::fpga_error_info *to_grpc_fpga_error_info(const fpga_error_info &info);
fpga_error_info to_opae_fpga_error_info(const opaegrpc::fpga_error_info &ginfo);

opaegrpc::metric_value *to_grpc_metric_value(const metric_value &val,
                                             fpga_metric_datatype ty);
metric_value to_opae_metric_value(const opaegrpc::metric_value &gval,
                                  opaegrpc::fpga_metric_datatype ty);

opaegrpc::fpga_metric_info *to_grpc_fpga_metric_info(
    const fpga_metric_info &minfo);
fpga_metric_info to_opae_fpga_metric_info(
    const opaegrpc::fpga_metric_info &ginfo);

opaegrpc::fpga_metric *to_grpc_fpga_metric(const fpga_metric &m,
                                           fpga_metric_datatype ty);
fpga_metric to_opae_fpga_metric(const opaegrpc::fpga_metric &gm,
                                opaegrpc::fpga_metric_datatype ty);

opaegrpc::threshold *to_grpc_threshold(const threshold &t);
threshold to_opae_threshold(const opaegrpc::threshold &gt);

opaegrpc::metric_threshold *to_grpc_metric_threshold(
    const metric_threshold &mt);
metric_threshold to_opae_metric_threshold(
    const opaegrpc::metric_threshold &gmt);

opaegrpc::fpga_remote_id *to_grpc_fpga_remote_id(const fpga_remote_id &id);
fpga_remote_id to_opae_fpga_remote_id(const opaegrpc::fpga_remote_id &gid);

void to_grpc_token_header(const fpga_token_header &hdr,
                          opaegrpc::fpga_token_header *gh);
fpga_token_header to_opae_token_header(const opaegrpc::fpga_token_header &ghdr);

opaegrpc::fpga_handle_header *to_grpc_handle_header(
    const fpga_handle_header &hdr);
fpga_handle_header to_opae_handle_header(
    const opaegrpc::fpga_handle_header &ghdr);

class OPAEClient;
class OPAEServiceImpl;

void to_grpc_fpga_properties(OPAEClient *client,
                             opaegrpc::fpga_properties *gprops,
                             const _fpga_properties *p);
void to_grpc_fpga_properties(OPAEServiceImpl *server,
                             opaegrpc::fpga_properties *gprops,
                             const _fpga_properties *p);

fpga_properties to_opae_fpga_properties(
    OPAEClient *client, const opaegrpc::fpga_properties &gprops);
fpga_properties to_opae_fpga_properties(
    OPAEServiceImpl *server, const opaegrpc::fpga_properties &gprops);

template <typename K, typename V>
class opae_map_helper {
 public:
  opae_map_helper(V none) : none_(none) {}

  V find(const K &key) const {
    typename std::map<K, V>::const_iterator it = map_.find(key);
    return (it == map_.end()) ? none_ : it->second;
  }

  bool add(const K &key, V value) {
    typename std::pair<typename std::map<K, V>::iterator, bool> res =
        map_.insert(std::make_pair(key, value));
    return res.second;
  }

  bool remove(const K &key) {
    typename std::map<K, V>::iterator it = map_.find(key);
    if (it == map_.end()) return false;
    map_.erase(it);
    return true;
  }

 private:
  V none_;
  std::map<K, V> map_;
};
