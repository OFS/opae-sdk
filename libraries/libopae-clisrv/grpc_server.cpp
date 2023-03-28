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
  if (debug_) std::cout << "fpgaEnumerate request " << *request << std::endl;

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

    resp_max_tokens = std::min(resp_max_tokens, resp_num_matches);

    // Walk through the tokens, opening each and grabbing its header.
    for (i = 0; i < resp_max_tokens; ++i) {
      opae_wrapped_token *wt =
          reinterpret_cast<opae_wrapped_token *>(tokens[i]);
      fpga_token_header *hdr =
          reinterpret_cast<fpga_token_header *>(wt->opae_token);

      // If we're not already tracking this token_id, then add it.
      if (!token_map_.find(hdr->token_id))
        token_map_.add(hdr->token_id, tokens[i]);

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

  if (debug_) std::cout << "fpgaDestroyToken request " << *request << std::endl;

  req_token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(req_token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  token_map_.remove(req_token_id);

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

  if (debug_) std::cout << "fpgaCloneToken request " << *request << std::endl;

  req_src_token_id = to_opae_fpga_remote_id(request->src_token_id());
  src_token = token_map_.find(req_src_token_id);

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
    token_map_.add(resp_token_hdr.token_id, dest_token);

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

  if (debug_)
    std::cout << "fpgaGetProperties request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

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

  if (debug_)
    std::cout << "fpgaUpdateProperties request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);
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

  if (debug_) std::cout << "fpgaOpen request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

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

  if (!handle_map_.find(hdr->handle_id))
    handle_map_.add(hdr->handle_id, handle);

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

  if (debug_) std::cout << "fpgaClose request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaClose(handle);
  if (res == FPGA_OK) handle_map_.remove(handle_id);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReset(ServerContext *context,
                                  const ResetRequest *request,
                                  ResetReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_result res;

  if (debug_) std::cout << "fpgaReset request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaReset(handle);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetPropertiesFromHandle(
    ServerContext *context, const GetPropertiesFromHandleRequest *request,
    GetPropertiesFromHandleReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_properties resp_props = nullptr;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetPropertiesFromHandle request " << *request
              << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaGetProperties(nullptr, &resp_props);
  if (res) {
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  res = ::fpgaGetPropertiesFromHandle(handle, &resp_props);
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

Status OPAEServiceImpl::fpgaMapMMIO(ServerContext *context,
                                    const MapMMIORequest *request,
                                    MapMMIOReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t *mmio_ptr = nullptr;
  fpga_remote_id mmio_id;
  fpga_result res;

  if (debug_) std::cout << "fpgaMapMMIO request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  res = ::fpgaMapMMIO(handle, mmio_num, &mmio_ptr);

  if (res) {
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  opae_get_remote_id(&mmio_id);
  mmio_map_.add(mmio_id, mmio_ptr);

  reply->set_allocated_mmio_id(to_grpc_fpga_remote_id(mmio_id));
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaUnmapMMIO(ServerContext *context,
                                      const UnmapMMIORequest *request,
                                      UnmapMMIOReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_remote_id mmio_id;
  fpga_handle handle;
  uint32_t mmio_num;
  fpga_result res;

  if (debug_) std::cout << "fpgaUnmapMMIO request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();

  res = ::fpgaUnmapMMIO(handle, mmio_num);
  if (res) {
    reply->set_result(to_grpc_fpga_result[res]);
    return Status::OK;
  }

  mmio_id = to_opae_fpga_remote_id(request->mmio_id());
  mmio_map_.remove(mmio_id);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReadMMIO32(ServerContext *context,
                                       const ReadMMIO32Request *request,
                                       ReadMMIO32Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t offset;
  uint32_t value = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaReadMMIO32 request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  offset = request->offset();

  res = ::fpgaReadMMIO32(handle, mmio_num, offset, &value);

  reply->set_value(value);
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaWriteMMIO32(ServerContext *context,
                                        const WriteMMIO32Request *request,
                                        WriteMMIO32Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t offset;
  uint32_t value;
  fpga_result res;

  if (debug_) std::cout << "fpgaWriteMMIO32 request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  offset = request->offset();
  value = request->value();

  res = ::fpgaWriteMMIO32(handle, mmio_num, offset, value);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReadMMIO64(ServerContext *context,
                                       const ReadMMIO64Request *request,
                                       ReadMMIO64Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t offset;
  uint64_t value = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaReadMMIO64 request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  offset = request->offset();

  res = ::fpgaReadMMIO64(handle, mmio_num, offset, &value);

  reply->set_value(value);
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaWriteMMIO64(ServerContext *context,
                                        const WriteMMIO64Request *request,
                                        WriteMMIO64Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t offset;
  uint64_t value;
  fpga_result res;

  if (debug_) std::cout << "fpgaWriteMMIO64 request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  offset = request->offset();
  value = request->value();

  res = ::fpgaWriteMMIO64(handle, mmio_num, offset, value);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

constexpr size_t bits_to_bytes(size_t bits) { return bits / 8; }

Status OPAEServiceImpl::fpgaWriteMMIO512(ServerContext *context,
                                         const WriteMMIO512Request *request,
                                         WriteMMIO512Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t mmio_num;
  uint64_t offset;
  fpga_result res;

  if (debug_) std::cout << "fpgaWriteMMIO512 request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  mmio_num = request->mmio_num();
  offset = request->offset();
  const std::string &values = request->values();

  if (values.length() < bits_to_bytes(512)) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaWriteMMIO512(handle, mmio_num, offset, values.data());

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaPrepareBuffer(ServerContext *context,
                                          const PrepareBufferRequest *request,
                                          PrepareBufferReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t length;
  bool have_buf_addr;
  void **buf_addr = nullptr;
  void *addr = nullptr;
  int flags;
  uint64_t wsid = 0;
  fpga_remote_id buf_id;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaPrepareBuffer request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  length = request->length();
  have_buf_addr = request->have_buf_addr();

  if (have_buf_addr) {
    buf_addr = &addr;
    addr = (void *)request->pre_allocated_addr();
  }

  flags = request->flags();

  res = ::fpgaPrepareBuffer(handle, length, buf_addr, &wsid, flags);

  if ((res == FPGA_OK) && buf_addr) {
    // Allocate a new remote ID for the buffer.
    opae_get_remote_id(&buf_id);

    OPAEBufferInfo *binfo =
        new OPAEBufferInfo(handle_id, length, *buf_addr, wsid, flags);
    binfo_map_.add(buf_id, binfo);

    reply->set_allocated_buf_id(to_grpc_fpga_remote_id(buf_id));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReleaseBuffer(ServerContext *context,
                                          const ReleaseBufferRequest *request,
                                          ReleaseBufferReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id buf_id;
  OPAEBufferInfo *binfo;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaReleaseBuffer request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  buf_id = to_opae_fpga_remote_id(request->buf_id());
  binfo = binfo_map_.find(buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  res = ::fpgaReleaseBuffer(handle, binfo->wsid_);
  if (res == FPGA_OK) binfo_map_.remove(buf_id);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetIOAddress(ServerContext *context,
                                         const GetIOAddressRequest *request,
                                         GetIOAddressReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id buf_id;
  OPAEBufferInfo *binfo;
  uint64_t ioaddr = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaGetIOAddress request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  buf_id = to_opae_fpga_remote_id(request->buf_id());
  binfo = binfo_map_.find(buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  res = ::fpgaGetIOAddress(handle, binfo->wsid_, &ioaddr);

  reply->set_ioaddr(ioaddr);
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReadError(ServerContext *context,
                                      const ReadErrorRequest *request,
                                      ReadErrorReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  uint32_t error_num;
  uint64_t value = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaReadError request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  error_num = request->error_num();

  res = ::fpgaReadError(token, error_num, &value);

  reply->set_value(value);
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetErrorInfo(ServerContext *context,
                                         const GetErrorInfoRequest *request,
                                         GetErrorInfoReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  uint32_t error_num;
  fpga_error_info error_info;
  fpga_result res;

  if (debug_) std::cout << "fpgaGetErrorInfo request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  error_num = request->error_num();

  res = ::fpgaGetErrorInfo(token, error_num, &error_info);

  reply->set_allocated_error_info(to_grpc_fpga_error_info(error_info));
  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaClearError(ServerContext *context,
                                       const ClearErrorRequest *request,
                                       ClearErrorReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  uint32_t error_num;
  fpga_result res;

  if (debug_) std::cout << "fpgaClearError request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  error_num = request->error_num();

  res = ::fpgaClearError(token, error_num);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaClearAllErrors(ServerContext *context,
                                           const ClearAllErrorsRequest *request,
                                           ClearAllErrorsReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaClearAllErrors request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaClearAllErrors(token);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaTokenGetObject(ServerContext *context,
                                           const TokenGetObjectRequest *request,
                                           TokenGetObjectReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id token_id;
  fpga_token token;
  int flags;
  fpga_object obj = nullptr;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaTokenGetObject request " << *request << std::endl;

  token_id = to_opae_fpga_remote_id(request->token_id());
  token = token_map_.find(token_id);

  if (!token) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  flags = request->flags();
  const std::string &name = request->name();

  res = ::fpgaTokenGetObject(token, name.c_str(), &obj, flags);
  if (res == FPGA_OK) {
    fpga_remote_id object_id;

    // Allocate a new remote ID for the object.
    opae_get_remote_id(&object_id);

    sysobj_map_.add(object_id, obj);

    reply->set_allocated_object_id(to_grpc_fpga_remote_id(object_id));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaDestroyObject(ServerContext *context,
                                          const DestroyObjectRequest *request,
                                          DestroyObjectReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaDestroyObject request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaDestroyObject(&sysobj);
  if (res == FPGA_OK) sysobj_map_.remove(object_id);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectGetType(ServerContext *context,
                                          const ObjectGetTypeRequest *request,
                                          ObjectGetTypeReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  fpga_sysobject_type type = FPGA_OBJECT_ATTRIBUTE;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectGetType request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaObjectGetType(sysobj, &type);
  if (res == FPGA_OK) reply->set_type(to_grpc_fpga_sysobject_type[type]);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectGetName(ServerContext *context,
                                          const ObjectGetNameRequest *request,
                                          ObjectGetNameReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectGetName request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  char buf[PATH_MAX] = {
      0,
  };
  res = ::fpgaObjectGetName(sysobj, buf, sizeof(buf));

  if (res == FPGA_OK) reply->set_allocated_name(new std::string(buf));

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectGetSize(ServerContext *context,
                                          const ObjectGetSizeRequest *request,
                                          ObjectGetSizeReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  int flags;
  uint32_t value = 0;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectGetSize request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  flags = request->flags();

  res = ::fpgaObjectGetSize(sysobj, &value, flags);

  if (res == FPGA_OK) reply->set_value(value);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectRead(ServerContext *context,
                                       const ObjectReadRequest *request,
                                       ObjectReadReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  uint64_t offset;
  uint64_t length;
  int flags;
  fpga_result res;

  if (debug_) std::cout << "fpgaObjectRead request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  offset = request->offset();
  length = request->length();
  flags = request->flags();

  char buf[4096] = {
      0,
  };
  res = ::fpgaObjectRead(sysobj, (uint8_t *)buf, offset, length, flags);

  if (res == FPGA_OK) {
    reply->set_allocated_value(new std::string(buf, length));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectRead64(ServerContext *context,
                                         const ObjectRead64Request *request,
                                         ObjectRead64Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  int flags;
  uint64_t value = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaObjectRead64 request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  flags = request->flags();

  res = ::fpgaObjectRead64(sysobj, &value, flags);

  if (res == FPGA_OK) reply->set_value(value);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectWrite64(ServerContext *context,
                                          const ObjectWrite64Request *request,
                                          ObjectWrite64Reply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id object_id;
  fpga_object sysobj;
  uint64_t value;
  int flags;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectWrite64 request " << *request << std::endl;

  object_id = to_opae_fpga_remote_id(request->object_id());
  sysobj = sysobj_map_.find(object_id);

  if (!sysobj) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  value = request->value();
  flags = request->flags();

  res = ::fpgaObjectWrite64(sysobj, value, flags);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaHandleGetObject(
    ServerContext *context, const HandleGetObjectRequest *request,
    HandleGetObjectReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  int flags;
  fpga_object sysobj = nullptr;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaHandleGetObject request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  const std::string &name = request->name();
  flags = request->flags();

  res = ::fpgaHandleGetObject(handle, name.c_str(), &sysobj, flags);
  if (res == FPGA_OK) {
    fpga_remote_id object_id;

    // Allocate a new remote ID for the object.
    opae_get_remote_id(&object_id);

    sysobj_map_.add(object_id, sysobj);

    reply->set_allocated_object_id(to_grpc_fpga_remote_id(object_id));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectGetObject(
    ServerContext *context, const ObjectGetObjectRequest *request,
    ObjectGetObjectReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id parent_id;
  fpga_object parent;
  fpga_object child = nullptr;
  int flags;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectGetObject request " << *request << std::endl;

  parent_id = to_opae_fpga_remote_id(request->object_id());
  parent = sysobj_map_.find(parent_id);

  if (!parent) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  const std::string &name = request->name();
  flags = request->flags();

  res = ::fpgaObjectGetObject(parent, name.c_str(), &child, flags);
  if (res == FPGA_OK) {
    fpga_remote_id child_id;

    // Allocate a new remote ID for the child object.
    opae_get_remote_id(&child_id);

    sysobj_map_.add(child_id, child);

    reply->set_allocated_object_id(to_grpc_fpga_remote_id(child_id));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaObjectGetObjectAt(
    ServerContext *context, const ObjectGetObjectAtRequest *request,
    ObjectGetObjectAtReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id parent_id;
  fpga_object parent;
  uint64_t index;
  fpga_object child = nullptr;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaObjectGetObjectAt request " << *request << std::endl;

  parent_id = to_opae_fpga_remote_id(request->object_id());
  parent = sysobj_map_.find(parent_id);

  if (!parent) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  index = request->index();

  res = ::fpgaObjectGetObjectAt(parent, index, &child);

  if (res == FPGA_OK) {
    fpga_remote_id child_id;

    // Allocate a new remote ID for the child object.
    opae_get_remote_id(&child_id);

    sysobj_map_.add(child_id, child);

    reply->set_allocated_object_id(to_grpc_fpga_remote_id(child_id));
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaSetUserClock(ServerContext *context,
                                         const SetUserClockRequest *request,
                                         SetUserClockReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t high_clk;
  uint64_t low_clk;
  int flags;
  fpga_result res;

  if (debug_) std::cout << "fpgaSetUserClock request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  high_clk = request->high_clk();
  low_clk = request->low_clk();
  flags = request->flags();

  res = ::fpgaSetUserClock(handle, high_clk, low_clk, flags);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetUserClock(ServerContext *context,
                                         const GetUserClockRequest *request,
                                         GetUserClockReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  int flags;
  uint64_t high_clk = 0;
  uint64_t low_clk = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaGetUserClock request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  flags = request->flags();

  res = ::fpgaGetUserClock(handle, &high_clk, &low_clk, flags);

  if (res == FPGA_OK) {
    reply->set_high_clk(high_clk);
    reply->set_low_clk(low_clk);
  }

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetNumMetrics(ServerContext *context,
                                          const GetNumMetricsRequest *request,
                                          GetNumMetricsReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t num_metrics = 0;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetNumMetrics request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  res = ::fpgaGetNumMetrics(handle, &num_metrics);

  if (res == FPGA_OK) reply->set_num_metrics(num_metrics);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetMetricsInfo(ServerContext *context,
                                           const GetMetricsInfoRequest *request,
                                           GetMetricsInfoReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t input_num_metrics;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetMetricsInfo request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  input_num_metrics = request->num_metrics();
  fpga_metric_info *info = nullptr;
  uint64_t output_num_metrics = input_num_metrics;

  if (input_num_metrics) {
    info = new fpga_metric_info[input_num_metrics];
    memset(info, 0, input_num_metrics * sizeof(fpga_metric_info));
  }

  res = ::fpgaGetMetricsInfo(handle, info, &output_num_metrics);

  output_num_metrics = std::min(input_num_metrics, output_num_metrics);

  if ((res == FPGA_OK) && info) {
    uint64_t i;

    for (i = 0; i < output_num_metrics; ++i)
      to_grpc_fpga_metric_info(info[i], reply->add_info());

    reply->set_num_metrics(output_num_metrics);
  }

  if (info) delete[] info;

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetMetricsByIndex(
    ServerContext *context, const GetMetricsByIndexRequest *request,
    GetMetricsByIndexReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t input_num_metric_indexes;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetMetricsByIndex request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  input_num_metric_indexes = request->num_metric_indexes();
  input_num_metric_indexes = std::min(input_num_metric_indexes,
                                      (uint64_t)request->metric_num().size());
  uint64_t *metric_num = nullptr;

  fpga_metric *metrics = nullptr;
  uint64_t output_num_metric_indexes = input_num_metric_indexes;

  if (input_num_metric_indexes) {
    metric_num = new uint64_t[input_num_metric_indexes];
    std::copy(request->metric_num().cbegin(),
              request->metric_num().cbegin() + input_num_metric_indexes,
              metric_num);
  }

  if (output_num_metric_indexes) {
    metrics = new fpga_metric[output_num_metric_indexes];
    memset(metrics, 0, output_num_metric_indexes * sizeof(fpga_metric));
  }

  res = ::fpgaGetMetricsByIndex(handle, metric_num, output_num_metric_indexes,
                                metrics);

  if ((res == FPGA_OK) && metrics) {
    uint64_t i;

    for (i = 0; i < output_num_metric_indexes; ++i)
      to_grpc_fpga_metric(metrics[i], reply->add_metrics());

    reply->set_num_metric_indexes(output_num_metric_indexes);
  }

  if (metric_num) delete[] metric_num;
  if (metrics) delete[] metrics;

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetMetricsByName(
    ServerContext *context, const GetMetricsByNameRequest *request,
    GetMetricsByNameReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint64_t input_num_metric_names;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetMetricsByName request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  input_num_metric_names = request->num_metric_names();
  input_num_metric_names = std::min(input_num_metric_names,
                                    (uint64_t)request->metrics_names().size());
  char **metrics_names = nullptr;

  fpga_metric *metrics = nullptr;
  uint64_t output_num_metric_names = input_num_metric_names;

  if (input_num_metric_names) {
    metrics_names = new char *[input_num_metric_names];
    uint64_t i;
    google::protobuf::RepeatedPtrField<std::string>::const_iterator it;
    for (i = 0, it = request->metrics_names().cbegin();
         i < input_num_metric_names; ++i, ++it)
      metrics_names[i] = const_cast<char *>(it->c_str());
  }

  if (output_num_metric_names) {
    metrics = new fpga_metric[output_num_metric_names];
    memset(metrics, 0, output_num_metric_names * sizeof(fpga_metric));
  }

  res = ::fpgaGetMetricsByName(handle, metrics_names, output_num_metric_names,
                               metrics);

  if ((res == FPGA_OK) && metrics) {
    uint64_t i;

    for (i = 0; i < output_num_metric_names; ++i)
      to_grpc_fpga_metric(metrics[i], reply->add_metrics());

    reply->set_num_metric_names(output_num_metric_names);
  }

  if (metrics_names) delete[] metrics_names;
  if (metrics) delete[] metrics;

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaGetMetricsThresholdInfo(
    ServerContext *context, const GetMetricsThresholdInfoRequest *request,
    GetMetricsThresholdInfoReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t input_num_thresholds;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaGetMetricsThresholdInfo request " << *request
              << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  input_num_thresholds = request->num_thresholds();
  metric_threshold *metric_thresholds = nullptr;

  if (input_num_thresholds) {
    metric_thresholds = new metric_threshold[input_num_thresholds];
    memset(metric_thresholds, 0,
           input_num_thresholds * sizeof(metric_threshold));
  }

  uint32_t output_num_thresholds = input_num_thresholds;
  res = ::fpgaGetMetricsThresholdInfo(handle, metric_thresholds,
                                      &output_num_thresholds);

  reply->set_num_thresholds(output_num_thresholds);

  if ((res == FPGA_OK) && metric_thresholds) {
    uint32_t i;
    for (i = 0; i < output_num_thresholds; ++i)
      to_grpc_metric_threshold(metric_thresholds[i],
                               reply->add_metric_threshold());
  }

  if (metric_thresholds) delete[] metric_thresholds;

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaReconfigureSlotByName(
    ServerContext *context, const ReconfigureSlotByNameRequest *request,
    ReconfigureSlotByNameReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  uint32_t slot;
  int flags;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaReconfigureSlotByName request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  slot = request->slot();
  flags = request->flags();

  res =
      ::fpgaReconfigureSlotByName(handle, slot, request->path().c_str(), flags);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaBufMemSet(ServerContext *context,
                                      const BufMemSetRequest *request,
                                      BufMemSetReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id buf_id;
  OPAEBufferInfo *binfo;
  size_t offset;
  int c;
  size_t n;
  fpga_result res;

  if (debug_) std::cout << "fpgaBufMemSet request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  buf_id = to_opae_fpga_remote_id(request->buf_id());
  binfo = binfo_map_.find(buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  offset = (size_t)request->offset();
  c = request->c();
  n = (size_t)request->n();

  res = ::fpgaBufMemSet(handle, binfo->wsid_, offset, c, n);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaBufMemCpyToRemote(
    ServerContext *context, const BufMemCpyToRemoteRequest *request,
    BufMemCpyToRemoteReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id dest_buf_id;
  OPAEBufferInfo *binfo;
  size_t dest_offset;
  size_t n;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaBufMemCpyToRemote request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  dest_buf_id = to_opae_fpga_remote_id(request->dest_buf_id());
  binfo = binfo_map_.find(dest_buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  dest_offset = (size_t)request->dest_offset();
  const std::string &src = request->src();
  n = (size_t)request->n();

  res = ::fpgaBufMemCpyToRemote(handle, binfo->wsid_, dest_offset,
                                const_cast<char *>(src.data()), n);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaBufPoll(ServerContext *context,
                                    const BufPollRequest *request,
                                    BufPollReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id buf_id;
  OPAEBufferInfo *binfo;
  size_t offset;
  int width;
  uint64_t mask;
  uint64_t expected_value;
  uint64_t sleep_interval;
  uint64_t loops_timeout;
  fpga_result res;

  if (debug_) std::cout << "fpgaBufPoll request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  buf_id = to_opae_fpga_remote_id(request->buf_id());
  binfo = binfo_map_.find(buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  offset = (size_t)request->offset();
  width = request->width();
  mask = request->mask();
  expected_value = request->expected_value();
  sleep_interval = request->sleep_interval();
  loops_timeout = request->loops_timeout();

  res = ::fpgaBufPoll(handle, binfo->wsid_, offset, width, mask, expected_value,
                      sleep_interval, loops_timeout);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaBufMemCmp(ServerContext *context,
                                      const BufMemCmpRequest *request,
                                      BufMemCmpReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id bufa_id;
  OPAEBufferInfo *binfoa;
  size_t bufa_offset;
  fpga_remote_id bufb_id;
  OPAEBufferInfo *binfob;
  size_t bufb_offset;
  size_t n;
  int cmp_result = 0;
  fpga_result res;

  if (debug_) std::cout << "fpgaBufMemCmp request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  bufa_id = to_opae_fpga_remote_id(request->bufa_id());
  binfoa = binfo_map_.find(bufa_id);

  if (!binfoa) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfoa->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  bufa_offset = (size_t)request->bufa_offset();

  bufb_id = to_opae_fpga_remote_id(request->bufb_id());
  binfob = binfo_map_.find(bufb_id);

  if (!binfob) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfob->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  bufb_offset = (size_t)request->bufb_offset();
  n = (size_t)request->n();

  res = ::fpgaBufMemCmp(handle, binfoa->wsid_, bufa_offset, binfob->wsid_,
                        bufb_offset, n, &cmp_result);

  if (res == FPGA_OK) reply->set_cmp_result(cmp_result);

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}

Status OPAEServiceImpl::fpgaBufWritePattern(
    ServerContext *context, const BufWritePatternRequest *request,
    BufWritePatternReply *reply) {
  UNUSED_PARAM(context);
  fpga_remote_id handle_id;
  fpga_handle handle;
  fpga_remote_id buf_id;
  OPAEBufferInfo *binfo;
  fpga_result res;

  if (debug_)
    std::cout << "fpgaBufWritePattern request " << *request << std::endl;

  handle_id = to_opae_fpga_remote_id(request->handle_id());
  handle = handle_map_.find(handle_id);

  if (!handle) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  buf_id = to_opae_fpga_remote_id(request->buf_id());
  binfo = binfo_map_.find(buf_id);

  if (!binfo) {
    reply->set_result(to_grpc_fpga_result[FPGA_INVALID_PARAM]);
    return Status::OK;
  }

  if (!opae_remote_ids_match(&handle_id, &binfo->handle_id_)) {
    reply->set_result(to_grpc_fpga_result[FPGA_NOT_FOUND]);
    return Status::OK;
  }

  const std::string &pattern_name = request->pattern_name();

  res = ::fpgaBufWritePattern(handle, binfo->wsid_, pattern_name.c_str());

  reply->set_result(to_grpc_fpga_result[res]);
  return Status::OK;
}
