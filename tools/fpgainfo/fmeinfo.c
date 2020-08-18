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

#define FPGA_BSID_REVISION(id)	(((id) >> 36) & 0xfff)
#define FPGA_BSID_INTERFACE(id)	(((id) >> 32) & 0xf)
#define FPGA_BSID_FLAGS(id)		(((id) >> 24) & 0xff)
#define FPGA_BSID_BUILD_VER(id)	(((id) >> 0) & 0xffffff)
#define FPGA_BSID_FLAG_FVL_BYPASS		0x01
#define FPGA_BSID_FLAG_MAC_LIGHTWEIGHT	0x02
#define FPGA_BSID_FLAG_DISAGGREGATE		0x04
#define FPGA_BSID_FLAG_LIGHTWEIGHT		0x08
#define FPGA_BSID_FLAG_SEU				0x10
#define FPGA_BSID_FLAG_PTP1588			0x20
#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)
#define DFL_BITSTREAM_ID    "bitstream_id"

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

static void print_fme_verbose_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	fpga_object fpga_object;
	uint64_t bitstream_id;
	uint32_t major = 0;
	uint32_t val = 0;
	printf("-----FME VERBOSE-----\n");
	res = fpgaTokenGetObject(token, DFL_BITSTREAM_ID, &fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return ;
	}

	res = fpgaObjectRead64(fpga_object, &bitstream_id, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		goto out_destroy;
	}

	char *platform[] = { "PAC N3000", "PAC GX FPGA", "PAC D5005",
					"PAC S10" };
	major = FPGA_BBS_VER_MAJOR(bitstream_id);
	printf("%-32s : ", "Platform");
	if (major < sizeof(platform) / sizeof(char *))
		printf("%s\n", platform[major]);
	else
		printf("unknown\n");

	val = FPGA_BBS_VER_MINOR(bitstream_id);
	printf("%-32s : 1.%u\n", "DCP Version", val);

	char *phase[] = { "Pre-Alpha", "Alpha", "Beta", "PV" };
	val = FPGA_BBS_VER_PATCH(bitstream_id);
	printf("%-32s : ", "Phase");
	if (val < sizeof(phase) / sizeof(char *))
		printf("%s\n", phase[val]);
	else
		printf("unknown\n");

	val = FPGA_BSID_REVISION(bitstream_id);
	printf("%-32s : %03x\n", "Revision", val);

	val = FPGA_BSID_INTERFACE(bitstream_id);
	if (major == 0) {	// Vista Creek
		char *intf[] = { "8x10G", "4x25G", "2x1x25G", "4x25G+2x25G", "2x2x25G",
						"2x1x25Gx2FVL", "1x2x25G" };
		printf("%-32s : ", "Interface");
		if (val < sizeof(intf) / sizeof(char *))
			printf("%s\n", intf[val]);
		else
			printf("unknown\n");
	} else {
		printf("%-32s : %x", "HSSI Id", val);
	}

	val = FPGA_BSID_FLAGS(bitstream_id);
	printf("%-32s : %s\n", "Bypass Mode",
		val & FPGA_BSID_FLAG_FVL_BYPASS ? "enabled" : "disabled");
	printf("%-32s : %s\n", "MAC Lightweight Mode",
		val & FPGA_BSID_FLAG_MAC_LIGHTWEIGHT ? "enabled" : "disabled");
	printf("%-32s : %s\n", "Disaggregate Mode",
		val & FPGA_BSID_FLAG_DISAGGREGATE ? "enabled" : "disabled");
	printf("%-32s : %s\n", "Lightweight Mode",
		val & FPGA_BSID_FLAG_LIGHTWEIGHT ? "enabled" : "disabled");
	printf("%-32s : %s\n", "SEU detection",
		val & FPGA_BSID_FLAG_SEU ? "enabled" : "disabled");
	printf("%-32s : %s\n", "PTP functionality",
		val & FPGA_BSID_FLAG_PTP1588 ? "enabled" : "disabled");

	val = FPGA_BSID_BUILD_VER(bitstream_id);
	printf("%-32s : %d.%d.%d\n", "Build Version",
		(val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);

out_destroy:
	res = fpgaDestroyObject(&fpga_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}

	return ;
}

static void print_fme_info(fpga_token token)
{
	fpga_properties props;
	fpga_result res = FPGA_OK;
	res = fpgaGetProperties(token, &props);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy,
			     "Failure reading properties from token");

	fpgainfo_board_info(token);
	fpgainfo_print_common("//****** FME ******//", props);

out_destroy:
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
		{"verbose", no_argument, NULL, 'v'},
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

		case 'v': /* verbose - UNDOCUMENTED */
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
			print_fme_verbose_info(tokens[i]);
		}
	}

	return res;
}
