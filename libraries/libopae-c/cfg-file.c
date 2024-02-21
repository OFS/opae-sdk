// Copyright(c) 2022-2023, Intel Corporation
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
#include <string.h>
#include <pwd.h>
#include <linux/limits.h>
#include <opae/log.h>
#include "opae_int.h"
#include "cfg-file.h"
#include "mock/opae_std.h"


#define CFG_PATH_MAX 64
#define HOME_CFG_PATHS 3
STATIC const char _opae_home_cfg_files[HOME_CFG_PATHS][CFG_PATH_MAX] = {
	{ "/.local/opae.cfg" },
	{ "/.local/opae/opae.cfg" },
	{ "/.config/opae/opae.cfg" },
};
#define SYS_CFG_PATHS 2
STATIC const char _opae_sys_cfg_files[SYS_CFG_PATHS][CFG_PATH_MAX] = {
	{ "/usr/local/etc/opae/opae.cfg" },
	{ "/etc/opae/opae.cfg" },
};

// Find the canonicalized configuration file. If NULL, the file was not found.
// Otherwise, it's the first configuration file found by checking environment
// variable "LIBOPAE_CFGFILE", then a list of possible hard-coded paths.
// Note: The char * returned is allocated here, caller must free.
char *opae_find_cfg_file(void)
{
	int i = 0;
	char *file_name = NULL;
	char home_cfg[PATH_MAX] = { 0, };
	char *home_cfg_ptr = NULL;
	size_t len;
	struct passwd *user_passwd;
	char *home_dir;

	file_name = getenv("LIBOPAE_CFGFILE");
	if (file_name) {
		file_name = opae_canonicalize_file_name(file_name);
		if (file_name) {
			OPAE_DBG("Found config file: %s", file_name);
			return file_name;
		}
	}

	// Let the HOME env variable take precedence over the pw database.
	home_dir = getenv("HOME");
	if (!home_dir) {
		// No HOME: get the user's home directory, according
		// to the pw database.
		user_passwd = getpwuid(getuid());
		if (user_passwd)
			home_dir = user_passwd->pw_dir;
	}

	if (home_dir) {
		for (i = 0 ; i < HOME_CFG_PATHS ; ++i) {
			len = strnlen(home_dir, sizeof(home_cfg) - 1);
			memcpy(home_cfg, home_dir, len);
			home_cfg[len] = '\0';

			home_cfg_ptr = home_cfg + strlen(home_cfg);

			len = strnlen(_opae_home_cfg_files[i], CFG_PATH_MAX);
			memcpy(home_cfg_ptr, _opae_home_cfg_files[i], len);
			home_cfg_ptr[len] = '\0';

			file_name = opae_canonicalize_file_name(home_cfg);
			if (file_name) {
				OPAE_DBG("Found config file: %s", file_name);
				return file_name;
			}

			home_cfg[0] = '\0';
		}
	}

	// Now, look in possible system paths.
	for (i = 0 ; i < SYS_CFG_PATHS ; ++i) {
		len = strnlen(_opae_sys_cfg_files[i], CFG_PATH_MAX);
		memcpy(home_cfg, _opae_sys_cfg_files[i], len);
		home_cfg[len] = '\0';

		file_name = opae_canonicalize_file_name(home_cfg);
		if (file_name) {
			OPAE_DBG("Found config file: %s", file_name);
			return file_name;
		}
	}

	return NULL;
}

#define MAX_CFG_SIZE (8 * 4096)
char *opae_read_cfg_file(const char *config_file_path)
{
	char *ptr = NULL;
	size_t file_size = 0;
	size_t bytes_read = 0;
	size_t total_read = 0;
	FILE *fp = NULL;

	if (!config_file_path) {
		OPAE_DBG("config file is NULL");
		return NULL;
	}

	fp = opae_fopen(config_file_path, "r");
	if (!fp) {
		OPAE_ERR("Error opening config file: %s",
			 config_file_path);
		goto out;
	}

	fseek(fp, 0, SEEK_END);
	file_size = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (file_size > MAX_CFG_SIZE) {
		OPAE_ERR("config file %s is too large. Max size: %ld",
			 config_file_path, MAX_CFG_SIZE);
		goto out_close;
	}

	ptr = opae_malloc(file_size + 1);
	if (!ptr) {
		OPAE_ERR("malloc failed for config buffer");
		goto out_close;
	}

	do {
		bytes_read = fread(ptr + total_read,
				   1,
				   file_size - total_read,
				   fp);
		total_read += bytes_read;

	} while ((total_read < file_size) && !ferror(fp));

	if (ferror(fp)) {
		OPAE_ERR("Error reading config file: %s - %s",
			 config_file_path, strerror(errno));
		goto out_free;
	}

	ptr[total_read] = '\0';
	goto out_close;

out_free:
	opae_free(ptr);
	ptr = NULL;
out_close:
	opae_fclose(fp);
out:
	return ptr;
}

int opae_parse_device_id(json_object *j_id,
			 opae_pci_device *dev)
{
	int len;
	int i;
	//                            VID,  DID,  SVID, SDID
	char *id_strings[ID_SIZE] = { NULL, NULL, NULL, NULL };
	unsigned long u;
	const char *json = json_object_get_string(j_id);

	if (!json_object_is_type(j_id, json_type_array)) {
		OPAE_ERR("\"id\" value is not an array: %s",
			 json);
		return 1;
	}

	len = json_object_array_length(j_id);
	if (len < ID_SIZE) {
		OPAE_ERR("\"id\" has fewer than 4 entries: %s",
			 json);
		return 2;
	}

	for (i = 0 ; i < ID_SIZE ; ++i) {
		json_object *j_id_i =
			json_object_array_get_idx(j_id, i);

		if (!json_object_is_type(j_id_i, json_type_string)) {
			OPAE_ERR("\"id\" array member %d not a string: %s",
				 i, json);
			return 3;
		}

		id_strings[i] = (char *)json_object_get_string(j_id_i);
	}

	u = 0;
	if (string_to_unsigned_wildcard(id_strings[0], &u, OPAE_VENDOR_ANY)) {
		OPAE_ERR("Vendor ID not an integer in %s", json);
		return 4;
	}
	dev->vendor_id = u;

	u = 0;
	if (string_to_unsigned_wildcard(id_strings[1], &u, OPAE_DEVICE_ANY)) {
		OPAE_ERR("Device ID not an integer in %s", json);
		return 5;
	}
	dev->device_id = u;

	u = 0;
	if (string_to_unsigned_wildcard(id_strings[2], &u, OPAE_VENDOR_ANY)) {
		OPAE_ERR("Subsystem Vendor ID not an integer in %s", json);
		return 6;
	}
	dev->subsystem_vendor_id = u;

	u = 0;
	if (string_to_unsigned_wildcard(id_strings[3], &u, OPAE_DEVICE_ANY)) {
		OPAE_ERR("Subsystem Device ID not an integer in %s", json);
		return 7;
	}
	dev->subsystem_device_id = u;

	return 0;
}


STATIC libopae_config_data default_libopae_config_table[] = {
	{ 0x1c2c, 0x1000, 0x0000,          0x0000,          "libxfpga.so",  "{}", 0 }, // N5010
	{ 0x1c2c, 0x1001, 0x0000,          0x0000,          "libxfpga.so",  "{}", 0 }, // N5011
	{ 0x1c2c, 0x1002, 0x0000,          0x0000,          "libxfpga.so",  "{}", 0 }, // N5013
	{ 0x1c2c, 0x1003, 0x0000,          0x0000,          "libxfpga.so",  "{}", 0 }, // N5014
	{ 0x1ded, 0x8103, 0x1ded,          0x4342,          "libxfpga.so",  "{}", 0 }, // F5
	{ 0x8086, 0xbcbd, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // MCP
	{ 0x8086, 0xbcc0, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // MCP
	{ 0x8086, 0xbcc1, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // MCP
	{ 0x8086, 0x09c4, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // A10GX
	{ 0x8086, 0x09c5, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // A10GX
	{ 0x8086, 0x0ddb, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // CMC
	{ 0x8086, 0x0ddb, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libopae-v.so",  "{}", 0 }, // CMC
	{ 0x8086, 0x0ddb, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libopae-u.so",  "{}", 0 }, // CMC

	{ 0x8086, 0x0b2b, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // D5005
	{ 0x8086, 0x0b2c, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // D5005
	{ 0x8086, 0xbcce, 0x8086,          0x138d,          "libxfpga.so",  "{}", 0 }, // D5005
	{ 0x8086, 0xbcce, 0x8086,          0x138d,          "libopae-v.so", "{}", 0 }, // D5005
	{ 0x8086, 0xbccf, 0x8086,          0x138d,          "libopae-v.so", "{}", 0 }, // D5005
	{ 0x8086, 0xbcce, 0x8086,          0x138d,          "libopae-u.so", "{}", 0 }, // D5005
	{ 0x8086, 0xbccf, 0x8086,          0x138d,          "libopae-u.so", "{}", 0 }, // D5005


	{ 0x8086, 0x0b30, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // N3000
	{ 0x8086, 0x0b31, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libxfpga.so",  "{}", 0 }, // N3000
	{ 0x8086, 0xaf00, 0x8086,          0x0000,          "libxfpga.so",  "{}", 0 }, // OFS EA
	{ 0x8086, 0xaf00, 0x8086,          0x0000,          "libopae-v.so", "{}", 0 }, // OFS EA
	{ 0x8086, 0xaf01, 0x8086,          0x0000,          "libopae-v.so", "{}", 0 }, // OFS EA
	{ 0x8086, 0xaf00, 0x8086,          0x0000,          "libopae-u.so", "{}", 0 }, // OFS EA
	{ 0x8086, 0xaf01, 0x8086,          0x0000,          "libopae-u.so", "{}", 0 }, // OFS EA


	{ 0x8086, 0xbcce, 0x8086,          0x0000,          "libxfpga.so",  "{}", 0 }, // OFS
	{ 0x8086, 0xbcce, 0x8086,          0x0000,          "libopae-v.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbccf, 0x8086,          0x0000,          "libopae-v.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbcce, 0x8086,          0x0000,          "libopae-u.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbccf, 0x8086,          0x0000,          "libopae-u.so", "{}", 0 }, // OFS


	{ 0x8086, 0xbcce, 0x8086,          0x0001,          "libxfpga.so",  "{}", 0 }, // OFS
	{ 0x8086, 0xbcce, 0x8086,          0x0001,          "libopae-v.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbccf, 0x8086,          0x0001,          "libopae-v.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbcce, 0x8086,          0x0001,          "libopae-u.so", "{}", 0 }, // OFS
	{ 0x8086, 0xbccf, 0x8086,          0x0001,          "libopae-u.so", "{}", 0 }, // OFS


	{ 0x8086, 0xbcce, 0x8086,          0x1770,          "libxfpga.so",  "{}", 0 }, // N6000
	{ 0x8086, 0xbcce, 0x8086,          0x1770,          "libopae-v.so", "{}", 0 }, // N6000
	{ 0x8086, 0xbccf, 0x8086,          0x1770,          "libopae-v.so", "{}", 0 }, // N6000
	{ 0x8086, 0xbcce, 0x8086,          0x1770,          "libopae-u.so", "{}", 0 }, // N6000
	{ 0x8086, 0xbccf, 0x8086,          0x1770,          "libopae-u.so", "{}", 0 }, // N6000


	{ 0x8086, 0xbcce, 0x8086,          0x1771,          "libxfpga.so",  "{}", 0 }, // N6001
	{ 0x8086, 0xbcce, 0x8086,          0x1771,          "libopae-v.so", "{}", 0 }, // N6001
	{ 0x8086, 0xbccf, 0x8086,          0x1771,          "libopae-v.so", "{}", 0 }, // N6001
	{ 0x8086, 0xbcce, 0x8086,          0x1771,          "libopae-u.so", "{}", 0 }, // N6001
	{ 0x8086, 0xbccf, 0x8086,          0x1771,          "libopae-u.so", "{}", 0 }, // N6001


	{ 0x8086, 0xbcce, 0x8086,          0x17d4,          "libxfpga.so",  "{}", 0 }, // C6100
	{ 0x8086, 0xbcce, 0x8086,          0x17d4,          "libopae-v.so", "{}", 0 }, // C6100
	{ 0x8086, 0xbccf, 0x8086,          0x17d4,          "libopae-v.so", "{}", 0 }, // C6100
	{ 0x8086, 0xbcce, 0x8086,          0x17d4,          "libopae-u.so", "{}", 0 }, // C6100
	{ 0x8086, 0xbccf, 0x8086,          0x17d4,          "libopae-u.so", "{}", 0 }, // C6100


	{      0,      0,      0,               0,          NULL,           NULL, 0 },
};

libopae_config_data *
opae_parse_libopae_json(const char *cfgfile, const char *json_input);

libopae_config_data *
opae_parse_libopae_config(const char *cfgfile, const char *json_input)
{
	libopae_config_data *c = NULL;

	if (json_input)
		c = opae_parse_libopae_json(cfgfile, json_input);

	return c ? c : default_libopae_config_table;
}

void opae_print_libopae_config(libopae_config_data *cfg)
{
#ifndef LIBOPAE_DEBUG
	UNUSED_PARAM(cfg);
#else
	const int l = OPAE_LOG_DEBUG;

	opae_print(l, "libopae config:\n");

	while (cfg->module_library) {
		opae_print(l,
			   "0x%04x 0x%04x ",
			   cfg->vendor_id, cfg->device_id);

		if (cfg->subsystem_vendor_id == OPAE_VENDOR_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subsystem_vendor_id);

		if (cfg->subsystem_device_id == OPAE_DEVICE_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subsystem_device_id);

		opae_print(l, "%-12s %s 0x%08x\n",
			   cfg->module_library,
			   cfg->config_json,
			   cfg->flags);

		++cfg;
	}
#endif // LIBOPAE_DEBUG
}

void opae_free_libopae_config(libopae_config_data *cfg)
{
	libopae_config_data *base;

	if (cfg == default_libopae_config_table || !cfg)
		return;

	base = cfg;
	while (cfg->module_library) {
		if (cfg->module_library)
			opae_free((char *)cfg->module_library);
		if (cfg->config_json)
			opae_free((char *)cfg->config_json);
		++cfg;
	}

	opae_free(base);
}


STATIC fpgainfo_config_data default_fpgainfo_config_table[] = {
	{ 0x8086, 0x09c4, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_a10gx.so", NULL,
	"Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA" },

	{ 0x8086, 0x09c5, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_a10gx.so", NULL,
	"Intel Programmable Acceleration Card with Intel Arria 10 GX FPGA" },

	{ 0x8086, 0x0b30, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_n3000.so", NULL,
	"Intel FPGA Programmable Acceleration Card N3000" },

	{ 0x8086, 0x0b31, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_n3000.so", NULL,
	"Intel FPGA Programmable Acceleration Card N3000" },

	{ 0x8086, 0x0b2b, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_d5005.so", NULL,
	"Intel FPGA Programmable Acceleration Card D5005" },

	{ 0x8086, 0x0b2c, 0x8086, 0x0, OPAE_FEATURE_ID_ANY, "libboard_d5005.so", NULL,
	"Intel FPGA Programmable Acceleration Card D5005" },

	// Max10 SPI feature id 0xe
	{ 0x1c2c, 0x1000, 0x0, 0x0, 0xe, "libboard_n5010.so", NULL,
	"Silicom FPGA SmartNIC N5010 Series" },

	{ 0x1c2c, 0x1001, 0x0, 0x0, 0xe, "libboard_n5010.so", NULL,
	"Silicom FPGA SmartNIC N5010 Series" },

	{ 0x1c2c, 0x1002, 0x0, 0x0, 0xe, "libboard_n5010.so", NULL,
	"Silicom FPGA SmartNIC N5013" },

	{ 0x1c2c, 0x1003, 0x0, 0x0, 0xe, "libboard_n5010.so", NULL,
	"Silicom FPGA SmartNIC N5014" },

	{ 0x8086, 0xaf00, 0x8086, 0x0, 0xe, "libboard_d5005.so", NULL,
	"Intel Open FPGA Stack Platform" },

	{ 0x8086, 0xbcce, 0x8086, 0x0, 0xe, "libboard_d5005.so", NULL,
	"Intel Open FPGA Stack Platform" },

	{ 0x8086, 0xbcce, 0x8086, 0x138d, 0xe, "libboard_d5005.so", NULL,
	"Intel Open FPGA Stack Platform" },

	// Max10 PMCI feature id 0x12
	{ 0x8086, 0xaf00, 0x8086, 0x0, 0x12, "libboard_n6000.so", NULL,
	"Intel Open FPGA Stack Platform" },

	{ 0x8086, 0xbcce, 0x8086, 0x1770, 0x12, "libboard_n6000.so", NULL,
	"Intel Acceleration Development Platform N6000" },

	{ 0x8086, 0xbcce, 0x8086, 0x1771, 0x12, "libboard_n6000.so", NULL,
	"Intel Acceleration Development Platform N6001" },

	{ 0x8086, 0xbcce, 0x8086, 0x17d4, 0x12, "libboard_c6100.so", NULL,
	"Intel IPU Platform F2000X-PL" },

	{ 0x8086, 0x0ddb, 0x8086, 0x0, 0x23, "libboard_cmc.so", NULL,
	"Intel Acceleration Development Platform CMC" },

	{ 0x8086, 0xbcce, 0x8086, 0x0001, 0x0, "libboard_jtag_pci_dk.so", NULL,
	"Intel Acceleration Development Platform 0001" },

	{ 0x8086, 0xbccf, 0x8086, 0x0001, 0x0, "libboard_jtag_pci_dk.so", NULL,
	"Intel Acceleration Development Platform 0001" },

	{ 0,      0, 0, 0, -1,         NULL, NULL, "" }
};

fpgainfo_config_data *
opae_parse_fpgainfo_json(const char *json_input);

fpgainfo_config_data *
opae_parse_fpgainfo_config(const char *json_input)
{
	fpgainfo_config_data *c = NULL;

	if (json_input)
		c = opae_parse_fpgainfo_json(json_input);

	return c ? c : default_fpgainfo_config_table;
}

void opae_print_fpgainfo_config(fpgainfo_config_data *cfg)
{
#ifndef LIBOPAE_DEBUG
	UNUSED_PARAM(cfg);
#else
	const int l = OPAE_LOG_DEBUG;

	opae_print(l, "fpgainfo config:\n");

	while (cfg->board_plugin) {
		opae_print(l,
			   "0x%04x 0x%04x ",
			   cfg->vendor_id, cfg->device_id);

		if (cfg->subvendor_id == OPAE_VENDOR_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subvendor_id);

		if (cfg->subdevice_id == OPAE_DEVICE_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subdevice_id);

		if (cfg->feature_id == OPAE_FEATURE_ID_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->feature_id);

		opae_print(l, "%-12s 0x%08x %s\n",
			   cfg->board_plugin,
			   cfg->dl_handle,
			   cfg->product_name);

		++cfg;
	}
#endif // LIBOPAE_DEBUG
}

void opae_free_fpgainfo_config(fpgainfo_config_data *cfg)
{
	fpgainfo_config_data *base;

	if (cfg == default_fpgainfo_config_table || !cfg)
		return;

	base = cfg;
	while (cfg->board_plugin) {
		opae_free((char *)cfg->board_plugin);
		++cfg;
	}

	opae_free(base);
}


STATIC fpgad_config_data default_fpgad_config_table[] = {
	{ 0x8086, 0xbcc0, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libfpgad-xfpga.so", 0, NULL, "{}" },

	{ 0x8086, 0x0b30, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [ { \"enabled\": true, \"name\": \"12V AUX Voltage\", \"low-warn\": 11.40, \"low-fatal\": 10.56 } ] }" },

	{ 0x1c2c, 0x1000,          0x0000,          0x0000, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x1c2c, 0x1001,          0x0000,          0x0000, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x8086, 0x0ddb,          OPAE_VENDOR_ANY,          OPAE_DEVICE_ANY, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

    { 0x8086, 0xbcce,          0x8086,          0x1770, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x8086, 0xbcce,          0x8086,          0x1771, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x8086, 0xbcce,          0x8086,          0x17d4, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x8086, 0xaf00,          0x8086,          0x0000, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{ 0x8086, 0xbcce,          0x8086,          0x0000, "libfpgad-vc.so", 0, NULL, "{ \"cool-down\": 30, \"get-aer\": [ \"setpci -s %s ECAP_AER+0x08.L\", \"setpci -s %s ECAP_AER+0x14.L\" ], \"disable-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0xffffffff\", \"setpci -s %s ECAP_AER+0x14.L=0xffffffff\" ], \"set-aer\": [ \"setpci -s %s ECAP_AER+0x08.L=0x%08x\", \"setpci -s %s ECAP_AER+0x14.L=0x%08x\" ], \"sensor-overrides\": [] }" },

	{      0,      0,               0,               0, NULL,             0, NULL, ""   },
};

fpgad_config_data *
opae_parse_fpgad_json(const char *json_input);

fpgad_config_data *
opae_parse_fpgad_config(const char *json_input)
{
	fpgad_config_data *c = NULL;

	if (json_input)
		c = opae_parse_fpgad_json(json_input);

	return c ? c : default_fpgad_config_table;
}

void opae_print_fpgad_config(fpgad_config_data *cfg)
{
#ifndef LIBOPAE_DEBUG
	UNUSED_PARAM(cfg);
#else
	const int l = OPAE_LOG_DEBUG;

	opae_print(l, "fpgad config:\n");

	while (cfg->module_library) {
		opae_print(l,
			   "0x%04x 0x%04x ",
			   cfg->vendor_id, cfg->device_id);

		if (cfg->subsystem_vendor_id == OPAE_VENDOR_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subsystem_vendor_id);

		if (cfg->subsystem_device_id == OPAE_DEVICE_ANY)
			opae_print(l, "*      ");
		else
			opae_print(l, "0x%04x ", cfg->subsystem_device_id);

		opae_print(l, "%-12s 0x%08x 0x%08x %s\n",
			   cfg->module_library,
			   cfg->flags,
			   cfg->dl_handle,
			   cfg->config_json);

		++cfg;
	}
#endif // LIBOPAE_DEBUG
}

void opae_free_fpgad_config(fpgad_config_data *cfg)
{
	fpgad_config_data *base;

	if (cfg == default_fpgad_config_table || !cfg)
		return;

	base = cfg;
	while (cfg->module_library) {
		if (cfg->module_library)
			opae_free((char *)cfg->module_library);
		if (cfg->config_json)
			opae_free((char *)cfg->config_json);
		++cfg;
	}

	opae_free(base);
}
