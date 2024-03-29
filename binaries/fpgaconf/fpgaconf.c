// Copyright(c) 2017-2022, Intel Corporation
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

/*
 * @file fpgaconf.c
 *
 * @brief FPGA configure command line tool
 *
 * fpgaconf allows you to program green bitstream files to an FPGA supported by
 * the intel-fpga driver and API.
 *
 * Features:
 *   * Auto-discovery of compatible slots for supplied bitstream
 *   * Dry-run mode ("what would happen if...?")
 */
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <uuid/uuid.h>

#include <opae/fpga.h>
#include <libbitstream/bitstream.h>
#include <argsfilter.h>
#include "mock/opae_std.h"

/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			print_err((desc), (res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)
/*
 * Global configuration, set during parse_args()
 */
#define MAX_FILENAME_LEN 256
struct config {
	unsigned int verbosity;
	bool dry_run;
	enum { INTERACTIVE, /* ask if ambiguous */
	       NORMAL,      /* stop if ambiguous */
	       AUTOMATIC    /* choose if ambiguous */
	} mode;
	int flags;
	char *filename;
} config = {.verbosity = 0,
	    .dry_run = false,
	    .mode = NORMAL,
	    .flags = 0,
	    .filename = NULL };

/*
 * Print readable error message for fpga_results
 */
void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

/*
 * Print message depending on verbosity
 */
void print_msg(unsigned int verbosity, const char *s)
{
	if (config.verbosity >= verbosity)
		printf("%s\n", s);
}

/*
 * Print help
 */
void help(void)
{
	printf("\n"
	       "fpgaconf\n"
	       "FPGA configuration utility\n"
	       "\n"
	       "Usage:\n"
	       "        fpgaconf [-hVvn] [-S <segment>] [-B <bus>] [-D <device>] [-F <function>] [PCI_ADDR] <gbs>\n"
	       "\n"
	       "                -h,--help           Print this help\n"
	       "                -V,--verbose        Increase verbosity\n"
	       "                -n,--dry-run        Don't actually perform actions\n"
	       "                --force             Attempt to reconfigure even if in use\n"
	       "                --skip-usrclk       Don't program user clocks\n"
	       "                -S,--segment        Set target segment number\n"
	       "                -B,--bus            Set target bus number\n"
	       "                -D,--device         Set target device number\n"
	       "                -F,--function       Set target function number\n"
	       "                -v,--version        Print version info and exit\n"
	       "\n");
}

/*
 * Parse command line arguments
 */
#define GETOPT_STRING ":hVvnAIQ"
int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"help",        no_argument,       NULL, 'h'},
		{"verbose",     no_argument,       NULL, 'V'},
		{"dry-run",     no_argument,       NULL, 'n'},
		{"force",       no_argument,       NULL, 0xf},
		{"skip-usrclk", no_argument,       NULL, 0x5},
		{"version",     no_argument,       NULL, 'v'},
		{0, 0, 0, 0} };

	int getopt_ret;
	int option_index;

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING, longopts,
					    &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h': /* help */
			help();
			return -1;

		case 'V': /* verbose */
			config.verbosity++;
			break;

		case 'n': /* dry-run */
			config.dry_run = true;
			break;

		case 0xf: /* force */
			config.flags |= FPGA_RECONF_FORCE;
			break;

		case 0x5: /* skip-usrclk */
			config.flags |= FPGA_RECONF_SKIP_USRCLK;
			break;

		case 'A': /* auto */
			config.mode = AUTOMATIC;
			break;

		case 'I': /* interactive */
			config.mode = INTERACTIVE;
			break;

		case 'Q': /* quiet */
			config.verbosity = 0;
			break;

		case 'v': /* version */
			fprintf(stdout, "fpgaconf %s %s%s\n",
					OPAE_VERSION,
					OPAE_GIT_COMMIT_HASH,
					OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			return -1;

		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			return -1;

		case '?':
		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			return -1;
		}
	}

	/* use first non-option argument as GBS filename */
	if (optind == argc) {
		fprintf(stderr, "No GBS file\n");
		return -1;
	}
	config.filename = opae_canonicalize_file_name(argv[optind]);
	if (config.filename) {
		return 0;
	} else {
		fprintf(stderr, "Error locating GBS file specified: \"%s\"\n", strerror(errno));
		return -1;
	}
}

fpga_result get_fpga_interface_id(fpga_token token, fpga_guid interface_id)
{
	fpga_result result = FPGA_OK;
	fpga_result resval = FPGA_OK;
	fpga_properties filter = NULL;
	fpga_objtype objtype;
	fpga_guid guid;

	result = fpgaGetProperties(token, &filter);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Token Properties Object");
		goto out;
	}

	result = fpgaPropertiesGetObjectType(filter, &objtype);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Token Properties Object");
		goto out_destroy;
	}

	if (objtype != FPGA_DEVICE) {
		OPAE_ERR("Invalid FPGA object type");
		result = FPGA_EXCEPTION;
		goto out_destroy;
	}

	result = fpgaPropertiesGetGUID(filter, &guid);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get PR guid");
		goto out_destroy;
	}

	memcpy(interface_id, guid, sizeof(fpga_guid));

out_destroy:
	resval = (result != FPGA_OK) ? result : resval;
	result = fpgaDestroyProperties(&filter);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to destroy properties");
	}

out:
	resval = (result != FPGA_OK) ? result : resval;
	return resval;
}

/*
 * Prints Actual and Expected Interface id
 */
int print_interface_id(fpga_properties device_filter,
		       fpga_guid actual_interface_id)
{
	fpga_properties filter = NULL;
	uint32_t num_matches = 0;
	int retval = -1;
	fpga_handle fpga_handle = NULL;
	fpga_result res = -1;
	fpga_token fpga_token = NULL;
	fpga_guid expt_interface_id = {0};
	char guid_str[37] = {0};

	res = fpgaCloneProperties(device_filter, &filter);
	ON_ERR_GOTO(res, out_err, "cloning properties");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting object type");

	res = fpgaEnumerate(&filter, 1, &fpga_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy, "enumerating FPGAs");

	if (num_matches > 0) {
		retval = (int)num_matches; /* FPGA found */
	} else {
		retval = 0; /* no FPGA found */
		goto out_destroy;
	}

	res = fpgaOpen(fpga_token, &fpga_handle, 0);
	ON_ERR_GOTO(res, out_destroy, "opening fpga");

	res = get_fpga_interface_id(fpga_token, expt_interface_id);
	ON_ERR_GOTO(res, out_close, "interfaceid get");

	uuid_unparse(expt_interface_id, guid_str);
	printf("Expected Interface id:  %s\n", guid_str);

	uuid_unparse(actual_interface_id, guid_str);
	printf("Actual Interface id:    %s\n", guid_str);


out_close:
	res = fpgaClose(fpga_handle);
	ON_ERR_GOTO(res, out_destroy, "closing fme");

out_destroy:
	if (fpga_token)
		fpgaDestroyToken(&fpga_token);
	res = fpgaDestroyProperties(&filter); /* not needed anymore */
	ON_ERR_GOTO(res, out_err, "destroying properties object");
out_err:
	return retval;
}

/*
 * Find first FPGA matching the interface ID of the GBS
 *
 * @returns the total number of FPGAs matching the interface ID
 */
int find_fpga(fpga_properties device_filter,
	      fpga_guid interface_id,
	      fpga_token *fpga)
{
	fpga_properties filter = NULL;
	uint32_t num_matches = 0;
	fpga_result res;
	int retval = -1;

	res = fpgaCloneProperties(device_filter, &filter);
	ON_ERR_GOTO(res, out_err, "cloning properties");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting object type");

	res = fpgaPropertiesSetGUID(filter, interface_id);
	ON_ERR_GOTO(res, out_destroy, "setting interface ID");

	/* Get number of FPGAs in system */
	res = fpgaEnumerate(&filter, 1, fpga, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy, "enumerating FPGAs");

	if (num_matches > 0) {
		retval = (int)num_matches; /* FPGA found */
	} else {
		retval = 0; /* no FPGA found */
	}

out_destroy:
	res = fpgaDestroyProperties(&filter); /* not needed anymore */
	ON_ERR_GOTO(res, out_err, "destroying properties object");
out_err:
	return retval;
}

int program_bitstream(fpga_token token, uint32_t slot_num,
		      opae_bitstream_info *info, int flags)
{
	fpga_handle handle;
	fpga_result res;

	print_msg(2, "Opening FPGA");
	res = fpgaOpen(token, &handle, 0);
	ON_ERR_GOTO(res, out_err, "opening FPGA");

	print_msg(1, "Writing bitstream");
	if (config.dry_run) {
		print_msg(1, "[--dry-run] Skipping reconfiguration");
	} else {
		res = fpgaReconfigureSlot(handle, slot_num, info->data,
					  info->data_len, flags);
		ON_ERR_GOTO(res, out_close, "writing bitstream to FPGA");
	}

	print_msg(2, "Closing FPGA");
	res = fpgaClose(handle);
	ON_ERR_GOTO(res, out_err, "closing FPGA");
	return 1;

out_close:
	res = fpgaClose(handle);
	ON_ERR_GOTO(res, out_err, "closing FPGA");
out_err:
	return -1;
}


int main(int argc, char *argv[])
{
	int res;
	fpga_result result = FPGA_OK;
	int retval = 0;
	opae_bitstream_info info;
	fpga_token token;
	uint32_t slot_num = 0; /* currently, we don't support multiple slots */
	fpga_properties device_filter = NULL;

	result = fpgaGetProperties(NULL, &device_filter);
	if (result) {
		print_err("failed to alloc properties", result);
		retval = 6;
		return retval;
	}

	if (opae_set_properties_from_args(device_filter,
					  &result,
					  &argc,
					  argv)) {
		print_err("failed arg parse", result);
		retval = 7;
		goto out_exit;
	} else if (result) {
		print_err("failed to set properties", result);
		retval = 8;
		goto out_exit;
	}

	/* parse command line arguments */
	res = parse_args(argc, argv);
	if (res < 0) {
		retval = 1;
		goto out_exit;
	}

	if (config.dry_run)
		printf("--dry-run is set\n");

	/* allocate memory and read bitstream data */
	print_msg(1, "Reading bitstream");
	result = opae_load_bitstream(config.filename, &info);
	if (result != FPGA_OK) {
		retval = 2;
		goto out_exit;
	}

	/* find suitable slot */
	print_msg(1, "Looking for slot");
	res = find_fpga(device_filter, info.pr_interface_id, &token);
	if (res < 0) {
		retval = 3;
		goto out_free;
	}
	if (res == 0) {
		fprintf(stderr, "No suitable slots found.\n");
		retval = 4;
		if (config.verbosity > 0)
			print_interface_id(device_filter, info.pr_interface_id);
		goto out_free;
	}
	if (res > 1) {
		fprintf(stderr,
			"Found more than one suitable slot, please be more specific.\n");
		retval = 5;
		goto out_destroy;
	}
	print_msg(1, "Found slot");

	/* program bitstream */
	print_msg(1, "Programming bitstream");
	res = program_bitstream(token, slot_num, &info, config.flags);
	if (res < 0) {
		retval = 5;
		goto out_destroy;
	}
	print_msg(1, "Done");

	/* clean up */
out_destroy:
	fpgaDestroyToken(&token);
out_free:
	opae_unload_bitstream(&info);
out_exit:
	if (config.filename) {
		opae_free(config.filename);
		config.filename = NULL;
	}
	fpgaDestroyProperties(&device_filter);
	return retval;
}
