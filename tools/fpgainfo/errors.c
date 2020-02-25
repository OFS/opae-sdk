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
#include <stdlib.h>

#include "fpgainfo.h"
#include "safe_string/safe_string.h"
#include <opae/properties.h>
#include "errors.h"

#define FPGA_BIT_IS_SET(val, index) (((val) >> (index)) & 1)

const char *supported_verbs[] = {"all", "fme", "port"};
enum verbs_index { VERB_ALL = 0, VERB_FME, VERB_PORT, VERB_MAX };

#define FME_ERROR_COUNT 7
static const char *const FME_ERROR[FME_ERROR_COUNT] = {
	"Fabric error detected",
	"Fabric fifo under / overflow error detected",
	"KTI CDC Parity Error detected",
	"KTI CDC Parity Error detected",
	"IOMMU Parity error detected",
	"AFU PF/VF access mismatch detected",
	"Indicates an MBP event error detected"};

#define PCIE0_ERROR_COUNT 10
static const char *const PCIE0_ERROR[PCIE0_ERROR_COUNT] = {
	"TLP format/type error detected",   "TTLP MW address error detected",
	"TLP MW length error detected",     "TLP MR address error detected",
	"TLP MR length error detected",     "TLP CPL tag error detected",
	"TLP CPL status error detected",    "TLP CPL timeout error detected",
	"CCI bridge parity error detected", "TLP with EP  error  detected"};

#define PCIE1_ERROR_COUNT 10
static const char *const PCIE1_ERROR[PCIE1_ERROR_COUNT] = {
	"TLP format/type error detected",   "TTLP MW address error detected",
	"TLP MW length error detected",     "TLP MR address error detected",
	"TLP MR length error detected",     "TLP CPL tag error detected",
	"TLP CPL status error detected",    "TLP CPL timeout error detected",
	"CCI bridge parity error detected", "TLP with EP  error  detected"};

#define NONFATAL_ERROR_COUNT 13
static const char *const NONFATAL_ERROR[NONFATAL_ERROR_COUNT] = {
	"Temperature threshold triggered AP1 detected",
	"Temperature threshold triggered AP2 detected",
	"PCIe error detected",
	"AFU port Fatal error detected",
	"ProcHot event error detected",
	"AFU PF/VF access mismatch error detected",
	"Injected Warning Error detected",
	"Reserved",
	"Reserved",
	"Temperature threshold triggered AP6 detected",
	"Power threshold triggered AP1 error detected",
	"Power threshold triggered AP2 error detected",
	"MBP event error detected"};

#define CATFATAL_ERROR_COUNT 12
static const char *const CATFATAL_ERROR[CATFATAL_ERROR_COUNT] = {
	"KTI link layer error detected.",
	"tag-n-cache error detected.",
	"CCI error detected.",
	"KTI protocol error detected.",
	"Fatal DRAM error detected",
	"IOMMU fatal parity error detected.",
	"Fabric fatal error detected",
	"Poison error from any of PCIe ports detected",
	"Injected Fatal Error detected",
	"Catastrophic CRC error detected",
	"Catastrophic thermal runaway event detected",
	"Injected Catastrophic Error detected"};

#define INJECT_ERROR_COUNT 3
static const char *const INJECT_ERROR[INJECT_ERROR_COUNT] = {
	"Set Catastrophic  error .", "Set Fatal error.",
	"Ser Non-fatal error ."};

#define PORT_ERROR_COUNT 60
static const char *const PORT_ERROR[PORT_ERROR_COUNT] = {
	// 0
	"Tx Channel 0 overflow error detected.",
	"Tx Channel 0 invalid request encoding error detected.",
	"Tx Channel 0 cl_len=3 not supported error detected.",
	"Tx Channel 0 request with cl_len=2 does NOT have a 2CL aligned address error detected.",
	"Tx Channel 0 request with cl_len=4 does NOT have a 4CL aligned address error detected.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"AFU MMIO RD received while PORT is in reset error detected",
	// 10
	"AFU MMIO WR received while PORT is in reset error detected",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"Tx Channel 1 invalid request encoding error detected",
	"Tx Channel 1 cl_len=3 not supported error detected.",
	"Tx Channel 1 request with cl_len=2 does NOT have a 2CL aligned address error detected",
	"Tx Channel 1 request with cl_len=4 does NOT have a 4CL aligned address error detected",
	// 20
	"Tx Channel 1 insufficient data payload Error detected",
	"Tx Channel 1 data payload overrun error detected",
	"Tx Channel 1 incorrect address on subsequent payloads error detected",
	"Tx Channel 1 Non-zero SOP detected for requests!=WrLine_* error detected",
	"Tx Channel 1 SOP expected to be 0 for req_type!=WrLine_*",
	"Tx Channel 1 Illegal VC_SEL. Atomic request is only supported on VL0 error detected",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	// 30
	"RSVD.",
	"RSVD.",
	"MMIO TimedOut error detected",
	"Tx Channel 2 fifo overflo error detected",
	"MMIO Read response received, with no matching request pending error detected",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	// 40
	"Number of pending requests: counter overflow error detected",
	"Request with Address violating SMM range error detected",
	"Request with Address violating second SMM range error detected",
	"Request with Address violating ME stolen range",
	"Request with Address violating Generic protected range error detected ",
	"Request with Address violating Legacy Range Low error detected",
	"Request with Address violating Legacy Range High error detected",
	"Request with Address violating VGA memory range error detected",
	"Page Fault error detected",
	"PMR Erro error detected",
	// 50
	"AP6 event detected",
	"VF FLR detected on port when PORT configured in PF access mode error detected ",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"RSVD.",
	"Tx Channel 1 byte_len cannot be zero",
	"Tx Channel 1 illegal operation: sum of byte_len and byte_start should be less than or equal to 64",
	"Tx Channel 1 illegal operation: cl_len cannot be non-zero when mode is eMOD_BYTE",
	"Tx Channel 1 byte_len and byte_start should be zero when mode is not eMOD_BYTE"
};

/*
 * errors command configuration, set during parse_args()
 */
static struct errors_config {
	bool clear;
	int force_count;
	enum verbs_index which;
	bool help_only;
} errors_config = {.clear = false, .which = VERB_ALL, .help_only = false};

/*
 * Print help
 */
void errors_help(void)
{
	unsigned int i;

	printf("\nPrint and clear errors\n"
	       "        fpgainfo errors [-h] [-c] {");
	printf("%s", supported_verbs[0]);
	for (i = 1; i < sizeof(supported_verbs) / sizeof(supported_verbs[0]);
	     i++) {
		printf(",%s", supported_verbs[i]);
	}
	printf("}\n\n"
	       "                -h,--help           Print this help\n"
	       "                -c,--clear          Clear all errors\n"
	       "                --force             Retry clearing errors 64 times\n"
	       "                                    to clear certain error conditions\n"
	       "\n");
	errors_config.help_only = true;
}

#define ERRORS_GETOPT_STRING ":chf"
int parse_error_args(int argc, char *argv[])
{
	optind = 0;
	struct option longopts[] = {
		{"clear", no_argument, NULL, 'c'},
		{"force", no_argument, NULL, 'f'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};

	int getopt_ret;
	int option_index;
	errors_config.force_count = 1;

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, ERRORS_GETOPT_STRING,
					    longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'c': /* clear */
			errors_config.clear = true;
			break;

		case 'f': /* Force */
			errors_config.clear = true;
			errors_config.force_count = 64;
			break;

		case 'h': /* help */
			errors_help();
			return -1;

		case ':': /* missing option argument */
			OPAE_ERR("Missing option argument\n");
			errors_help();
			return -1;

		case '?':
		default: /* invalid option */
			OPAE_ERR("Invalid cmdline options\n");
			errors_help();
			return -1;
		}
	}

	// The word after 'errors' should be what to operate on ("all", "fme",
	// or "port")
	optind++;
	if (argc < optind + 1) {
		OPAE_ERR("Not enough parameters\n");
		errors_help();
		return -1;
	}

	int cmp = 0;
	if ((optind < argc) && 
		strcmp_s(argv[optind - 1], RSIZE_MAX_STR, "errors", &cmp) == EOK &&
		cmp == 0) {
		char *verb = argv[optind];
		size_t idx = str_in_list(verb, supported_verbs, VERB_MAX);
		if (idx < VERB_MAX) {
			errors_config.which = idx;
		} else {
			OPAE_ERR("Not a valid errors resource spec: %s\n", verb);
			errors_help();
			return -1;
		}
	} else {
		OPAE_ERR("Not a valid errors resource spec: %s\n",
			argv[optind - 1]);
		errors_help();
		return -1;
	}

	return 0;
}

fpga_result errors_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res = FPGA_OK;
	if (0 == parse_error_args(argc, argv)) {
		switch (errors_config.which) {
		case VERB_FME:
			res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
			ON_FPGAINFO_ERR_GOTO(res, out,
					     "setting type to FPGA_DEVICE");
			break;
		case VERB_PORT:
			res = fpgaPropertiesSetObjectType(*filter,
							  FPGA_ACCELERATOR);
			ON_FPGAINFO_ERR_GOTO(
				res, out, "setting type to FPGA_ACCELERATOR");
			break;
		case VERB_ALL:
		default:
			break;
		}
	}
out:
	return res;
}

static void print_errors_info(fpga_token token, fpga_properties props,
			      struct fpga_error_info *errinfos,
			      uint32_t num_errors)
{
	int i;
	int j;
	fpga_result res = FPGA_OK;
	fpga_objtype objtype;
	const char *const *error_string = NULL;
	int size = 0;

	if ((NULL == errinfos) || (0 == num_errors)) {
		return;
	}

	if (errors_config.clear) {
		for (i = 0; i < errors_config.force_count; i++) {
			fpgaClearAllErrors(token);
		}
	}

	res = fpgaPropertiesGetObjectType(props, &objtype);
	fpgainfo_print_err("reading objtype from properties", res);

	if (((VERB_ALL == errors_config.which)
	     || (VERB_FME == errors_config.which))
	    && (FPGA_DEVICE == objtype)) {
		fpgainfo_print_common("//****** FME ERRORS ******//", props);

		for (i = 0; i < (int)num_errors; i++) {
			uint64_t error_value = 0;

			res = fpgaReadError(token, i, &error_value);
			fpgainfo_print_err("reading error for FME", res);

			printf("%-32s : 0x%" PRIX64 "\n", errinfos[i].name,
			       error_value);

			int cmp = 0;
			if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR, 
				    "Errors", &cmp) == EOK && cmp == 0) {
				size = FME_ERROR_COUNT;
				error_string = FME_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "Next Error", &cmp) == EOK && cmp == 0) {
				size = 0;
				error_string = NULL;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "First Error", &cmp) == EOK && cmp == 0) {
				size = 0;
				error_string = NULL;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "PCIe0 Errors", &cmp) == EOK && cmp == 0) {
				size = PCIE0_ERROR_COUNT;
				error_string = PCIE0_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "Inject Error", &cmp) == EOK && cmp == 0) {
				size = INJECT_ERROR_COUNT;
				error_string = INJECT_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "Catfatal Errors", &cmp) == EOK && cmp == 0) {
				size = CATFATAL_ERROR_COUNT;
				error_string = CATFATAL_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "Nonfatal Errors", &cmp) == EOK && cmp == 0) {
				size = NONFATAL_ERROR_COUNT;
				error_string = NONFATAL_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "PCIe1 Errors", &cmp) == EOK && cmp == 0) {
				size = PCIE1_ERROR_COUNT;
				error_string = PCIE1_ERROR;
			}

			for (j = 0; (j < size) && (NULL != error_string); j++) {
				if (FPGA_BIT_IS_SET(error_value, j)) {
					printf("\t %s \n", error_string[j]);
				}
			}
		}
	} else if (((VERB_ALL == errors_config.which)
		    || (VERB_PORT == errors_config.which))
		   && (FPGA_ACCELERATOR == objtype)) {
		fpgainfo_print_common("//****** PORT ERRORS ******//", props);

		for (i = 0; i < (int)num_errors; i++) {
			uint64_t error_value = 0;
			res = fpgaReadError(token, i, &error_value);
			fpgainfo_print_err("reading error for PORT", res);

			printf("%-32s : 0x%" PRIX64 "\n", errinfos[i].name,
			       error_value);

			int cmp = 0;
			if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "Errors", &cmp) == EOK && cmp == 0) {
				size = PORT_ERROR_COUNT;
				error_string = PORT_ERROR;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "First Malformed Req", &cmp) == EOK && cmp == 0) {
				size = 0;
				error_string = NULL;
			} else if (strcmp_s(errinfos[i].name, RSIZE_MAX_STR,
				    "First Error", &cmp) == EOK && cmp == 0) {
				size = 0;
				error_string = NULL;
			}

			for (j = 0; (j < size) && (NULL != error_string); j++) {
				if (FPGA_BIT_IS_SET(error_value, j)) {
					printf("\t %s \n", error_string[j]);
				}
			}
		}
	}
}

fpga_result errors_command(fpga_token *tokens, int num_tokens, int argc,
			   char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	fpga_properties props;
	struct fpga_error_info *errinfos = NULL;

	if (errors_config.help_only) {
		return res;
	}

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		uint32_t num_errors;

		res = fpgaGetProperties(tokens[i], &props);
		if (res == FPGA_OK) {
			res = fpgaPropertiesGetNumErrors(props, &num_errors);
			fpgainfo_print_err("reading errors from properties", res);

			if (num_errors != 0) {
				int j;
				errinfos = (struct fpga_error_info *)calloc(
					num_errors, sizeof(*errinfos));
				if (!errinfos) {
					res = FPGA_NO_MEMORY;
					OPAE_ERR("Error allocating memory");
					goto destroy_and_free;
				}

				for (j = 0; j < (int)num_errors; j++) {
					res = fpgaGetErrorInfo(tokens[i], j,
							       &errinfos[j]);
					fpgainfo_print_err(
						"reading error info structure", res);
					replace_chars(errinfos[j].name, '_', ' ');
					upcase_pci(errinfos[j].name, 
						    strnlen_s(errinfos[j].name, RSIZE_MAX_STR));
					upcase_first(errinfos[j].name);
				}
			}

			print_errors_info(tokens[i], props, errinfos, num_errors);
		destroy_and_free:
			free(errinfos);
			errinfos = NULL;
			fpgaDestroyProperties(&props);
			if (res == FPGA_NO_MEMORY) {
			    break;
			}
		} else {
			fpgainfo_print_err("reading properties from token", res);
		}
	}

	return res;
}
