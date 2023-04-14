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
  client = token_to_client(h->token);

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
  OPAEClient *client;
  fpga_remote_id *rid;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);
  rid = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaReleaseBuffer(h->hdr.handle_id, *rid);
}

fpga_result __REMOTE_API__ remote_fpgaGetIOAddress(fpga_handle handle,
                                                   uint64_t wsid,
                                                   uint64_t *ioaddr) {
  _remote_handle *h;
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
  client = token_to_client(h->token);
  rid = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaGetIOAddress(h->hdr.handle_id, *rid, *ioaddr);
}

fpga_result __REMOTE_API__ remote_fpgaBufMemSet(fpga_handle handle,
                                                uint64_t wsid, size_t offset,
                                                int c, size_t n) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_remote_id *buf_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  buf_id = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaBufMemSet(h->hdr.handle_id, *buf_id, offset, c, n);
}

fpga_result __REMOTE_API__ remote_fpgaBufMemCpyToRemote(fpga_handle handle,
                                                        uint64_t dest_wsid,
                                                        size_t dest_offset,
                                                        const void *src,
                                                        size_t n) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_remote_id *buf_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!src) {
    OPAE_ERR("NULL src buffer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  buf_id = reinterpret_cast<fpga_remote_id *>((void *)dest_wsid);

  return client->fpgaBufMemCpyToRemote(h->hdr.handle_id, *buf_id, dest_offset,
                                       src, n);
}

fpga_result __REMOTE_API__ remote_fpgaBufPoll(
    fpga_handle handle, uint64_t wsid, size_t offset, int width, uint64_t mask,
    uint64_t expected_value, uint64_t sleep_interval, uint64_t loops_timeout) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_remote_id *buf_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  buf_id = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaBufPoll(h->hdr.handle_id, *buf_id, offset, width, mask,
                             expected_value, sleep_interval, loops_timeout);
}

fpga_result __REMOTE_API__ remote_fpgaBufMemCmp(
    fpga_handle handle, uint64_t bufa_wsid, size_t bufa_offset,
    uint64_t bufb_wsid, size_t bufb_offset, size_t n, int *cmp_result) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_remote_id *bufa_id;
  fpga_remote_id *bufb_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!cmp_result) {
    OPAE_ERR("NULL cmp_result pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  bufa_id = reinterpret_cast<fpga_remote_id *>((void *)bufa_wsid);
  bufb_id = reinterpret_cast<fpga_remote_id *>((void *)bufb_wsid);

  return client->fpgaBufMemCmp(h->hdr.handle_id, *bufa_id, bufa_offset,
                               *bufb_id, bufb_offset, n, *cmp_result);
}

fpga_result __REMOTE_API__ remote_fpgaBufWritePattern(
    fpga_handle handle, uint64_t wsid, const char *pattern_name) {
  _remote_handle *h;
  OPAEClient *client;
  fpga_remote_id *buf_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!pattern_name) {
    OPAE_ERR("NULL pattern_name");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  client = token_to_client(h->token);

  buf_id = reinterpret_cast<fpga_remote_id *>((void *)wsid);

  return client->fpgaBufWritePattern(h->hdr.handle_id, *buf_id, pattern_name);
}
