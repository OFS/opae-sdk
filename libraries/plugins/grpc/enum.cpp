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

#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sstream>

#include <opae/types.h>
#include <opae/log.h>

#include "comms.h"
#include "remote.h"
#include "mock/opae_std.h"

#include "grpc_client.hpp"


#define OPAE_ENUM_STOP 1
#define OPAE_ENUM_CONTINUE 0

extern "C" {
int opae_plugin_mgr_for_each_remote
	(int (*callback)(opae_comms_channel *, void *),
	void *context);
}

struct _remote_token *
opae_create_remote_token(fpga_token_header *hdr,
			 opae_comms_channel *comms)
{
	struct _remote_token *t =
		(struct _remote_token *)opae_calloc(1, sizeof(*t));
	if (t) {
		t->hdr = *hdr;
		t->comms = comms;
	}
	return t;
}

void opae_destroy_remote_token(struct _remote_token *t)
{
	opae_free(t);
}

typedef struct _remote_enumeration_context {
	// <verbatim from fpgaEnumerate>
	std::vector<fpga_properties> filters;
	uint32_t num_filters;
	fpga_token *tokens;
	uint32_t max_tokens;
	uint32_t *num_matches;
	// </verbatim from fpgaEnumerate>

	uint32_t num_tokens;
	uint32_t errors;
} remote_enumeration_context;

static int remote_enumerate(opae_comms_channel *comms, void *context)
{
	remote_enumeration_context *ctx =
		(remote_enumeration_context *)context;

	uint32_t space_remaining;
  uint32_t num_matches = 0;
  uint32_t i;

  space_remaining = ctx->max_tokens - ctx->num_tokens;

	if (ctx->tokens && !space_remaining)
		return OPAE_ENUM_STOP;

	// Establish the remote connection.
	std::ostringstream target;
	target << comms->server_host << ':' << comms->server_port;

  OPAEClient *client = new OPAEClient(grpc::CreateChannel(
			target.str(), grpc::InsecureChannelCredentials()));

  comms->client = client;

  std::vector<fpga_token_header> token_hdrs;

  if (client->fpgaEnumerate(ctx->filters, ctx->num_filters,
	                          space_remaining, num_matches,
                            token_hdrs)) {
		++ctx->errors;
		return OPAE_ENUM_STOP;
	}

	*ctx->num_matches += num_matches;

	if (!ctx->tokens) {
		// requesting token count only.
		return OPAE_ENUM_CONTINUE;
	}

	if (space_remaining > num_matches)
		space_remaining = num_matches;

	for (i = 0 ; i < space_remaining ; ++i) {
		struct _remote_token *token =
			opae_create_remote_token(&token_hdrs[i], comms);

		if (!token) {
			++ctx->errors;
			return OPAE_ENUM_STOP;
		}

    if (!client->find_token(token_hdrs[i].token_id))
		  client->add_token(token_hdrs[i].token_id, token);

		ctx->tokens[ctx->num_tokens++] = token;
	}

	return ctx->num_tokens == ctx->max_tokens
			? OPAE_ENUM_STOP
			: OPAE_ENUM_CONTINUE;
}

fpga_result __REMOTE_API__ remote_fpgaEnumerate(const fpga_properties *filters,
				       uint32_t num_filters, fpga_token *tokens,
				       uint32_t max_tokens,
				       uint32_t *num_matches)
{
	fpga_result result = FPGA_NOT_FOUND;
	remote_enumeration_context enum_context;

	if (NULL == num_matches) {
		OPAE_MSG("num_matches is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* requiring a max number of tokens, but not providing a pointer to
	 * return them through is invalid */
	if ((max_tokens > 0) && (NULL == tokens)) {
		OPAE_MSG("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters > 0) && (NULL == filters)) {
		OPAE_MSG("num_filters > 0 with NULL filters");
		return FPGA_INVALID_PARAM;
	}

	if (!num_filters && (NULL != filters)) {
		OPAE_MSG("num_filters == 0 with non-NULL filters");
		return FPGA_INVALID_PARAM;
	}

	*num_matches = 0;

  enum_context.filters.reserve(num_filters);
	enum_context.filters.resize(num_filters);
  std::copy(filters, filters + num_filters, enum_context.filters.begin());
	enum_context.num_filters = num_filters;
	enum_context.tokens = tokens;
	enum_context.max_tokens = max_tokens;
	enum_context.num_matches = num_matches;
	enum_context.num_tokens = 0;
	enum_context.errors = 0;

	// perform the enumeration.
	opae_plugin_mgr_for_each_remote(remote_enumerate, &enum_context);

	result = (enum_context.errors > 0) ? FPGA_EXCEPTION : FPGA_OK;

	return result;
}

fpga_result __REMOTE_API__ remote_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
#if 1
(void) src;
(void) dst;

return FPGA_OK;
#else
	opae_fpgaCloneToken_request req;
	opae_fpgaCloneToken_response resp;
	struct _remote_token *tok;
	char *req_json;
	struct _remote_token *_dst;
	char *resp_json = NULL;
	fpga_result res;

	if (!src || !dst) {
		OPAE_ERR("src or dst token is NULL");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)src;

	req.src_token_id = tok->hdr.token_id;

	req_json = opae_encode_fpgaCloneToken_request_2(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaCloneToken_response_2(resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		_dst = opae_create_remote_token(&resp.dest_token,
						tok->comms,
						tok->json_to_string_flags);

		if (!_dst)
			return FPGA_NO_MEMORY;

		*dst = _dst;
	}

	return resp.result;
#endif
}

fpga_result __REMOTE_API__ remote_fpgaDestroyToken(fpga_token *token)
{
#if 1
(void) token;

return FPGA_OK;
#else
	opae_fpgaDestroyToken_request req;
	opae_fpgaDestroyToken_response resp;
	struct _remote_token *tok;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!token || !(*token)) {
		OPAE_MSG("Invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	tok = (struct _remote_token *)*token;

	req.token_id = tok->hdr.token_id;

	req_json = opae_encode_fpgaDestroyToken_request_1(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaDestroyToken_response_1(resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		opae_destroy_remote_token(tok);
		*token = NULL;
	}

	return resp.result;
#endif
}
