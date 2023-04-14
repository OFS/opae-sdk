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

fpga_result __REMOTE_API__ remote_fpgaReadError(fpga_token token,
                                                uint32_t error_num,
                                                uint64_t *value) {
  _remote_token *tok;
  OPAEClient *client;

  if (!token) {
    OPAE_ERR("NULL token");
    return FPGA_INVALID_PARAM;
  }

  if (!value) {
    OPAE_ERR("NULL value pointer");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = token_to_client(token);

  return client->fpgaReadError(tok->hdr.token_id, error_num, *value);
}

fpga_result __REMOTE_API__ remote_fpgaClearError(fpga_token token,
                                                 uint32_t error_num) {
  _remote_token *tok;
  OPAEClient *client;

  if (!token) {
    OPAE_ERR("NULL token");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = token_to_client(token);

  return client->fpgaClearError(tok->hdr.token_id, error_num);
}

fpga_result __REMOTE_API__ remote_fpgaClearAllErrors(fpga_token token) {
  _remote_token *tok;
  OPAEClient *client;

  if (!token) {
    OPAE_ERR("NULL token");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = token_to_client(token);

  return client->fpgaClearAllErrors(tok->hdr.token_id);
}

fpga_result __REMOTE_API__ remote_fpgaGetErrorInfo(
    fpga_token token, uint32_t error_num, struct fpga_error_info *error_info) {
  _remote_token *tok;
  OPAEClient *client;

  if (!token) {
    OPAE_ERR("NULL token");
    return FPGA_INVALID_PARAM;
  }

  if (!error_info) {
    OPAE_ERR("NULL error_info pointer");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = token_to_client(token);

  return client->fpgaGetErrorInfo(tok->hdr.token_id, error_num, *error_info);
}
