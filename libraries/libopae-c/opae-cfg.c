// Copyright(c) 2023, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <opae/log.h>
#include "mock/opae_std.h"
#include "cfg-file.h"

typedef struct _libopae_parse_context {
	libopae_config_data *cfg;
	libopae_config_data *current;
	int table_entries;
} libopae_parse_context;

STATIC int resize_context(libopae_parse_context *ctx,
			  int num_new_entries)
{
	libopae_config_data *save;
	int save_entries;
	int i;

	if (!ctx->table_entries) {
		// No table yet - allocate one.
		// +1 for NULL terminator row.
		ctx->cfg = opae_calloc(num_new_entries + 1,
				       sizeof(libopae_config_data));
		if (!ctx->cfg) {
			OPAE_ERR("calloc() failed");
			return 1;
		}
		ctx->current = ctx->cfg;
		ctx->table_entries = num_new_entries + 1;

		return 0;
	}

	// We've already allocated ctx->cfg, so we need
	// to resize it.
	save = ctx->cfg;
	save_entries = ctx->table_entries;

	ctx->table_entries += num_new_entries;
	ctx->cfg = opae_calloc(ctx->table_entries,
			       sizeof(libopae_config_data));
	if (!ctx->cfg) {
		OPAE_ERR("calloc() failed");
		ctx->cfg = save;
		ctx->table_entries = save_entries;
		return 2;
	}

	for (i = 0 ; i < save_entries - 1 ; ++i)
		ctx->cfg[i] = save[i];

	ctx->current = &ctx->cfg[i];
	opae_free(save);

	return 0;
}

STATIC int parse_plugin_config(json_object *root,
			       const char *cfg_name,
			       libopae_parse_context *ctx)
{
	int res = 0;
	json_object *j_configurations = NULL;
	json_object *j_config = NULL;
	json_object *j_config_enabled = NULL;
	bool config_enabled = false;
	json_object *j_config_devices = NULL;
	int num_devices = 0;
	opae_pci_device *pci_devices = NULL;
	int d;
	int i;
	int j;
	json_object *j_opae = NULL;
	json_object *j_plugin = NULL;
	int num_plugins = 0;
	int table_entries = 0;

	// Get the main "configurations" key.
	if (!json_object_object_get_ex(root,
				       "configurations",
				       &j_configurations)) {
		OPAE_ERR("Error parsing JSON: missing 'configurations'");
		return 1;
	}

	// Get the config object corresponding to cfg_name.
	if (!json_object_object_get_ex(j_configurations,
				       cfg_name,
				       &j_config)) {
		OPAE_ERR("Error parsing JSON: missing '%s' config", cfg_name);
		return 2;
	}

	// Determine whether the config is enabled.
	j_config_enabled = parse_json_boolean(j_config,
					      "enabled",
					      &config_enabled);
	if (!j_config_enabled || !config_enabled) {
		// "enabled" key not found or is false.
		OPAE_DBG("config %s is disabled", cfg_name);
		return 0; // not fatal
	}

	// Get the "devices" array from the current config.
	j_config_devices = parse_json_array(j_config,
					    "devices",
					    &num_devices);
	if (!j_config_devices || !num_devices) {
		OPAE_ERR("\"devices\" not found or empty");
		return 3;
	}

	// Allocate a chunk of memory to hold the parsed "devices" data.
	pci_devices = opae_calloc(num_devices, sizeof(opae_pci_device));
	if (!pci_devices) {
		OPAE_ERR("calloc() failed");
		return 4;
	}

	// Parse each "devices" array entry.
	for (d = 0 ; d < num_devices ; ++d) {
		json_object *j_config_devices_i =
			json_object_array_get_idx(j_config_devices, d);
		char *name = NULL;
		json_object *j_id = NULL;
		int id_len = 0;

		if (!parse_json_string(j_config_devices_i,
				       "name",
				       &name)) {
			OPAE_ERR("config's devices[%d] "
				 "has no \"name\" key", d);
			res = 5;
			goto out_free;
		}
		pci_devices[d].name = name;

		j_id = parse_json_array(j_config_devices_i,
					"id",
					&id_len);
		if (!j_id || id_len != ID_SIZE) {
			OPAE_ERR("devices[%d] \"id\" field not present "
				 "or wrong size", d);
			res = 6;
			goto out_free;
		}

		if (opae_parse_device_id(j_id, &pci_devices[d])) {
			res = 7;
			goto out_free;
		}
	}

	// Get the "opae" object from the config.
	if (!json_object_object_get_ex(j_config, "opae", &j_opae)) {
		OPAE_ERR("Error parsing JSON: missing \"opae\" in config");
		res = 8;
		goto out_free;
	}

	// Get the "plugin" object from "opae".
	j_plugin = parse_json_array(j_opae, "plugin", &num_plugins);
	if (!j_plugin || !num_plugins) {
		OPAE_DBG("Skipping \"plugin\" section in %s", cfg_name);
		goto out_free; // not fatal (res == 0)
	}

	// First scan of "plugin" counts the required number of
	// configuration table entries.
	for (i = 0 ; i < num_plugins ; ++i) {
		json_object *j_plugin_i =
			json_object_array_get_idx(j_plugin, i);
		int num_devices_i = 0;
		json_object *j_devices = NULL;
		json_object *j_enabled = NULL;
		bool enabled = false;

		j_enabled =
			parse_json_boolean(j_plugin_i, "enabled", &enabled);
		if (!j_enabled || !enabled) {
			OPAE_DBG("plugin[%d] in %s not enabled. skipping",
				 i, cfg_name);
			continue;
		}

		j_devices =
			parse_json_array(j_plugin_i, "devices", &num_devices_i);
		if (!j_devices || !num_devices_i) {
			OPAE_DBG("plugin[%d] \"devices\" in %s missing "
				 "or empty. skipping", i, cfg_name);
			continue;
		}

		table_entries += num_devices_i;
	}

	// Resize our configuration data table.
	if (resize_context(ctx, table_entries)) {
		res = 9;
		goto out_free;
	}

	// Second scan of "plugin" populates the context's
	// configuration table entries.
	for (i = 0 ; i < num_plugins ; ++i) {
		json_object *j_plugin_i =
			json_object_array_get_idx(j_plugin, i);
		int num_devices_i = 0;
		json_object *j_devices = NULL;
		json_object *j_enabled = NULL;
		bool enabled = false;
		json_object *j_module = NULL;
		char *module = NULL;
		json_object *j_configuration = NULL;
		char *configuration = NULL;

		j_enabled =
			parse_json_boolean(j_plugin_i, "enabled", &enabled);
		if (!j_enabled || !enabled) {
			OPAE_DBG("plugin[%d] in %s not enabled. skipping",
				 i, cfg_name);
			continue;
		}

		j_devices =
			parse_json_array(j_plugin_i, "devices", &num_devices_i);
		if (!j_devices || !num_devices_i) {
			OPAE_DBG("plugin[%d] \"devices\" in %s missing "
				 "or empty. skipping", i, cfg_name);
			continue;
		}

		j_module =
			parse_json_string(j_plugin_i, "module", &module);
		if (!j_module || !strlen(module)) {
			OPAE_ERR("plugin[%d] \"module\" in %s missing "
				 "or empty.", i, cfg_name);
			res = 10;
			goto out_free;
		}

		if (!json_object_object_get_ex(j_plugin_i,
					       "configuration",
					       &j_configuration)) {
			OPAE_ERR("plugin[%d] \"configuration\" in %s missing "
				 "or invalid.", i, cfg_name);
			res = 11;
			goto out_free;
		}

		configuration = (char *)
			json_object_to_json_string_ext(j_configuration,
						       JSON_C_TO_STRING_PLAIN);

		for (j = 0 ; j < num_devices_i ; ++j) {
			json_object *j_device_j =
				json_object_array_get_idx(j_devices, j);
			const char *dev_name = NULL;
			libopae_config_data *pcfg;
			opae_pci_device *dev;

			if (!json_object_is_type(j_device_j,
						 json_type_string)) {
				OPAE_ERR("Non-string JSON item "
					 "found in 'devices[%d]'", j);
				res = 11;
				goto out_free;
			}

			dev_name = json_object_get_string(j_device_j);

			// Find the device named dev_name in pci_devices.
			for (d = 0 ; d < num_devices ; ++d) {
				if (!strncmp(dev_name,
					     pci_devices[d].name,
					     MAX_DEV_NAME))
					break;
			}

			if (d == num_devices) {
				OPAE_ERR("device %s not found\n", dev_name);
				res = 12;
				goto out_free;
			}

			pcfg = ctx->current;
			++(ctx->current);
			dev = &pci_devices[d];

			pcfg->vendor_id = dev->vendor_id;
			pcfg->device_id = dev->device_id;
			pcfg->subsystem_vendor_id = dev->subsystem_vendor_id;
			pcfg->subsystem_device_id = dev->subsystem_device_id;

			pcfg->module_library = opae_strdup(module);
			pcfg->config_json = opae_strdup(configuration);

			if (!pcfg->module_library || !pcfg->config_json) {
				OPAE_ERR("strdup() failed");
				res = 13;
				goto out_free;
			}
		}
	}

out_free:
	opae_free(pci_devices);
	return res;
}

libopae_config_data *
opae_parse_libopae_json(const char *json_input)
{
	enum json_tokener_error j_err = json_tokener_success;
	json_object *root = NULL;
	json_object *j_configs = NULL;

	int i;
	int num_configs = 0;

	libopae_parse_context ctx = { NULL, NULL, 0 };

	root = json_tokener_parse_verbose(json_input, &j_err);
	if (!root) {
		OPAE_ERR("Config JSON parse failed: %s",
			 json_tokener_error_desc(j_err));
		goto out_free;
	}

	j_configs = parse_json_array(root, "configs", &num_configs);
	if (!j_configs) {
		OPAE_ERR("Failed to find \"configs\" in configuration JSON.");
		OPAE_ERR("You may have encountered an old config file.");
		OPAE_ERR("Enable debug logging to see which file is found.");
		goto out_free;
	}

	for (i = 0 ; i < num_configs ; ++i) {
		json_object *j_config_i =
			json_object_array_get_idx(j_configs, i);
		const char *config_i =
			json_object_get_string(j_config_i);

		if (parse_plugin_config(root, config_i, &ctx))
			goto out_parse_failed;
	}

	goto out_free;

out_parse_failed:
	opae_free_libopae_config(ctx.cfg);
	ctx.cfg = NULL;
out_free:
	if (root)
		json_object_put(root);
	opae_free((char *)json_input);
	return ctx.cfg;
}
