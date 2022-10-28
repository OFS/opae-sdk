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

#include "cfg-file.h"
#include "response.h"
#include "opae_int.h"
#include "props.h"
#include "mock/opae_std.h"

STATIC bool opae_add_response_header_obj(struct json_object *root,
					 opae_response_header *resphdr)
{
	struct json_object *header;

	header = json_object_new_object();

	json_object_object_add(header,
			       "request_id",
			       json_object_new_int(resphdr->request_id));

	json_object_object_add(header,
			       "request_name",
			       json_object_new_string(resphdr->request_name));

	json_object_object_add(header,
			       "response_name",
			       json_object_new_string(resphdr->response_name));

	json_object_object_add(header,
			       "serial",
			       json_object_new_int(resphdr->serial));

	json_object_object_add(header,
			       "from",
			       json_object_new_string(resphdr->from));

	json_object_object_add(header,
			       "to",
			       json_object_new_string(resphdr->to));

	json_object_object_add(root, "header", header);

	return true;
}

/*
{
    "header": { ... },
    "tokens": [
        {
            <token 0>
	},
	{
            <token 1>
	}
    ],
    "num_matches": 3,
    "fpga_result": "FPGA_OK"
}
*/
char *opae_encode_fpgaEnumerate_response_0(opae_fpgaEnumerate_response *resp,
                                           int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtokens;
	size_t i;
	size_t num_tokens;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jtokens = json_object_new_array();

	num_tokens = resp->max_tokens;
	if (num_tokens > resp->num_matches)
		num_tokens = resp->num_matches;

	for (i = 0 ; i < num_tokens ; ++i) {
		struct json_object *array_i =
			json_object_new_object();

		if (!opae_ser_token_header_to_json_obj(
			&resp->tokens[i], array_i))
			goto out_err;

		json_object_array_put_idx(jtokens, i, array_i);
	}

	json_object_object_add(root, "tokens", jtokens);

	json_object_object_add(root,
			       "num_matches",
			       json_object_new_int(resp->num_matches));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

STATIC bool
opae_decode_response_header_obj(struct json_object *root,
				opae_response_header *header)
{
	struct json_object *jheader = NULL;
	char *str = NULL;

	memset(header, 0, sizeof(opae_response_header));

        if (!json_object_object_get_ex(root, "header", &jheader)) {
                OPAE_DBG("Error parsing JSON: missing 'header'");
                return false;
        }

	if (!parse_json_u64(jheader, "request_id", &header->request_id))
		return false;

	if (!parse_json_string(jheader, "request_name", &str))
		return false;
	memcpy(header->request_name, str, strlen(str));

	str = NULL;
	if (!parse_json_string(jheader, "response_name", &str))
		return false;
	memcpy(header->response_name, str, strlen(str));

	if (!parse_json_u64(jheader, "serial", &header->serial))
		return false;

	str = NULL;
	if (!parse_json_string(jheader, "from", &str))
		return false;
	memcpy(header->from, str, strlen(str));

	str = NULL;
	if (!parse_json_string(jheader, "to", &str))
		return false;
	memcpy(header->to, str, strlen(str));

	return true;
}

bool opae_decode_fpgaEnumerate_response_0(const char *json,
					  opae_fpgaEnumerate_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jtokens = NULL;
	size_t i;
	size_t len;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "tokens", &jtokens)) {
                OPAE_DBG("Error parsing JSON: missing 'tokens'");
                goto out_put;
        }

	len = json_object_array_length(jtokens);

	resp->num_matches = 0;
	resp->tokens = NULL;
	if (len) {
		resp->tokens = opae_calloc(len, sizeof(fpga_token_header));
		if (!resp->tokens) {
			OPAE_ERR("calloc failed");
			goto out_put;
		}
	}

	for (i = 0 ; i < len ; ++i) {
		struct json_object *array_i =
			json_object_array_get_idx(jtokens, i);

		if (!opae_ser_json_to_token_header_obj(array_i,
			&resp->tokens[i]))
			goto out_put;
	}

	if (!parse_json_u32(root, "num_matches", &resp->num_matches))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root,
		&resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

/*
{
    "header": { ... },
    "fpga_result": "FPGA_OK"
}
*/
char *opae_encode_fpgaDestroyToken_response_1(opae_fpgaDestroyToken_response *resp,
					      int json_flags)
{
	struct json_object *root;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaDestroyToken_response_1(const char *json,
                                             opae_fpgaDestroyToken_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaCloneToken_response_2(opae_fpgaCloneToken_response *resp,
                                            int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *dest_token;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	dest_token = json_object_new_object();
	if (!dest_token) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&resp->dest_token, dest_token))
		goto out_err;

	json_object_object_add(root, "dest_token", dest_token);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaCloneToken_response_2(const char *json,
                                           opae_fpgaCloneToken_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jdest_token;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "dest_token", &jdest_token)) {
                OPAE_DBG("Error parsing JSON: missing 'dest_token'");
                goto out_put;
        }

	if (!opae_ser_json_to_token_header_obj(jdest_token, &resp->dest_token))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetProperties_response_3(opae_fpgaGetProperties_response *resp,
                                               int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jproperties;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (resp->properties) {
		jproperties = json_object_new_object();
		if (!jproperties) {
			OPAE_ERR("out of memory");
			goto out_err;
		}

		if (!opae_ser_properties_to_json_obj(resp->properties,
						     jproperties))
			goto out_err;

		json_object_object_add(root, "properties", jproperties);
	}

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetProperties_response_3(const char *json,
                                              opae_fpgaGetProperties_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jproperties = NULL;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	resp->properties = NULL;
	if (json_object_object_get_ex(root, "properties", &jproperties)) {
		if (!opae_ser_json_to_properties_obj(jproperties,
			&resp->properties))
			goto out_put;
        }

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaUpdateProperties_response_4(opae_fpgaUpdateProperties_response *resp,
						  int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jproperties;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (resp->properties) {
		jproperties = json_object_new_object();
		if (!jproperties) {
			OPAE_ERR("out of memory");
			goto out_err;
		}

		if (!opae_ser_properties_to_json_obj(resp->properties,
						     jproperties))
			goto out_err;

		json_object_object_add(root, "properties", jproperties);
	}

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaUpdateProperties_response_4(const char *json,
						 opae_fpgaUpdateProperties_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jproperties = NULL;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	resp->properties = NULL;
	if (json_object_object_get_ex(root, "properties", &jproperties)) {
		if (!opae_ser_json_to_properties_obj(jproperties,
			&resp->properties))
			goto out_put;
        }

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaOpen_response_5(opae_fpgaOpen_response *resp,
				      int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&resp->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaOpen_response_5(const char *json,
                                     opae_fpgaOpen_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jhandle = NULL;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_response_header_obj(root, &resp->header)) {
		OPAE_ERR("response header decode failed");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

	if (!opae_ser_json_to_handle_header_obj(jhandle, &resp->handle))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}
