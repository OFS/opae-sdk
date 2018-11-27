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
#include "argsfilter.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define EX_OK 0
#define EX_USAGE (-1)
#define EX_SOFTWARE (-2)
#else
#include <sysexits.h>
#endif

#define RETURN_ON_ERR(res, desc)                                               \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			optind = 1;                                            \
			opterr = old_opterr;                                   \
			fprintf(stderr, "Error %s: %s\n", (desc),              \
				fpgaErrStr(res));                              \
			return EX_SOFTWARE;                                    \
		}                                                              \
	} while (0)

int set_properties_from_args(fpga_properties filter, fpga_result *result,
			     int *argc, char *argv[])
{
	// prefix the short options with '-' so that unrecognized options are
	// ignored
	const char *short_opts = "-:B:D:F:S:";
	struct option longopts[] = {
		{"bus", required_argument, NULL, 'B'},
		{"device", required_argument, NULL, 'D'},
		{"function", required_argument, NULL, 'F'},
		{"socket-id", required_argument, NULL, 'S'},
		{"segment", required_argument, NULL, 0xe},
		{0, 0, 0, 0},
	};
	int supported_options = sizeof(longopts) / sizeof(longopts[0]) - 1;
	int getopt_ret = -1;
	int option_index = 0;
	char *endptr = NULL;
	int found_opts[] = {0, 0, 0, 0, 0};
	int next_found = 0;
	int old_opterr = opterr;
	opterr = 0;
	struct _args_filter_config {
		int bus;
		int device;
		int function;
		int socket_id;
                int segment;
	} args_filter_config = {
		.bus = -1, .device = -1, .function = -1, .socket_id = -1, 
                .segment = -1 };

	while (-1
	       != (getopt_ret = getopt_long(*argc, argv, short_opts, longopts,
					    &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg))
			++tmp_optarg;

		switch (getopt_ret) {
		case 'B': /* bus */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			args_filter_config.bus =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid bus: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			found_opts[next_found++] = optind - 2;
			break;

		case 'D': /* device */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			args_filter_config.device =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid device: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			found_opts[next_found++] = optind - 2;
			break;

		case 'F': /* function */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			args_filter_config.function =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid function: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			found_opts[next_found++] = optind - 2;
			break;

		case 'S': /* socket */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			args_filter_config.socket_id =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid socket: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			found_opts[next_found++] = optind - 2;
			break;
                case 0xe: /* segment */
			if (NULL == tmp_optarg)
				break;
			endptr = NULL;
			args_filter_config.segment =
				(int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strlen(tmp_optarg)) {
				fprintf(stderr, "invalid socket: %s\n",
					tmp_optarg);
				return EX_USAGE;
			}
			found_opts[next_found++] = optind - 2;
                        break;
		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			return EX_USAGE;

		case '?':
			break;
		case 1:
			break;
		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			return EX_USAGE;
		}
	}

	if (-1 != args_filter_config.bus) {
		*result = fpgaPropertiesSetBus(filter, args_filter_config.bus);
		RETURN_ON_ERR(*result, "setting bus");
	}
	if (-1 != args_filter_config.device) {
		*result = fpgaPropertiesSetDevice(filter,
						  args_filter_config.device);
		RETURN_ON_ERR(*result, "setting device");
	}

	if (-1 != args_filter_config.function) {
		*result = fpgaPropertiesSetFunction(
			filter, args_filter_config.function);
		RETURN_ON_ERR(*result, "setting function");
	}

	if (-1 != args_filter_config.socket_id) {
		*result = fpgaPropertiesSetSocketID(
			filter, args_filter_config.socket_id);
		RETURN_ON_ERR(*result, "setting socket id");
	}

	if (-1 != args_filter_config.segment) {
		*result = fpgaPropertiesSetSegment(
			filter, args_filter_config.segment);
		RETURN_ON_ERR(*result, "setting segment");
	}
	// using the list of optind values
	// shorten the argv vector starting with a decrease
	// of 2 and incrementing that amount by two for each option found
	int removed = 0;
	int i, j;
	for (i = 0; i < supported_options; ++i) {
		if (found_opts[i]) {
			for (j = found_opts[i] - removed; j < *argc - 2; j++) {
				argv[j] = argv[j + 2];
			}
			removed += 2;
		} else {
			break;
		}
	}
	*argc -= removed;
	// restore getopt variables
	// setting optind to zero will cause getopt to reinitialize for future
	// calls within the program
	optind = 0;
	opterr = old_opterr;
	return EX_OK;
}
