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

#include <getopt.h>
#include "fpgainfo.h"
#include "tempinfo.h"
#include "bmcdata.h"
#include <opae/fpga.h>
#include <wchar.h>

#include "safe_string/safe_string.h"

/*
 * Print help
 */
void temp_help(void)
{
	printf("\nPrint thermal metrics\n"
	       "        fpgainfo temp [-h]\n"
	       "                -h,--help           Print this help\n"
	       "\n");
}

static void print_temp_info(fpga_token token)
{
	fpga_properties props;
	fpga_object obj = NULL;
	fpga_metric_info metrics_info[METRICS_MAX_NUM];
	fpga_metric metrics[METRICS_MAX_NUM];
	uint64_t num_metrics;
	fpga_result res = FPGA_OK;
	uint64_t pkg_temp;

	res = fpgaGetProperties(token, &props);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "Failure reading properties from token");
	fpgainfo_print_common("//****** TEMP ******//", props);

	res = fpgaTokenGetObject(token, PKG_TEMP_NAME, &obj, FPGA_OBJECT_GLOB);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "Failure getting temp object from token");
	res = fpgaObjectRead64(obj, &pkg_temp, FPGA_OBJECT_SYNC);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "Failure reading package temperature value");

	printf("%-29s : %02ld %s\n", "Package Temperature", pkg_temp, "Centigrade");

	res = get_metrics(token, THERMAL, metrics_info, metrics, &num_metrics);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading metrics from BMC");

	print_metrics(metrics_info, metrics, num_metrics);

out_destroy:
	res = fpgaDestroyObject(&obj);
	ON_FPGAINFO_ERR_GOTO(res, out_exit, "destroying object");

	res = fpgaDestroyProperties(&props);
	ON_FPGAINFO_ERR_GOTO(res, out_exit, "destroying properties");

out_exit:
	return;
}

fpga_result temp_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

fpga_result temp_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[])
{
	(void)tokens;
	(void)num_tokens;
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
			temp_help();
			return res;

		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			temp_help();
			return FPGA_INVALID_PARAM;

		case '?':
		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			temp_help();
			return FPGA_INVALID_PARAM;
		}
	}

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		print_temp_info(tokens[i]);
	}

	return res;
}
