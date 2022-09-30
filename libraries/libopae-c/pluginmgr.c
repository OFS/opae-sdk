// Copyright(c) 2018-2022, Intel Corporation
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
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/limits.h>
#include <pthread.h>
#include <pwd.h>
#include <unistd.h>

#include "pluginmgr.h"
#include "opae_int.h"
#include "mock/opae_std.h"
#include "cfg-file.h"

#define OPAE_PLUGIN_CONFIGURE "opae_plugin_configure"
typedef int (*opae_plugin_configure_t)(opae_api_adapter_table *, const char *);

static libopae_config_data *platform_data_table;

int initialized;
STATIC int finalizing;

STATIC opae_api_adapter_table *adapter_list = (void *)0;
static pthread_mutex_t adapter_list_lock =
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC void *opae_plugin_mgr_find_plugin(const char *lib_path)
{
	char plugin_path[PATH_MAX];
	const char *search_paths[] = { OPAE_MODULE_SEARCH_PATHS };
	unsigned i;
	void *dl_handle;

	for (i = 0 ;
		i < sizeof(search_paths) / sizeof(search_paths[0]) ; ++i) {

		snprintf(plugin_path, sizeof(plugin_path),
			 "%s%s", search_paths[i], lib_path);

		dl_handle = dlopen(plugin_path, RTLD_LAZY | RTLD_LOCAL);

		if (dl_handle)
			return dl_handle;
	}

	return NULL;
}

STATIC opae_api_adapter_table *opae_plugin_mgr_alloc_adapter(const char *lib_path)
{
	void *dl_handle;
	opae_api_adapter_table *adapter;

	dl_handle = opae_plugin_mgr_find_plugin(lib_path);

	if (!dl_handle) {
		char *err = dlerror();
		OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err : "");
		return NULL;
	}

	adapter = (opae_api_adapter_table *)opae_calloc(
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

	opae_free(adapter);

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
	libopae_config_data *cfg;

	opae_mutex_lock(res, &adapter_list_lock);

	if (finalizing) {
		opae_mutex_unlock(res, &adapter_list_lock);
		return 0;
	}

	finalizing = 1;

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

	if (platform_data_table) {
		for (cfg = platform_data_table ; cfg->module_library ; ++cfg) {
			cfg->flags = 0;
		}
	}

	opae_free_libopae_config(platform_data_table);
	platform_data_table = NULL;

	initialized = 0;
	finalizing = 0;
	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;
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

STATIC void opae_plugin_mgr_detect_platform(opae_pci_device *dev)
{
	int i;
	bool match;

	for (i = 0 ; platform_data_table[i].module_library ; ++i) {
		match = true;

		if ((platform_data_table[i].vendor_id != dev->vendor_id) ||
		    (platform_data_table[i].device_id != dev->device_id))
			match = false;

		if ((platform_data_table[i].subsystem_vendor_id !=
			OPAE_VENDOR_ANY) &&
		    (platform_data_table[i].subsystem_vendor_id !=
			dev->subsystem_vendor_id))
			match = false;

		if ((platform_data_table[i].subsystem_device_id !=
			OPAE_DEVICE_ANY) &&
		    (platform_data_table[i].subsystem_device_id !=
			dev->subsystem_device_id))
			match = false;

		if (match) {
			OPAE_DBG("platform detected: 0x%04x:0x%04x 0x%04x:0x%04x -> %s",
				 dev->vendor_id, dev->device_id,
				 dev->subsystem_vendor_id, dev->subsystem_device_id,
				 platform_data_table[i].module_library);

			platform_data_table[i].flags |= OPAE_PLATFORM_DATA_DETECTED;
		}

	}
}

STATIC int opae_plugin_mgr_detect_platforms(bool with_ase)
{
	DIR *dir;
	char base_dir[PATH_MAX];
	char file_path[PATH_MAX];
	struct dirent *dirent;
	int errors = 0;

	if (with_ase) {
		opae_pci_device ase_pf = {
			.name = "ase",
			.vendor_id = 0x8086,
			.device_id = 0x0a5e,
			.subsystem_vendor_id = 0x8086,
			.subsystem_device_id = 0x0a5e
		};

		opae_plugin_mgr_detect_platform(&ase_pf);
		return 0;
	}

	// Iterate over the directories in /sys/bus/pci/devices.
	// This directory contains symbolic links to device directories
	// where 'vendor', 'device', 'subsystem_vendor', and
	// 'subsystem_device' files exist.

	memcpy(base_dir, "/sys/bus/pci/devices", 21);

	dir = opae_opendir(base_dir);
	if (!dir) {
		OPAE_ERR("Failed to open %s. Aborting platform detection.", base_dir);
		return 1;
	}

	while ((dirent = readdir(dir)) != NULL) {
		FILE *fp;
		opae_pci_device dev = { NULL, 0, 0, 0, 0 };
		unsigned vendor_id = 0;
		unsigned device_id = 0;
		unsigned subsystem_vendor_id = 0;
		unsigned subsystem_device_id = 0;

		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, ".."))
			continue;

		// Read the 'vendor' file.
		if (snprintf(file_path, sizeof(file_path),
			     "%s/%s/vendor",
			     base_dir,
			     dirent->d_name) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			++errors;
			goto out_close;
		}

		fp = opae_fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &vendor_id)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			opae_fclose(fp);
			++errors;
			goto out_close;
		}

		opae_fclose(fp);

		// Read the 'device' file.
		if (snprintf(file_path, sizeof(file_path),
			     "%s/%s/device",
			     base_dir,
			     dirent->d_name) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			++errors;
			goto out_close;
		}

		fp = opae_fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &device_id)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			opae_fclose(fp);
			++errors;
			goto out_close;
		}

		opae_fclose(fp);

		// Read the 'subsystem_vendor' file.
		if (snprintf(file_path, sizeof(file_path),
			     "%s/%s/subsystem_vendor",
			     base_dir,
			     dirent->d_name) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			++errors;
			goto out_close;
		}

		fp = opae_fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &subsystem_vendor_id)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			opae_fclose(fp);
			++errors;
			goto out_close;
		}

		opae_fclose(fp);

		// Read the 'subsystem_device' file.
		if (snprintf(file_path, sizeof(file_path),
			     "%s/%s/subsystem_device",
			     base_dir,
			     dirent->d_name) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			++errors;
			goto out_close;
		}

		fp = opae_fopen(file_path, "r");
		if (!fp) {
			OPAE_ERR("Failed to open %s. Aborting platform detection.", file_path);
			++errors;
			goto out_close;
		}

		if (EOF == fscanf(fp, "%x", &subsystem_device_id)) {
			OPAE_ERR("Failed to read %s. Aborting platform detection.", file_path);
			opae_fclose(fp);
			++errors;
			goto out_close;
		}

		opae_fclose(fp);

		// Detect platform for this opae_pci_device.
		dev.vendor_id = (uint16_t)vendor_id;
		dev.device_id = (uint16_t)device_id;
		dev.subsystem_vendor_id = (uint16_t)subsystem_vendor_id;
		dev.subsystem_device_id = (uint16_t)subsystem_device_id;

		opae_plugin_mgr_detect_platform(&dev);
	}

out_close:
	opae_closedir(dir);
	return errors;
}

STATIC int opae_plugin_mgr_load_plugins(int *platforms_detected)
{
	int i = 0;
	int j = 0;
	int res = 0;
	opae_api_adapter_table *adapter = NULL;
	int errors;

	errors = opae_plugin_mgr_detect_platforms(getenv("WITH_ASE") != NULL);
	if (errors)
		return errors;

	// Load each of the plugins that were detected.
	*platforms_detected = 0;

	for (i = 0 ; platform_data_table[i].module_library ; ++i) {
		const char *plugin;
		bool already_loaded;

		if (!(platform_data_table[i].flags & OPAE_PLATFORM_DATA_DETECTED))
			continue; // This platform was not detected.

		plugin = platform_data_table[i].module_library;
		(*platforms_detected)++;

		// Iterate over the table again to prevent multiple loads
		// of the same plugin.
		already_loaded = false;
		for (j = 0 ; platform_data_table[j].module_library ; ++j) {

			if (!strcmp(plugin, platform_data_table[j].module_library) &&
			    (platform_data_table[j].flags & OPAE_PLATFORM_DATA_LOADED)) {
				already_loaded = true;
				break;
			}

		}

		if (already_loaded)
			continue;

		adapter = opae_plugin_mgr_alloc_adapter(plugin);

		if (!adapter) {
			OPAE_ERR("calloc failed");
			return ++errors;
		}

		res = opae_plugin_mgr_configure_plugin(adapter,
			platform_data_table[i].config_json);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("failed to configure plugin \"%s\"", plugin);
			++errors;
			continue; // Keep going.
		}

		res = opae_plugin_mgr_register_adapter(adapter);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("Failed to register \"%s\"", plugin);
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
	bool free_config = false;
	char *raw_config = NULL;
	int platforms_detected = 0;
	int errors = 0;

	opae_mutex_lock(res, &adapter_list_lock);

	if (initialized) { // prevent multiple init.
		opae_mutex_unlock(res, &adapter_list_lock);
		return 0;
	}
	initialized = 1;

	// If we were given a path to a cfg_file, then assume the
	// caller will free() it. Otherwise, when we search for
	// one with opae_find_cfg_file(), the path will be allocated,
	// and we'll need to free it here in this function.
	if (!cfg_file) {
		cfg_file = (const char *)opae_find_cfg_file();
		if (cfg_file)
			free_config = true;
	}

	if (cfg_file) {
		raw_config = opae_read_cfg_file(cfg_file);
	}

	// Parse the config file content, allocating and initializing
	// our configuration table. If raw_config is non-NULL, then
	// this function will delete it.
	platform_data_table = opae_parse_libopae_config(raw_config);

	// Print the config table for debug builds.
	opae_print_libopae_config(platform_data_table);

	errors = opae_plugin_mgr_load_plugins(&platforms_detected);
	if (errors) {
		initialized = 0;
		goto out_unlock;
	}

	// Call each plugin's initialization routine.
	errors += opae_plugin_mgr_initialize_all();

	initialized = 0;
	if (!errors && platforms_detected)
		initialized = 1;

out_unlock:
	if (!initialized) {
		opae_free_libopae_config(platform_data_table);
		platform_data_table = NULL;
	}
	if (free_config)
		opae_free((char *)cfg_file);
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
		switch (cb_res) {
		case FPGA_OK:        // Fall through
		case FPGA_NO_DRIVER: // Fall through
		case FPGA_NOT_FOUND:
			break;
		default:
			goto out_unlock;
		}
	}

out_unlock:
	opae_mutex_unlock(res, &adapter_list_lock);

	return cb_res;
}
