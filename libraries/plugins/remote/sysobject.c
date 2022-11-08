// Copyright(c) 2022, Intel Corporation
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
#endif // HAVE_CONFIG_H

#include <opae/types.h>
#include <opae/log.h>

#include "remote.h"
#include "request.h"
#include "response.h"

#include "mock/opae_std.h"

struct _remote_sysobject *
opae_create_remote_sysobject(struct _remote_token *token,
			     fpga_remote_id *rid)
{
	struct _remote_sysobject *s =
		(struct _remote_sysobject *)opae_calloc(1, sizeof(*s));
	if (s) {
		s->token = token;
		s->object_id = *rid;
	}
	return s;
}

void opae_destroy_remote_sysobject(struct _remote_sysobject *s)
{
	opae_free(s);
}

fpga_result __REMOTE_API__
remote_fpgaTokenGetObject(fpga_token token,
			  const char *name,
			  fpga_object *object,
			  int flags)
{
	opae_fpgaTokenGetObject_request req;
	opae_fpgaTokenGetObject_response resp;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!token) {
		OPAE_ERR("NULL token");
		return FPGA_INVALID_PARAM;
	}

	if (!object) {
		OPAE_ERR("NULL object pointer");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)token;
	req.token = tok->hdr;

	memset(req.name, 0, sizeof(req.name));
	len = strnlen(name, OPAE_SYSOBJECT_NAME_MAX);
	memcpy(req.name, name, len + 1);

	req.flags = flags;

	req_json = opae_encode_fpgaTokenGetObject_request_23(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaTokenGetObject_response_23(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		struct _remote_sysobject *o =
			opae_create_remote_sysobject(tok, &resp.object_id);

		if (!o) {
			OPAE_ERR("calloc failed");
			*object = NULL;
			return FPGA_NO_MEMORY;
		}

		*object = o;
	}

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaHandleGetObject(fpga_handle handle,
			   const char *name,
			   fpga_object *object,
			   int flags)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) name;
(void) object;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetObject(fpga_object parent,
			   const char *name,
			   fpga_object *object,
			   int flags)
{
	fpga_result result = FPGA_OK;
(void) parent;
(void) name;
(void) object;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaCloneObject(fpga_object src, fpga_object *dst)
{
	fpga_result result = FPGA_OK;
(void) src;
(void) dst;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetObjectAt(fpga_object parent,
			     size_t idx,
			     fpga_object *object)
{
	fpga_result result = FPGA_OK;
(void) parent;
(void) idx;
(void) object;





	return result;
}

fpga_result __REMOTE_API__ remote_fpgaDestroyObject(fpga_object *obj)
{
	opae_fpgaDestroyObject_request req;
	opae_fpgaDestroyObject_response resp;
	struct _remote_sysobject *o;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!obj || !*obj) {
		OPAE_ERR("invalid sysobject pointer");
		return FPGA_INVALID_PARAM;
	}

	o = (struct _remote_sysobject *)*obj;
	tok = o->token;

	req.object_id = o->object_id;

	req_json = opae_encode_fpgaDestroyObject_request_24(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaDestroyObject_response_24(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*obj = NULL;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetSize(fpga_object obj,
			 uint32_t *size,
			 int flags)
{
	opae_fpgaObjectGetSize_request req;
	opae_fpgaObjectGetSize_response resp;
	struct _remote_sysobject *o;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!obj) {
		OPAE_ERR("NULL obj");
		return FPGA_INVALID_PARAM;
	}

	if (!size) {
		OPAE_ERR("NULL size pointer");
		return FPGA_INVALID_PARAM;
	}

	o = (struct _remote_sysobject *)obj;
	tok = o->token;

	req.object_id = o->object_id;
	req.flags = flags;

	req_json = opae_encode_fpgaObjectGetSize_request_27(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaObjectGetSize_response_27(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*size = resp.value;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectRead64(fpga_object obj,
			uint64_t *value,
			int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) value;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectRead(fpga_object obj,
		      uint8_t *buffer,
		      size_t offset,
		      size_t len,
		      int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) buffer;
(void) offset;
(void) len;
(void) flags;




	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectWrite64(fpga_object obj,
			 uint64_t value,
			 int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) value;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetType(fpga_object obj, enum fpga_sysobject_type *type)
{
	opae_fpgaObjectGetType_request req;
	opae_fpgaObjectGetType_response resp;
	struct _remote_sysobject *o;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!obj) {
		OPAE_ERR("NULL obj");
		return FPGA_INVALID_PARAM;
	}

	if (!type) {
		OPAE_ERR("NULL type pointer");
		return FPGA_INVALID_PARAM;
	}

	o = (struct _remote_sysobject *)obj;
	tok = o->token;

	req.object_id = o->object_id;

	req_json = opae_encode_fpgaObjectGetType_request_25(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaObjectGetType_response_25(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*type = resp.type;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetName(fpga_object obj, char *name, size_t max_len)
{
	opae_fpgaObjectGetName_request req;
	opae_fpgaObjectGetName_response resp;
	struct _remote_sysobject *o;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!obj) {
		OPAE_ERR("NULL obj");
		return FPGA_INVALID_PARAM;
	}

	if (!name) {
		OPAE_ERR("NULL name");
		return FPGA_INVALID_PARAM;
	}

	o = (struct _remote_sysobject *)obj;
	tok = o->token;

	req.object_id = o->object_id;

	req_json = opae_encode_fpgaObjectGetName_request_26(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaObjectGetName_response_26(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		len = strnlen(resp.name, max_len - 1);
		memcpy(name, resp.name, len + 1);
	}

	return resp.result;
}
