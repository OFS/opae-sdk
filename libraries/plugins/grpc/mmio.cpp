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

#include <opae/types.h>

#include "grpc_client.hpp"
#include "mock/opae_std.h"
#include "remote.h"

fpga_result __REMOTE_API__ remote_fpgaWriteMMIO32(fpga_handle handle,
                                                  uint32_t mmio_num,
                                                  uint64_t offset,
                                                  uint32_t value) {
#if 1
  (void)handle;
  (void)mmio_num;
  (void)offset;
  (void)value;

  return FPGA_OK;
#else
  opae_fpgaWriteMMIO32_request req;
  opae_fpgaWriteMMIO32_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  req.handle_id = h->hdr.handle_id;
  req.mmio_num = mmio_num;
  req.offset = offset;
  req.value = value;

  req_json =
      opae_encode_fpgaWriteMMIO32_request_12(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaWriteMMIO32_response_12(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaReadMMIO32(fpga_handle handle,
                                                 uint32_t mmio_num,
                                                 uint64_t offset,
                                                 uint32_t *value) {
#if 1
  (void)handle;
  (void)mmio_num;
  (void)offset;
  (void)value;

  return FPGA_OK;
#else
  opae_fpgaReadMMIO32_request req;
  opae_fpgaReadMMIO32_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  if (!value) {
    OPAE_ERR("NULL value pointer");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  req.handle_id = h->hdr.handle_id;
  req.mmio_num = mmio_num;
  req.offset = offset;

  req_json =
      opae_encode_fpgaReadMMIO32_request_11(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaReadMMIO32_response_11(resp_json, &resp))
    return FPGA_EXCEPTION;

  if (resp.result == FPGA_OK) *value = resp.value;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaWriteMMIO64(fpga_handle handle,
                                                  uint32_t mmio_num,
                                                  uint64_t offset,
                                                  uint64_t value) {
#if 1
  (void)handle;
  (void)mmio_num;
  (void)offset;
  (void)value;

  return FPGA_OK;
#else
  opae_fpgaWriteMMIO64_request req;
  opae_fpgaWriteMMIO64_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  req.handle_id = h->hdr.handle_id;
  req.mmio_num = mmio_num;
  req.offset = offset;
  req.value = value;

  req_json =
      opae_encode_fpgaWriteMMIO64_request_14(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaWriteMMIO64_response_14(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaReadMMIO64(fpga_handle handle,
                                                 uint32_t mmio_num,
                                                 uint64_t offset,
                                                 uint64_t *value) {
#if 1
  (void)handle;
  (void)mmio_num;
  (void)offset;
  (void)value;

  return FPGA_OK;
#else
  opae_fpgaReadMMIO64_request req;
  opae_fpgaReadMMIO64_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  if (!value) {
    OPAE_ERR("NULL value pointer");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  req.handle_id = h->hdr.handle_id;
  req.mmio_num = mmio_num;
  req.offset = offset;

  req_json =
      opae_encode_fpgaReadMMIO64_request_13(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaReadMMIO64_response_13(resp_json, &resp))
    return FPGA_EXCEPTION;

  if (resp.result == FPGA_OK) *value = resp.value;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaWriteMMIO512(fpga_handle handle,
                                                   uint32_t mmio_num,
                                                   uint64_t offset,
                                                   const void *value) {
#if 1
  (void)handle;
  (void)mmio_num;
  (void)offset;
  (void)value;

  return FPGA_OK;
#else
  opae_fpgaWriteMMIO512_request req;
  opae_fpgaWriteMMIO512_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)handle;
  tok = h->token;

  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  req.handle_id = h->hdr.handle_id;
  req.mmio_num = mmio_num;
  req.offset = offset;
  memcpy(req.values, value, sizeof(req.values));

  req_json =
      opae_encode_fpgaWriteMMIO512_request_15(&req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaWriteMMIO512_response_15(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaMapMMIO(fpga_handle handle,
                                              uint32_t mmio_num,
                                              uint64_t **mmio_ptr) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_result res;
  fpga_remote_id *mmio_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  if (h->mmio_regions[mmio_num]) {
    OPAE_MSG(
        "MMIO %u is already mapped. "
        "Proceeding anyway..",
        mmio_num);
    opae_free(h->mmio_regions[mmio_num]);
    h->mmio_regions[mmio_num] = NULL;
  }

  mmio_id = reinterpret_cast<fpga_remote_id *>(
      opae_calloc(1, sizeof(fpga_remote_id)));
  if (!mmio_id) {
    OPAE_ERR("calloc failed");
    return FPGA_NO_MEMORY;
  }

  res = client->fpgaMapMMIO(h->hdr.handle_id, mmio_num, *mmio_id);
  if (res == FPGA_OK) {
    h->mmio_regions[mmio_num] = mmio_id;

    if (mmio_ptr) {
      *mmio_ptr = NULL;
      OPAE_ERR(
          "Access to the raw MMIO pointer "
          "is not provided by this plugin.");
    }
  } else {
    opae_free(mmio_id);
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaUnmapMMIO(fpga_handle handle,
                                                uint32_t mmio_num) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_result res;
  fpga_remote_id *mmio_id;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
    OPAE_ERR("mmio_num out of range 0-%d", OPAE_MMIO_REGIONS_MAX - 1);
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  if (!h->mmio_regions[mmio_num]) {
    OPAE_ERR("MMIO %u is not mapped.", mmio_num);
    return FPGA_INVALID_PARAM;
  }

  mmio_id = h->mmio_regions[mmio_num];
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaUnmapMMIO(h->hdr.handle_id, *mmio_id, mmio_num);
  if (res == FPGA_OK) {
    opae_free(mmio_id);
    h->mmio_regions[mmio_num] = NULL;
  }

  return res;
}
