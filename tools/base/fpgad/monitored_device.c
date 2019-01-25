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

#include <stdio.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <glob.h>

#include "monitored_device.h"
#include "monitor_thread.h"
#include "api/sysfs.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("monitored_device: " format, ##__VA_ARGS__)

fpgad_supported_device supported_devices_table[] = {
	{ 0x8086, 0xbcc0, "libfpgad-xfpga.so", 0, NULL },
	{ 0x8086, 0xbcc1, "libfpgad-xfpga.so", 0, NULL },
	{ 0x8086, 0x0b30,    "libfpgad-vc.so", 0, NULL },
	{ 0x8086, 0x0b31,    "libfpgad-vc.so", 0, NULL },
	{      0,      0,                NULL, 0, NULL },
};

STATIC fpgad_supported_device *mon_is_loaded(const char *library_path)
{
	errno_t err;
	unsigned i;
	int res = 0;

	for (i = 0 ; supported_devices_table[i].library_path ; ++i) {
		fpgad_supported_device *d = &supported_devices_table[i];

		err = strcmp_s(library_path, PATH_MAX,
				d->library_path, &res);
		if (err) {
			LOG("strcmp_s failed, skipping");
			continue;
		}

		if (!res && (d->flags & FPGAD_DEV_LOADED))
			return d;
	}
	return NULL;
}

STATIC fpgad_monitored_device *
allocate_monitored_device(struct fpgad_config *config,
			  fpgad_supported_device *supported,
			  fpga_token token,
			  uint64_t object_id,
			  fpga_objtype object_type,
			  struct bitstream_info *bitstr)
{
	fpgad_monitored_device *d;

	d = (fpgad_monitored_device *) calloc(
			1, sizeof(fpgad_monitored_device));

	if (!d) {
		LOG("out of memory");
		return NULL;
	}

	d->config = config;
	d->supported = supported;
	d->token = token;
	d->object_id = object_id;
	d->object_type = object_type;
	d->bitstr = bitstr;

	return d;
}

STATIC bool mon_consider_device(struct fpgad_config *c, fpga_token token)
{
	unsigned i;
	fpga_properties props = NULL;
	fpga_result res;
	uint16_t vendor_id;
	uint16_t device_id;
	uint64_t object_id;
	fpga_objtype object_type;
	struct bitstream_info *bitstr = NULL;
	bool added = false;

	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		LOG("failed to get properties\n");
		return false;
	}

	vendor_id = 0;
	res = fpgaPropertiesGetVendorID(props, &vendor_id);
	if (res != FPGA_OK) {
		LOG("failed to get vendor ID\n");
		fpgaDestroyProperties(&props);
		return false;
	}

	device_id = 0;
	res = fpgaPropertiesGetDeviceID(props, &device_id);
	if (res != FPGA_OK) {
		LOG("failed to get device ID\n");
		fpgaDestroyProperties(&props);
		return false;
	}

	object_id = 0;
	res = fpgaPropertiesGetObjectID(props, &object_id);
	if (res != FPGA_OK) {
		LOG("failed to get object ID\n");
		fpgaDestroyProperties(&props);
		return false;
	}

	object_type = FPGA_ACCELERATOR;
	res = fpgaPropertiesGetObjectType(props, &object_type);
	if (res != FPGA_OK) {
		LOG("failed to get object type\n");
		fpgaDestroyProperties(&props);
		return false;
	}

	// Do we have a NULL GBS from the command line
	// that matches this device?

	if (object_type == FPGA_DEVICE) {
		fpga_guid pr_ifc_id;
		unsigned i;

		// The token's guid is the PR interface ID.

		res = fpgaPropertiesGetGUID(props, &pr_ifc_id);
		if (res != FPGA_OK) {
			LOG("failed to get PR interface ID\n");
			fpgaDestroyProperties(&props);
			return false;
		}

		for (i = 0 ; i < c->num_null_gbs ; ++i) {
			if (!uuid_compare(c->null_gbs[i].interface_id,
					  pr_ifc_id)) {
				bitstr = &c->null_gbs[i];
				break;
			}
		}
	} else {
		// The parent token's guid is the PR interface ID.
		fpga_token parent = NULL;
		fpga_properties parent_props = NULL;
		fpga_guid pr_ifc_id;
		unsigned i;

		res = fpgaPropertiesGetParent(props, &parent);
		if (res != FPGA_OK) {
			LOG("failed to get parent token\n");
			fpgaDestroyProperties(&props);
			return false;
		}

		res = fpgaGetProperties(parent, &parent_props);
		if (res != FPGA_OK) {
			LOG("failed to get parent properties\n");
			fpgaDestroyToken(&parent);
			fpgaDestroyProperties(&props);
			return false;
		}

		res = fpgaPropertiesGetGUID(parent_props, &pr_ifc_id);
		if (res != FPGA_OK) {
			LOG("failed to get PR interface ID\n");
			fpgaDestroyProperties(&parent_props);
			fpgaDestroyToken(&parent);
			fpgaDestroyProperties(&props);
			return false;
		}

		fpgaDestroyProperties(&parent_props);
		fpgaDestroyToken(&parent);

		for (i = 0 ; i < c->num_null_gbs ; ++i) {
			if (!uuid_compare(c->null_gbs[i].interface_id,
					  pr_ifc_id)) {
				bitstr = &c->null_gbs[i];
				break;
			}
		}
	}

	fpgaDestroyProperties(&props);

	for (i = 0 ; supported_devices_table[i].library_path ; ++i) {
		fpgad_supported_device *d = &supported_devices_table[i];

		// Do we support this device?
		if (d->vendor_id == vendor_id &&
		    d->device_id == device_id) {
			fpgad_supported_device *loaded_by;
			fpgad_monitored_device *monitored;
			fpgad_plugin_configure_t cfg;

			d->flags |= FPGAD_DEV_DETECTED;

			// Is the fpgad plugin already loaded?
			loaded_by = mon_is_loaded(d->library_path);

			if (loaded_by) {
				// The two table entries will share the
				// same plugin handle (but only loaded_by
				// will have FPGAD_DEV_LOADED).
				d->dl_handle = loaded_by->dl_handle;
			} else {
				// Plugin hasn't been loaded.
				// Load it now.
				d->dl_handle = dlopen(d->library_path,
							RTLD_LAZY|RTLD_LOCAL);
				if (!d->dl_handle) {
					char *err = dlerror();
					LOG("failed to load \"%s\" %s\n",
							d->library_path,
							err ? err : "");
					continue;
				}

				d->flags |= FPGAD_DEV_LOADED;
			}

			if (!bitstr) {
				LOG("Warning: no NULL GBS for vid=0x%04x "
					"did=0x%04x objid=0x%x (%s)\n",
				vendor_id,
				device_id,
				object_id,
				object_type == FPGA_ACCELERATOR ?
				"accelerator" : "device");
			}

			// Add the device to the monitored list.
			monitored = allocate_monitored_device(c,
							      d,
							      token,
							      object_id,
							      object_type,
							      bitstr);
			if (!monitored) {
				LOG("failed to add device 0x%04x:0x%04x\n",
					vendor_id, device_id);
				continue;
			}

			// Success
			cfg = (fpgad_plugin_configure_t)
				dlsym(d->dl_handle,
				      FPGAD_PLUGIN_CONFIGURE);
			if (!cfg) {
				LOG("failed to find %s in \"%s\"\n",
					FPGAD_PLUGIN_CONFIGURE,
					d->library_path);
				free(monitored);
				continue;
			}

			/* TODO pass configuration settings */
			cfg(monitored, NULL);

			if (monitored->type == FPGAD_PLUGIN_TYPE_THREAD) {

				if (monitored->thread_fn) {

					if (pthread_create(&monitored->thread,
							   NULL,
							   monitored->thread_fn,
							   monitored)) {
						LOG("failed to create thread"
						    " for \"%s\"\n",
						    d->library_path);
						free(monitored);
						continue;
					}

				} else {
					LOG("Thread plugin \"%s\" has no "
					    "thread_fn\n", d->library_path);
					free(monitored);
					continue;
				}

			}

			mon_monitor_device(monitored);
			added = true;
			break;
		}
	}

	return added;
}

int mon_enumerate(struct fpgad_config *c)
{
	fpga_token *tokens = NULL;
	fpga_result res;
	uint32_t num_matches = 0;
	uint32_t i;
	unsigned monitored_devices = 0;

	res = fpgaEnumerate(NULL, 0, NULL, 0, &num_matches);
	if (res != FPGA_OK) {
		LOG("enumeration failed\n");
		return res;
	}

	if (!num_matches) {
		res = 1;
		return res;
	}

	tokens = calloc(num_matches, sizeof(fpga_token));
	if (!tokens) {
		res = 1;
		LOG("out of memory\n");
		return res;
	}

	res = fpgaEnumerate(NULL, 0, tokens, num_matches, &num_matches);
	if (res != FPGA_OK) {
		LOG("enumeration failed (2)\n");
		goto out_exit;
	}

	for (i = 0 ; i < num_matches ; ++i) {
		if (!mon_consider_device(c, tokens[i])) {
			// Not monitoring it. Destroy the token.
			fpgaDestroyToken(&tokens[i]);
		} else {
			++monitored_devices;
		}
	}

out_exit:
	if (tokens)
		free(tokens);
	return res + (monitored_devices ? 0 : 1);
}
