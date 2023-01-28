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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <algorithm>
#include <cstring>
#include <iostream>

#include "convert.hpp"
#include "grpc_server.hpp"

Status OPAEServiceImpl::fpgaEnumerate(ServerContext *context,
                                      const EnumerateRequest *request,
                                      EnumerateReply *reply) {
  std::cout << "fpgaEnumerate request " << *request << std::endl;

  UNUSED_PARAM(context);
  std::vector<fpga_properties> req_filters;
  fpga_properties *pfilters = nullptr;
  uint32_t req_num_filters;
  uint32_t req_max_tokens;

  req_filters.reserve(request->filters().size());
  for (const auto &f : request->filters()) {
    req_filters.push_back(to_opae_fpga_properties(this, f));
  }
  if (req_filters.size() > 0) pfilters = req_filters.data();

  req_num_filters = request->num_filters();
  req_max_tokens = request->max_tokens();

  fpga_token *tokens = nullptr;

  if (req_max_tokens) {
    tokens = new fpga_token[req_max_tokens];
    memset(tokens, 0, req_max_tokens * sizeof(fpga_token));
  }

  uint32_t resp_max_tokens = 0;
  uint32_t resp_num_matches = 0;
  fpga_result result;

  result = ::fpgaEnumerate(pfilters, req_num_filters, tokens, req_max_tokens,
                           &resp_num_matches);
  resp_max_tokens = req_max_tokens;

  if (tokens) {
    uint32_t i;
    const uint32_t num_tokens = std::min(resp_num_matches, req_max_tokens);

    // Walk through the tokens, opening each and grabbing its header.
    for (i = 0; i < num_tokens; ++i) {
      opae_wrapped_token *wt =
          reinterpret_cast<opae_wrapped_token *>(tokens[i]);
      fpga_token_header *hdr =
          reinterpret_cast<fpga_token_header *>(wt->opae_token);

      // If we're not already tracking this token_id, then add it.
      if (!find_token(hdr->token_id)) add_token(hdr->token_id, tokens[i]);

      // Add the token header to our outbound reply.
      to_grpc_token_header(*hdr, reply->add_tokens());
    }
  }

  reply->set_max_tokens(resp_max_tokens);
  reply->set_num_matches(resp_num_matches);
  reply->set_result(to_grpc_fpga_result[result]);

  if (tokens) delete[] tokens;

  return Status::OK;
}

Status OPAEServiceImpl::fpgaDestroyToken(ServerContext *context,
                                         const DestroyTokenRequest *request,
                                         DestroyTokenReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id req_token_id;
  fpga_token token;
  fpga_result res;

  std::cout << "fpgaDestroyToken request " << *request << std::endl;

  req_token_id = to_opae_fpga_remote_id(request->token_id());
  token = find_token(req_token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  remove_token(req_token_id);

  res = ::fpgaDestroyToken(&token);
  reply->set_result(to_grpc_fpga_result[res]);

  return Status::OK;
}

Status OPAEServiceImpl::fpgaCloneToken(ServerContext *context,
                                       const CloneTokenRequest *request,
                                       CloneTokenReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id req_src_token_id;
  fpga_token src_token;
  fpga_token dest_token = NULL;
  fpga_token_header resp_token_hdr;
  fpga_result res;

  std::cout << "fpgaCloneToken request " << *request << std::endl;

  req_src_token_id = to_opae_fpga_remote_id(request->src_token_id());
  src_token = find_token(req_src_token_id);

  if (!src_token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaCloneToken(src_token, &dest_token);
  if (res == FPGA_OK) {
    opae_wrapped_token *wt = reinterpret_cast<opae_wrapped_token *>(dest_token);
    fpga_token_header *hdr =
        reinterpret_cast<fpga_token_header *>(wt->opae_token);

    resp_token_hdr = *hdr;
    add_token(resp_token_hdr.token_id, dest_token);

    opaegrpc::fpga_token_header *ghdr = new opaegrpc::fpga_token_header();
    to_grpc_token_header(resp_token_hdr, ghdr);

    reply->set_allocated_dest_token(ghdr);
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetProperties(ServerContext *context,
                                          const GetPropertiesRequest *request,
                                          GetPropertiesReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  _fpga_properties *_props = nullptr;
  fpga_result res;

  std::cout << "fpgaGetProperties request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = find_token(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res =
      ::fpgaGetProperties(token, reinterpret_cast<fpga_properties *>(&_props));
  if (!_props) {
    reply->set_result(to_grpc_fpga_result[FPGA_NO_MEMORY]);
    return Status::OK;
  }

  opaegrpc::fpga_properties *gprops = new opaegrpc::fpga_properties();
  to_grpc_fpga_properties(this, gprops, _props);

  reply->set_allocated_properties(gprops);

  ::fpgaDestroyProperties(reinterpret_cast<fpga_properties *>(&_props));

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaUpdateProperties(
    ServerContext *context, const UpdatePropertiesRequest *request,
    UpdatePropertiesReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  fpga_properties resp_props = nullptr;
  fpga_result res;

  std::cout << "fpgaUpdateProperties request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = find_token(token_id);
  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaGetProperties(nullptr, &resp_props);
  if (res) {
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  res = ::fpgaUpdateProperties(token, resp_props);
  if (res) {
    ::fpgaDestroyProperties(&resp_props);
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  opaegrpc::fpga_properties *gprops = new opaegrpc::fpga_properties();
  to_grpc_fpga_properties(
      this, gprops, reinterpret_cast<const _fpga_properties *>(resp_props));
  reply->set_allocated_properties(gprops);

  ::fpgaDestroyProperties(&resp_props);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaOpen(ServerContext *context,
                                 const OpenRequest *request, OpenReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  fpga_handle handle = nullptr;
  int flags;
  fpga_result res;

  std::cout << "fpgaOpen request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = find_token(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  flags = request->flags();

  res = ::fpgaOpen(token, &handle, flags);
  if (res) {
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  opae_wrapped_handle *wh = reinterpret_cast<opae_wrapped_handle *>(handle);
  fpga_handle_header *hdr =
      reinterpret_cast<fpga_handle_header *>(wh->opae_handle);

  if (!find_handle(hdr->handle_id)) add_handle(hdr->handle_id, handle);

  reply->set_allocated_handle(to_grpc_handle_header(*hdr));

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaClose(ServerContext *context,
                                  const CloseRequest *request,
                                  CloseReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_result res;

  std::cout << "fpgaClose request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = find_handle(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaClose(handle);
  if (res == FPGA_OK) remove_handle(handle_id);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}
