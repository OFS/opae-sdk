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

#include "grpc_client.hpp"
#include "mock/opae_std.h"
#include "remote.h"

struct _remote_sysobject *opae_create_remote_sysobject(
    struct _remote_token *token, fpga_remote_id *rid) {
  struct _remote_sysobject *s =
      (struct _remote_sysobject *)opae_calloc(1, sizeof(*s));
  if (s) {
    s->token = token;
    s->object_id = *rid;
  }
  return s;
}

void opae_destroy_remote_sysobject(struct _remote_sysobject *s) {
  opae_free(s);
}

fpga_result __REMOTE_API__ remote_fpgaTokenGetObject(fpga_token token,
                                                     const char *name,
                                                     fpga_object *object,
                                                     int flags) {
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id object_id;
  fpga_result res;

  if (!token) {
    OPAE_ERR("NULL token");
    return FPGA_INVALID_PARAM;
  }

  if (!name) {
    OPAE_ERR("NULL name");
    return FPGA_INVALID_PARAM;
  }

  if (!object) {
    OPAE_ERR("NULL object pointer");
    return FPGA_INVALID_PARAM;
  }

  tok = reinterpret_cast<_remote_token *>(token);
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaTokenGetObject(tok->hdr.token_id, name, flags, object_id);
  if (res == FPGA_OK) {
    _remote_sysobject *o = opae_create_remote_sysobject(tok, &object_id);

    if (!o) {
      OPAE_ERR("calloc failed");
      *object = NULL;
      return FPGA_NO_MEMORY;
    }

    *object = o;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaHandleGetObject(fpga_handle handle,
                                                      const char *name,
                                                      fpga_object *object,
                                                      int flags) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id object_id;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!name) {
    OPAE_ERR("NULL name");
    return FPGA_INVALID_PARAM;
  }

  if (!object) {
    OPAE_ERR("NULL object pointer");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaHandleGetObject(h->hdr.handle_id, name, flags, object_id);
  if (res == FPGA_OK) {
    _remote_sysobject *o = opae_create_remote_sysobject(tok, &object_id);

    if (!o) {
      OPAE_ERR("calloc failed");
      *object = NULL;
      return FPGA_NO_MEMORY;
    }

    *object = o;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaObjectGetObject(fpga_object parent,
                                                      const char *name,
                                                      fpga_object *object,
                                                      int flags) {
  _remote_sysobject *par;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id child_id;
  fpga_result res;

  if (!parent) {
    OPAE_ERR("NULL parent");
    return FPGA_INVALID_PARAM;
  }

  if (!name) {
    OPAE_ERR("NULL name");
    return FPGA_INVALID_PARAM;
  }

  if (!object) {
    OPAE_ERR("NULL object pointer");
    return FPGA_INVALID_PARAM;
  }

  par = reinterpret_cast<_remote_sysobject *>(parent);
  tok = par->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaObjectGetObject(par->object_id, name, flags, child_id);

  if (res == FPGA_OK) {
    struct _remote_sysobject *o = opae_create_remote_sysobject(tok, &child_id);

    if (!o) {
      OPAE_ERR("calloc failed");
      *object = NULL;
      return FPGA_NO_MEMORY;
    }

    *object = o;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaObjectGetObjectAt(fpga_object parent,
                                                        size_t idx,
                                                        fpga_object *object) {
  _remote_sysobject *par;
  _remote_token *tok;
  OPAEClient *client;
  fpga_remote_id child_id;
  fpga_result res;

  if (!parent) {
    OPAE_ERR("NULL parent");
    return FPGA_INVALID_PARAM;
  }

  if (!object) {
    OPAE_ERR("NULL object pointer");
    return FPGA_INVALID_PARAM;
  }

  par = reinterpret_cast<_remote_sysobject *>(parent);
  tok = par->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaObjectGetObjectAt(par->object_id, idx, child_id);

  if (res == FPGA_OK) {
    struct _remote_sysobject *o = opae_create_remote_sysobject(tok, &child_id);

    if (!o) {
      OPAE_ERR("calloc failed");
      *object = NULL;
      return FPGA_NO_MEMORY;
    }

    *object = o;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaDestroyObject(fpga_object *obj) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;
  fpga_result res;

  if (!obj || !*obj) {
    OPAE_ERR("invalid sysobject pointer");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(*obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  res = client->fpgaDestroyObject(o->object_id);

  if (res == FPGA_OK) {
    opae_destroy_remote_sysobject(o);
    *obj = NULL;
  }

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaObjectGetSize(fpga_object obj,
                                                    uint32_t *size, int flags) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  if (!size) {
    OPAE_ERR("NULL size pointer");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectGetSize(o->object_id, flags, *size);
}

fpga_result __REMOTE_API__ remote_fpgaObjectRead64(fpga_object obj,
                                                   uint64_t *value, int flags) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  if (!value) {
    OPAE_ERR("NULL value pointer");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectRead64(o->object_id, flags, *value);
}

fpga_result __REMOTE_API__ remote_fpgaObjectRead(fpga_object obj,
                                                 uint8_t *buffer, size_t offset,
                                                 size_t blen, int flags) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  if (!buffer) {
    OPAE_ERR("NULL buffer");
    return FPGA_INVALID_PARAM;
  }

  if (!blen) {
    OPAE_ERR("Invalid buffer length");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectRead(o->object_id, buffer, offset, blen, flags);
}

fpga_result __REMOTE_API__ remote_fpgaObjectWrite64(fpga_object obj,
                                                    uint64_t value, int flags) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectWrite64(o->object_id, value, flags);
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetType(fpga_object obj, enum fpga_sysobject_type *type) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  if (!type) {
    OPAE_ERR("NULL type pointer");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectGetType(o->object_id, *type);
}

fpga_result __REMOTE_API__ remote_fpgaObjectGetName(fpga_object obj, char *name,
                                                    size_t max_len) {
  _remote_sysobject *o;
  _remote_token *tok;
  OPAEClient *client;

  if (!obj) {
    OPAE_ERR("NULL obj");
    return FPGA_INVALID_PARAM;
  }

  if (!name) {
    OPAE_ERR("NULL name");
    return FPGA_INVALID_PARAM;
  }

  o = reinterpret_cast<_remote_sysobject *>(obj);
  tok = o->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);

  return client->fpgaObjectGetName(o->object_id, name, max_len);
}
