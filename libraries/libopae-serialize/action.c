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

STATIC uint64_t next_remote_id = 1;
static pthread_mutex_t next_remote_id_lock =
        PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC uint64_t opae_next_remote_id(void)
{
        uint64_t next;
        int res;

        opae_mutex_lock(res, &next_remote_id_lock);
        next = next_remote_id++;
        opae_mutex_unlock(res, &next_remote_id_lock);

        return next;
}

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

STATIC int opae_token_header_to_hash_key(fpga_token_header *hdr,
					 char *buf,
					 size_t len)
{
	return snprintf(buf, len,
			"0x%016" PRIx64 "@%s",
			hdr->remote_id,
			hdr->hostname);
}

STATIC char *opae_token_header_to_hash_key_alloc(fpga_token_header *hdr)
{
	char buf[OPAE_MAX_TOKEN_HASH];

	if (opae_token_header_to_hash_key(hdr, buf, sizeof(buf)) >=
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


	return FPGA_OK;
}

fpga_result opae_release_remote_context(opae_remote_context *c)
{
	opae_hash_map_destroy(&c->remote_id_to_token_map);
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

			if (!hdr->remote_id)
				hdr->remote_id = opae_next_remote_id();

			// Place tokens[i] in a map structure keyed by
			// remote_id@hostname from the token header.
			hash_key = opae_token_header_to_hash_key_alloc(hdr);
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

	opae_token_header_to_hash_key(&req.token,
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

	opae_token_header_to_hash_key(&req.src_token,
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

		if (!hdr->remote_id)
			hdr->remote_id = opae_next_remote_id();

		resp.dest_token = *hdr;

		dest_hash_key =
			opae_token_header_to_hash_key_alloc(&resp.dest_token);

		if (!dest_hash_key) {
			OPAE_ERR("strdup failed");
			goto out;
		}

		result = opae_hash_map_add(
				&c->remote_id_to_token_map,
				dest_hash_key,
				dest_token);
		if (result) {
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

	opae_token_header_to_hash_key(&req.token,
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

	opae_token_header_to_hash_key(&req.token,
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
