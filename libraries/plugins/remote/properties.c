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
#include <opae/properties.h>

#include "props.h"

#include "request.h"
#include "response.h"
#include "remote.h"

#include "mock/opae_std.h"

fpga_result __REMOTE_API__
remote_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	opae_fpgaGetPropertiesFromHandle_request req;
	opae_fpgaGetPropertiesFromHandle_response resp;
	struct _remote_handle *h;
	struct _remote_token *tok;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!prop) {
		OPAE_ERR("NULL properties pointer");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	req.handle_id = h->hdr.handle_id;

	req_json = opae_encode_fpgaGetPropertiesFromHandle_request_8(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaGetPropertiesFromHandle_response_8(
		resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*prop = resp.properties;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	fpga_result result = FPGA_OK;
	struct _fpga_properties *_prop = NULL;

	ASSERT_NOT_NULL(prop);

	result = fpgaGetProperties(NULL, (fpga_properties *)&_prop);

	ASSERT_RESULT(result);

	if (token) {
		result = remote_fpgaUpdateProperties(token, _prop);
		if (result != FPGA_OK)
			goto out_free;
	}

	*prop = (fpga_properties)_prop;
	return result;

out_free:
	opae_free(_prop);
	return result;
}

fpga_result __REMOTE_API__
remote_fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	opae_fpgaUpdateProperties_request req;
	opae_fpgaUpdateProperties_response resp;
	struct _remote_token *tok;
	char *req_json;
	char *resp_json = NULL;
	fpga_result result;
	struct _fpga_properties *p;
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

	tok = (struct _remote_token *)token;

	req.token_id = tok->hdr.token_id;

	req_json = opae_encode_fpgaUpdateProperties_request_4(
		&req, tok->json_to_string_flags);

	result = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (result)
		return result;

	if (!opae_decode_fpgaUpdateProperties_response_4(resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		p = opae_validate_and_lock_properties(prop);
		if (!p) {
			fpgaDestroyProperties(&resp.properties);
			return FPGA_INVALID_PARAM;
		}

		save_lock = p->lock;

		*p = *(struct _fpga_properties *)resp.properties;

		p->lock = save_lock;

		opae_mutex_unlock(res, &p->lock);
	}

	return resp.result;
}
