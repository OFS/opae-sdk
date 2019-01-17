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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/limits.h>
#define __USE_GNU
#include <pthread.h>

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
	{ 0x8086, 0x0b30, "libxfpga.so", 0 },
	{      0,      0,          NULL, 0 },
};

static int initialized;

STATIC opae_api_adapter_table *adapter_list = (void *)0;
static pthread_mutex_t adapter_list_lock =
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

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

	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;
}

STATIC int opae_plugin_mgr_parse_config(/* json_object *jobj */)
{

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

int opae_plugin_mgr_initialize(const char *cfg_file)
{
	int i;
	int j;
	int res;
	int errors = 0;
	int platforms_detected = 0;
	opae_api_adapter_table *adapter;

	// TODO: parse config file
	UNUSED_PARAM(cfg_file);
	opae_plugin_mgr_parse_config();

	opae_mutex_lock(res, &adapter_list_lock);

	if (initialized) { // prevent multiple init.
		opae_mutex_unlock(res, &adapter_list_lock);
		return 0;
	}

	errors = opae_plugin_mgr_detect_platforms();
	if (errors)
		goto out_unlock;

	// Load each of the native plugins that were detected.

	for (i = 0 ; platform_data_table[i].native_plugin ; ++i) {
		const char *native_plugin;
		int already_loaded;

		if (!(platform_data_table[i].flags & OPAE_PLATFORM_DATA_DETECTED))
			continue; // This platform was not detected.

		native_plugin = platform_data_table[i].native_plugin;
		platforms_detected++;

		// Iterate over the table again to prevent multiple loads
		// of the same native plugin.
		already_loaded = 0;
		for (j = 0 ; platform_data_table[j].native_plugin ; ++j) {

			if (EOK != strcmp_s(native_plugin, strnlen_s(native_plugin, 256),
						platform_data_table[j].native_plugin, &res)) {
				OPAE_ERR("strcmp_s failed");
				++errors;
				goto out_unlock;
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
			++errors;
			goto out_unlock;
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

	// TODO: load non-native plugins described in config file.

	// Call each plugin's initialization routine.
	errors += opae_plugin_mgr_initialize_all();

	if (!errors && platforms_detected)
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
