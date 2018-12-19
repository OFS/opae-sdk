// Copyright(c) 2017-2018, Intel Corporation
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
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "safe_string/safe_string.h"

#include "opae/fpga.h"
#include "bitstream-tools.h"

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
	struct target {
		int segment;
		int bus;
		int device;
		int function;
		int socket;
	} target;
	char *filename;
} config = {.verbosity = 0,
	    .dry_run = false,
	    .mode = NORMAL,
	    .flags = 0,
	    .target = {.segment = -1, .bus = -1, .device = -1, .function = -1, .socket = -1},
	    .filename = NULL };

struct bitstream_info {
	char *filename;
	uint8_t *data;
	size_t data_len;
	uint8_t *rbf_data;
	size_t rbf_len;
	fpga_guid interface_id;
};

fpga_result get_bitstream_ifc_id(const uint8_t *bitstream, size_t bs_len,
				 fpga_guid *guid)
{
	fpga_result result = FPGA_EXCEPTION;
	char *json_metadata = NULL;
	uint32_t json_len = 0;
	const uint8_t *json_metadata_ptr = NULL;
	json_object *root = NULL;
	json_object *afu_image = NULL;
	json_object *interface_id = NULL;
	errno_t e;

	if (check_bitstream_guid(bitstream) != FPGA_OK)
		goto out_free;

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN,
					   sizeof(uint32_t));
	if (json_len == 0) {
		OPAE_MSG("Bitstream has no metadata");
		result = FPGA_OK;
		goto out_free;
	}

	if (json_len > bs_len) {
		OPAE_ERR("invalid bitstream metadata size");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *)malloc(json_len + 1);
	if (json_metadata == NULL) {
		OPAE_ERR("Could not allocate memory for metadata!");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len + 1, json_metadata_ptr, json_len);
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}
	json_metadata[json_len] = '\0';

	root = json_tokener_parse(json_metadata);

	if (root != NULL) {
		if (json_object_object_get_ex(root, GBS_AFU_IMAGE,
					      &afu_image)) {
			json_object_object_get_ex(afu_image, BBS_INTERFACE_ID,
						  &interface_id);

			if (interface_id == NULL) {
				OPAE_ERR("Invalid metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			result = string_to_guid(
				json_object_get_string(interface_id), guid);
			if (result != FPGA_OK) {
				OPAE_ERR("Invalid BBS interface id ");
				goto out_free;
			}
		} else {
			OPAE_ERR("Invalid metadata");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}
	}

out_free:
	if (root)
		json_object_put(root);
	if (json_metadata)
		free(json_metadata);

	return result;
}


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
 * TODO: uncomment options as they are implemented
 */
void help(void)
{
	printf("\n"
	       "fpgaconf\n"
	       "FPGA configuration utility\n"
	       "\n"
	       "Usage:\n"
	       //"        fpgaconf [-hvnAIQ] [-B <bus>] [-D <device>] [-F
	       //<function>] [-S <socket-id>] <gbs>\n"
	       "        fpgaconf [-hvn] [-B <bus>] [-D <device>] [-F <function>] [-S <socket-id>] <gbs>\n"
	       "\n"
	       "                -h,--help           Print this help\n"
	       "                -v,--verbose        Increase verbosity\n"
	       "                -n,--dry-run        Don't actually perform actions\n"
	       "                --force             Don't try to open accelerator resource\n"
	       "                --segment           Set target segment number\n"
	       "                -B,--bus            Set target bus number\n"
	       "                -D,--device         Set target device number\n"
	       "                -F,--function       Set target function number\n"
	       "                -S,--socket-id      Set target socket number\n"
	       /* "                -A,--auto           Automatically choose
		  target slot if\n" */
	       /* "                                    multiple valid slots are
		  available\n" */
	       /* "                -I,--interactive    Prompt user to choose
		  target slot if\n" */
	       /* "                                    multiple valid slots are
		  available\n" */
	       /* "                -Q,--quiet          Don't print any messages
		  except errors\n" */
	       "\n");
}

/*
 * Parse command line arguments
 * TODO: uncomment options as they are implemented
 */
#define GETOPT_STRING ":hvnB:D:F:S:AIQ"
int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"help",      no_argument,       NULL, 'h'},
		{"verbose",   no_argument,       NULL, 'v'},
		{"dry-run",   no_argument,       NULL, 'n'},
		{"segment",   required_argument, NULL, 0xe},
		{"bus",       required_argument, NULL, 'B'},
		{"device",    required_argument, NULL, 'D'},
		{"function",  required_argument, NULL, 'F'},
		{"socket-id", required_argument, NULL, 'S'},
		{"force",     no_argument,       NULL, 0xf},
		/* {"auto",          no_argument,       NULL, 'A'}, */
		/* {"interactive",   no_argument,       NULL, 'I'}, */
		/* {"quiet",         no_argument,       NULL, 'Q'}, */
		{0, 0, 0, 0} };

	int getopt_ret;
	int option_index;
	char *endptr = NULL;

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

		case 'v': /* verbose */
			config.verbosity++;
			break;

		case 'n': /* dry-run */
			config.dry_run = true;
			break;

		case 0xf: /* force */
			config.flags |= FPGA_RECONF_FORCE;
			break;

		case 0xe: /* segment */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.segment =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid segment: %s\n",
					tmp_optarg);
				return -1;
			}
			break;

		case 'B': /* bus */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.bus =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid bus: %s\n",
					tmp_optarg);
				return -1;
			}
			break;

		case 'D': /* device */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.device =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid device: %s\n",
					tmp_optarg);
				return -1;
			}
			break;

		case 'F': /* function */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.function =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid function: %s\n",
					tmp_optarg);
				return -1;
			}
			break;

		case 'S': /* socket */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.socket =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid socket: %s\n",
					tmp_optarg);
				return -1;
			}
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
	config.filename = canonicalize_file_name(argv[optind]);
	if (config.filename) {
		return 0;
	} else {
		fprintf(stderr, "Error locating GBS file specified: \"%s\"\n", strerror(errno));
		return -1;
	}
}

/*
 * Check for bitstream header and fill out bistream_info fields
 */
#define MAGIC 0x1d1f8680
#define MAGIC_SIZE 4
#define HEADER_SIZE 20
int parse_metadata(struct bitstream_info *info)
{
	unsigned i = 0;

	if (!info)
		return -EINVAL;

	if (info->data_len < HEADER_SIZE) {
		fprintf(stderr, "File too small to be GBS\n");
		return -1;
	}

	if (((uint32_t *)info->data)[0] != MAGIC) {
		fprintf(stderr, "No valid GBS header\n");
		return -1;
	}

	/* reverse byte order when reading GBS */
	for (i = 0; i < sizeof(info->interface_id); i++)
		info->interface_id[i] =
			info->data[MAGIC_SIZE + sizeof(info->interface_id) - 1
				   - i];

	info->rbf_data = &info->data[HEADER_SIZE];
	info->rbf_len = info->data_len - HEADER_SIZE;

	return 0;
}

/*
 * Prints Actual and Expected Interface id
 */
int print_interface_id(fpga_guid actual_interface_id)
{
	fpga_properties filter = NULL;
	uint32_t num_matches = 0;
	int retval = -1;
	uint64_t intfc_id_l = 0;
	uint64_t intfc_id_h = 0;
	fpga_handle fpga_handle = NULL;
	fpga_result res = -1;
	fpga_token fpga_token = NULL;
	fpga_guid expt_interface_id = {0};
	char guid_str[37] = {0};

	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_err, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting object type");

	if (-1 != config.target.segment) {
		res = fpgaPropertiesSetSegment(filter, config.target.segment);
		ON_ERR_GOTO(res, out_destroy, "setting segment");
	}

	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}

	if (-1 != config.target.device) {
		res = fpgaPropertiesSetDevice(filter, config.target.device);
		ON_ERR_GOTO(res, out_destroy, "setting device");
	}

	if (-1 != config.target.function) {
		res = fpgaPropertiesSetFunction(filter, config.target.function);
		ON_ERR_GOTO(res, out_destroy, "setting function");
	}

	if (-1 != config.target.socket) {
		res = fpgaPropertiesSetSocketID(filter, config.target.socket);
		ON_ERR_GOTO(res, out_destroy, "setting socket id");
	}

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

	res = get_fpga_interface_id(fpga_token, &intfc_id_l, &intfc_id_h);
	ON_ERR_GOTO(res, out_close, "interfaceid get");

	fpga_guid_to_fpga(intfc_id_h, intfc_id_l, expt_interface_id);

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
 * Read bitstream from file and populate bitstream_info structure
 */
// TODO: remove this check when all bitstreams conform to JSON
// metadata spec.
static bool skip_header_checks;
int read_bitstream(char *filename, struct bitstream_info *info)
{
	FILE *f;
	long len;
	int ret;
	struct stat file_mode;
	memset_s(&file_mode, sizeof(file_mode), 0);

	if (!filename || !info)
		return -EINVAL;

	info->filename = filename;

	/* open file */
	f = fopen(filename, "rb");
	if (!f) {
		perror(filename);
		return -1;
	}

	if (fstat(fileno(f), &file_mode) != 0) {
		perror(filename);
		goto out_close;
	}

	if (S_ISREG(file_mode.st_mode) == 0) {
		fprintf(stderr, "Invalid input GBS file\n");
		goto out_close;
	}

	/* get filesize */
	ret = fseek(f, 0, SEEK_END);
	if (ret < 0) {
		perror(filename);
		goto out_close;
	}
	len = ftell(f);
	if (len < 0) {
		perror(filename);
		goto out_close;
	}

	/* allocate memory */
	info->data = (uint8_t *)malloc(len);
	if (!info->data) {
		perror("malloc");
		goto out_close;
	}

	/* read bistream data */
	ret = fseek(f, 0, SEEK_SET);
	if (ret < 0) {
		perror(filename);
		goto out_free;
	}
	info->data_len = fread(info->data, 1, len, f);
	if (ferror(f)) {
		perror(filename);
		goto out_free;
	}
	if (info->data_len != (size_t)len) {
		fprintf(stderr,
			"Filesize and number of bytes read don't match\n");
		goto out_free;
	}

	if (check_bitstream_guid(info->data) == FPGA_OK) {
		skip_header_checks = true;

		if (get_bitstream_ifc_id(info->data, info->data_len, &(info->interface_id))
		    != FPGA_OK) {
			fprintf(stderr, "Invalid metadata in the bitstream\n");
			goto out_free;
		}
	}

	if (!skip_header_checks) {
		/* populate remaining bitstream_info fields */
		ret = parse_metadata(info);
		if (ret < 0)
			goto out_free;
	}

	fclose(f);
	return 0;

out_free:
	free((void *)info->data);
out_close:
	fclose(f);
	return -1;
}

/*
 * Find first FPGA matching the interface ID of the GBS
 *
 * @returns the total number of FPGAs matching the interface ID
 */
int find_fpga(fpga_guid interface_id, fpga_token *fpga)
{
	fpga_properties filter = NULL;
	uint32_t num_matches;
	fpga_result res;
	int retval = -1;

	/* Get number of FPGAs in system */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_err, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting object type");

	res = fpgaPropertiesSetGUID(filter, interface_id);
	ON_ERR_GOTO(res, out_destroy, "setting interface ID");

	if (-1 != config.target.segment) {
		res = fpgaPropertiesSetSegment(filter, config.target.segment);
		ON_ERR_GOTO(res, out_destroy, "setting segment");
	}

	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}

	if (-1 != config.target.device) {
		res = fpgaPropertiesSetDevice(filter, config.target.device);
		ON_ERR_GOTO(res, out_destroy, "setting device");
	}

	if (-1 != config.target.function) {
		res = fpgaPropertiesSetFunction(filter, config.target.function);
		ON_ERR_GOTO(res, out_destroy, "setting function");
	}

	if (-1 != config.target.socket) {
		res = fpgaPropertiesSetSocketID(filter, config.target.socket);
		ON_ERR_GOTO(res, out_destroy, "setting socket id");
	}

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
		      struct bitstream_info *info, int flags)
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
	int retval = 0;
	struct bitstream_info info;
	fpga_token token;
	uint32_t slot_num = 0; /* currently, we don't support multiple slots */

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
	res = read_bitstream(config.filename, &info);
	if (res < 0) {
		retval = 2;
		goto out_exit;
	}

	/* find suitable slot */
	print_msg(1, "Looking for slot");
	res = find_fpga(info.interface_id, &token);
	if (res < 0) {
		retval = 3;
		goto out_free;
	}
	if (res == 0) {
		fprintf(stderr, "No suitable slots found.\n");
		retval = 4;
		if (config.verbosity > 0)
			print_interface_id(info.interface_id);
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
		goto out_free;
	}
	print_msg(1, "Done");

	/* clean up */
out_destroy:
	fpgaDestroyToken(&token);
out_free:
	free(info.data);
out_exit:
	if (config.filename) {
		free(config.filename);
		config.filename = NULL;
	}
	return retval;
}
