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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#ifdef _WIN32
#define EX_OK 0
#define EX_USAGE (-1)
#define EX_SOFTWARE (-2)
#define EX_TEMPFAIL (-3)
#else
#include <sysexits.h>
#endif

#include "safe_string/safe_string.h"
#include "argsfilter.h"
#include "opae/fpga.h"

//#include "sysinfo.h"
#include "fpgainfo.h"

#include "errors.h"
#include "fmeinfo.h"
#include "portinfo.h"
#include "tempinfo.h"
#include "powerinfo.h"
#include "bmcinfo.h"
#include "board.h"

void help(void);

typedef fpga_result (*filter_fn)(fpga_properties *, int, char **);
typedef fpga_result (*command_fn)(fpga_token *, int, int, char **);
typedef void (*help_fn)(void);

// define a list of command words and
// function ptrs to the command handler
static struct command_handler {
	const char *command;
	filter_fn filter;
	command_fn run;
	help_fn help;
} cmd_array[] = {
	{.command = "errors",
	 .filter = errors_filter,
	 .run = errors_command,
	 .help = errors_help},
	{.command = "power",
	 .filter = power_filter,
	 .run = power_command,
	 .help = power_help},
	{.command = "temp",
	 .filter = temp_filter,
	 .run = temp_command,
	 .help = temp_help},
	{.command = "fme",
	 .filter = fme_filter,
	 .run = fme_command,
	 .help = fme_help},
	{.command = "port",
	 .filter = port_filter,
	 .run = port_command,
	 .help = port_help},
	{.command = "perf",
	 .filter = bmc_filter,
	 .run = perf_command,
	 .help = perf_help},
	{.command = "bmc",
	 .filter = bmc_filter,
	 .run = bmc_command,
	 .help = bmc_help},
	{.command = "mac",
	 .filter = mac_filter,
	 .run = mac_command,
	 .help = mac_help},
	{.command = "phy",
	 .filter = phy_filter,
	 .run = phy_command,
	 .help = phy_help},

	{.command = "security",
	 .filter = sec_filter,
	 .run = sec_command,
	 .help = sec_help},
};

/*
 * Parse command line arguments
 */
#define MAIN_GETOPT_STRING "+hv"
int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{0, 0, 0, 0},
	};

	int getopt_ret = -1;
	int option_index = 0;
	if (argc < 2) {
		help();
		return EX_USAGE;
	}

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, MAIN_GETOPT_STRING,
					    longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg))
			++tmp_optarg;

		switch (getopt_ret) {
		case 'h': /* help */
			help();
			return EX_TEMPFAIL;

		case 'v': /* version */
			printf("fpgainfo %s %s%s\n",
			       INTEL_FPGA_API_VERSION,
			       INTEL_FPGA_API_HASH,
			       INTEL_FPGA_TREE_DIRTY ? "*":"");
			return EX_TEMPFAIL;

		case ':': /* missing option argument */
			OPAE_ERR("Missing option argument\n");
			return EX_USAGE;

		case '?':
		default: /* invalid option */
			OPAE_ERR("Invalid cmdline options\n");
			return EX_USAGE;
		}
	}

	optind = 0;
	return EX_OK;
}

struct command_handler *get_command(char *cmd)
{
	int cmd_size = sizeof(cmd_array) / sizeof(cmd_array[0]);
	// find the command handler for the command
        int cmp = 0;
        int i = 0;
	for (i = 0; i < cmd_size; ++i) {
		if (strcmp_s(cmd, RSIZE_MAX_STR, cmd_array[i].command, &cmp) == EOK &&
                        cmp == 0) {
			return &cmd_array[i];
		}
	}
	return NULL;
}

/*
 * Print help
 */
void help(void)
{
	unsigned int i;

	printf("\n"
	       "fpgainfo\n"
	       "FPGA information utility\n"
	       "\n"
	       "Usage:\n"
	       "        fpgainfo [-h] [-B <bus>] [-D <device>] "
	       "[-F <function>] [-S <socket-id>] {");
	printf("%s", cmd_array[0].command);
	for (i = 1; i < sizeof(cmd_array) / sizeof(cmd_array[0]); i++) {
		printf(",%s", cmd_array[i].command);
	}
	printf("}\n\n"
	       "                -h,--help           Print this help\n"
	       "                -v,--version        Print version and exit\n"
	       "                -B,--bus            Set target bus number\n"
	       "                -D,--device         Set target device number\n"
	       "                -F,--function       Set target function number\n"
	       "                -S,--socket-id      Set target socket number\n"
	       "                --segment           Set target segment\n"
	       "\n");

	printf("Subcommands:\n");
	for (i = 0; i < sizeof(cmd_array) / sizeof(cmd_array[0]); i++) {
		cmd_array[i].help();
	}
}

int main(int argc, char *argv[])
{
	int ret_value = EX_OK;
	fpga_result res = FPGA_OK;
	uint32_t matches = 0;
	uint32_t i = 0;
	fpga_properties filter = NULL;
	fpga_token *tokens = NULL;

	if (NULL == setlocale(LC_ALL, "")) {
		OPAE_ERR("Could not set locale\n");
		return EX_SOFTWARE;
	}

	// start a filter using the first level command line arguments
	res = fpgaGetProperties(NULL, &filter);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "creating properties object");

	ret_value = set_properties_from_args(filter, &res, &argc, argv);
	if (ret_value != EX_OK) {
		goto out_destroy;
	}

	ret_value = parse_args(argc, argv);
	if (ret_value != EX_OK) {
    		fpgaDestroyProperties(&filter);
		return ret_value == EX_TEMPFAIL ? EX_OK : ret_value;
	}

	uint32_t num_tokens = 0;
	struct command_handler *handler = get_command(argv[1]);
	if (handler == NULL) {
		OPAE_ERR("Invalid command specified\n");
		help();
		goto out_destroy;
	}
	if (handler->filter) {
		res = handler->filter(&filter, argc, argv);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy, 0);
	}
	res = fpgaEnumerate(&filter, 1, NULL, 0, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	if (0 == matches) {
		ret_value = EX_SOFTWARE;
		OPAE_ERR("No FPGA resources found.\n");
		goto out_destroy;
	}

	num_tokens = matches;
	tokens = (fpga_token *)malloc(num_tokens * sizeof(fpga_token));
	res = fpgaEnumerate(&filter, 1, tokens, num_tokens, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_tokens, "enumerating resources");
	if (num_tokens != matches) {
		ret_value = EX_SOFTWARE;
		OPAE_ERR("token list changed in between enumeration calls\n");
		goto out_destroy_tokens;
	}

	res = handler->run(tokens, matches, argc, argv);

out_destroy_tokens:
        for (i = 0; i < num_tokens; i++) {
            fpgaDestroyToken(&tokens[i]);
        }
        free(tokens);

out_destroy:
	if (res != FPGA_OK)
		ret_value = EX_SOFTWARE;
	fpgaDestroyProperties(&filter); /* not needed anymore */
out_err:
	return ret_value;
}
