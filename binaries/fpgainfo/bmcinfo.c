// Copyright(c) 2018-2020, Intel Corporation
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

#include "fpgainfo.h"
#include "bmcinfo.h"
#include "bmcdata.h"
#include "board.h"
#include <opae/fpga.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <glob.h>


/*
 * Print help
 */
void bmc_help(void)
{
	printf("\nPrint all Board Management Controller sensor values\n"
	       "        fpgainfo bmc [-h]\n"
	       "                -h,--help           Print this help\n"
	       "\n");
}

/*
 * Print help
 */
void perf_help(void)
{
	printf("\nPrint performance counter values\n"
	       "        fpgainfo perf [-h]\n"
	       "                -h,--help           Print this help\n"
	       "\n");
}

fpga_result bmc_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

static void print_bmc_info(fpga_token token)
{
	fpga_properties props = NULL;
	fpga_metric_info metrics_info[METRICS_MAX_NUM];
	fpga_metric metrics[METRICS_MAX_NUM] = { { 0 } };
	uint64_t num_metrics;
	uint64_t num_metrics_info;
	fpga_result res = FPGA_OK;

	res = fpgaGetProperties(token, &props);
	ON_FPGAINFO_ERR_GOTO(res, out_exit,
			     "reading properties from token");

	fpgainfo_board_info(token);
	fpgainfo_print_common("//****** BMC SENSORS ******//", props);

	res = get_metrics(token, FPGA_ALL, metrics_info, &num_metrics_info, metrics, &num_metrics);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy,
			     "reading metrics from BMC");

	print_metrics(metrics_info, num_metrics_info, metrics, num_metrics);

out_destroy:
	res = fpgaDestroyProperties(&props);
	ON_FPGAINFO_ERR_GOTO(res, out_exit, "destroying properties");

out_exit:
	return;
}


fpga_result bmc_command(fpga_token *tokens, int num_tokens, int argc,
			char *argv[])
{
	(void)argc;
	(void)argv;

	fpga_result res = FPGA_OK;

	optind = 0;
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};

	int getopt_ret;
	int option_index;

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, ":h", longopts,
					    &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h': /* help */
			bmc_help();
			return res;

		case ':': /* missing option argument */
			OPAE_ERR("Missing option argument\n");
			bmc_help();
			return FPGA_INVALID_PARAM;

		case '?':
		default: /* invalid option */
			OPAE_ERR("Invalid cmdline options\n");
			bmc_help();
			return FPGA_INVALID_PARAM;
		}
	}

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		print_bmc_info(tokens[i]);
	}

	return res;
}


