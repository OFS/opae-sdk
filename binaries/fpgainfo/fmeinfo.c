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

#include <getopt.h>
#include "fpgainfo.h"
#include "fmeinfo.h"
#include "board.h"
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include <inttypes.h>



/*
 * Print help
 */
void fme_help(void)
{
	printf("\nPrint FME information\n"
	       "        fpgainfo fme [-h]\n"
	       "                -h,--help           Print this help\n"
	       "\n");
}

static void print_fme_info(fpga_token token)
{
	fpga_properties props = NULL;
	fpga_result res = FPGA_OK;
	res = fpgaGetProperties(token, &props);
	ON_FPGAINFO_ERR_GOTO(res, out_exit,
			     "Failure reading properties from token");

	fpgainfo_board_info(token);
	fpgainfo_print_common("//****** FME ******//", props);
	fpga_boot_info(token);
	fpga_image_info(token);

	res = fpgaDestroyProperties(&props);
	ON_FPGAINFO_ERR_GOTO(res, out_exit,
			     "destroying properties");

out_exit:
	return;
}

fpga_result fme_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

fpga_result fme_command(fpga_token *tokens, int num_tokens, int argc,
			char *argv[])
{
	(void)tokens;
	(void)num_tokens;
	(void)argc;
	(void)argv;

	fpga_result res = FPGA_OK;
	int verbose_opt = 0;
	optind = 0;
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'V'},
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
			fme_help();
			return res;

		case ':': /* missing option argument */
			OPAE_ERR("Missing option argument\n");
			fme_help();
			return FPGA_INVALID_PARAM;

		case 'V': /* verbose */
			verbose_opt = 1;
			break;

		case '?':
		default: /* invalid option */
			OPAE_ERR("Invalid cmdline options\n");
			fme_help();
			return FPGA_INVALID_PARAM;
		}
	}

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		print_fme_info(tokens[i]);
		if (verbose_opt) {
			fme_verbose_info(tokens[i]);
		}
	}

	return res;
}
