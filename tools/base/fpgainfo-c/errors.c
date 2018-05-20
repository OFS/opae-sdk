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
 * @file errors.c
 *
 * @brief fpga error reporting
 *
 */
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "fpgainfo.h"
#include "safe_string/safe_string.h"
#include <opae/properties.h>

#include "errors.h"

/*
 * errors command configuration, set during parse_args()
 */
struct errors_config {
	bool clear;
	const char *which;
} errors_config = {.clear = false, .which = "all"};

// TODO: Move this to a common file for reuse in other fpgainfo files
static int str_in_list(const char *key, const char *list[], size_t size)
{
	for (size_t i = 0; i < size; ++i) {
		if (!strcmp(key, list[i])) {
			return true;
		}
	}
	return false;
}

#define MAX_VERB_LENGTH 8
#define GETOPT_STRING ":c"
int parse_error_args(int argc, char *argv[])
{
	const char *supported_verbs[] = {"all", "fme", "port"};
	const size_t verb_count = 3;
	optind = 0;
	struct option longopts[] = {{"clear", no_argument, NULL, 'c'},
				    {0, 0, 0, 0}};

	int getopt_ret;
	int option_index;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
					       longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'c': /* help */
			errors_config.clear = true;
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

	/* use first non-option argument as what errors to operate on */
	if (optind < argc && !strcmp(argv[optind], "errors")) {
		if (str_in_list(argv[optind+1], supported_verbs, verb_count)) {
			errors_config.which = argv[optind];
		} else {
			fprintf(stderr,
				"Not a valid errors resource spec: %s\n",
				argv[optind]);
			return -1;
		}
	}

	return 0;
}

fpga_result errors_filter(fpga_properties *filter, int argc, char *argv[])
{
	errno_t err;
	int ind;
	fpga_result res = FPGA_OK;
	if (0 == parse_error_args(argc, argv)) {
		err = strcmp_s("fme", 3, errors_config.which, &ind);
		if (err) {
			fprintf(stderr, "error with command line arguments\n");
			return EX_DATAERR;
		}
		if (!ind) {
			res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
			ON_FPGAINFO_ERR_GOTO(res, out,
					     "setting type to FPGA_DEVICE");
		} else {
			err = strcmp_s("port", 4, errors_config.which, &ind);
			if (err) {
				fprintf(stderr,
					"error with command line arguments\n");
				res = EX_DATAERR;
			}
			if (!ind) {
				res = fpgaPropertiesSetObjectType(
					*filter, FPGA_ACCELERATOR);
				ON_FPGAINFO_ERR_GOTO(
					res, out,
					"setting type to FPGA_ACCELERATOR");
			}
		}
	}
out:
	return res;
}

fpga_result errors_command(fpga_token *tokens, int num_tokens, int argc,
			   char *argv[])
{
	(void)tokens;
	(void)num_tokens;
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;

	return res;
}
