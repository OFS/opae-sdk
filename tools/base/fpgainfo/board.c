// Copyright(c) 2019, Intel Corporation
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


#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <getopt.h>
#include "fpgainfo.h"
#include <opae/fpga.h>
#include <wchar.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <pthread.h>

#include "safe_string/safe_string.h"

#include "board.h"


static pthread_mutex_t board_plugin_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

// Board plug-in table
static platform_data platform_data_table[] = {
	{ 0x8086, 0x09c4, "libboard_rc.so", NULL },
	{ 0x8086, 0x09c5, "libboard_rc.so", NULL },
	{ 0,      0,          NULL, NULL },
};


fpga_result load_board_plugin(fpga_token token, void** dl_handle)
{
	fpga_result res                = FPGA_OK;
	fpga_result resval             = FPGA_OK;
	fpga_properties props          = NULL;
	uint16_t vendor_id             = 0;
	uint16_t device_id             = 0;
	int i                          = 0;

	if (token == NULL || dl_handle == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get properties\n");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaPropertiesGetDeviceID(props, &device_id);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get device ID\n");
		resval = res;
		goto destroy;
	}

	res = fpgaPropertiesGetVendorID(props, &vendor_id);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get vendor ID\n");
		resval = res;
		goto destroy;
	}

	if (pthread_mutex_lock(&board_plugin_lock) != 0 ) {
		OPAE_ERR("pthread mutex lock failed \n");
		resval = FPGA_EXCEPTION;
		goto destroy;
	}

	for (i = 0; platform_data_table[i].board_plugin; ++i) {

		if (platform_data_table[i].device_id == device_id &&
			platform_data_table[i].vendor_id == vendor_id) {

			// Loaded lib or found
			if (platform_data_table[i].dl_handle) {
				*dl_handle = platform_data_table[i].dl_handle;
				resval = FPGA_OK;
				goto unlock_destroy;
			}

			platform_data_table[i].dl_handle = dlopen(platform_data_table[i].board_plugin, RTLD_LAZY | RTLD_LOCAL);
			if (!platform_data_table[i].dl_handle) {
				char *err = dlerror();
				OPAE_ERR("Failed to load \"%s\" %s", platform_data_table[i].board_plugin, err ? err : "");
				resval = FPGA_EXCEPTION;
				goto unlock_destroy;
			} else {
				// Dynamically loaded board module
				*dl_handle = platform_data_table[i].dl_handle;
				resval = FPGA_OK;
				goto unlock_destroy;
			}
		} //end if

	} // end for


unlock_destroy:

	if (pthread_mutex_unlock(&board_plugin_lock) != 0) {
		OPAE_ERR("pthread mutex unlock failed \n");
		resval = FPGA_EXCEPTION;
	}

destroy:
	res = fpgaDestroyProperties(&props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}

	return resval;
}

int unload_board_plugin(void)
{
	int i              = 0;
	fpga_result res    = FPGA_OK;
	fpga_result resval = FPGA_OK;

	if (pthread_mutex_lock(&board_plugin_lock) != 0) {
		OPAE_ERR("pthread mutex lock failed \n");
		return FPGA_EXCEPTION;
	}

	for (i = 0; platform_data_table[i].board_plugin; ++i) {

		if (platform_data_table[i].dl_handle) {

			res = dlclose(platform_data_table[i].dl_handle);
			if (res) {
				char *err = dlerror();
				OPAE_ERR("dlclose failed with %d %s", res, err ? err : "");
				resval = FPGA_EXCEPTION;
			} else {
				platform_data_table[i].dl_handle = NULL;
			}
		} //end if

	} // end for

	if (pthread_mutex_unlock(&board_plugin_lock) != 0) {
		OPAE_ERR("pthread mutex unlock failed \n");
		resval = FPGA_EXCEPTION;
	}

	return resval;
}

/*
 * Print help
 */
void mac_help(void)
{
	printf("\nPrint MAC information\n"
		"        fpgainfo mac [-h]\n"
		"                -h,--help           Print this help\n"
		"\n");
}

#define MAC_GETOPT_STRING ":h"
int parse_mac_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};
	int getopt_ret;
	int option_index;

	optind = 0;
	while (-1 != (getopt_ret = getopt_long(argc, argv, MAC_GETOPT_STRING,
		longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h':   /* help */
			mac_help();
			return -1;

		case ':':   /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			mac_help();
			return -1;

		case '?':
		default:    /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			mac_help();
			return -1;
		}
	}

	return 0;
}

fpga_result mac_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res = FPGA_INVALID_PARAM;

	if (0 == parse_mac_args(argc, argv)) {
		res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
		fpgainfo_print_err("Setting type to FPGA_DEVICE", res);
	}
	return res;
}

fpga_result mac_command(fpga_token *tokens, int num_tokens, int argc,
	char *argv[])
{
	(void)argc;
	(void)argv;

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		mac_info(tokens[i]);

	}

	return FPGA_OK;
}


//phy

/*
 * Print help
 */
void phy_help(void)
{
	printf("\nPrint PHY information\n"
		"        fpgainfo phy [-h] [-G <group-number>]\n"
		"                -h,--help           Print this help\n"
		"                -G,--group          Select PHY group {0,1,all}\n"
		"\n");
}

#define PHY_GETOPT_STRING ":G:h"
int group_num;
int parse_phy_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"group", required_argument, NULL, 'G'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};
	int getopt_ret;
	int option_index;

	/* default configuration */
	group_num = -1;

	optind = 0;
	while (-1 != (getopt_ret = getopt_long(argc, argv, PHY_GETOPT_STRING,
		longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'G':
			if (NULL == tmp_optarg) {
				fprintf(stderr, "Invalid argument group\n");
				return -1;
			}
			if (!strcmp("0", tmp_optarg)) {
				group_num = 0;
			}
			else if (!strcmp("1", tmp_optarg)) {
				group_num = 1;
			}
			else if (!strcmp("all", tmp_optarg)) {
				group_num = -1;
			}
			else {
				fprintf(stderr, "Invalid argument '%s' of option group\n",
					tmp_optarg);
				return -1;
			}
			break;

		case 'h':   /* help */
			phy_help();
			return -1;

		case ':':   /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			phy_help();
			return -1;

		case '?':
		default:    /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			phy_help();
			return -1;
		}
	}

	return 0;
}

fpga_result phy_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res = FPGA_INVALID_PARAM;

	if (0 == parse_phy_args(argc, argv)) {
		res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
		fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	}
	return res;
}

fpga_result phy_command(fpga_token *tokens, int num_tokens, int argc,
	char *argv[])
{
	(void)argc;
	(void)argv;

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		phy_group_info(tokens[i]);

	}

	return FPGA_OK;
}


// prints board version info
fpga_result fpgainfo_board_info(fpga_token token)
{
	fpga_result res        = FPGA_OK;
	void* dl_handle = NULL;

	// Board version
	fpga_result(*print_board_info)(fpga_token token);

	res = load_board_plugin(token, &dl_handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to load board plugin\n");
		goto out;
	}

	print_board_info = dlsym(dl_handle, "print_board_info");
	if (print_board_info) {
		res = print_board_info(token);
	} else {
		OPAE_ERR("No print_board_info entry point:%s\n", dlerror());
		res = FPGA_NOT_FOUND;
	}

out:
	return res;
}

// Prints mac info
fpga_result mac_info(fpga_token token)
{
	fpga_result res       = FPGA_OK;
	void* dl_handle       = NULL;

	// mack
	fpga_result(*print_mac_info)(fpga_token token);

	res = load_board_plugin(token, &dl_handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to load board plugin\n");
		goto out;
	}

	print_mac_info = dlsym(dl_handle, "print_mac_info");
	if (print_mac_info) {
		res = print_mac_info(token);
	} else {
		OPAE_ERR("No print_mac_info entry point:%s\n", dlerror());
		res = FPGA_NOT_FOUND;
	}

out:
	return res;
}

// prints PHY group info
fpga_result phy_group_info(fpga_token token)
{
	fpga_result res         = FPGA_OK;
	void* dl_handle = NULL;

	// phy group info
	fpga_result(*print_phy_info)(fpga_token token);

	res = load_board_plugin(token, &dl_handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to load board plugin\n");
		goto out;
	}

	print_phy_info = dlsym(dl_handle, "print_phy_info");
	if (print_phy_info) {
		res = print_phy_info(token);
	} else {
		OPAE_ERR("No print_phy_info entry point:%s\n", dlerror());
		res = FPGA_NOT_FOUND;
	}

out:
	return res;
}

