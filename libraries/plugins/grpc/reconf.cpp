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

#include "remote.h"
//#include "request.h"
//#include "response.h"

#include "mock/opae_std.h"

fpga_result __REMOTE_API__ remote_fpgaReconfigureSlot(fpga_handle fpga,
                                                      uint32_t slot,
                                                      const uint8_t *bitstream,
                                                      size_t bitstream_len,
                                                      int flags) {
  OPAE_MSG("remote_fpgaReconfigureSlot not supported");
  UNUSED_PARAM(fpga);
  UNUSED_PARAM(slot);
  UNUSED_PARAM(bitstream);
  UNUSED_PARAM(bitstream_len);
  UNUSED_PARAM(flags);
  return FPGA_NOT_SUPPORTED;
}

fpga_result __REMOTE_API__ remote_fpgaReconfigureSlotByName(fpga_handle fpga,
                                                            uint32_t slot,
                                                            const char *path,
                                                            int flags) {
#if 1
  (void)fpga;
  (void)slot;
  (void)path;
  (void)flags;

  return FPGA_OK;
#else
  opae_fpgaReconfigureSlotByName_request req;
  opae_fpgaReconfigureSlotByName_response resp;
  struct _remote_handle *h;
  struct _remote_token *tok;
  char *req_json;
  char *resp_json = NULL;
  fpga_result res;
  size_t len;

  if (!fpga) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!path) {
    OPAE_ERR("NULL path");
    return FPGA_INVALID_PARAM;
  }

  h = (struct _remote_handle *)fpga;
  tok = h->token;

  req.handle_id = h->hdr.handle_id;
  req.slot = slot;

  len = strnlen(path, PATH_MAX - 1);
  memcpy(req.path, path, len + 1);

  req.flags = flags;

  req_json = opae_encode_fpgaReconfigureSlotByName_request_41(
      &req, tok->json_to_string_flags);

  res = opae_client_send_and_receive(tok, req_json, &resp_json);
  if (res) return res;

  if (!opae_decode_fpgaReconfigureSlotByName_response_41(resp_json, &resp))
    return FPGA_EXCEPTION;

  return resp.result;
#endif
}
