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
#ifndef __OPAE_SERIALIZE_H__
#define __OPAE_SERIALIZE_H__
#include <stdbool.h>
#include <opae/types_enum.h>
#include <opae/types.h>
#include <opae/log.h>
#include <json-c/json.h>
#include <uuid/uuid.h>

#define OPAE_RECEIVE_BUF_MAX (32 * 1024)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char *opae_ser_properties_to_json(fpga_properties prop,
				  int json_flags);
bool opae_ser_properties_to_json_obj(fpga_properties prop,
				     struct json_object *parent);

bool opae_ser_json_to_properties(const char *json,
				 fpga_properties *props);
bool opae_ser_json_to_properties_obj(struct json_object *jobj,
				     fpga_properties *props);
				     

char *opae_ser_token_header_to_json(fpga_token_header *hdr,
				    int json_flags);
bool opae_ser_token_header_to_json_obj(fpga_token_header *hdr,
				       struct json_object *parent);

bool opae_ser_json_to_token_header(const char *json,
				   fpga_token_header *hdr);
bool opae_ser_json_to_token_header_obj(struct json_object *jobj,
				       fpga_token_header *hdr);

bool opae_ser_fpga_result_to_json_obj(fpga_result res,
				      struct json_object *parent);
bool opae_ser_json_to_fpga_result_obj(struct json_object *jobj,
				      fpga_result *res);


static inline json_object *
parse_json_int(json_object *parent, const char *name, int *value)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
	        OPAE_DBG("Error parsing JSON: missing '%s'", name);
	        return NULL;
	}

	if (!json_object_is_type(jname, json_type_int)) {
	        OPAE_DBG("'%s' JSON object not int", name);
	        return NULL;
	}

	if (value)
	        *value = json_object_get_int(jname);

	return jname;
}

static inline json_object *
parse_json_u32(json_object *parent, const char *name, uint32_t *value)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
	        OPAE_DBG("Error parsing JSON: missing '%s'", name);
	        return NULL;
	}

	if (!json_object_is_type(jname, json_type_int)) {
	        OPAE_DBG("'%s' JSON object not int", name);
	        return NULL;
	}

	if (value)
	        *value = json_object_get_int(jname);

	return jname;
}

static inline json_object *
parse_json_u64(json_object *parent, const char *name, uint64_t *value)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
	        OPAE_DBG("Error parsing JSON: missing '%s'", name);
	        return NULL;
	}

	if (!json_object_is_type(jname, json_type_int)) {
	        OPAE_DBG("'%s' JSON object not int", name);
	        return NULL;
	}

	if (value)
	        *value = json_object_get_int(jname);

	return jname;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_SERIALIZE_H__