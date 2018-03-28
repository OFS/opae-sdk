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
/*
 * main.c
 * Copyright (C) 2018 ubuntu <ubuntu@ubuntu-xenial>
 *
 * Distributed under terms of the %LICENSE% license.
 */

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "safe_string/safe_string.h"

#include "opae/fpga.h"

#include "fpgainfo.h"

#include "errors.h"
#include "fmeinfo.h"
#include "portinfo.h"

/*
 * Global configuration, set during parse_args()
 */
struct config {
	unsigned int verbosity;
	struct target {
		int bus;
		int device;
		int function;
		int socket;
	} target;
} config = {.verbosity = 0,
	    .target = {.bus = -1, .device = -1, .function = -1, .socket = -1} };

/*
 * Print help
 * TODO: uncomment options as they are implemented
 */
void help(void)
{
	printf(
	    "\n"
	    "fpgainfo\n"
	    "FPGA information utility\n"
	    "\n"
	    "Usage:\n"
	    //"        fpgainfo [-hv] [-b <bus>] [-d <device>] [-f <function>]
	    //<command>\n"
	    "        fpgainfo [-hv] [-b <bus>] [-d <device>] [-f <function>] "
	    "[-s <socket>] {errors,power,temp,fme,port}\n"
	    "\n"
	    "                -h,--help           Print this help\n"
	    "                -v,--verbose        Increase verbosity\n"
	    "                -b,--bus            Set target bus number\n"
	    "                -d,--device         Set target device number\n"
	    "                -f,--function       Set target function number\n"
	    "                -s,--socket         Set target socket number\n"
	    "\n");
}
/*
 * Parse command line arguments
 * TODO: uncomment options as they are implemented
 */
#define GETOPT_STRING "+hvb:d:f:s:"
int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {{"help", no_argument, NULL, 'h'},
				    {"verbose", no_argument, NULL, 'v'},
				    {"bus", required_argument, NULL, 'b'},
				    {"device", required_argument, NULL, 'd'},
				    {"function", required_argument, NULL, 'f'},
				    {"socket", required_argument, NULL, 's'},
				    {0, 0, 0, 0} };

	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
					       longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg))
			++tmp_optarg;

		switch (getopt_ret) {
		case 'h': /* help */
			help();
			return EX_USAGE;

		case 'v': /* verbose */
			config.verbosity++;
			break;

		case 'b': /* bus */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.bus =
			    (int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid bus: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			break;

		case 'd': /* device */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.device =
			    (int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid device: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			break;

		case 'f': /* function */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.function =
			    (int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid function: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			break;

		case 's': /* socket */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			config.target.socket =
			    (int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid socket: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			break;
		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			return EX_USAGE;

		case '?':
		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			return EX_USAGE;
		}
	}

	/* use first non-option argument as action */
	if (optind == argc) {
		fprintf(stderr, "No subcommand specified\n");
		return EX_USAGE;
	}

	return EX_OK;
}

fpga_result initialize_filter(fpga_properties *filter)
{
	fpga_result res = FPGA_OK;
	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(*filter, config.target.bus);
		ON_FPGAINFO_ERR_GOTO(res, out, "setting bus");
	}

	if (-1 != config.target.device) {
		res = fpgaPropertiesSetDevice(filter, config.target.device);
		ON_FPGAINFO_ERR_GOTO(res, out, "setting device");
	}

	if (-1 != config.target.function) {
		res = fpgaPropertiesSetFunction(filter, config.target.function);
		ON_FPGAINFO_ERR_GOTO(res, out, "setting function");
	}

	if (-1 != config.target.socket) {
		res = fpgaPropertiesSetSocketID(filter, config.target.socket);
		ON_FPGAINFO_ERR_GOTO(res, out, "setting socket id");
	}
out:
	return res;
}

typedef fpga_result (*filter_fn)(fpga_properties *, int, char **);
typedef fpga_result (*command_fn)(fpga_token *, int, int, char **);

#define CMD_SIZE 3
int main(int argc, char *argv[])
{
	int ret_value;
	fpga_result res = FPGA_OK;
	errno_t err;
	int ind = -1;
	uint32_t matches = 0;
	fpga_properties filter;
	fpga_token *tokens = NULL;
	// define a list of command words and
	// function ptrs to the command handler
	struct command_handler {
		const char *command;
		filter_fn filter;
		command_fn run;
	} cmd_array[CMD_SIZE] = {
	    {.command = "errors",
	     .filter = errors_filter,
	     .run = errors_command},
	    {.command = "fme", .filter = fme_filter, .run = fme_command},
	    {.command = "port", .filter = port_filter, .run = port_command} };

	ret_value = parse_args(argc, argv);
	if (ret_value == 0) {
		ret_value = EX_SOFTWARE;
		int remaining_argc = argc - optind;
		char **remaining_argv = &argv[optind];
		size_t i;
		uint32_t num_tokens = 0;
		// start a filter using the first level command line arguments
		res = fpgaGetProperties(NULL, &filter);
		ON_FPGAINFO_ERR_GOTO(res, out_err, "creating properties object");
		res = initialize_filter(&filter);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);

		// find the command handler for the command
		for (i = 0; i < CMD_SIZE; ++i) {
			err = strcmp_s(argv[optind], 6, cmd_array[i].command,
				       &ind);
			if (err) {
				fprintf(stderr,
					"error with command line arguments\n");
				ret_value = EX_DATAERR;
			} else if (!ind) {
				if (cmd_array[i].filter) {
					res = cmd_array[i].filter(
					    &filter, remaining_argc,
					    remaining_argv);
					ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);
				}
				res = fpgaEnumerate(&filter, 1, NULL, 0,
						    &matches);
				ON_FPGAINFO_ERR_GOTO(res, out_destroy,
					    "enumerating resources");
				num_tokens = matches;
				tokens = (fpga_token *)malloc(
				    num_tokens * sizeof(fpga_token));
				res = fpgaEnumerate(&filter, 1, tokens,
						    num_tokens, &matches);
				ON_FPGAINFO_ERR_GOTO(res, out_destroy,
					    "enumerating resources");
				if (num_tokens != matches) {
					fprintf(stderr, "token list changed in "
							"between enumeration "
							"calls\n");
					goto out_destroy;
				}
				res = cmd_array[i].run(tokens, matches,
						       remaining_argc,
						       remaining_argv);
				ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);
				break;
			}
		}
		if (i == CMD_SIZE) {
			fprintf(stderr, "no subcommand specified\n");
			ret_value = EX_USAGE;
		}
	}
out_destroy:
	if (res != FPGA_OK)
		ret_value = EX_SOFTWARE;
	res = fpgaDestroyProperties(&filter); /* not needed anymore */
	ON_FPGAINFO_ERR_GOTO(res, out_err, "destroying properties object");
out_err:
	return ret_value;
}
