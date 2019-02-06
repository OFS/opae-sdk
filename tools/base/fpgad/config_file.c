// Copyright(c) 2018-2019, Intel Corporation
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

#include "config_file.h"

#include <json-c/json.h>

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("cfg: " format, ##__VA_ARGS__)

#define CFG_TRY_FILE(__f) \
do { \
	canon = canonicalize_file_name(__f); \
	if (canon) { \
		err = strncpy_s(c->cfgfile, \
				sizeof(c->cfgfile), \
				canon, \
				strnlen_s(canon, PATH_MAX)); \
		if (err) \
			LOG("strncpy_s failed.\n"); \
		else { \
			free(canon); \
			return 0; \
		} \
 \
		free(canon); \
	} \
} while (0)

int cfg_find_config_file(struct fpgad_config *c)
{
	char path[PATH_MAX];
	char *e;
	char *canon = NULL;
	errno_t err;

	e = getenv("HOME");
	if (e) {
		// try $HOME/.opae/fpgad.cfg
		snprintf_s_s(path, sizeof(path),
			     "%s/.opae/fpgad.cfg", e);

		CFG_TRY_FILE(path);
	}

	CFG_TRY_FILE("/etc/opae/fpgad.cfg");
	CFG_TRY_FILE("/var/lib/opae/fpgad.cfg");

	return 1; // not found
}

STATIC char *cfg_read_file(const char *file)
{
	FILE *fp;
	size_t len;
	char *buf;

	fp = fopen(file, "r");
	if (!fp) {
		LOG("fopen failed.\n");
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END)) {
		LOG("fseek failed.\n");
		fclose(fp);
		return NULL;
	}

	len = (size_t)ftell(fp);
	++len; // for \0

	if (len == 1) {
		LOG("%s is empty.\n", file);
		fclose(fp);
		return NULL;
	}

	if (fseek(fp, 0, SEEK_SET)) {
		LOG("fseek failed.\n");
		fclose(fp);
		return NULL;
	}

	buf = (char *)malloc(len);
	if (!buf) {
		LOG("malloc failed.\n");
		fclose(fp);
		return NULL;
	}

	if ((fread(buf, 1, len - 1, fp) != len - 1) ||
	    ferror(fp)) {
		LOG("fread failed.\n");
		fclose(fp);
		free(buf);
		return NULL;
	}

	fclose(fp);
	buf[len - 1] = '\0';

	return buf;
}

typedef struct _cfg_vendor_device_id {
	uint16_t vendor_id;
	uint16_t device_id;
	struct _cfg_vendor_device_id *next;
} cfg_vendor_device_id;

typedef struct _cfg_plugin_configuration {
	bool enabled;
	char *library;
	cfg_vendor_device_id *devices;
	struct _cfg_plugin_configuration *next;
} cfg_plugin_configuration;

#if 0
STATIC cfg_vendor_device_id *alloc_device(uint16_t vendor_id,
					  uint16_t device_id)
{
	cfg_vendor_device_id *p;

	p = (cfg_vendor_device_id *)malloc(sizeof(cfg_vendor_device_id));
	if (p) {
		p->vendor_id = vendor_id;
		p->device_id = device_id;
		p->next = NULL;
	}

	return p;
}

STATIC cfg_plugin_configuration *alloc_configuration(bool enabled,
						     char *library,
						     cfg_vendor_device_id *devs)
{
	cfg_plugin_configuration *p;

	p = (cfg_plugin_configuration *)
		malloc(sizeof(cfg_plugin_configuration));
	if (p) {
		p->enabled = enabled;
		p->library = library;
		p->devices = devs;
		p->next = NULL;
	}

	return p;
}
#endif

int cfg_load_config(struct fpgad_config *c)
{
	char *cfg_buf;
	json_object *root = NULL;
	json_object *j_configurations = NULL;
	json_object *j_plugins = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	int res = 1;
	int num_plugins;
	int i;
	cfg_plugin_configuration *configurations = NULL;

	(void)configurations;

	cfg_buf = cfg_read_file(c->cfgfile);
	if (!cfg_buf)
		return res;

	root = json_tokener_parse_verbose(cfg_buf, &j_err);
	if (!root) {
		LOG("error parsing %s: %s\n",
		    c->cfgfile,
		    json_tokener_error_desc(j_err));
		goto out_free;
	}

	if (!json_object_object_get_ex(root,
				       "configurations",
				       &j_configurations)) {
		LOG("failed to find configurations section in %s.\n",
		    c->cfgfile);
		goto out_put;
	}

	if (!json_object_object_get_ex(root, "plugins", &j_plugins)) {
		LOG("failed to find plugins section in %s.\n", c->cfgfile);
		goto out_put;
	}

	if (!json_object_is_type(j_plugins, json_type_array)) {
		LOG("'plugins' JSON object not array.\n");
		goto out_put;
	}

	num_plugins = json_object_array_length(j_plugins);
	for (i = 0 ; i < num_plugins ; ++i) {
		json_object *j_plugin;
		const char *plugin_name;

		j_plugin = json_object_array_get_idx(j_plugins, i);
		plugin_name = json_object_get_string(j_plugin);

		(void)plugin_name;

	}

out_put:
	json_object_put(root);
out_free:
	free(cfg_buf);
	return res;
}
