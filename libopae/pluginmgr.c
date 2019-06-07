// Copyright(c) 2018, Intel Corporation
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
#endif /* HAVE_CONFIG_H */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/limits.h>
#include <pthread.h>
#include <pwd.h>
#include <unistd.h>

#include <json-c/json.h>
#include "safe_string/safe_string.h"

#include "pluginmgr.h"
#include "opae_int.h"

#define OPAE_PLUGIN_CONFIGURE "opae_plugin_configure"
typedef int (*opae_plugin_configure_t)(opae_api_adapter_table *, const char *);

typedef struct _platform_data {
	uint16_t vendor_id;
	uint16_t device_id;
	const char *native_plugin;
	uint32_t flags;
#define OPAE_PLATFORM_DATA_DETECTED 0x00000001
#define OPAE_PLATFORM_DATA_LOADED   0x00000002
} platform_data;

static platform_data platform_data_table[] = {
	{ 0x8086, 0xbcbd, "libxfpga.so", 0 },
	{ 0x8086, 0xbcc0, "libxfpga.so", 0 },
	{ 0x8086, 0xbcc1, "libxfpga.so", 0 },
	{ 0x8086, 0x09c4, "libxfpga.so", 0 },
	{ 0x8086, 0x09c5, "libxfpga.so", 0 },
	{ 0x8086, 0x0b2b, "libxfpga.so", 0 },
	{ 0x8086, 0x0b2c, "libxfpga.so", 0 },
	{ 0x8086, 0x0b30, "libxfpga.so", 0 },
	{ 0x8086, 0x0b31, "libxfpga.so", 0 },
	{      0,      0,          NULL, 0 },
};

static int initialized;

STATIC opae_api_adapter_table *adapter_list = (void *)0;
static pthread_mutex_t adapter_list_lock =
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define MAX_PLUGINS PLUGIN_SUPPORTED_DEVICES_MAX
STATIC plugin_cfg *opae_plugin_mgr_config_list;
STATIC int opae_plugin_mgr_plugin_count;

#define HOME_CFG_PATHS 3
STATIC const char *_opae_home_cfg_files[HOME_CFG_PATHS] = {
	"/.local/opae.cfg",
	"/.local/opae/opae.cfg",
	"/.config/opae/opae.cfg",
};
#define SYS_CFG_PATHS 2
STATIC const char *_opae_sys_cfg_files[SYS_CFG_PATHS] = {
	"/usr/local/etc/opae/opae.cfg",
	"/etc/opae/opae.cfg",
};


// Find the canonicalized configuration file. If null, the file was not found.
// Otherwise, it's the first configuration file found from a list of possible
// paths. Note: The char * returned is allocated here, caller must free.
STATIC char *find_cfg()
{
	int i = 0;
	char *file_name = NULL;
	char home_cfg[PATH_MAX] = {0};
	char *home_cfg_ptr = &home_cfg[0];
	// get the user's home directory
	struct passwd *user_passwd = getpwuid(getuid());

	// first look in possible paths in the users home directory
	for (i = 0; i < HOME_CFG_PATHS; ++i) {
		if (strcpy_s(home_cfg, PATH_MAX, user_passwd->pw_dir)) {
			OPAE_ERR("error copying pw_dir string");
			return NULL;
		}
		home_cfg_ptr = home_cfg + strlen(home_cfg);
		if (strcpy_s(home_cfg_ptr, PATH_MAX, _opae_home_cfg_files[i])) {
			OPAE_ERR("error copying opae cfg dir string: %s",
				 _opae_home_cfg_files[i]);
			return NULL;
		}
		file_name = canonicalize_file_name(home_cfg);
		if (file_name) {
			return file_name;
		} else {
			home_cfg[0] = '\0';
		}
	}
	// now look in possible system paths
	for (i = 0; i < SYS_CFG_PATHS; ++i) {
		strcpy_s(home_cfg, PATH_MAX, _opae_sys_cfg_files[i]);
		file_name = canonicalize_file_name(home_cfg);
		if (file_name) {
			return file_name;
		}
	}
	return NULL;
}

STATIC opae_api_adapter_table *opae_plugin_mgr_alloc_adapter(const char *lib_path)
{
	void *dl_handle;
	opae_api_adapter_table *adapter;

	dl_handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);

	if (!dl_handle) {
		char *err = dlerror();
		OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err : "");
		return NULL;
	}

	adapter = (opae_api_adapter_table *)calloc(
		1, sizeof(opae_api_adapter_table));

	if (!adapter) {
		dlclose(dl_handle);
		OPAE_ERR("out of memory");
		return NULL;
	}

	adapter->plugin.path = (char *)lib_path;
	adapter->plugin.dl_handle = dl_handle;

	return adapter;
}

STATIC int opae_plugin_mgr_free_adapter(opae_api_adapter_table *adapter)
{
	int res;
	char *err;

	res = dlclose(adapter->plugin.dl_handle);

	if (res) {
		err = dlerror();
		OPAE_ERR("dlclose failed with %d %s", res, err ? err : "");
	}

	free(adapter);

	return res;
}

STATIC int opae_plugin_mgr_configure_plugin(opae_api_adapter_table *adapter,
					    const char *config)
{
	opae_plugin_configure_t cfg;

	cfg = (opae_plugin_configure_t)dlsym(adapter->plugin.dl_handle,
					     OPAE_PLUGIN_CONFIGURE);

	if (!cfg) {
		OPAE_ERR("failed to find %s in \"%s\"", OPAE_PLUGIN_CONFIGURE,
			 adapter->plugin.path);
		return 1;
	}

	return cfg(adapter, config);
}

STATIC void opae_plugin_mgr_reset_cfg(void)
{
	plugin_cfg *ptr = opae_plugin_mgr_config_list;
	plugin_cfg *tmp = NULL;
	while (ptr) {
		tmp = ptr;
		ptr = ptr->next;
		free(tmp->cfg);
		free(tmp);
	}
	opae_plugin_mgr_config_list = NULL;
	opae_plugin_mgr_plugin_count = 0;
}

STATIC void opae_plugin_mgr_add_plugin(plugin_cfg *cfg)
{
	plugin_cfg *ptr = opae_plugin_mgr_config_list;
	cfg->next = NULL;
	if (!ptr) {
		opae_plugin_mgr_config_list = cfg;
	} else {
		while (ptr->next) {
			ptr = ptr->next;
		}
		ptr->next = cfg;
	}
	opae_plugin_mgr_plugin_count++;
}

STATIC int opae_plugin_mgr_initialize_all(void)
{
	int res;
	opae_api_adapter_table *aptr;
	int errors = 0;

	for (aptr = adapter_list; aptr; aptr = aptr->next) {

		if (aptr->initialize) {
			res = aptr->initialize();
			if (res) {
				OPAE_MSG("\"%s\" initialize() routine failed",
					 aptr->plugin.path);
				++errors;
			}
		}
	}

	return errors;
}

int opae_plugin_mgr_finalize_all(void)
{
	int res;
	opae_api_adapter_table *aptr;
	int errors = 0;
	int i = 0;

	opae_mutex_lock(res, &adapter_list_lock);

	for (aptr = adapter_list; aptr;) {
		opae_api_adapter_table *trash;

		if (aptr->finalize) {
			res = aptr->finalize();
			if (res) {
				OPAE_MSG("\"%s\" finalize() routine failed",
					 aptr->plugin.path);
				++errors;
			}
		}

		trash = aptr;
		aptr = aptr->next;

		if (opae_plugin_mgr_free_adapter(trash))
			++errors;
	}

	adapter_list = NULL;

	// reset platforms detected to 0
	for (i = 0 ; platform_data_table[i].native_plugin ; ++i) {
		platform_data_table[i].flags = 0;
	}

	initialized = 0;
	opae_plugin_mgr_reset_cfg();
	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;
}

#define JSON_GET(_jobj, _key, _jvar)                                           \
	do {                                                                   \
		if (!json_object_object_get_ex(_jobj, _key, _jvar)) {          \
			OPAE_ERR("Error getting object: %s", _key);            \
			return 1;                                              \
		}                                                              \
	} while (0)

#define MAX_PLUGIN_CFG_SIZE 1024
STATIC int process_plugin(const char *name, json_object *j_config)
{
	json_object *j_plugin = NULL;
	json_object *j_plugin_cfg = NULL;
	json_object *j_enabled = NULL;
	const char *stringified = NULL;
	
	JSON_GET(j_config, "plugin", &j_plugin);
	JSON_GET(j_config, "configuration", &j_plugin_cfg);
	JSON_GET(j_config, "enabled", &j_enabled);
	
	plugin_cfg *cfg = malloc(sizeof(plugin_cfg));
	if (!cfg) {
		OPAE_ERR("Could not allocate memory for plugin cfg");
		return 1;
	}

	cfg->cfg = NULL;
	if (json_object_get_string_len(j_plugin) > PLUGIN_NAME_MAX) {
		OPAE_ERR("plugin name too long");
		goto out_err;
	}

	stringified = json_object_to_json_string_ext(j_plugin_cfg, JSON_C_TO_STRING_PLAIN);
	if (!stringified) {
		OPAE_ERR("error getting plugin configuration");
		goto out_err;
	}

	cfg->cfg_size = strlen(stringified) + 1;
	cfg->cfg = malloc(cfg->cfg_size);
	if (!cfg->cfg) {
		OPAE_ERR("error allocating memory for plugin configuration");
		cfg->cfg_size = 0;
		goto out_err;
	}

	if (strncpy_s(cfg->cfg, MAX_PLUGIN_CFG_SIZE, stringified, cfg->cfg_size)) {
		OPAE_ERR("error copying plugin configuration");
		goto out_err;
	}

	if (strcpy_s(cfg->name, PLUGIN_NAME_MAX, name)) {
		OPAE_ERR("error copying plugin name");
		goto out_err;
	}

	if (strcpy_s(cfg->plugin, PLUGIN_NAME_MAX, json_object_get_string(j_plugin))) {
		OPAE_ERR("error copying plugin file name");
		goto out_err;
	}

	cfg->enabled = json_object_get_boolean(j_enabled);
	opae_plugin_mgr_add_plugin(cfg);
	return 0;
out_err:
	if (cfg->cfg) {
		free(cfg->cfg);
		cfg->cfg = NULL;
	}
	free(cfg);
	return 1;
}

STATIC int process_cfg_buffer(const char *buffer, const char *filename)
{
	int num_plugins = 0;
	int num_errors = 0;
	int i = 0;
	int res = 1;
	json_object *root = NULL;
	json_object *j_plugins = NULL;
	json_object *j_configs = NULL;
	json_object *j_plugin = NULL;
	json_object *j_config = NULL;
	const char *plugin_name = NULL;
	enum json_tokener_error j_err = json_tokener_success;

	root = json_tokener_parse_verbose(buffer, &j_err);
	if (!root) {
		OPAE_ERR("Error parsing config file: '%s' - %s", filename,
			 json_tokener_error_desc(j_err));
		goto out_free;
	}

	if (!json_object_object_get_ex(root, "plugins", &j_plugins)) {
		OPAE_ERR("Error parsing config file: '%s' - missing 'plugins'", filename);
		goto out_free;
	}
	if (!json_object_object_get_ex(root, "configurations", &j_configs)) {
		OPAE_ERR("Error parsing config file: '%s' - missing 'configs'", filename);
		goto out_free;
	}

	if (!json_object_is_type(j_plugins, json_type_array)) {
		OPAE_ERR("'plugins' JSON object not array type");
		goto out_free;
	}

	num_plugins = json_object_array_length(j_plugins);
	num_errors = 0;
	for (i = 0; i < num_plugins; ++i) {
		j_plugin = json_object_array_get_idx(j_plugins, i);
		plugin_name = json_object_get_string(j_plugin);

		if (json_object_object_get_ex(j_configs, plugin_name, &j_config)) {
			num_errors += process_plugin(plugin_name, j_config);
		} else {
			OPAE_ERR("Could not find plugin configuration for '%s'", plugin_name);
			num_errors += 1;
		}
	}
	res = num_errors;


out_free:
	json_object_put(root);
	return res;
}

#define MAX_CFG_SIZE 4096
STATIC int opae_plugin_mgr_parse_config(const char *filename)
{
	char buffer[MAX_CFG_SIZE] = { 0 };
	char *ptr = &buffer[0];
	size_t bytes_read = 0, total_read = 0;
	FILE *fp = NULL;
	if (filename) {
		fp = fopen(filename, "r");
	} else {
		OPAE_MSG("config file is NULL");
		return 1;
	}

	if (!fp) {
		OPAE_ERR("Error opening config file: %s", filename);
		return 1;
	}

	while ((bytes_read = fread(ptr + total_read, 1, 1, fp))
	       && total_read < MAX_CFG_SIZE) {
		total_read += bytes_read;
	}

	if (ferror(fp)) {
		OPAE_ERR("Error reading config file: %s - %s", filename, strerror(errno));
		goto out_err;
	}
	if (!feof(fp)) {
		OPAE_ERR("Unknown error reading config file: %s", filename);
		goto out_err;
	}
	fclose(fp);
	fp = NULL;

	return process_cfg_buffer(buffer, filename);
out_err:
	fclose(fp);
	fp = NULL;
	return 1;
}

STATIC int opae_plugin_mgr_register_adapter(opae_api_adapter_table *adapter)
{
	opae_api_adapter_table *aptr;

	adapter->next = NULL;

	if (!adapter_list) {
		adapter_list = adapter;
		return 0;
	}

	// new entries go to the end of the list.
	for (aptr = adapter_list; aptr->next; aptr = aptr->next)
		/* find the last entry */;

	aptr->next = adapter;

	return 0;
}

STATIC void opae_plugin_mgr_detect_platform(uint16_t vendor, uint16_t device)
{
	int i;

	for (i = 0 ; platform_data_table[i].native_plugin ; ++i) {

		if (platform_data_table[i].vendor_id == vendor &&
		    platform_data_table[i].device_id == device) {
			OPAE_DBG("platform detected: vid=0x%04x did=0x%04x -> %s",
					vendor, device,
					platform_data_table[i].native_plugin);

			platform_data_table[i].flags |= OPAE_PLATFORM_DATA_DETECTED;
		}

	}
}

STATIC int opae_plugin_mgr_detect_platforms(void)
{
	DIR *dir;
	char base_dir[PATH_MAX];
	char file_path[PATH_MAX];
	struct dirent *dirent;
	int res;
	int errors = 0;

	// Iterate over the directories in /sys/bus/pci/devices.
	// This directory contains symbolic links to device directories
	// where 'vendor' and 'device' files exist.

	strncpy_s(base_dir, sizeof(base_dir),
			"/sys/bus/pci/devices", 21);

	dir = opendir(base_dir);
	if (!dir) {
		OPAE_ERR("Failed to open %s. Aborting platform detection.", base_dir);
		return 1;
	}

	while ((dirent = readdir(dir)) != NULL) {
		FILE *fp;
		unsigned vendor = 0;
		unsigned device = 0;

		if (EOK != strcmp_s(dirent->d_name, sizeof(dirent->d_name),
					".", &res)) {
			OPAE_ERR("strcmp_s failed");
			++errors;
			goto out_close;
		}

		if (0 == res) // don't process .
			continue;

		if (EOK != strcmp_s(dirent->d_name, sizeof(dirent->d_name),
					"..", &res)) {
			OPAE_ERR("strcmp_s failed");
			++errors;
			goto out_close;
		}

		if (0 == res) // don't process ..
			continue;

		// Read the 'vendor' file.
		snprintf_s_ss(file_path, sizeof(file_path),
				"%s/%s/vendor", base_dir, dirent->d_name);

		fp = fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &vendor)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			fclose(fp);
			++errors;
			goto out_close;
		}

		fclose(fp);

		// Read the 'device' file.
		snprintf_s_ss(file_path, sizeof(file_path),
				"%s/%s/device", base_dir, dirent->d_name);

		fp = fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &device)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			fclose(fp);
			++errors;
			goto out_close;
		}

		fclose(fp);

		// Detect platform for this (vendor, device).
		opae_plugin_mgr_detect_platform((uint16_t) vendor, (uint16_t) device);
	}

out_close:
	closedir(dir);
	return errors;
}

STATIC int opae_plugin_mgr_load_cfg_plugin(plugin_cfg *cfg)
{
	int res = 0;
	opae_api_adapter_table *adapter = NULL;

	if (cfg->enabled && cfg->cfg && cfg->cfg_size) {
		adapter = opae_plugin_mgr_alloc_adapter(cfg->plugin);
		if (!adapter) {
			OPAE_ERR("malloc failed");
			return 1;
		}
		res = opae_plugin_mgr_configure_plugin(adapter, cfg->cfg);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("failed to configure plugin \"%s\"",
				 cfg->name);
			return 1;
		}

		res = opae_plugin_mgr_register_adapter(adapter);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("Failed to register \"%s\"", cfg->name);
			return 1;
		}

	}

	return 0;
}

STATIC int opae_plugin_mgr_load_cfg_plugins(void)
{
	plugin_cfg *ptr = opae_plugin_mgr_config_list;
	int errors = 0;
	while (ptr) {
		errors += opae_plugin_mgr_load_cfg_plugin(ptr);
		ptr = ptr->next;
	}
	return errors;
}

STATIC int opae_plugin_mgr_load_dflt_plugins(int *platforms_detected)
{
	int i = 0, j = 0;
	int res = 0;
	opae_api_adapter_table *adapter = NULL;
	int errors = opae_plugin_mgr_detect_platforms();
	if (errors)
		return errors;
	// Load each of the native plugins that were detected.
	*platforms_detected = 0;

	for (i = 0 ; platform_data_table[i].native_plugin ; ++i) {
		const char *native_plugin;
		int already_loaded;

		if (!(platform_data_table[i].flags & OPAE_PLATFORM_DATA_DETECTED))
			continue; // This platform was not detected.

		native_plugin = platform_data_table[i].native_plugin;
		(*platforms_detected)++;

		// Iterate over the table again to prevent multiple loads
		// of the same native plugin.
		already_loaded = 0;
		for (j = 0 ; platform_data_table[j].native_plugin ; ++j) {

			if (EOK != strcmp_s(native_plugin, strnlen_s(native_plugin, 256),
						platform_data_table[j].native_plugin, &res)) {
				OPAE_ERR("strcmp_s failed");
				return ++errors;
			}

			if (!res &&
			    (platform_data_table[j].flags & OPAE_PLATFORM_DATA_LOADED)) {
				already_loaded = 1;
				break;
			}
		}

		if (already_loaded)
			continue;

		adapter = opae_plugin_mgr_alloc_adapter(native_plugin);

		if (!adapter) {
			OPAE_ERR("malloc failed");
			return ++errors;
		}

		// TODO: pass serialized json for native plugin
		res = opae_plugin_mgr_configure_plugin(adapter, "");
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("failed to configure plugin \"%s\"",
				 native_plugin);
			++errors;
			continue; // Keep going.
		}

		res = opae_plugin_mgr_register_adapter(adapter);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("Failed to register \"%s\"", native_plugin);
			++errors;
			continue; // Keep going.
		}

		platform_data_table[i].flags |= OPAE_PLATFORM_DATA_LOADED;
	}
	return errors;
}

int opae_plugin_mgr_initialize(const char *cfg_file)
{
	int res;
	int errors = 0;
	int platforms_detected = 0;
	opae_plugin_mgr_plugin_count = 0;
	char *found_cfg = NULL;
	const char *use_cfg = NULL;

	opae_mutex_lock(res, &adapter_list_lock);

	if (initialized) { // prevent multiple init.
		opae_mutex_unlock(res, &adapter_list_lock);
		return 0;
	}
	found_cfg = find_cfg();
	use_cfg = cfg_file ? cfg_file : found_cfg;
	if (use_cfg) {
		opae_plugin_mgr_parse_config(use_cfg);
		if (found_cfg) {
			free(found_cfg);
		}
	}

	if (opae_plugin_mgr_plugin_count) {
		errors = opae_plugin_mgr_load_cfg_plugins();
	} else {
		// fail-safe, try to detect plugins based on supported devices
		errors = opae_plugin_mgr_load_dflt_plugins(&platforms_detected);
	}

	if (errors)
		goto out_unlock;

	// Call each plugin's initialization routine.
	errors += opae_plugin_mgr_initialize_all();

	if (!errors && (opae_plugin_mgr_plugin_count || platforms_detected))
		initialized = 1;

out_unlock:
	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;
}

int opae_plugin_mgr_for_each_adapter
	(int (*callback)(const opae_api_adapter_table *, void *), void *context)
{
	int res;
	int cb_res = OPAE_ENUM_CONTINUE;
	opae_api_adapter_table *aptr;

	if (!callback) {
		OPAE_ERR("NULL callback passed to %s()", __func__);
		return OPAE_ENUM_STOP;
	}

	opae_mutex_lock(res, &adapter_list_lock);

	for (aptr = adapter_list; aptr; aptr = aptr->next) {
		cb_res = callback(aptr, context);
		if (cb_res)
			break;
	}

	opae_mutex_unlock(res, &adapter_list_lock);

	return cb_res;
}
