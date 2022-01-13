// Copyright(c) 2021, Intel Corporation
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

#include <getopt.h>

#include "fpgainfo.h"
#include "events.h"
#include "board.h"

static bool parse_count(uint32_t *count, const char *str, const char *arg)
{
	char *endptr = NULL;

	*count = strtoull(str, &endptr, 0);
	if (endptr == NULL || *endptr != '\0') {
		fprintf(stderr, "invalid %s value: '%s'\n", arg, str);
		return false;
	}

	return true;
}

void events_help(void)
{
	printf("\nPrint event log\n"
	       "        fpgainfo events [-h]");
	printf("}\n"
	       "                -l,--list              List boots (implies --all)\n"
	       "                -b,--boot              Boot index to use, i.e:\n"
	       "                                         0 for current boot (default),\n"
	       "                                         1 for previous boot, etc\n"
	       "                -c,--count             Number of events to print\n"
	       "                -a,--all               Print all events\n"
	       "                -s,--sensors           Print sensor data too\n"
	       "                -i,--bits              Print bit values too\n"
	       "                -h,--help           Print this help\n"
	       "\n");
}

fpga_result events_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res;

	(void)argc;
	(void)argv;

	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	if (res != FPGA_OK)
		OPAE_MSG("setting type to FPGA_DEVICE");

	return res;
}

fpga_result events_command(fpga_token *tokens, int num_tokens, int argc,
			   char *argv[])
{
	fpga_result res = FPGA_OK;
	uint32_t first = 0, last = 1;
	bool print_sensors = false;
	bool print_bits = false;
	bool print_list = false;
	uint32_t count;
	int i;

	struct option options[] = {
		{ "list",    no_argument,       NULL, 'l' },
		{ "boot",    required_argument, NULL, 'b' },
		{ "count",   required_argument, NULL, 'c' },
		{ "all",     no_argument,       NULL, 'a' },
		{ "sensors", no_argument,       NULL, 's' },
		{ "bits",    no_argument,       NULL, 'i' },
		{ "help",    no_argument,       NULL, 'h' },
		{ 0 },
	};

	while (true) {
		int opt = getopt_long(argc, argv, "lb:c:asih", options, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case 'l':
			print_list = true;
			last = first;
			break;
		case 'b':
			if (!parse_count(&count, optarg, "--boot"))
				return FPGA_INVALID_PARAM;

			first += count;
			last += count;
			break;
		case 'c':
			if (!parse_count(&count, optarg, "--count"))
				return FPGA_INVALID_PARAM;

			if (count == 0) {
				fprintf(stderr, "invalid --count value: 0\n");
				return FPGA_INVALID_PARAM;
			}

			last += count - 1;
			break;
		case 'a':
			last = first;
			break;
		case 's':
			print_sensors = true;
			break;
		case 'i':
			print_bits = true;
			break;
		case 'h':
			events_help();
			return FPGA_OK;
		case ':':
		case '?':
		default:
			events_help();
			return FPGA_INVALID_PARAM;
		}
	}

	for (i = 0; i < num_tokens; i++) {
		res = fpga_event_log(tokens[i], first, last, print_list, print_sensors, print_bits);
		if (res != FPGA_OK)
			break;
	}

	return res;
}
