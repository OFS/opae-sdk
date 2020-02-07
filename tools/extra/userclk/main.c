// Copyright(c) 2017-2020, Intel Corporation
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
#include <stdbool.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <limits.h>

#include "safe_string/safe_string.h"
#include <opae/fpga.h>



#define GETOPT_STRING ":hB:D:F:S:H:L:v"

struct option longopts[] = {
	{ "help",      no_argument,       NULL, 'h' },
	{ "segment",   required_argument, NULL, 0xe },
	{ "bus",       required_argument, NULL, 'B' },
	{ "device",    required_argument, NULL, 'D' },
	{ "function",  required_argument, NULL, 'F' },
	{ "socket-id", required_argument, NULL, 'S' },
	{ "freq-high", required_argument, NULL, 'H' },
	{ "freq-low",  required_argument, NULL, 'L' },
	{ "version",   no_argument,       NULL, 'v' },
	{ NULL, 0, NULL, 0 }
};

// User clock Command line struct
struct  UserClkCommandLine
{
	int      segment;
	int      bus;
	int      device;
	int      function;
	int      socket;
	int      freq_high;
	int      freq_low;

};

struct UserClkCommandLine userclkCmdLine = { -1, -1, -1, -1, -1, -1, -1 };

// User clock Command line input help
void UserClkAppShowHelp(void)
{
	printf("Usage:\n");
	printf("userclk\n");
	printf("<Segment>             --segment=<SEGMENT NUMBER>\n");
	printf("<Bus>                 --bus=<BUS NUMBER>           OR  -B=<BUS NUMBER>\n");
	printf("<Device>              --device=<DEVICE NUMBER>     OR  -D=<DEVICE NUMBER>\n");
	printf("<Function>            --function=<FUNCTION NUMBER> OR  -F=<FUNCTION NUMBER>\n");
	printf("<Socket-id>           --socket-id=<socket NUMBER>  OR  -S=<SOCKET NUMBER>\n");
	printf("<freq high>           --freq-high                  OR  -H=<User clock high>\n");
	printf("<freq low>            --freq-low                   OR  -L=<User clock low>\n");
	printf("<version>             -v,--version\n");
	printf("\n");

}

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)                    \
		do {                                       \
			if ((res) != FPGA_OK) {            \
				print_err((desc), (res));  \
				goto label;                \
			}                                  \
		} while (0)

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

int ParseCmds(struct UserClkCommandLine *userclkCmdLine, int argc, char *argv[]);

int main(int argc, char *argv[])
{
	fpga_properties filter             = NULL;
	uint32_t num_matches               = 1;
	fpga_result result                 = FPGA_OK;
	fpga_result res                    = FPGA_OK;
	uint64_t userclk_high              = 0;
	uint64_t userclk_low               = 0;
	fpga_token accel_token             = NULL;
	int high                           = 0;
	int low                            = 0;

	fpga_handle        accelerator_handle;

	// Parse command line
	if ( argc < 2 ) {
		UserClkAppShowHelp();
		return 1;
	} else if ( 0!= ParseCmds(&userclkCmdLine, argc, argv) ) {
		return 2;
	}

	printf(" ------- Command line Input START ----\n \n");

	printf(" Segment               : %d\n", userclkCmdLine.segment);
	printf(" Bus                   : %d\n", userclkCmdLine.bus);
	printf(" Device                : %d\n", userclkCmdLine.device);
	printf(" Function              : %d\n", userclkCmdLine.function);
	printf(" Socket-id             : %d\n", userclkCmdLine.socket);
	printf(" Freq High             : %d\n", userclkCmdLine.freq_high);
	printf(" Freq Low              : %d\n", userclkCmdLine.freq_low);

	printf(" ------- Command line Input END   ----\n\n");

	result = fpgaInitialize(NULL);
	ON_ERR_GOTO(result, out_exit, "Failed to initilize ");

	// Enum FPGA device
	result = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(result, out_exit, "creating properties object");

	result = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(result, out_destroy_prop, "setting object type");

	if (-1 != userclkCmdLine.segment) {
		result = fpgaPropertiesSetSegment(filter, userclkCmdLine.segment);
		ON_ERR_GOTO(result, out_destroy_prop, "setting segment");
	}

	if (-1 != userclkCmdLine.bus) {
		result = fpgaPropertiesSetBus(filter, userclkCmdLine.bus);
		ON_ERR_GOTO(result, out_destroy_prop, "setting bus");
	}

	if (-1 != userclkCmdLine.device) {
		result = fpgaPropertiesSetDevice(filter, userclkCmdLine.device);
		ON_ERR_GOTO(result, out_destroy_prop, "setting device");
	}

	if (-1 != userclkCmdLine.function){
		result = fpgaPropertiesSetFunction(filter, userclkCmdLine.function);
		ON_ERR_GOTO(result, out_destroy_prop, "setting function");
	}

	if (-1 != userclkCmdLine.socket){
		result = fpgaPropertiesSetSocketID(filter, userclkCmdLine.socket);
		ON_ERR_GOTO(result, out_destroy_prop, "setting socket");
	}

	result = fpgaEnumerate(&filter, 1, &accel_token,1, &num_matches);
	ON_ERR_GOTO(result, out_destroy_prop, "enumerating FPGAs");

	if (num_matches < 1) {
		OPAE_ERR("FPGA Resource not found.");
		res = FPGA_NOT_FOUND;
		goto out_destroy_prop;
	}
	printf("AFU Resource found.\n");

	result = fpgaOpen(accel_token, &accelerator_handle, 0);
	ON_ERR_GOTO(result, out_destroy_prop, "opening accelerator");

	res = fpgaGetUserClock(accelerator_handle, &userclk_high, &userclk_low, 0);
	ON_ERR_GOTO(res, out_close, "Failed to get user clock");

 	printf("\nApproximate frequency:\n"
		"High clock = %5.1f MHz\n"
		"Low clock  = %5.1f MHz\n \n",
		userclk_high / 1.0e6, userclk_low / 1.0e6);

	if (userclkCmdLine.freq_high > 0 || userclkCmdLine.freq_low > 0 ) {
		high = userclkCmdLine.freq_high;
		low = userclkCmdLine.freq_low;
		if (low <= 0) {
			low = userclkCmdLine.freq_high / 2;
		} else if (high <= 0) {
			high = userclkCmdLine.freq_low * 2;
		} else if ((abs(high - (2 * low))) > 1) {
			res = FPGA_INVALID_PARAM;
			OPAE_ERR("High freq must be ~ (2 * Low freq)");
			goto out_close;
		}
	} else {
		res = FPGA_INVALID_PARAM;
		OPAE_ERR("Please specify one or both of -H and -L");
		goto out_close;
	}

	res = fpgaSetUserClock(accelerator_handle, high, low, 0);
	ON_ERR_GOTO(res, out_close, "Failed to set user clock");

	res = fpgaGetUserClock(accelerator_handle, &userclk_high, &userclk_low, 0);
	ON_ERR_GOTO(res, out_close, "Failed to get user clock");

	printf("\nApproximate frequency:\n"
		"High clock = %5.1f MHz\n"
		"Low clock  = %5.1f MHz\n \n",
		userclk_high / 1.0e6, userclk_low / 1.0e6);

out_close:
	result = fpgaClose(accelerator_handle);
	ON_ERR_GOTO(result, out_destroy_tok, "closing accelerator");

out_destroy_tok:
	result = fpgaDestroyToken(&accel_token);
	ON_ERR_GOTO(result, out_destroy_prop, "destroying token object");

	/* Destroy properties object */
out_destroy_prop:
	result = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(result, out_exit, "destroying properties object");

out_exit:
	return (res != FPGA_OK) ? res : result;
}

// parse Input command line
int ParseCmds(struct UserClkCommandLine *userclkCmdLine, int argc, char *argv[])
{
	int getopt_ret     = 0;
	int option_index   = 0;
	char *endptr       = NULL;

	while( -1 != ( getopt_ret = getopt_long(argc, argv, GETOPT_STRING, longopts, &option_index))){
		const char *tmp_optarg = optarg;

		if((optarg) &&
		   ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		if((!optarg) && (optind < argc) &&
		   (NULL != argv[optind]) &&
	 	   ('-' != argv[optind][0]) ) {
			tmp_optarg = argv[optind++];
		}

		switch(getopt_ret){
		case 'h':
			// Command line help
			UserClkAppShowHelp();
			return -2;
			break;

		case 0xe:
			// segment number
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->segment = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'B':
			// bus number
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->bus = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'D':
			// Device number
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->device = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'F':
			// Function number
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->function = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'S':
			// Socket number
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->socket = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'H':
			// User clock High
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->freq_high = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'L':
			// User clock low
			if (!tmp_optarg)
				return -1;
			endptr = NULL;
			userclkCmdLine->freq_low = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'v':
			printf("userclk %s %s%s\n",
			       OPAE_VERSION,
			       OPAE_GIT_COMMIT_HASH,
			       OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			return -2;

		case '?':
		default:    /* invalid option */
			printf("Invalid cmdline options.\n");
			return -1;
		}
	}

	return 0;
}
