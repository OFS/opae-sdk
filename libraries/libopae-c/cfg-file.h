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
#ifndef __OPAE_CFG_FILE_H__
#define __OPAE_CFG_FILE_H__
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <opae/log.h>
#include <json-c/json.h>

// Returns the canonicalized path to the first config
// file found, or NULL if not found. When the return
// value is non-NULL, it was allocated by the function.
// Caller must free it.
char *opae_find_cfg_file(void);

// Returns an allocated buffer containing the contents
// of the config file found at config_file_path. The
// config_file_path parameter is assumed to have been
// allocated by opae_find_cfg_file(), and is freed by
// this function.
// return: NULL on error or an allocated buffer on success.
// Caller must free any allocated buffer.
char *opae_read_cfg_file(char *config_file_path);

// VID, DID, SVID, SDID
#define ID_SIZE              4

#define MAX_DEV_NAME       256

#define OPAE_VENDOR_ANY 0xffff
#define OPAE_DEVICE_ANY 0xffff

typedef struct _opae_pci_device {
	const char *name;
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_device_id;
} opae_pci_device;

int opae_parse_device_id(json_object *j_id,
			 opae_pci_device *dev);


typedef struct _libopae_config_data {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_device_id;
	const char *module_library;
	const char *config_json;
	uint32_t flags;
#define OPAE_PLATFORM_DATA_DETECTED 0x00000001
#define OPAE_PLATFORM_DATA_LOADED   0x00000002
} libopae_config_data;

libopae_config_data *
opae_parse_libopae_config(const char *json_input);

void opae_print_libopae_config(libopae_config_data *cfg);

void opae_free_libopae_config(libopae_config_data *cfg);


#define OPAE_FEATURE_ID_ANY -1
typedef struct _fpgainfo_config_data {
        uint16_t vendor_id;
        uint16_t device_id;
        uint16_t subvendor_id;
        uint16_t subdevice_id;
        int32_t feature_id;
        char *board_plugin;
        void *dl_handle;
        char product_name[256];
} fpgainfo_config_data;

fpgainfo_config_data *
opae_parse_fpgainfo_config(const char *json_input);

void opae_print_fpgainfo_config(fpgainfo_config_data *cfg);

void opae_free_fpgainfo_config(fpgainfo_config_data *cfg);


typedef struct _fpgad_config_data {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_device_id;
	const char *module_library;
#define FPGAD_DEV_DETECTED 0x00000001
#define FPGAD_DEV_LOADED   0x00000002
	uint32_t flags;
	void *dl_handle;
	const char *config_json;
} fpgad_config_data;

fpgad_config_data *
opae_parse_fpgad_config(const char *json_input);

void opae_print_fpgad_config(fpgad_config_data *cfg);

void opae_free_fpgad_config(fpgad_config_data *cfg);



static inline json_object *
parse_json_array(json_object *parent, const char *name, int *len)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
                OPAE_DBG("Error parsing JSON: missing '%s'", name);
		return NULL;
        }

	if (!json_object_is_type(jname, json_type_array)) {
                OPAE_DBG("'%s' JSON object not array type", name);
                return NULL;
        }

	if (len)
		*len = json_object_array_length(jname);

	return jname;
}

static inline json_object *
parse_json_boolean(json_object *parent, const char *name, bool *value)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
                OPAE_DBG("Error parsing JSON: missing '%s'", name);
		return NULL;
        }

	if (!json_object_is_type(jname, json_type_boolean)) {
                OPAE_DBG("'%s' JSON object not boolean", name);
                return NULL;
        }

	if (value)
		*value = json_object_get_boolean(jname);

	return jname;
}

static inline json_object *
parse_json_string(json_object *parent, const char *name, char **value)
{
	json_object *jname = NULL;

	if (!json_object_object_get_ex(parent, name, &jname)) {
                OPAE_DBG("Error parsing JSON: missing '%s'", name);
		return NULL;
        }

	if (!json_object_is_type(jname, json_type_string)) {
                OPAE_DBG("'%s' JSON object not string", name);
                return NULL;
        }

	if (value)
		*value = (char *)json_object_get_string(jname);

	return jname;
}

static inline int
string_to_unsigned_wildcard(const char *s,
			    unsigned long *u,
			    unsigned long w)
{
	char *endptr = NULL;
	unsigned long res;

	if (*s == '*') {
		if (u)
			*u = w;
		return 0;
	}

	res = strtoul(s, &endptr, 0);
	if (endptr == s + strlen(s)) {
		if (u)
			*u = res;
		return 0;
	}

	return 1;
}

static inline int
string_to_signed_wildcard(const char *s,
			  long *l,
			  long w)
{
	char *endptr = NULL;
	long res;

	if (*s == '*') {
		if (l)
			*l = w;
		return 0;
	}

	res = strtol(s, &endptr, 0);
	if (endptr == s + strlen(s)) {
		if (l)
			*l = res;
		return 0;
	}

	return 1;
}

#endif // __OPAE_CFG_FILE_H__
