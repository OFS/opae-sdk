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

#define _GNU_SOURCE
#include <pthread.h>

#include <opae/fpga.h>

#include "action.h"
#include "props.h"
#include "mock/opae_std.h"

/*
STATIC uint32_t
opae_u64_key_hash(uint32_t num_buckets, uint32_t hash_seed, void *key)
{
	UNUSED_PARAM(hash_seed);
	uint64_t remote_id = (uint64_t)key;
	uint64_t hash = remote_id % 17659;
	return (uint32_t)(hash % num_buckets);
}
*/

STATIC int
opae_str_key_compare(void *keya, void *keyb)
{
	const char *stra = (const char *)keya;
	const char *strb = (const char *)keyb;
	return strncmp(stra, strb, OPAE_MAX_TOKEN_HASH);
}

STATIC void opae_str_key_cleanup(void *key)
{
	opae_free(key);
}

STATIC int opae_remote_id_to_hash_key(const fpga_remote_id *rid,
				      char *buf,
				      size_t len)
{
	return snprintf(buf, len,
			"0x%016" PRIx64 "@%s",
			rid->unique_id,
			rid->hostname);
}

STATIC char *opae_remote_id_to_hash_key_alloc(const fpga_remote_id *rid)
{
	char buf[OPAE_MAX_TOKEN_HASH];

	if (opae_remote_id_to_hash_key(rid, buf, sizeof(buf)) >=
		OPAE_MAX_TOKEN_HASH) {
		OPAE_ERR("snprintf buffer overflow");
		return NULL;
	}

	return opae_strdup(buf);
}

fpga_result opae_init_remote_context(opae_remote_context *c)
{
	fpga_result res;

	c->json_to_string_flags = JSON_C_TO_STRING_SPACED |
				  JSON_C_TO_STRING_PRETTY;

	res = opae_hash_map_init(&c->remote_id_to_token_map,
				 1024, /* num_buckets   */
                                 0,    /* hash_seed     */
				 murmur3_32_string_hash,
				 opae_str_key_compare,
				 opae_str_key_cleanup,
				 NULL  /* value_cleanup */);
	if (res)
		return res;

	res = opae_hash_map_init(&c->remote_id_to_handle_map,
				 1024, /* num_buckets   */
                                 0,    /* hash_seed     */
				 murmur3_32_string_hash,
				 opae_str_key_compare,
				 opae_str_key_cleanup,
				 NULL  /* value_cleanup */);
	if (res)
		return res;


	return FPGA_OK;
}

fpga_result opae_release_remote_context(opae_remote_context *c)
{
	opae_hash_map_destroy(&c->remote_id_to_token_map);
	opae_hash_map_destroy(&c->remote_id_to_handle_map);
	return FPGA_OK;
}

STATIC void request_header_to_response_header(opae_request_header *reqhdr,
					      opae_response_header *resphdr,
					      const char *response_name)
{
	resphdr->request_id = reqhdr->request_id;
	memcpy(resphdr->request_name, reqhdr->request_name,
		sizeof(resphdr->request_name));
	memcpy(resphdr->response_name, response_name,
		strlen(response_name) + 1);
	resphdr->serial = reqhdr->serial;
	opae_get_host_name_buf(resphdr->from, HOST_NAME_MAX);
	memcpy(resphdr->to, reqhdr->from, sizeof(resphdr->to));
}

bool opae_handle_fpgaEnumerate_request_0(opae_remote_context *c,
					 const char *req_json,
					 char **resp_json)
{
	bool res = false;
	uint32_t i;
	fpga_token *tokens = NULL;
	opae_fpgaEnumerate_request req;
	opae_fpgaEnumerate_response resp;

	if (!opae_decode_fpgaEnumerate_request_0(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaEnumerate request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaEnumerate_response_0");

	resp.num_matches = 0;
	resp.tokens = NULL;

	if (req.max_tokens) {
		resp.tokens = opae_calloc(req.max_tokens,
				sizeof(fpga_token_header));
		if (!resp.tokens) {
			OPAE_ERR("calloc failed");
			goto out_cleanup;
		}

		tokens = opae_calloc(req.max_tokens, sizeof(fpga_token));
		if (!tokens) {
			OPAE_ERR("calloc failed");
			goto out_cleanup;
		}
	}

	resp.result = fpgaEnumerate(req.filters,
				    req.num_filters,
				    tokens,
				    req.max_tokens,
				    &resp.num_matches);
	resp.max_tokens = req.max_tokens;

	if (tokens) {
		// Walk through each token, peek inside, and grab its header.
		for (i = 0 ; i < resp.num_matches ; ++i) {
			char *hash_key;
			fpga_result result;
			opae_wrapped_token *wt =
				(opae_wrapped_token *)tokens[i];
			fpga_token_header *hdr =
				(fpga_token_header *)wt->opae_token;

			// Place tokens[i] in a map structure keyed by
			// remote_id@hostname from the token header.
			hash_key =
				opae_remote_id_to_hash_key_alloc(
					&hdr->token_id);
			if (!hash_key) {
				OPAE_ERR("strdup failed");
				goto out_cleanup;
			}

			// If we don't have an entry already.
			if (opae_hash_map_find(&c->remote_id_to_token_map,
					       hash_key,
					       NULL)) {

				result = opae_hash_map_add(
						&c->remote_id_to_token_map,
						hash_key, 
						tokens[i]);
				if (result) {
					opae_str_key_cleanup(hash_key);
					goto out_cleanup;
				}

			} else { // tokens[i] is already mapped.
				opae_str_key_cleanup(hash_key);
			}

			if (i < req.max_tokens)
				resp.tokens[i] = *hdr;
		}
	}

	*resp_json = opae_encode_fpgaEnumerate_response_0(
			&resp,
			c->json_to_string_flags);

	res = true;

out_cleanup:
	if (tokens)
		opae_free(tokens);

	if (resp.tokens)
		opae_free(resp.tokens);

	if (req.filters) {
		for (i = 0 ; i < req.num_filters ; ++i)
			fpgaDestroyProperties(&req.filters[i]);
		opae_free(req.filters);
	}

	return res;
}

bool opae_handle_fpgaDestroyToken_request_1(opae_remote_context *c,
					    const char *req_json,
					    char **resp_json)
{
	bool res = false;
	opae_fpgaDestroyToken_request req;
	opae_fpgaDestroyToken_response resp;
	char hash_key[OPAE_MAX_TOKEN_HASH];
	fpga_token token = NULL;

	if (!opae_decode_fpgaDestroyToken_request_1(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaDestroyToken request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaDestroyToken_response_1");
	resp.result = FPGA_INVALID_PARAM;

	opae_remote_id_to_hash_key(&req.token.token_id,
				   hash_key,
				   sizeof(hash_key));

	// Find the token in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_token_map,
				hash_key,
				&token) != FPGA_OK) {
		OPAE_ERR("token lookup failed for %s", hash_key);
		*resp_json = opae_encode_fpgaDestroyToken_response_1(
				&resp,
				c->json_to_string_flags);
		goto out;
	}

	// Remove token.
	opae_hash_map_remove(&c->remote_id_to_token_map, hash_key);

	// Destroy token.
	resp.result = fpgaDestroyToken(&token);

	*resp_json = opae_encode_fpgaDestroyToken_response_1(
			&resp,
			c->json_to_string_flags);

	res = true;

out:
	return res;
}

bool opae_handle_fpgaCloneToken_request_2(opae_remote_context *c,
					  const char *req_json,
					  char **resp_json)
{
	bool res = false;
	opae_fpgaCloneToken_request req;
	opae_fpgaCloneToken_response resp;
	char hash_key[OPAE_MAX_TOKEN_HASH];
	fpga_token src_token = NULL;
	fpga_token dest_token = NULL;

	if (!opae_decode_fpgaCloneToken_request_2(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaCloneToken request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaCloneToken_response_2");
	resp.result = FPGA_INVALID_PARAM;
	memset(&resp.dest_token, 0, sizeof(resp.dest_token));

	opae_remote_id_to_hash_key(&req.src_token.token_id,
				   hash_key,
				   sizeof(hash_key));

	// Find the source token in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_token_map,
				hash_key,
				&src_token) != FPGA_OK) {
		OPAE_ERR("token lookup failed for %s", hash_key);
		*resp_json = opae_encode_fpgaCloneToken_response_2(
				&resp,
				c->json_to_string_flags);
		goto out;
	}

	resp.result = fpgaCloneToken(src_token, &dest_token);
	if (resp.result == FPGA_OK) {
		opae_wrapped_token *wt =
			(opae_wrapped_token *)dest_token;
		fpga_token_header *hdr =
			(fpga_token_header *)wt->opae_token;
		char *dest_hash_key;
		fpga_result result;

		resp.dest_token = *hdr;

		dest_hash_key =
			opae_remote_id_to_hash_key_alloc(
				&resp.dest_token.token_id);

		if (!dest_hash_key) {
			fpgaDestroyToken(&dest_token);
			OPAE_ERR("strdup failed");
			goto out;
		}

		result = opae_hash_map_add(
				&c->remote_id_to_token_map,
				dest_hash_key,
				dest_token);
		if (result) {
			fpgaDestroyToken(&dest_token);
			opae_str_key_cleanup(dest_hash_key);
			goto out;
		}

		res = true;
	} else {
		memset(&resp.dest_token, 0, sizeof(resp.dest_token));
	}

out:
	*resp_json = opae_encode_fpgaCloneToken_response_2(
			&resp,
			c->json_to_string_flags);

	return res;
}

bool opae_handle_fpgaGetProperties_request_3(opae_remote_context *c,
                                             const char *req_json,
                                             char **resp_json)
{
	bool res = false;
	opae_fpgaGetProperties_request req;
	opae_fpgaGetProperties_response resp;
	char hash_key[OPAE_MAX_TOKEN_HASH];
	fpga_token token = NULL;

	if (!opae_decode_fpgaGetProperties_request_3(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaGetProperties request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaGetProperties_response_3");

	resp.result = FPGA_INVALID_PARAM;
	resp.properties = NULL;

	opae_remote_id_to_hash_key(&req.token.token_id,
				   hash_key,
				   sizeof(hash_key));

	// Find the token in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_token_map,
				hash_key,
				&token) != FPGA_OK) {
		OPAE_ERR("token lookup failed for %s", hash_key);
		*resp_json = opae_encode_fpgaGetProperties_response_3(
				&resp,
				c->json_to_string_flags);
		goto out_destroy;
	}

	resp.result = fpgaGetProperties(token, &resp.properties);

	*resp_json = opae_encode_fpgaGetProperties_response_3(
			&resp,
			c->json_to_string_flags);

	res = true;

out_destroy:
	if (resp.properties)
		fpgaDestroyProperties(&resp.properties);
	return res;
}

bool opae_handle_fpgaUpdateProperties_request_4(opae_remote_context *c,
						const char *req_json,
						char **resp_json)
{
	bool res = false;
	opae_fpgaUpdateProperties_request req;
	opae_fpgaUpdateProperties_response resp;
	char hash_key[OPAE_MAX_TOKEN_HASH];
	fpga_token token = NULL;

	if (!opae_decode_fpgaUpdateProperties_request_4(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaUpdateProperties request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaUpdateProperties_response_4");

	resp.result = FPGA_INVALID_PARAM;
	resp.properties = NULL;

	if (fpgaGetProperties(NULL, &resp.properties) != FPGA_OK) {
		*resp_json = opae_encode_fpgaUpdateProperties_response_4(
				&resp,
				c->json_to_string_flags);
		goto out_destroy;
	}

	opae_remote_id_to_hash_key(&req.token.token_id,
				   hash_key,
				   sizeof(hash_key));

	// Find the token in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_token_map,
				hash_key,
				&token) != FPGA_OK) {
		OPAE_ERR("token lookup failed for %s", hash_key);
		*resp_json = opae_encode_fpgaUpdateProperties_response_4(
				&resp,
				c->json_to_string_flags);
		goto out_destroy;
	}

	resp.result = fpgaUpdateProperties(token, resp.properties);

	*resp_json = opae_encode_fpgaUpdateProperties_response_4(
			&resp,
			c->json_to_string_flags);

	res = true;

out_destroy:
	if (resp.properties)
		fpgaDestroyProperties(&resp.properties);
	return res;
}

bool opae_handle_fpgaOpen_request_5(opae_remote_context *c,
				    const char *req_json,
				    char **resp_json)
{
	bool res = false;
	opae_fpgaOpen_request req;
	opae_fpgaOpen_response resp;
	char hash_key_buf[OPAE_MAX_TOKEN_HASH];
	fpga_token token = NULL;
	fpga_handle handle = NULL;

	if (!opae_decode_fpgaOpen_request_5(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaOpen request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaOpen_response_5");

	resp.result = FPGA_NOT_FOUND;
	memset(&resp.handle, 0, sizeof(fpga_handle_header));

	opae_remote_id_to_hash_key(&req.token.token_id,
				   hash_key_buf,
				   sizeof(hash_key_buf));

	// Find the token in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_token_map,
				hash_key_buf,
				&token) != FPGA_OK) {
		OPAE_ERR("token lookup failed for %s", hash_key_buf);
		goto out_respond;
	}

	resp.result = fpgaOpen(token, &handle, req.flags);

	if (resp.result == FPGA_OK) {
		opae_wrapped_handle *wh =
			(opae_wrapped_handle *)handle;
		fpga_handle_header *hdr =
			(fpga_handle_header *)wh->opae_handle;
		char *hash_key;
		fpga_result result;

		// Place handle in a map structure keyed by
		// remote_id@hostname from the handle header.

		hash_key =
			opae_remote_id_to_hash_key_alloc(
				&hdr->handle_id);
		if (!hash_key) {
			OPAE_ERR("strdup failed");
			goto out_respond;
		}

		// If we don't have an entry already.
		if (opae_hash_map_find(&c->remote_id_to_handle_map,
				       hash_key,
				       NULL)) {

			result = opae_hash_map_add(
					&c->remote_id_to_handle_map,
					hash_key, 
					handle);
			if (result) {
				opae_str_key_cleanup(hash_key);
				goto out_respond;
			}

		} else { // handle is already mapped.
			opae_str_key_cleanup(hash_key);
		}

		resp.handle = *hdr;
		res = true;
	}

out_respond:
	*resp_json = opae_encode_fpgaOpen_response_5(
			&resp,
			c->json_to_string_flags);
	return res;
}

bool opae_handle_fpgaClose_request_6(opae_remote_context *c,
				     const char *req_json,
				     char **resp_json)
{
	bool res = false;
	opae_fpgaClose_request req;
	opae_fpgaClose_response resp;
	char hash_key_buf[OPAE_MAX_TOKEN_HASH];
	fpga_handle handle = NULL;

	if (!opae_decode_fpgaClose_request_6(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaClose request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaClose_response_6");

	resp.result = FPGA_EXCEPTION;

	opae_remote_id_to_hash_key(&req.handle.handle_id,
				   hash_key_buf,
				   sizeof(hash_key_buf));

	// Find the handle in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_handle_map,
				hash_key_buf,
				&handle) != FPGA_OK) {
		OPAE_ERR("handle lookup failed for %s", hash_key_buf);
		goto out_respond;
	}

	resp.result = fpgaClose(handle);

	// Remove the handle from our remote context object.
	opae_hash_map_remove(&c->remote_id_to_handle_map, hash_key_buf);

	res = true;

out_respond:
	*resp_json = opae_encode_fpgaClose_response_6(
			&resp,
			c->json_to_string_flags);
	return res;
}

bool opae_handle_fpgaReset_request_7(opae_remote_context *c,
				     const char *req_json,
				     char **resp_json)
{
	bool res = false;
	opae_fpgaReset_request req;
	opae_fpgaReset_response resp;
	char hash_key_buf[OPAE_MAX_TOKEN_HASH];
	fpga_handle handle = NULL;

	if (!opae_decode_fpgaReset_request_7(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaReset request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaReset_response_7");

	resp.result = FPGA_EXCEPTION;

	opae_remote_id_to_hash_key(&req.handle.handle_id,
				   hash_key_buf,
				   sizeof(hash_key_buf));

	// Find the handle in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_handle_map,
				hash_key_buf,
				&handle) != FPGA_OK) {
		OPAE_ERR("handle lookup failed for %s", hash_key_buf);
		goto out_respond;
	}

	resp.result = fpgaReset(handle);

	res = true;

out_respond:
	*resp_json = opae_encode_fpgaReset_response_7(
			&resp,
			c->json_to_string_flags);
	return res;
}

bool opae_handle_fpgaGetPropertiesFromHandle_request_8(opae_remote_context *c,
				     const char *req_json,
				     char **resp_json)
{
	bool res = false;
	opae_fpgaGetPropertiesFromHandle_request req;
	opae_fpgaGetPropertiesFromHandle_response resp;
	char hash_key[OPAE_MAX_TOKEN_HASH];
	fpga_handle handle = NULL;

	if (!opae_decode_fpgaGetPropertiesFromHandle_request_8(req_json, &req)) {
		OPAE_ERR("failed to decode fpgaGetPropertiesFromHandle request");
		return false;
	}

	request_header_to_response_header(&req.header,
					  &resp.header,
					  "fpgaGetPropertiesFromHandle_response_8");

	resp.result = FPGA_NOT_FOUND;
	resp.properties = NULL;

	opae_remote_id_to_hash_key(&req.handle.handle_id,
				   hash_key,
				   sizeof(hash_key));

	// Find the handle in our remote context.
	if (opae_hash_map_find(&c->remote_id_to_handle_map,
				hash_key,
				&handle) != FPGA_OK) {
		OPAE_ERR("handle lookup failed for %s", hash_key);
		goto out_respond;
	}

	resp.result = fpgaGetPropertiesFromHandle(handle, &resp.properties);

	res = true;

out_respond:
	*resp_json = opae_encode_fpgaGetPropertiesFromHandle_response_8(
				&resp,
				c->json_to_string_flags);
	if (resp.properties)
		fpgaDestroyProperties(&resp.properties);
	return res;
}
