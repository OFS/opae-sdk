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

#include <opae/properties.h>
#include <opae/types.h>

#include "grpc_client.hpp"
#include "mock/opae_std.h"
#include "props.h"
#include "remote.h"

fpga_result __REMOTE_API__
remote_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop) {
  struct _remote_handle *h;
  struct _remote_token *tok;
  OPAEClient *client;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!prop) {
    OPAE_ERR("NULL properties pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaGetPropertiesFromHandle(h->hdr.handle_id, *prop);
}

fpga_result __REMOTE_API__ remote_fpgaGetProperties(fpga_token token,
                                                    fpga_properties *prop) {
  fpga_result res;
  _fpga_properties *_prop = nullptr;

  ASSERT_NOT_NULL(prop);

  res =
      ::fpgaGetProperties(nullptr, reinterpret_cast<fpga_properties *>(&_prop));

  ASSERT_RESULT(res);

  if (token) {
    res = remote_fpgaUpdateProperties(token, _prop);
    if (res) {
      opae_free(_prop);
      return res;
    }
  }

  *prop = _prop;
  return res;
}

fpga_result __REMOTE_API__ remote_fpgaUpdateProperties(fpga_token token,
                                                       fpga_properties prop) {
  OPAEClient *client;
  struct _remote_token *tok;
  fpga_result result;
  struct _fpga_properties *p;
  fpga_properties resp_props = nullptr;
  int res;
  pthread_mutex_t save_lock;

  if (!token) {
    OPAE_MSG("Invalid token");
    return FPGA_INVALID_PARAM;
  }

  if (!prop) {
    OPAE_MSG("Invalid properties object");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  result = client->fpgaUpdateProperties(tok->hdr.token_id, resp_props);
  if (result) return result;

  p = opae_validate_and_lock_properties(prop);
  if (!p) {
    ::fpgaDestroyProperties(&resp_props);
    return FPGA_INVALID_PARAM;
  }

  save_lock = p->lock;

  *p = *reinterpret_cast<_fpga_properties *>(resp_props);

  p->lock = save_lock;

  opae_mutex_unlock(res, &p->lock);

  ::fpgaDestroyProperties(&resp_props);

  return result;
}
