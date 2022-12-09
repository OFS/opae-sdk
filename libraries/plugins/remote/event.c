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

#include "mock/opae_std.h"
#include "remote.h"
#include "request.h"
#include "response.h"

enum request_type { REGISTER_EVENT = 0, UNREGISTER_EVENT = 1 };

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};

struct _remote_event_handle *
opae_create_remote_event_handle(void)
{
	return (struct _remote_event_handle *)
		opae_calloc(1, sizeof(struct _remote_event_handle));
}

void opae_destroy_remote_event_handle(struct _remote_event_handle *eh)
{
	opae_free(eh);
}

fpga_result __REMOTE_API__
remote_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	if (!event_handle) {
		OPAE_ERR("NULL event_handle pointer");
		return FPGA_INVALID_PARAM;
	}

	// We don't have a token, nor a remoting interface
	// at this point. Just create and return an empty
	// _remote_event_handle struct.

	*event_handle = opae_create_remote_event_handle();
	if (!*event_handle) {
		OPAE_ERR("calloc() failed");
		return FPGA_NO_MEMORY;
	}

	return FPGA_OK;
}

fpga_result __REMOTE_API__
remote_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	struct _remote_event_handle *eh;
	fpga_result res = FPGA_OK;

	if (!event_handle || !*event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	eh = *event_handle;

	if (eh->handle) {
		opae_fpgaDestroyEventHandle_request req;
		opae_fpgaDestroyEventHandle_response resp;
		struct _remote_token *tok;
		struct _remote_handle *h;
		char *req_json;
		char *resp_json = NULL;

		h = eh->handle;
		tok = h->token;

		req.eh_id = eh->eh_id;

		req_json = opae_encode_fpgaDestroyEventHandle_request_51(
			&req, tok->json_to_string_flags);

		res = opae_client_send_and_receive(tok, req_json, &resp_json);
		if (res)
			goto out_destroy;

		if (!opae_decode_fpgaDestroyEventHandle_response_51(
			resp_json, &resp)) {
			res = FPGA_EXCEPTION;
			goto out_destroy;
		}

		res = resp.result;
	}

out_destroy:
	opae_destroy_remote_event_handle(eh);
	*event_handle = NULL;

	return res;
}

fpga_result __REMOTE_API__
remote_fpgaGetOSObjectFromEventHandle(
	const fpga_event_handle event_handle,
	int *fd)
{
	opae_fpgaGetOSObjectFromEventHandle_request req;
	opae_fpgaGetOSObjectFromEventHandle_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	struct _remote_event_handle *eh;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	if (!fd) {
		OPAE_ERR("NULL fd pointer");
		return FPGA_INVALID_PARAM;
	}

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle) {
		OPAE_ERR("You must call fpgaRegisterEvent() prior "
			 "to requesting the OS Object.");
		return FPGA_INVALID_PARAM;
	}

	h = eh->handle;
	tok = h->token;

	req.eh_id = eh->eh_id;

	req_json = opae_encode_fpgaGetOSObjectFromEventHandle_request_50(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaGetOSObjectFromEventHandle_response_50(
		resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*fd = resp.fd;

	return resp.result;
}

fpga_result __REMOTE_API__ remote_fpgaRegisterEvent(fpga_handle handle,
						 fpga_event_type event_type,
						 fpga_event_handle event_handle,
						 uint32_t flags)
{
	opae_fpgaRegisterEvent_request req;
	opae_fpgaRegisterEvent_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	struct _remote_event_handle *eh;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle) {
		// We were created by fpgaCreateEventHandle(), but
		// that function has no handle/token nor a remoting
		// interface. Do what fpgaCreateEventHandle() would
		// have done, if it had a remoting interface.
		opae_fpgaCreateEventHandle_request create_req;
		opae_fpgaCreateEventHandle_response create_resp;

		eh->handle = h;

		req_json = opae_encode_fpgaCreateEventHandle_request_47(
				&create_req, tok->json_to_string_flags);

		res = opae_client_send_and_receive(tok, req_json, &resp_json);
		if (res)
			return res;

		if (!opae_decode_fpgaCreateEventHandle_response_47(
			resp_json, &create_resp))
			return FPGA_EXCEPTION;

		if (create_resp.result != FPGA_OK)
			return create_resp.result;

		eh->eh_id = create_resp.eh_id;
	}

	req.handle_id = h->hdr.handle_id;
	req.event_type = event_type;
	req.eh_id = eh->eh_id;
	req.flags = flags;

	req_json = opae_encode_fpgaRegisterEvent_request_48(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaRegisterEvent_response_48(resp_json, &resp))
		return FPGA_EXCEPTION;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
			   fpga_event_handle event_handle)
{
	opae_fpgaUnregisterEvent_request req;
	opae_fpgaUnregisterEvent_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	struct _remote_event_handle *eh;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle) {
		OPAE_ERR("no event registered");
		return FPGA_INVALID_PARAM;
	}

	if (eh->handle != h) {
		OPAE_ERR("handle / event_handle mismatch");
		return FPGA_INVALID_PARAM;
	}

	req.handle_id = h->hdr.handle_id;
	req.event_type = event_type;
	req.eh_id = eh->eh_id;

	req_json = opae_encode_fpgaUnregisterEvent_request_49(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaUnregisterEvent_response_49(resp_json, &resp))
		return FPGA_EXCEPTION;

	return resp.result;
}
