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
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <opae/types.h>

#include "remote.h"
//#include "request.h"
//#include "response.h"

#include "mock/opae_std.h"

fpga_result __REMOTE_API__ remote_fpgaClose(fpga_handle handle)
{
#if 1
(void) handle;

return FPGA_OK;
#else
	opae_fpgaClose_request req;
	opae_fpgaClose_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	req.handle_id = h->hdr.handle_id;

	req_json = opae_encode_fpgaClose_request_6(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaClose_response_6(resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		opae_destroy_remote_handle(h);

	return resp.result;
#endif
}
