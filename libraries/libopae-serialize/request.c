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

#include "cfg-file.h"
#include "request.h"
#include "opae_int.h"
#include "props.h"
#include "mock/opae_std.h"

STATIC uint64_t next_serial = 0;
static pthread_mutex_t next_serial_lock =
        PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC uint64_t opae_next_serial(void)
{
	uint64_t next;
	int res;

	opae_mutex_lock(res, &next_serial_lock);
	next = next_serial++;
	opae_mutex_unlock(res, &next_serial_lock);

	return next;
}

STATIC bool opae_add_request_header_obj(struct json_object *root,
					int request_id,
					const char *request_name)
{
	uint64_t serial;
	struct json_object *header;

	header = json_object_new_object();

	json_object_object_add(header,
			       "request_id",
			       json_object_new_int(request_id));

	json_object_object_add(header,
			       "request_name",
			       json_object_new_string(request_name));

	serial = opae_next_serial();
	json_object_object_add(header,
			       "serial",
			       json_object_new_int(serial));

	json_object_object_add(header,
			       "from",
			       json_object_new_string(opae_get_host_name()));

	json_object_object_add(root, "header", header);

	return true;
}

/*
{
    "header": { ... },
    "filters": [
        {
            <fpga_properties 0>
	},
        {
            <fpga_properties 1>
	}
    ],
    "max_tokens": 3
}
*/
char *opae_encode_fpgaEnumerate_request_0(opae_fpgaEnumerate_request *req,
					  int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jfilters;
	size_t i;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root, 0, "fpgaEnumerate_request_0"))
		goto out_err;

	jfilters = json_object_new_array();

	for (i = 0 ; i < (size_t)req->num_filters ; ++i) {
		struct json_object *array_i =
			json_object_new_object();

		if (!opae_ser_properties_to_json_obj(req->filters[i], array_i))
			goto out_err;

		json_object_array_put_idx(jfilters, i, array_i);
	}

	json_object_object_add(root, "filters", jfilters);

	json_object_object_add(root,
			       "max_tokens",
			       json_object_new_int(req->max_tokens));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_request_header_obj(struct json_object *root,
				    opae_request_header *header)
{
	struct json_object *jheader = NULL;
	char *str = NULL;

	memset(header, 0, sizeof(opae_request_header));

        if (!json_object_object_get_ex(root, "header", &jheader)) {
                OPAE_DBG("Error parsing JSON: missing 'header'");
                return false;
        }

	if (!parse_json_u64(jheader, "request_id", &header->request_id))
		return false;

	if (!parse_json_string(jheader, "request_name", &str))
		return false;
	memcpy(header->request_name, str, strlen(str));

	if (!parse_json_u64(jheader, "serial", &header->serial))
		return false;

	str = NULL;
	if (!parse_json_string(jheader, "from", &str))
		return false;
	memcpy(header->from, str, strlen(str));

	return true;
}

bool opae_decode_fpgaEnumerate_request_0(const char *json,
                                         opae_fpgaEnumerate_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jfilters = NULL;
	size_t i;
	size_t len;

        root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "filters", &jfilters)) {
                OPAE_DBG("Error parsing JSON: missing 'filters'");
                goto out_put;
        }

	len = json_object_array_length(jfilters);

	req->num_filters = (uint32_t)len;
	req->filters = NULL;
	if (len) {
		req->filters = opae_calloc(len, sizeof(fpga_properties));
		if (!req->filters) {
			OPAE_ERR("calloc failed");
			goto out_put;
		}
	}

	for (i = 0 ; i < len ; ++i) {
		struct json_object *array_i =
			json_object_array_get_idx(jfilters, i);

		if (!opae_ser_json_to_properties_obj(array_i,
			&req->filters[i]))
			goto out_put;
	}

	if (!parse_json_u32(root, "max_tokens", &req->max_tokens))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

/*
{
    "header": { ... },
    "token": { <the token> }
}
*/
char *opae_encode_fpgaDestroyToken_request_1(opae_fpgaDestroyToken_request *req,
                                             int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *token;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		1, "fpgaDestroyToken_request_1"))
		goto out_err;

	token = json_object_new_object();
	if (!token) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, token))
		goto out_err;

	json_object_object_add(root, "token", token);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaDestroyToken_request_1(const char *json,
                                            opae_fpgaDestroyToken_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

	if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaCloneToken_request_2(opae_fpgaCloneToken_request *req,
                                           int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *src_token;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		2, "fpgaCloneToken_request_2"))
		goto out_err;

	src_token = json_object_new_object();
	if (!src_token) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->src_token, src_token))
		goto out_err;

	json_object_object_add(root, "src_token", src_token);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaCloneToken_request_2(const char *json,
                                          opae_fpgaCloneToken_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jsrc_token = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "src_token", &jsrc_token)) {
                OPAE_DBG("Error parsing JSON: missing 'src_token'");
                goto out_put;
        }

	if (!opae_ser_json_to_token_header_obj(jsrc_token, &req->src_token))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetProperties_request_3(opae_fpgaGetProperties_request *req,
                                              int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		3, "fpgaGetProperties_request_3"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetProperties_request_3(const char *json,
                                             opae_fpgaGetProperties_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

	if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaUpdateProperties_request_4(opae_fpgaUpdateProperties_request *req,
						 int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		4, "fpgaUpdateProperties_request_4"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaUpdateProperties_request_4(const char *json,
                                                opae_fpgaUpdateProperties_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

	if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaOpen_request_5(opae_fpgaOpen_request *req,
                                     int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		5, "fpgaOpen_request_5"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaOpen_request_5(const char *json,
                                    opae_fpgaOpen_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put; 

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaClose_request_6(opae_fpgaClose_request *req,
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

	if (!opae_add_request_header_obj(root,
		6, "fpgaClose_request_6"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaClose_request_6(const char *json,
				     opae_fpgaClose_request *req)
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

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReset_request_7(opae_fpgaReset_request *req,
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

	if (!opae_add_request_header_obj(root,
		7, "fpgaReset_request_7"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReset_request_7(const char *json,
				     opae_fpgaReset_request *req)
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

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetPropertiesFromHandle_request_8(
	opae_fpgaGetPropertiesFromHandle_request *req,
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

	if (!opae_add_request_header_obj(root,
		8, "fpgaGetPropertiesFromHandle_request_8"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetPropertiesFromHandle_request_8(
	const char *json,
	opae_fpgaGetPropertiesFromHandle_request *req)
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

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaMapMMIO_request_9(opae_fpgaMapMMIO_request *req,
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

	if (!opae_add_request_header_obj(root,
		9, "fpgaMapMMIO_request_9"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaMapMMIO_request_9(const char *json,
				       opae_fpgaMapMMIO_request *req)
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

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaUnmapMMIO_request_10(opae_fpgaUnmapMMIO_request *req,
					   int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	struct json_object *jmmio_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		10, "fpgaUnmapMMIO_request_10"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	jmmio_id = json_object_new_object();
	if (!jmmio_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	json_object_object_add(root, "mmio_id", jmmio_id);

	if (!opae_ser_remote_id_to_json_obj(&req->mmio_id, jmmio_id))
		goto out_err;

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaUnmapMMIO_request_10(const char *json,
					  opae_fpgaUnmapMMIO_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	struct json_object *jmmio_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!json_object_object_get_ex(root, "mmio_id", &jmmio_id)) {
                OPAE_DBG("Error parsing JSON: missing 'mmio_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jmmio_id, &req->mmio_id))
		goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReadMMIO32_request_11(opae_fpgaReadMMIO32_request *req,
					    int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		11, "fpgaReadMMIO32_request_11"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "offset",
                               json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadMMIO32_request_11(const char *json,
					   opae_fpgaReadMMIO32_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	char *str;
	char *endptr;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaReadMMIO32_request decode "
		"failed (offset)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaWriteMMIO32_request_12(opae_fpgaWriteMMIO32_request *req,
                                             int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		12, "fpgaWriteMMIO32_request_12"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "offset",
                               json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%08" PRIx32, req->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "value",
                               json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaWriteMMIO32_request_12(const char *json,
					    opae_fpgaWriteMMIO32_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	char *str;
	char *endptr;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaWriteMMIO32_request decode "
		"failed (offset)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	req->value = (uint32_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaWriteMMIO32_request decode "
		"failed (value)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReadMMIO64_request_13(opae_fpgaReadMMIO64_request *req,
					    int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		13, "fpgaReadMMIO64_request_13"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "offset",
                               json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadMMIO64_request_13(const char *json,
					   opae_fpgaReadMMIO64_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	char *str;
	char *endptr;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaReadMMIO64_request decode "
		"failed (offset)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaWriteMMIO64_request_14(opae_fpgaWriteMMIO64_request *req,
                                             int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		14, "fpgaWriteMMIO64_request_14"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "offset",
                               json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "value",
                               json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaWriteMMIO64_request_14(const char *json,
					    opae_fpgaWriteMMIO64_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	char *str;
	char *endptr;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaWriteMMIO64_request decode "
		"failed (offset)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	req->value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaWriteMMIO64_request decode "
		"failed (value)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaWriteMMIO512_request_15(opae_fpgaWriteMMIO512_request *req,
					      int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	struct json_object *jvalues;
	char buf[32];
	size_t i;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		15, "fpgaWriteMMIO512_request_15"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	json_object_object_add(root, "mmio_num",
			json_object_new_int(req->mmio_num));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "offset",
                               json_object_new_string(buf));

	jvalues = json_object_new_array();

	for (i = 0 ; i < 8 ; ++i) {
		struct json_object *array_i;

		if (snprintf(buf, sizeof(buf),
			     "0x%016" PRIx64, req->values[i]) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}

		array_i = json_object_new_string(buf);
		json_object_array_put_idx(jvalues, i, array_i);
	}

        json_object_object_add(root, "values", jvalues);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaWriteMMIO512_request_15(const char *json,
					     opae_fpgaWriteMMIO512_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	struct json_object *jvalues = NULL;
	char *str;
	char *endptr;
	size_t len;
	size_t i;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!parse_json_u32(root, "mmio_num", &req->mmio_num))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaWriteMMIO512_request decode "
		"failed (offset)");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "values", &jvalues)) {
                OPAE_DBG("Error parsing JSON: missing 'values'");
                goto out_put;
        }

	len = json_object_array_length(jvalues);
	if (len != 8) {
		OPAE_DBG("Error parsing JSON: 'values' "
			 "has incorrect size %u", len);
		goto out_put;
	}

	for (i = 0 ; i < len ; ++i) {
		struct json_object *array_i =
			json_object_array_get_idx(jvalues, i);

		if (!json_object_is_type(array_i, json_type_string)) {
			OPAE_DBG("'values[%u]' not string", i);
			goto out_put;
		}

		str = (char *)json_object_get_string(array_i);

		endptr = NULL;
		req->values[i] = strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpgaWriteMMIO512_request decode "
				 "failed (values[%u])", i);
			goto out_put;
		}
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaPrepareBuffer_request_16(opae_fpgaPrepareBuffer_request *req,
					       int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		16, "fpgaPrepareBuffer_request_16"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->len) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

        json_object_object_add(root,
                               "len",
                               json_object_new_string(buf));

	json_object_object_add(root,
			       "have_buf_addr",
			       json_object_new_boolean(req->have_buf_addr));

	if (req->pre_allocated_addr) {

		if (snprintf(buf, sizeof(buf),
			     "0x%016" PRIx64,
			     (uint64_t)req->pre_allocated_addr) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}

	        json_object_object_add(root,
				       "pre_allocated_addr",
				       json_object_new_string(buf));

	} else {
		json_object_object_add(root,
				       "pre_allocated_addr",
				       json_object_new_string("NULL"));
	}

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaPrepareBuffer_request_16(const char *json,
					      opae_fpgaPrepareBuffer_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	char *str;
	char *endptr;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "len", &str))
		goto out_put;

	req->len = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaPrepareBuffer_request decode "
		"failed (len)");
		goto out_put;
	}

	if (!parse_json_boolean(root, "have_buf_addr", &req->have_buf_addr))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "pre_allocated_addr", &str))
		goto out_put;

	if (!strcmp(str, "NULL")) {
		req->pre_allocated_addr = NULL;
	} else {
		req->pre_allocated_addr = (void *)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpgaPrepareBuffer_request decode "
				 "failed (pre_allocated_addr)");
			goto out_put;
		}
	}

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put; 

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReleaseBuffer_request_17(opae_fpgaReleaseBuffer_request *req,
					       int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	struct json_object *jbuf_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		17, "fpgaReleaseBuffer_request_17"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	jbuf_id = json_object_new_object();
	json_object_object_add(root, "buf_id", jbuf_id);

	if (!opae_ser_remote_id_to_json_obj(&req->buf_id, jbuf_id))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReleaseBuffer_request_17(const char *json,
					      opae_fpgaReleaseBuffer_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	struct json_object *jbuf_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
                OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &req->buf_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetIOAddress_request_18(opae_fpgaGetIOAddress_request *req,
					      int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle;
	struct json_object *jbuf_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		18, "fpgaGetIOAddress_request_18"))
		goto out_err;

	jhandle = json_object_new_object();
	if (!jhandle) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_handle_header_to_json_obj(&req->handle, jhandle))
		goto out_err;

	json_object_object_add(root, "handle", jhandle);

	jbuf_id = json_object_new_object();
	json_object_object_add(root, "buf_id", jbuf_id);

	if (!opae_ser_remote_id_to_json_obj(&req->buf_id, jbuf_id))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetIOAddress_request_18(const char *json,
					     opae_fpgaGetIOAddress_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle = NULL;
	struct json_object *jbuf_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "handle", &jhandle)) {
                OPAE_DBG("Error parsing JSON: missing 'handle'");
                goto out_put;
        }

        if (!opae_ser_json_to_handle_header_obj(jhandle, &req->handle))
                goto out_put;

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
                OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &req->buf_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReadError_request_19(opae_fpgaReadError_request *req,
					   int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		19, "fpgaReadError_request_19"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json_object_object_add(root,
			       "error_num",
			       json_object_new_int(req->error_num));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReadError_request_19(const char *json,
					  opae_fpgaReadError_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	if (!parse_json_u32(root, "error_num", &req->error_num))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetErrorInfo_request_20(
	opae_fpgaGetErrorInfo_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		20, "fpgaGetErrorInfo_request_20"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json_object_object_add(root,
			       "error_num",
			       json_object_new_int(req->error_num));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetErrorInfo_request_20(
	const char *json,
	opae_fpgaGetErrorInfo_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	if (!parse_json_u32(root, "error_num", &req->error_num))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaClearError_request_21(opae_fpgaClearError_request *req,
					    int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		21, "fpgaClearError_request_21"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json_object_object_add(root,
			       "error_num",
			       json_object_new_int(req->error_num));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaClearError_request_21(const char *json,
					   opae_fpgaClearError_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	if (!parse_json_u32(root, "error_num", &req->error_num))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaClearAllErrors_request_22(
	opae_fpgaClearAllErrors_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		22, "fpgaClearAllErrors_request_22"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaClearAllErrors_request_22(
	const char *json,
	opae_fpgaClearAllErrors_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaTokenGetObject_request_23(
	opae_fpgaTokenGetObject_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		23, "fpgaTokenGetObject_request_23"))
		goto out_err;

	jtoken = json_object_new_object();
	if (!jtoken) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_token_header_to_json_obj(&req->token, jtoken))
		goto out_err;

	json_object_object_add(root, "token", jtoken);

	json_object_object_add(root,
			       "name",
			       json_object_new_string(req->name));

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaTokenGetObject_request_23(
	const char *json,
	opae_fpgaTokenGetObject_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jtoken = NULL;
	char *str;
	size_t len;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "token", &jtoken)) {
                OPAE_DBG("Error parsing JSON: missing 'token'");
                goto out_put;
        }

        if (!opae_ser_json_to_token_header_obj(jtoken, &req->token))
                goto out_put;

	str = NULL;
	if (!parse_json_string(root, "name", &str))
		goto out_put;

	len = strnlen(str, OPAE_SYSOBJECT_NAME_MAX - 1);
	memcpy(req->name, str, len + 1);

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaDestroyObject_request_24(
	opae_fpgaDestroyObject_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		24, "fpgaDestroyObject_request_24"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaDestroyObject_request_24(
	const char *json,
	opae_fpgaDestroyObject_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
                goto out_put;
        }

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetType_request_25(
	opae_fpgaObjectGetType_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		25, "fpgaObjectGetType_request_25"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetType_request_25(
	const char *json,
	opae_fpgaObjectGetType_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
                goto out_put;
        }

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetName_request_26(
	opae_fpgaObjectGetName_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		26, "fpgaObjectGetName_request_26"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetName_request_26(
	const char *json,
	opae_fpgaObjectGetName_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
                goto out_put;
        }

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectGetSize_request_27(
	opae_fpgaObjectGetSize_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		27, "fpgaObjectGetSize_request_27"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetSize_request_27(
	const char *json,
	opae_fpgaObjectGetSize_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;

	root = json_tokener_parse_verbose(json, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
        }

	if (!opae_decode_request_header_obj(root, &req->header)) {
		OPAE_ERR("request header decode failed");
		goto out_put;
	}

        if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
                OPAE_DBG("Error parsing JSON: missing 'object_id'");
                goto out_put;
        }

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}
