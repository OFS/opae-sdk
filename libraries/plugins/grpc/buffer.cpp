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

#include <opae/log.h>
#include <opae/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "grpc_client.hpp"
#include "mock/opae_std.h"
#include "remote.h"

fpga_result __REMOTE_API__ remote_fpgaPrepareBuffer(fpga_handle handle,
                                                    uint64_t blen,
                                                    void **buf_addr,
                                                    uint64_t *wsid, int flags) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id buf_id;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!wsid) {
    OPAE_ERR("NULL wsid");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaPrepareBuffer(h->hdr.handle_id, blen, buf_addr, flags,
                                  buf_id);
  if (res == FPGA_OK) {
    /*
    ** Special case: when flags contains FPGA_BUF_PREALLOCATED,
    ** and when buf_addr and blen are NULL and 0, then an FPGA_OK
    ** indicates that the API supports preallocated buffers.
    */
    if ((flags & FPGA_BUF_PREALLOCATED) && !buf_addr && !blen) return FPGA_OK;

    if (buf_addr) *buf_addr = nullptr;

    fpga_remote_id *rid = new fpga_remote_id(buf_id);
    *wsid = (uint64_t)rid;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaReleaseBuffer(fpga_handle handle,
                                                    uint64_t wsid) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);
  rid = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaReleaseBuffer(h->hdr.handle_id, *rid);
}

fpga_result __REMOTE_API__ remote_fpgaGetIOAddress(fpga_handle handle,
                                                   uint64_t wsid,
                                                   uint64_t *ioaddr) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!ioaddr) {
    OPAE_ERR("NULL ioaddr pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);
  rid = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaGetIOAddress(h->hdr.handle_id, *rid, *ioaddr);
}

fpga_result __REMOTE_API__ remote_fpgaBufMemSet(fpga_handle handle,
                                                uint64_t wsid, size_t offset,
                                                int c, size_t n) {
#if 1
  (void)handle;
  (void)wsid;
  (void)offset;
  (void)c;
  (void)n;

  return FPGA_OK;
#else
  opae_fpgaBufMemSet_request req;
  opae_fpgaBufMemSet_response resp;
  struct _remote_token *tok;
  struct _remote_handle *h;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  rid = (fpga_remote_id *)(void *)wsid;

  req.handle_id = h->hdr.handle_id;
  req.buf_id = *rid;
  req.offset = offset;
  req.c = c;
  req.n = n;

  req_json =
      opae_encode_fpgaBufMemSet_request_42(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaBufMemSet_response_42(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaBufMemCpyToRemote(fpga_handle handle,
                                                        uint64_t dest_wsid,
                                                        size_t dest_offset,
                                                        void *src, size_t n) {
#if 1
  (void)handle;
  (void)dest_wsid;
  (void)dest_offset;
  (void)src;
  (void)n;

  return FPGA_OK;
#else
  opae_fpgaBufMemCpyToRemote_request req;
  opae_fpgaBufMemCpyToRemote_response resp;
  struct _remote_token *tok;
  struct _remote_handle *h;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!src) {
    OPAE_ERR("NULL src buffer");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  rid = (fpga_remote_id *)(void *)dest_wsid;

  req.handle_id = h->hdr.handle_id;
  req.dest_buf_id = *rid;
  req.dest_offset = dest_offset;
  req.src = src;
  req.n = n;

  req_json = opae_encode_fpgaBufMemCpyToRemote_request_43(
      &req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaBufMemCpyToRemote_response_43(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaBufPoll(
    fpga_handle handle, uint64_t wsid, size_t offset, int width, uint64_t mask,
    uint64_t expected_value, uint64_t sleep_interval, uint64_t loops_timeout) {
#if 1
  (void)handle;
  (void)wsid;
  (void)offset;
  (void)width;
  (void)mask;
  (void)expected_value;
  (void)sleep_interval;
  (void)loops_timeout;

  return FPGA_OK;
#else
  opae_fpgaBufPoll_request req;
  opae_fpgaBufPoll_response resp;
  struct _remote_token *tok;
  struct _remote_handle *h;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  rid = (fpga_remote_id *)(void *)wsid;

  req.handle_id = h->hdr.handle_id;
  req.buf_id = *rid;
  req.offset = offset;
  req.width = width;
  req.mask = mask;
  req.expected_value = expected_value;
  req.sleep_interval = sleep_interval;
  req.loops_timeout = loops_timeout;

  req_json =
      opae_encode_fpgaBufPoll_request_44(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaBufPoll_response_44(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaBufMemCmp(
    fpga_handle handle, uint64_t bufa_wsid, size_t bufa_offset,
    uint64_t bufb_wsid, size_t bufb_offset, size_t n, int *cmp_result) {
#if 1
  (void)handle;
  (void)bufa_wsid;
  (void)bufa_offset;
  (void)bufb_wsid;
  (void)bufb_offset;
  (void)n;
  (void)cmp_result;

  return FPGA_OK;
#else
  opae_fpgaBufMemCmp_request req;
  opae_fpgaBufMemCmp_response resp;
  struct _remote_token *tok;
  struct _remote_handle *h;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  fpga_remote_id *rid_a;
  fpga_remote_id *rid_b;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!cmp_result) {
    OPAE_ERR("NULL cmp_result pointer");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  rid_a = (fpga_remote_id *)(void *)bufa_wsid;
  rid_b = (fpga_remote_id *)(void *)bufb_wsid;

  req.handle_id = h->hdr.handle_id;
  req.bufa_id = *rid_a;
  req.bufa_offset = bufa_offset;
  req.bufb_id = *rid_b;
  req.bufb_offset = bufb_offset;
  req.n = n;

  req_json =
      opae_encode_fpgaBufMemCmp_request_45(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaBufMemCmp_response_45(resp_json, &resp))
    return FPGA_EXCEPTION;

  *cmp_result = resp.cmp_result;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaBufWritePattern(
    fpga_handle handle, uint64_t wsid, const char *pattern_name) {
#if 1
  (void)handle;
  (void)wsid;
  (void)pattern_name;

  return FPGA_OK;
#else
  opae_fpgaBufWritePattern_request req;
  opae_fpgaBufWritePattern_response resp;
  struct _remote_token *tok;
  struct _remote_handle *h;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  size_t len;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!pattern_name) {
    OPAE_ERR("NULL pattern_name");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  rid = (fpga_remote_id *)(void *)wsid;

  req.handle_id = h->hdr.handle_id;
  req.buf_id = *rid;

  memset(req.pattern_name, 0, sizeof(req.pattern_name));
  len = strnlen(pattern_name, OPAE_REQUEST_NAME_MAX - 1);
  memcpy(req.pattern_name, pattern_name, len + 1);

  req_json = opae_encode_fpgaBufWritePattern_request_46(
      &req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaBufWritePattern_response_46(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}
