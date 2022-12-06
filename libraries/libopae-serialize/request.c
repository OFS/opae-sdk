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

char *opae_encode_fpgaDestroyToken_request_1(opae_fpgaDestroyToken_request *req,
					     int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		1, "fpgaDestroyToken_request_1"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jsrc_token_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		2, "fpgaCloneToken_request_2"))
		goto out_err;

	jsrc_token_id = json_object_new_object();
	if (!jsrc_token_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->src_token_id, jsrc_token_id))
		goto out_err;

	json_object_object_add(root, "src_token_id", jsrc_token_id);

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
	struct json_object *jsrc_token_id = NULL;

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

	if (!json_object_object_get_ex(root, "src_token_id", &jsrc_token_id)) {
		OPAE_DBG("Error parsing JSON: missing 'src_token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jsrc_token_id, &req->src_token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		3, "fpgaGetProperties_request_3"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		4, "fpgaUpdateProperties_request_4"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		5, "fpgaOpen_request_5"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
		goto out_put;

	if (!parse_json_int(root, "flags", &req->flags))
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
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		6, "fpgaClose_request_6"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		7, "fpgaReset_request_7"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		8, "fpgaGetPropertiesFromHandle_request_8"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		9, "fpgaMapMMIO_request_9"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	struct json_object *jmmio_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		10, "fpgaUnmapMMIO_request_10"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		11, "fpgaReadMMIO32_request_11"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		12, "fpgaWriteMMIO32_request_12"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		13, "fpgaReadMMIO64_request_13"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		14, "fpgaWriteMMIO64_request_14"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
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

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		16, "fpgaPrepareBuffer_request_16"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	struct json_object *jbuf_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		17, "fpgaReleaseBuffer_request_17"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jhandle_id;
	struct json_object *jbuf_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		18, "fpgaGetIOAddress_request_18"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		19, "fpgaReadError_request_19"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		20, "fpgaGetErrorInfo_request_20"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		21, "fpgaClearError_request_21"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		22, "fpgaClearAllErrors_request_22"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;

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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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
	struct json_object *jtoken_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		23, "fpgaTokenGetObject_request_23"))
		goto out_err;

	jtoken_id = json_object_new_object();
	if (!jtoken_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->token_id, jtoken_id))
		goto out_err;

	json_object_object_add(root, "token_id", jtoken_id);

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
	struct json_object *jtoken_id = NULL;
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

	if (!json_object_object_get_ex(root, "token_id", &jtoken_id)) {
		OPAE_DBG("Error parsing JSON: missing 'token_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jtoken_id, &req->token_id))
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

char *opae_encode_fpgaObjectRead_request_28(
	opae_fpgaObjectRead_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		28, "fpgaObjectRead_request_28"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64,
		     (uint64_t)req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "offset",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64,
		     (uint64_t)req->len) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "len",
			       json_object_new_string(buf));

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectRead_request_28(
	const char *json,
	opae_fpgaObjectRead_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
		OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaObjectRead_request decode "
			 "failed (offset)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "len", &str))
		goto out_put;

	req->len = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaObjectRead_request decode "
			 "failed (len)");
		goto out_put;
	}

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaObjectRead64_request_29(
	opae_fpgaObjectRead64_request *req,
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
		29, "fpgaObjectRead64_request_29"))
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

bool opae_decode_fpgaObjectRead64_request_29(
	const char *json,
	opae_fpgaObjectRead64_request *req)
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

char *opae_encode_fpgaObjectWrite64_request_30(
	opae_fpgaObjectWrite64_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		30, "fpgaObjectWrite64_request_30"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "value",
			       json_object_new_string(buf));

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectWrite64_request_30(
	const char *json,
	opae_fpgaObjectWrite64_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
		OPAE_DBG("Error parsing JSON: missing 'object_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "value", &str))
		goto out_put;

	req->value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaObjectWrite64_request decode "
			 "failed (value)");
		goto out_put;
	}

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaHandleGetObject_request_31(
	opae_fpgaHandleGetObject_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		31, "fpgaHandleGetObject_request_31"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

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

bool opae_decode_fpgaHandleGetObject_request_31(
	const char *json,
	opae_fpgaHandleGetObject_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
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

char *opae_encode_fpgaObjectGetObject_request_32(
	opae_fpgaObjectGetObject_request *req,
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
		32, "fpgaObjectGetObject_request_32"))
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

bool opae_decode_fpgaObjectGetObject_request_32(
	const char *json,
	opae_fpgaObjectGetObject_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
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

char *opae_encode_fpgaObjectGetObjectAt_request_33(
	opae_fpgaObjectGetObjectAt_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jobject_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		33, "fpgaObjectGetObjectAt_request_33"))
		goto out_err;

	jobject_id = json_object_new_object();
	if (!jobject_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->object_id, jobject_id))
		goto out_err;

	json_object_object_add(root, "object_id", jobject_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->idx) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "idx",
			       json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaObjectGetObjectAt_request_33(
	const char *json,
	opae_fpgaObjectGetObjectAt_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jobject_id = NULL;
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

	if (!json_object_object_get_ex(root, "object_id", &jobject_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jobject_id, &req->object_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "idx", &str))
		goto out_put;

	req->idx = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaObjectGetObjectAt_request decode "
			 "failed (idx)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaSetUserClock_request_34(
	opae_fpgaSetUserClock_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		34, "fpgaSetUserClock_request_34"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->high_clk) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "high_clk",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->low_clk) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "low_clk",
			       json_object_new_string(buf));

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaSetUserClock_request_34(
	const char *json,
	opae_fpgaSetUserClock_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "high_clk", &str))
		goto out_put;

	req->high_clk = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaSetUserClock_request decode "
			 "failed (high_clk)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "low_clk", &str))
		goto out_put;

	req->low_clk = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaSetUserClock_request decode "
			 "failed (low_clk)");
		goto out_put;
	}

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetUserClock_request_35(
	opae_fpgaGetUserClock_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		35, "fpgaGetUserClock_request_35"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetUserClock_request_35(
	const char *json,
	opae_fpgaGetUserClock_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetNumMetrics_request_36(
	opae_fpgaGetNumMetrics_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		36, "fpgaGetNumMetrics_request_36"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetNumMetrics_request_36(
	const char *json,
	opae_fpgaGetNumMetrics_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetMetricsInfo_request_37(
	opae_fpgaGetMetricsInfo_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		37, "fpgaGetMetricsInfo_request_37"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	json_object_object_add(root,
			       "num_metrics",
			       json_object_new_int(req->num_metrics));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetMetricsInfo_request_37(
	const char *json,
	opae_fpgaGetMetricsInfo_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!parse_json_u64(root, "num_metrics", &req->num_metrics))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetMetricsByIndex_request_38(
	opae_fpgaGetMetricsByIndex_request *req,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jmetric_num;
	char *json = NULL;
	struct json_object *jhandle_id;
	uint64_t i;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		38, "fpgaGetMetricsByIndex_request_38"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jmetric_num = json_object_new_array();

	for (i = 0 ; i < req->num_metric_indexes ; ++i) {
		struct json_object *array_i;

		array_i = json_object_new_int(req->metric_num[i]);
		json_object_array_put_idx(jmetric_num, i, array_i);
	}

	json_object_object_add(root, "metric_num", jmetric_num);

	json_object_object_add(root,
			       "num_metric_indexes",
			       json_object_new_int(req->num_metric_indexes));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetMetricsByIndex_request_38(
	const char *json,
	opae_fpgaGetMetricsByIndex_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jmetric_num = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "metric_num", &jmetric_num)) {
		OPAE_DBG("Error parsing JSON: missing 'metric_num'");
		goto out_put;
	}

	len = json_object_array_length(jmetric_num);

	req->metric_num = opae_calloc(len, sizeof(uint64_t));
	if (!req->metric_num) {
		OPAE_ERR("calloc failed");
		goto out_put;
	}

	for (i = 0 ; i < len ; ++i) {
		struct json_object *array_i =
			json_object_array_get_idx(jmetric_num, i);

		if (!json_object_is_type(array_i, json_type_int)) {
			OPAE_DBG("'metric_num[%u]' not integer", i);
			goto out_put;
		}

		req->metric_num[i] = json_object_get_int(array_i);
	}

	if (!parse_json_u64(root, "num_metric_indexes", &req->num_metric_indexes))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetMetricsByName_request_39(
	opae_fpgaGetMetricsByName_request *req,
	int json_flags)
{
	struct json_object *root;
	struct json_object *jmetrics_names;
	char *json = NULL;
	struct json_object *jhandle_id;
	uint64_t i;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		39, "fpgaGetMetricsByName_request_39"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jmetrics_names = json_object_new_array();

	for (i = 0 ; i < req->num_metric_names ; ++i) {
		struct json_object *array_i;

		array_i = json_object_new_string(req->metrics_names[i]);
		json_object_array_put_idx(jmetrics_names, i, array_i);
	}

	json_object_object_add(root, "metrics_names", jmetrics_names);

	json_object_object_add(root,
			       "num_metric_names",
			       json_object_new_int(req->num_metric_names));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetMetricsByName_request_39(
	const char *json,
	opae_fpgaGetMetricsByName_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jmetrics_names = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "metrics_names", &jmetrics_names)) {
		OPAE_DBG("Error parsing JSON: missing 'metrics_names'");
		goto out_put;
	}

	len = json_object_array_length(jmetrics_names);

	req->metrics_names = opae_calloc(len, sizeof(char *));
	if (!req->metrics_names) {
		OPAE_ERR("calloc failed");
		goto out_put;
	}

	for (i = 0 ; i < len ; ++i) {
		struct json_object *array_i =
			json_object_array_get_idx(jmetrics_names, i);

		if (!json_object_is_type(array_i, json_type_string)) {
			OPAE_DBG("'metrics_names[%u]' not string", i);
			goto out_put;
		}

		req->metrics_names[i] = opae_strdup(json_object_get_string(array_i));
	}

	if (!parse_json_u64(root, "num_metric_names", &req->num_metric_names))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaGetMetricsThresholdInfo_request_40(
	opae_fpgaGetMetricsThresholdInfo_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		40, "fpgaGetMetricsThresholdInfo_request_40"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	json_object_object_add(root,
			       "num_thresholds",
			       json_object_new_int(req->num_thresholds));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaGetMetricsThresholdInfo_request_40(
	const char *json,
	opae_fpgaGetMetricsThresholdInfo_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;

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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!parse_json_u32(root, "num_thresholds", &req->num_thresholds))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaReconfigureSlotByName_request_41(
	opae_fpgaReconfigureSlotByName_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		41, "fpgaReconfigureSlotByName_request_41"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	json_object_object_add(root,
			       "slot",
			       json_object_new_int(req->slot));

	json_object_object_add(root,
			       "path",
			       json_object_new_string(req->path));

	json_object_object_add(root,
			       "flags",
			       json_object_new_int(req->flags));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaReconfigureSlotByName_request_41(
	const char *json,
	opae_fpgaReconfigureSlotByName_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!parse_json_u32(root, "slot", &req->slot))
		goto out_put;

	str = NULL;
	if (!parse_json_string(root, "path", &str))
		goto out_put;

	memset(req->path, 0, sizeof(req->path));
	len = strnlen(str, PATH_MAX - 1);

	memcpy(req->path, str, len + 1);

	if (!parse_json_int(root, "flags", &req->flags))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaBufMemSet_request_42(
	opae_fpgaBufMemSet_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	struct json_object *jbuf_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		42, "fpgaBufMemSet_request_42"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jbuf_id = json_object_new_object();
	if (!jbuf_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->buf_id, jbuf_id))
		goto out_err;

	json_object_object_add(root, "buf_id", jbuf_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "offset",
			       json_object_new_string(buf));

	json_object_object_add(root,
			       "c",
			       json_object_new_int(req->c));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->n) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "n",
			       json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaBufMemSet_request_42(
	const char *json,
	opae_fpgaBufMemSet_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jbuf_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
		OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &req->buf_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemSet_request decode "
			 "failed (offset)");
		goto out_put;
	}

	if (!parse_json_int(root, "c", &req->c))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "n", &str))
		goto out_put;

	req->n = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemSet_request decode "
			 "failed (n)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaBufMemCpyToRemote_request_43(
	opae_fpgaBufMemCpyToRemote_request *req,
	int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	struct json_object *jdest_buf_id;
	struct json_object *jsrc;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		43, "fpgaBufMemCpyToRemote_request_43"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jdest_buf_id = json_object_new_object();
	if (!jdest_buf_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->dest_buf_id, jdest_buf_id))
		goto out_err;

	json_object_object_add(root, "dest_buf_id", jdest_buf_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->dest_offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "dest_offset",
			       json_object_new_string(buf));

	jsrc = json_object_new_array();

	if (!opae_ser_buffer_to_json_obj(req->src, req->n, jsrc))
		goto out_err;

	json_object_object_add(root, "src", jsrc);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->n) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "n",
			       json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaBufMemCpyToRemote_request_43(
	const char *json,
	opae_fpgaBufMemCpyToRemote_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jdest_buf_id = NULL;
	struct json_object *jsrc = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "dest_buf_id", &jdest_buf_id)) {
		OPAE_DBG("Error parsing JSON: missing 'dest_buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jdest_buf_id, &req->dest_buf_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "dest_offset", &str))
		goto out_put;

	req->dest_offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemCpyToRemote_request decode "
			 "failed (dest_offset)");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "src", &jsrc)) {
		OPAE_DBG("Error parsing JSON: missing 'src'");
		goto out_put;
	}

	if (!opae_ser_json_to_buffer_obj(jsrc, &req->src, &req->n))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "n", &str))
		goto out_put;

	req->n = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemCpyToRemote_request decode "
			 "failed (n)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaBufPoll_request_44(
	opae_fpgaBufPoll_request *req, int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	struct json_object *jbuf_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		44, "fpgaBufPoll_request_44"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jbuf_id = json_object_new_object();
	if (!jbuf_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->buf_id, jbuf_id))
		goto out_err;

	json_object_object_add(root, "buf_id", jbuf_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "offset",
			       json_object_new_string(buf));

	json_object_object_add(root,
			       "width",
			       json_object_new_int(req->width));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->mask) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "mask",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->expected_value) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "expected_value",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->sleep_interval) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "sleep_interval",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, req->loops_timeout) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "loops_timeout",
			       json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaBufPoll_request_44(
	const char *json,
	opae_fpgaBufPoll_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jbuf_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
		OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &req->buf_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "offset", &str))
		goto out_put;

	req->offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufPoll_request decode "
			 "failed (offset)");
		goto out_put;
	}

	if (!parse_json_int(root, "width", &req->width))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "mask", &str))
		goto out_put;

	req->mask = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufPoll_request decode "
			 "failed (mask)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "expected_value", &str))
		goto out_put;

	req->expected_value = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufPoll_request decode "
			 "failed (expected_value)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "sleep_interval", &str))
		goto out_put;

	req->sleep_interval = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufPoll_request decode "
			 "failed (sleep_interval)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "loops_timeout", &str))
		goto out_put;

	req->loops_timeout = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufPoll_request decode "
			 "failed (loops_timeout)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaBufMemCmp_request_45(
	opae_fpgaBufMemCmp_request *req, int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	struct json_object *jbufa_id;
	struct json_object *jbufb_id;
	char buf[32];

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		45, "fpgaBufMemCmp_request_45"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jbufa_id = json_object_new_object();
	if (!jbufa_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->bufa_id, jbufa_id))
		goto out_err;

	json_object_object_add(root, "bufa_id", jbufa_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->bufa_offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "bufa_offset",
			       json_object_new_string(buf));

	jbufb_id = json_object_new_object();
	if (!jbufb_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->bufb_id, jbufb_id))
		goto out_err;

	json_object_object_add(root, "bufb_id", jbufb_id);

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->bufb_offset) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "bufb_offset",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, (uint64_t)req->n) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}

	json_object_object_add(root,
			       "n",
			       json_object_new_string(buf));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaBufMemCmp_request_45(
	const char *json,
	opae_fpgaBufMemCmp_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jbufa_id = NULL;
	struct json_object *jbufb_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "bufa_id", &jbufa_id)) {
		OPAE_DBG("Error parsing JSON: missing 'bufa_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbufa_id, &req->bufa_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "bufa_offset", &str))
		goto out_put;

	req->bufa_offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemCmp_request decode "
			 "failed (bufa_offset)");
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "bufb_id", &jbufb_id)) {
		OPAE_DBG("Error parsing JSON: missing 'bufb_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbufb_id, &req->bufb_id))
		goto out_put;

	str = endptr = NULL;
	if (!parse_json_string(root, "bufb_offset", &str))
		goto out_put;

	req->bufb_offset = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemCmp_request decode "
			 "failed (bufb_offset)");
		goto out_put;
	}

	str = endptr = NULL;
	if (!parse_json_string(root, "n", &str))
		goto out_put;

	req->n = (size_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpgaBufMemCmp_request decode "
			 "failed (n)");
		goto out_put;
	}

	res = true;

out_put:
	json_object_put(root);
	return res;
}

char *opae_encode_fpgaBufWritePattern_request_46(
	opae_fpgaBufWritePattern_request *req, int json_flags)
{
	struct json_object *root;
	char *json = NULL;
	struct json_object *jhandle_id;
	struct json_object *jbuf_id;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_add_request_header_obj(root,
		46, "fpgaBufWritePattern_request_46"))
		goto out_err;

	jhandle_id = json_object_new_object();
	if (!jhandle_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->handle_id, jhandle_id))
		goto out_err;

	json_object_object_add(root, "handle_id", jhandle_id);

	jbuf_id = json_object_new_object();
	if (!jbuf_id) {
		OPAE_ERR("out of memory");
		goto out_err;
	}

	if (!opae_ser_remote_id_to_json_obj(&req->buf_id, jbuf_id))
		goto out_err;

	json_object_object_add(root, "buf_id", jbuf_id);

	json_object_object_add(root,
			       "pattern_name",
			       json_object_new_string(req->pattern_name));

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_decode_fpgaBufWritePattern_request_46(
	const char *json,
	opae_fpgaBufWritePattern_request *req)
{
	struct json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;
	struct json_object *jhandle_id = NULL;
	struct json_object *jbuf_id = NULL;
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

	if (!json_object_object_get_ex(root, "handle_id", &jhandle_id)) {
		OPAE_DBG("Error parsing JSON: missing 'handle_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jhandle_id, &req->handle_id))
		goto out_put;

	if (!json_object_object_get_ex(root, "buf_id", &jbuf_id)) {
		OPAE_DBG("Error parsing JSON: missing 'buf_id'");
		goto out_put;
	}

	if (!opae_ser_json_to_remote_id_obj(jbuf_id, &req->buf_id))
		goto out_put;

	str = NULL;
	if (!parse_json_string(root, "pattern_name", &str))
		goto out_put;

	memset(req->pattern_name, 0, sizeof(req->pattern_name));
	len = strnlen(str, OPAE_REQUEST_NAME_MAX - 1);
	memcpy(req->pattern_name, str, len + 1);

	res = true;

out_put:
	json_object_put(root);
	return res;
}
