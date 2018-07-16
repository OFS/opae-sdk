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
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "safe_string/safe_string.h"
#include "argsfilter.h"
#include "opae/fpga.h"

#include "fpgainfo.h"

#include "errors.h"
#include "fmeinfo.h"
#include "portinfo.h"

/*
 * Print help
 * TODO: uncomment options as they are implemented
 */
void help(void)
{
	printf("\n"
	       "fpgainfo\n"
	       "FPGA information utility\n"
	       "\n"
	       "Usage:\n"
	       "        fpgainfo [-hv] [-b <bus>] [-d <device>]"
	       "[-f <function>] [-s <socket>]"
	       "{errors,power,temp,fme,port}\n"
	       "\n"
	       "                -h,--help           Print this help\n"
	       "                -B,--bus            Set target bus number\n"
	       "                -D,--device         Set target device number\n"
	       "                -F,--function       Set target function "
	       "number\n"
	       "                -S,--socket-id      Set target socket number\n"
	       "\n");
}


/*
 * Parse command line arguments
 */
#define GETOPT_STRING "+h"
int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {{"help", no_argument, NULL, 'h'},
				    {0, 0, 0, 0} };

	int getopt_ret;
	int option_index;
	if (argc < 2) {
		help();
		return EX_USAGE;
	}

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
					       longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg))
			++tmp_optarg;

		switch (getopt_ret) {
		case 'h': /* help */
			help();
			return EX_USAGE;

		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			return EX_USAGE;

		case '?':
		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			return EX_USAGE;
		}
	}

	optind = 0;
	return EX_OK;
}

typedef fpga_result (*filter_fn)(fpga_properties *, int, char **);
typedef fpga_result (*command_fn)(fpga_token *, int, int, char **);

#define CMD_SIZE 3
// define a list of command words and
// function ptrs to the command handler
struct command_handler {
	const char *command;
	filter_fn filter;
	command_fn run;
} cmd_array[CMD_SIZE] = {
	{.command = "errors", .filter = errors_filter, .run = errors_command},
	{.command = "fme", .filter = fme_filter, .run = fme_command},
	{.command = "port", .filter = port_filter, .run = port_command} };

struct command_handler *get_command(char *cmd)
{
	// find the command handler for the command
	for (int i = 0; i < CMD_SIZE; ++i) {
		if (!strcmp(cmd, cmd_array[i].command)) {
			return &cmd_array[i];
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int ret_value = EX_OK;
	fpga_result res = FPGA_OK;
	uint32_t matches = 0;
	fpga_properties filter = NULL;
	fpga_token *tokens = NULL;
	ret_value = parse_args(argc, argv);
	if (ret_value != EX_OK) {
		return ret_value;
	}
	// start a filter using the first level command line arguments
	res = fpgaGetProperties(NULL, &filter);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "creating properties object");

	ret_value = set_properties_from_args(filter, &res, &argc, argv);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "creating filter from args");
	if (ret_value != EX_OK) {
		goto out_destroy;
	}
	uint32_t num_tokens = 0;
	struct command_handler *handler = get_command(argv[1]);
	if (handler == NULL) {
		fprintf(stderr, "Invalid command specified\n");
		goto out_destroy;
	}
	if (handler->filter) {
		res = handler->filter(&filter, argc, argv);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);
	}
	res = fpgaEnumerate(&filter, 1, NULL, 0, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	num_tokens = matches;
	tokens = (fpga_token *)malloc(num_tokens * sizeof(fpga_token));
	res = fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");
	if (num_tokens != matches) {
		ret_value = EX_SOFTWARE;
		fprintf(stderr,
			"token list changed in "
			"between enumeration "
			"calls\n");
		goto out_destroy;
	}
	res = handler->run(tokens, matches, argc, argv);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);

out_destroy:
	if (res != FPGA_OK)
		ret_value = EX_SOFTWARE;
	res = fpgaDestroyProperties(&filter); /* not needed anymore */
	ON_FPGAINFO_ERR_GOTO(res, out_err, "destroying properties object");
out_err:
	return ret_value;
}
