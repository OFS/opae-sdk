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

#include "mock/opae_std.h"

#include "remote.h"
#include "request.h"
#include "response.h"

fpga_result __REMOTE_API__
remote_fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value)
{
	opae_fpgaReadError_request req;
	opae_fpgaReadError_response resp;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!token) {
		OPAE_ERR("NULL token");
		return FPGA_INVALID_PARAM;
	}

	if (!value) {
		OPAE_ERR("NULL value pointer");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)token;
	req.token_id = tok->hdr.token_id;
	req.error_num = error_num;

	req_json = opae_encode_fpgaReadError_request_19(
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

	if (!opae_decode_fpgaReadError_response_19(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*value = resp.value;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaClearError(fpga_token token, uint32_t error_num)
{
	opae_fpgaClearError_request req;
	opae_fpgaClearError_response resp;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!token) {
		OPAE_ERR("NULL token");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)token;
	req.token_id = tok->hdr.token_id;
	req.error_num = error_num;

	req_json = opae_encode_fpgaClearError_request_21(
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

	if (!opae_decode_fpgaClearError_response_21(recvbuf, &resp))
		return FPGA_EXCEPTION;

	return resp.result;
}

fpga_result __REMOTE_API__ remote_fpgaClearAllErrors(fpga_token token)
{
	opae_fpgaClearAllErrors_request req;
	opae_fpgaClearAllErrors_response resp;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!token) {
		OPAE_ERR("NULL token");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)token;
	req.token_id = tok->hdr.token_id;

	req_json = opae_encode_fpgaClearAllErrors_request_22(
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

	if (!opae_decode_fpgaClearAllErrors_response_22(recvbuf, &resp))
		return FPGA_EXCEPTION;

	return resp.result;
}

fpga_result __REMOTE_API__ remote_fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info)
{
	opae_fpgaGetErrorInfo_request req;
	opae_fpgaGetErrorInfo_response resp;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!token) {
		OPAE_ERR("NULL token");
		return FPGA_INVALID_PARAM;
	}

	if (!error_info) {
		OPAE_ERR("NULL error_info pointer");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)token;
	req.token_id = tok->hdr.token_id;
	req.error_num = error_num;

	req_json = opae_encode_fpgaGetErrorInfo_request_20(
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

	if (!opae_decode_fpgaGetErrorInfo_response_20(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*error_info = resp.error_info;

	return resp.result;
}
