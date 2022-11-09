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

char *opae_encode_fpgaClose_response_6(opae_fpgaClose_response *resp,
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

bool opae_decode_fpgaClose_response_6(const char *json,
				      opae_fpgaClose_response *resp)
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

char *opae_encode_fpgaReset_response_7(opae_fpgaReset_response *resp,
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

bool opae_decode_fpgaReset_response_7(const char *json,
				      opae_fpgaReset_response *resp)
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

char *opae_encode_fpgaGetPropertiesFromHandle_response_8(
	opae_fpgaGetPropertiesFromHandle_response *resp,
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

bool opae_decode_fpgaGetPropertiesFromHandle_response_8(
	const char *json,
	opae_fpgaGetPropertiesFromHandle_response *resp)
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

char *opae_encode_fpgaMapMMIO_response_9(opae_fpgaMapMMIO_response *resp,
					 int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jmmio_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jmmio_id = json_object_new_object();
	json_object_object_add(root, "mmio_id", jmmio_id);

	if (!opae_ser_remote_id_to_json_obj(&resp->mmio_id, jmmio_id))
		goto out_err;

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaMapMMIO_response_9(const char *json,
					opae_fpgaMapMMIO_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	struct json_object *jmmio_id = NULL;

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

	if (!json_object_object_get_ex(root, "mmio_id", &jmmio_id)) {
                OPAE_DBG("Error parsing JSON: missing 'mmio_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jmmio_id, &resp->mmio_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaUnmapMMIO_response_10(opae_fpgaUnmapMMIO_response *resp,
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

bool opae_decode_fpgaUnmapMMIO_response_10(const char *json,
					   opae_fpgaUnmapMMIO_response *resp)
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

char *opae_encode_fpgaReadMMIO32_response_11(opae_fpgaReadMMIO32_response *resp,
					     int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (snprintf(buf, sizeof(buf),
		     "0x%08" PRIx32, resp->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "value",
			       json_object_new_string(buf));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadMMIO32_response_11(const char *json,
					    opae_fpgaReadMMIO32_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
	char *endptr;

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

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	resp->value = (uint32_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaReadMMIO32_response decode "
		"failed (value)");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaWriteMMIO32_response_12(opae_fpgaWriteMMIO32_response *resp,
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

bool opae_decode_fpgaWriteMMIO32_response_12(const char *json,
					     opae_fpgaWriteMMIO32_response *resp)
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

char *opae_encode_fpgaReadMMIO64_response_13(opae_fpgaReadMMIO64_response *resp,
					     int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, resp->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "value",
			       json_object_new_string(buf));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadMMIO64_response_13(const char *json,
					    opae_fpgaReadMMIO64_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
	char *endptr;

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

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	resp->value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaReadMMIO64_response decode "
		"failed (value)");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaWriteMMIO64_response_14(opae_fpgaWriteMMIO64_response *resp,
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

bool opae_decode_fpgaWriteMMIO64_response_14(const char *json,
					     opae_fpgaWriteMMIO64_response *resp)
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

char *opae_encode_fpgaWriteMMIO512_response_15(opae_fpgaWriteMMIO512_response *resp,
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

bool opae_decode_fpgaWriteMMIO512_response_15(const char *json,
					      opae_fpgaWriteMMIO512_response *resp)
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

char *opae_encode_fpgaPrepareBuffer_response_16(opae_fpgaPrepareBuffer_response *resp,
						int json_flags)
{
	struct json_object *root;
	struct json_object *jbuf_id;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jbuf_id = json_object_new_object();
	json_object_object_add(root, "buf_id", jbuf_id);

	if (!opae_ser_remote_id_to_json_obj(&resp->buf_id, jbuf_id))
		goto out_err;
	
	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaPrepareBuffer_response_16(const char *json,
					       opae_fpgaPrepareBuffer_response *resp)
{
        struct json_object *root = NULL;
	struct json_object *jbuf_id = NULL;
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

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
                OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &resp->buf_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReleaseBuffer_response_17(opae_fpgaReleaseBuffer_response *resp,
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

bool opae_decode_fpgaReleaseBuffer_response_17(const char *json,
					       opae_fpgaReleaseBuffer_response *resp)
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

char *opae_encode_fpgaGetIOAddress_response_18(opae_fpgaGetIOAddress_response *resp,
					       int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, resp->ioaddr) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "ioaddr",
			       json_object_new_string(buf));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetIOAddress_response_18(const char *json,
					      opae_fpgaGetIOAddress_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
	char *endptr;

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

	str = endptr = NULL;
	if (!parse_json_string(root, "ioaddr", &str))
		goto out_put;

	resp->ioaddr = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaGetIOAddress_response decode "
			 "failed (ioaddr)");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReadError_response_19(opae_fpgaReadError_response *resp,
					    int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, resp->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "value",
			       json_object_new_string(buf));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadError_response_19(const char *json,
					   opae_fpgaReadError_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
	char *endptr;

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

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	resp->value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaReadError_response decode "
			 "failed (value)");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetErrorInfo_response_20(opae_fpgaGetErrorInfo_response *resp,
					       int json_flags)
{
	struct json_object *root;
	struct json_object *jerror_info;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jerror_info = json_object_new_object();
	json_object_object_add(root, "error_info", jerror_info);

	if (!opae_ser_error_info_to_json_obj(&resp->error_info, jerror_info))
		goto out_err;

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetErrorInfo_response_20(const char *json,
					      opae_fpgaGetErrorInfo_response *resp)
{
        struct json_object *root = NULL;
        struct json_object *jerror_info = NULL;
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

	if (!json_object_object_get_ex(root, "error_info", &jerror_info)) {
                OPAE_DBG("Error parsing JSON: missing 'error_info'");
		goto out_put;
	}

	if (!opae_ser_json_to_error_info_obj(jerror_info, &resp->error_info))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaClearError_response_21(opae_fpgaClearError_response *resp,
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

bool opae_decode_fpgaClearError_response_21(const char *json,
					    opae_fpgaClearError_response *resp)
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

char *opae_encode_fpgaClearAllErrors_response_22(
	opae_fpgaClearAllErrors_response *resp,
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

bool opae_decode_fpgaClearAllErrors_response_22(
	const char *json,
	opae_fpgaClearAllErrors_response *resp)
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

char *opae_encode_fpgaTokenGetObject_response_23(
	opae_fpgaTokenGetObject_response *resp,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jobject_id;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&resp->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaTokenGetObject_response_23(
	const char *json,
	opae_fpgaTokenGetObject_response *resp)
{
        struct json_object *root = NULL;
        struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &resp->object_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaDestroyObject_response_24(
	opae_fpgaDestroyObject_response *resp,
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

bool opae_decode_fpgaDestroyObject_response_24(
	const char *json,
	opae_fpgaDestroyObject_response *resp)
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

char *opae_encode_fpgaObjectGetType_response_25(
	opae_fpgaObjectGetType_response *resp,
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

	if (!opae_ser_fpga_sysobject_type_to_json_obj(
		resp->type, root))
		goto out_err;

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetType_response_25(
	const char *json,
	opae_fpgaObjectGetType_response *resp)
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

	if (!opae_ser_json_to_fpga_sysobject_type_obj(
		root, &resp->type))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetName_response_26(
	opae_fpgaObjectGetName_response *resp,
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

	json_object_object_add(root,
			       "name",
			       json_object_new_string(resp->name));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetName_response_26(
	const char *json,
	opae_fpgaObjectGetName_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
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

	str = NULL;
	if (!parse_json_string(root, "name", &str))
		goto out_put;

	len = strnlen(str, OPAE_SYSOBJECT_NAME_MAX - 1);
	memcpy(resp->name, str, len + 1);

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetSize_response_27(
	opae_fpgaObjectGetSize_response *resp,
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

	json_object_object_add(root,
			       "value",
			       json_object_new_int(resp->value));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetSize_response_27(
	const char *json,
	opae_fpgaObjectGetSize_response *resp)
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

	if (!parse_json_u32(root, "value", &resp->value))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectRead_response_28(
	opae_fpgaObjectRead_response *resp,
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

	json_object_object_add(root,
			       "value",
			       json_object_new_string(resp->value));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectRead_response_28(
	const char *json,
	opae_fpgaObjectRead_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str = NULL;
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

	if (!parse_json_string(root, "value", &str))
		goto out_put;

	len = strnlen(str, OPAE_SYSOBJECT_VALUE_MAX - 1);
	memcpy(resp->value, str, len + 1);

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectRead64_response_29(
	opae_fpgaObjectRead64_response *resp,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, resp->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "value",
			       json_object_new_string(buf));

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectRead64_response_29(
	const char *json,
	opae_fpgaObjectRead64_response *resp)
{
        struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
        bool res = false;
	char *str;
	char *endptr;

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

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	resp->value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaObjectRead64_response decode "
			 "failed (value)");
		goto out_put;
	}

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectWrite64_response_30(
	opae_fpgaObjectWrite64_response *resp,
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

bool opae_decode_fpgaObjectWrite64_response_30(
	const char *json,
	opae_fpgaObjectWrite64_response *resp)
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

char *opae_encode_fpgaHandleGetObject_response_31(
	opae_fpgaHandleGetObject_response *resp,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jobject_id;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&resp->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaHandleGetObject_response_31(
	const char *json,
	opae_fpgaHandleGetObject_response *resp)
{
        struct json_object *root = NULL;
        struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &resp->object_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetObject_response_32(
	opae_fpgaObjectGetObject_response *resp,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jobject_id;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&resp->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetObject_response_32(
	const char *json,
	opae_fpgaObjectGetObject_response *resp)
{
        struct json_object *root = NULL;
        struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &resp->object_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetObjectAt_response_33(
	opae_fpgaObjectGetObjectAt_response *resp,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jobject_id;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_response_header_obj(root, &resp->header))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&resp->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (!opae_ser_fpga_result_to_json_obj(resp->result, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetObjectAt_response_33(
	const char *json,
	opae_fpgaObjectGetObjectAt_response *resp)
{
        struct json_object *root = NULL;
        struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &resp->object_id))
		goto out_put;

	if (!opae_ser_json_to_fpga_result_obj(root, &resp->result))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}
