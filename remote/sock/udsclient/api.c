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

#include "mock/opae_std.h"
#include "request.h"
#include "response.h"

#include "api.h"
#include "udsclient.h"

#define RECEIVE_BUF_SIZE (8 * 1024)

extern uds_client_context client;

int json_to_string_flags = JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;

fpga_result remote_fpgaEnumerate(const fpga_properties *filters,
				 uint32_t num_filters, fpga_token *tokens,
				 uint32_t max_tokens, uint32_t *num_matches)
{
	opae_fpgaEnumerate_request req;
	opae_fpgaEnumerate_response resp;
	char *json;
	char buf[RECEIVE_BUF_SIZE];

	req.filters = (fpga_properties *)filters;
	req.num_filters = num_filters;
	req.max_tokens = max_tokens;

	json = opae_encode_fpgaEnumerate_request_0(&req, json_to_string_flags);

	send(client.client_socket, json, strlen(json) + 1, 0);

	opae_free(json);

	recv(client.client_socket, buf, sizeof(buf), 0);

	printf("%s\n", buf);

	opae_decode_fpgaEnumerate_response_0(buf, &resp);

	*num_matches = resp.num_matches;


(void) tokens;

	if (resp.tokens)
		opae_free(resp.tokens);

	return resp.result;
}
