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
#include "monitored_device.h"
#include "api/sysfs.h"

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
 \
		if (!cmd_path_is_symlink(__f)) { \
 \
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
	char *configuration;
	bool enabled;
	char *library;
	cfg_vendor_device_id *devices;
	struct _cfg_plugin_configuration *next;
} cfg_plugin_configuration;

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

STATIC cfg_plugin_configuration *alloc_configuration(char *configuration,
						     bool enabled,
						     char *library,
						     cfg_vendor_device_id *devs)
{
	cfg_plugin_configuration *p;

	p = (cfg_plugin_configuration *)
		malloc(sizeof(cfg_plugin_configuration));
	if (p) {
		p->configuration = configuration;
		p->enabled = enabled;
		p->library = library;
		p->devices = devs;
		p->next = NULL;
	}

	return p;
}

STATIC cfg_vendor_device_id *
cfg_process_plugin_devices(const char *name,
			   json_object *j_devices)
{
	int i;
	int devs;
	cfg_vendor_device_id *head = NULL;
	cfg_vendor_device_id *id = NULL;
	uint16_t vendor_id;
	uint16_t device_id;
	const char *s;
	char *endptr;

	if (!json_object_is_type(j_devices, json_type_array)) {
		LOG("'devices' JSON object not array.\n");
		return NULL;
	}

	devs = json_object_array_length(j_devices);
	for (i = 0 ; i < devs ; ++i) {
		json_object *j_dev = json_object_array_get_idx(j_devices, i);
		json_object *j_vid;
		json_object *j_did;

		if (!json_object_is_type(j_dev, json_type_array)) {
			LOG("%s 'devices' entry %d not array.\n",
				name, i);
			goto out_free;
		}

		if (json_object_array_length(j_dev) != 2) {
			LOG("%s 'devices' entry %d not array[2].\n",
				name, i);
			goto out_free;
		}

		j_vid = json_object_array_get_idx(j_dev, 0);
		if (json_object_is_type(j_vid, json_type_string)) {
			s = json_object_get_string(j_vid);
			endptr = NULL;

			vendor_id = (uint16_t)strtoul(s, &endptr, 0);
			if (*endptr != '\0') {
				LOG("%s malformed Vendor ID at devices[%d]\n",
					name, i);
				goto out_free;
			}

		} else if (json_object_is_type(j_vid, json_type_int)) {
			vendor_id = (uint16_t)json_object_get_int(j_vid);
		} else {
			LOG("%s invalid Vendor ID at devices[%d]\n",
				name, i);
			goto out_free;
		}

		j_did = json_object_array_get_idx(j_dev, 1);
		if (json_object_is_type(j_did, json_type_string)) {
			s = json_object_get_string(j_did);
			endptr = NULL;

			device_id = (uint16_t)strtoul(s, &endptr, 0);
			if (*endptr != '\0') {
				LOG("%s malformed Device ID at devices[%d]\n",
					name, i);
				goto out_free;
			}

		} else if (json_object_is_type(j_did, json_type_int)) {
			device_id = (uint16_t)json_object_get_int(j_did);
		} else {
			LOG("%s invalid Device ID at devices[%d]\n",
				name, i);
			goto out_free;
		}

		if (!head) {
			head = alloc_device(vendor_id, device_id);
			if (!head) {
				LOG("malloc failed.\n");
				goto out_free;
			}

			id = head;
		} else {
			id->next = alloc_device(vendor_id, device_id);
			if (!id->next) {
				LOG("malloc failed.\n");
				goto out_free;
			}

			id = id->next;
		}
	}

	return head;

out_free:
	for (id = head ; id ; ) {
		cfg_vendor_device_id *trash = id;
		id = id->next;
		free(trash);
	}
	return NULL;
}

STATIC int cfg_process_plugin(const char *name,
			      json_object *j_configurations,
			      cfg_plugin_configuration **list)
{
	json_object *j_cfg_plugin = NULL;
	json_object *j_cfg_plugin_configuration = NULL;
	json_object *j_enabled = NULL;
	json_object *j_plugin = NULL;
	json_object *j_devices = NULL;
	char *configuration = NULL;
	bool enabled = false;
	char *plugin = NULL;
	cfg_plugin_configuration *c = NULL;

	if (!json_object_object_get_ex(j_configurations,
				       name,
				       &j_cfg_plugin)) {
		LOG("couldn't find configurations section"
		    " for %s.\n", name);
		return 1;
	}

	if (!json_object_object_get_ex(j_cfg_plugin,
				       "configuration",
				       &j_cfg_plugin_configuration)) {
		LOG("couldn't find %s configuration section.\n", name);
		return 1;
	}

	configuration = (char *)json_object_to_json_string_ext(
				j_cfg_plugin_configuration,
				JSON_C_TO_STRING_PLAIN);
	if (!configuration) {
		LOG("failed to parse configuration for %s.\n", name);
		return 1;
	}

	configuration = cstr_dup(configuration);
	if (!configuration) {
		LOG("cstr_dup failed.\n");
		return 1;
	}

	if (!json_object_object_get_ex(j_cfg_plugin,
				       "enabled",
				       &j_enabled)) {
		LOG("couldn't find enabled key"
		    " for %s.\n", name);
		goto out_free;
	}

	if (!json_object_is_type(j_enabled, json_type_boolean)) {
		LOG("enabled key for %s not boolean.\n", name);
		goto out_free;
	}

	enabled = json_object_get_boolean(j_enabled);

	if (!json_object_object_get_ex(j_cfg_plugin,
				       "plugin",
				       &j_plugin)) {
		LOG("couldn't find plugin key"
		    " for %s.\n", name);
		goto out_free;
	}

	if (!json_object_is_type(j_plugin, json_type_string)) {
		LOG("plugin key for %s not string.\n", name);
		goto out_free;
	}

	plugin = cstr_dup(json_object_get_string(j_plugin));
	if (!plugin) {
		LOG("cstr_dup failed.\n");
		goto out_free;
	}

	if (!json_object_object_get_ex(j_cfg_plugin,
				       "devices",
				       &j_devices)) {
		LOG("couldn't find devices key"
		    " for %s.\n", name);
		goto out_free;
	}

	if (!(*list)) { // list is empty
		c = alloc_configuration(configuration,
					enabled,
					plugin,
					NULL);
		if (!c) {
			LOG("malloc failed.\n");
			goto out_free;
		}

		*list = c;
	} else {
		for (c = *list ; c->next ; c = c->next)
		/* find the end of the list */ ;

		c->next = alloc_configuration(configuration,
					      enabled,
					      plugin,
					      NULL);
		if (!c->next) {
			LOG("malloc failed.\n");
			goto out_free;
		}

		c = c->next;
	}

	c->devices = cfg_process_plugin_devices(name, j_devices);

	return 0;

out_free:
	if (configuration)
		free(configuration);
	if (plugin)
		free(plugin);
	if (c)
		free(c);
	return 1;
}

STATIC fpgad_supported_device *
cfg_json_to_supported(cfg_plugin_configuration *configurations)
{
	cfg_plugin_configuration *c;
	cfg_vendor_device_id *d;
	size_t num_devices = 0;
	fpgad_supported_device *supported;
	int i;

	// find the number of devices
	for (c = configurations ; c ; c = c->next) {
		if (!c->enabled) // skip it
			continue;
		for (d = c->devices ; d ; d = d->next) {
			++num_devices;
		}
	}

	++num_devices; // +1 for NULL terminator

	supported = calloc(num_devices, sizeof(fpgad_supported_device));
	if (!supported) {
		LOG("calloc failed.\n");
		return NULL;
	}

	i = 0;
	for (c = configurations ; c ; c = c->next) {
		if (!c->enabled) // skip it
			continue;
		for (d = c->devices ; d ; d = d->next) {
			fpgad_supported_device *dev = &supported[i++];

			dev->vendor_id = d->vendor_id;
			dev->device_id = d->device_id;
			dev->library_path = cstr_dup(c->library);
			dev->config = cstr_dup(c->configuration);
		}
	}

	for (c = configurations ; c ; ) {
		cfg_plugin_configuration *ctrash = c;

		for (d = c->devices ; d ; ) {
			cfg_vendor_device_id *dtrash = d;
			d = d->next;
			free(dtrash);
		}

		c = c->next;

		if (ctrash->configuration)
			free(ctrash->configuration);
		if (ctrash->library)
			free(ctrash->library);
		free(ctrash);
	}

	return supported;
}

STATIC bool cfg_verify_supported_devices(fpgad_supported_device *d)
{
	while (d->library_path) {
		char *sub = NULL;
		errno_t err;

		if (d->library_path[0] == '/') {
			LOG("plugin library paths may not "
			    "be absolute paths: %s\n", d->library_path);
			return false;
		}

		if (cmd_path_is_symlink(d->library_path)) {
			LOG("plugin library paths may not "
			    "contain links: %s\n", d->library_path);
			return false;
		}

		err = strstr_s((char *)d->library_path, PATH_MAX,
			       "..", 2, &sub);
		if (EOK == err) {
			LOG("plugin library paths may not "
			    "contain .. : %s\n", d->library_path);
			return false;
		}

		++d;
	}

	return true;
}

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

		if (cfg_process_plugin(plugin_name,
				       j_configurations,
				       &configurations))
			goto out_put;
	}

	if (!configurations) {
		LOG("no configurations found in %s.\n", c->cfgfile);
		goto out_put;
	}

	c->supported_devices = cfg_json_to_supported(configurations);

	if (c->supported_devices) {

		if (cfg_verify_supported_devices(c->supported_devices)) {
			res = 0;
		} else {
			fpgad_supported_device *trash = c->supported_devices;

			LOG("invalid configuration file\n");
		
			while (trash->library_path) {
				free((void *)trash->library_path);
				if (trash->config)
					free((void *)trash->config);

				++trash;
			}	

			free(c->supported_devices);
			c->supported_devices = NULL;
		}

	}

out_put:
	json_object_put(root);
out_free:
	free(cfg_buf);
	return res;
}
