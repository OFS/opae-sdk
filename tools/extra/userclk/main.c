// Copyright(c) 2017, Intel Corporation
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
#include <stdbool.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <limits.h>

#include "safe_string/safe_string.h"
#include "opae/fpga.h"
#include "types_int.h"

#include "usrclk/user_clk_pgm_uclock.h"

#define GETOPT_STRING ":hB:D:F:S:P:H:L"

struct option longopts[] = {
		{"help",                no_argument,       NULL, 'h'},
		{"bus-number",          required_argument, NULL, 'B'},
		{"device-number",       required_argument, NULL, 'D'},
		{"function-number",     required_argument, NULL, 'F'},
		{"socket-number",       required_argument, NULL, 'S'},
		{"port",                required_argument, NULL, 'P'},
		{"freq-high",           required_argument, NULL, 'H'},
		{"freq-low",            required_argument, NULL, 'L'},
		{0,0,0,0}
};

// User clock Command line struct
struct  UserClkCommandLine
{
	int      bus;
	int      device;
	int      function;
	int      socket;
	int      port;
	int      freq_high;
	int      freq_low;

};

struct UserClkCommandLine userclkCmdLine = { -1, -1, -1, -1, 0, -1,-1};

// User clock Command line input help
void UserClkAppShowHelp()
{
	printf("Usage:\n");
	printf("./userclk \n");
	printf("<Bus>                 --bus=<BUS NUMBER>           OR  -B=<BUS NUMBER>\n");
	printf("<Device>              --device=<DEVICE NUMBER>     OR  -D=<DEVICE NUMBER>\n");
	printf("<Function>            --function=<FUNCTION NUMBER> OR  -F=<FUNCTION NUMBER>\n");
	printf("<Socket>              --socket=<socket NUMBER>     OR  -S=<SOCKET NUMBER>\n");
	printf("<Port>                --port                       OR  -P=<Port id> \n");
	printf("<freq high>           --freq-high                  OR  -H=<User clock high> \n");
	printf("<freq low>            --freq-low                   OR  -L=<User clock low> \n");
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

fpga_result get_fpga_port_sysfs(fpga_token token,char* sysfs_port,int portid);
int ParseCmds(struct UserClkCommandLine *userclkCmdLine, int argc, char *argv[]);

int main( int argc, char** argv )
{
	fpga_properties filter             = NULL;
	uint32_t num_matches               = 1;
	char sysfs_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                 = FPGA_OK;
	uint64_t userclk_high              = 0;
	uint64_t userclk_low               = 0;
	fpga_token fme_token               = NULL;

	// Parse command line
	if ( argc < 2 ) {
		UserClkAppShowHelp();
	return 1;
	} else if ( 0!= ParseCmds(&userclkCmdLine, argc, argv) ) {
		FPGA_ERR( "Error scanning command line \n.");
	return 2;
	}

	printf(" ------- Command line Input START ---- \n \n");

	printf(" Bus                   : %d\n", userclkCmdLine.bus);
	printf(" Device                : %d \n", userclkCmdLine.device);
	printf(" Function              : %d \n", userclkCmdLine.function);
	printf(" Socket                : %d \n", userclkCmdLine.socket);
	printf(" Port                  : %d \n", userclkCmdLine.port);
	printf(" Freq High             : %d \n", userclkCmdLine.freq_high);
	printf(" Freq Low              : %d \n", userclkCmdLine.freq_low);

	printf(" ------- Command line Input END ---- \n\n");

	// Enum FPGA device
	result = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(result, out_exit, "creating properties object");

	result = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(result, out_destroy_prop, "setting object type");

	if (userclkCmdLine.bus >0){
		result = fpgaPropertiesSetBus(filter, userclkCmdLine.bus);
		ON_ERR_GOTO(result, out_destroy_prop, "setting bus");
	}

	if (userclkCmdLine.device >0) {
		result = fpgaPropertiesSetDevice(filter, userclkCmdLine.device);
		ON_ERR_GOTO(result, out_destroy_prop, "setting device");
	}

	if (userclkCmdLine.function >0){
		result = fpgaPropertiesSetFunction(filter, userclkCmdLine.function);
		ON_ERR_GOTO(result, out_destroy_prop, "setting function");
	}

	if (userclkCmdLine.socket >0){
		result = fpgaPropertiesSetSocketID(filter, userclkCmdLine.socket);
		ON_ERR_GOTO(result, out_destroy_prop, "setting socket");
	}

	result = fpgaEnumerate(&filter, 1, &fme_token,1, &num_matches);
	ON_ERR_GOTO(result, out_destroy_prop, "enumerating FPGAs");

	if (num_matches < 1) {
		fprintf(stderr, "FPGA Resource not found.\n");
		result = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}
	fprintf(stderr, "FME Resource found.\n");

	// Read port sysfs path
	result = get_fpga_port_sysfs(fme_token, sysfs_path,userclkCmdLine.port);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get port syfs path");
		goto out_destroy_prop;
	}

	// read user clock
	result = get_userclock(sysfs_path, &userclk_high, &userclk_low);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get user clock ");
		goto out_destroy_prop;
	}

	if (userclkCmdLine.freq_high > 0 &&
		userclkCmdLine.freq_low  > 0) {

		// set user clock
		result = set_userclock(sysfs_path, userclkCmdLine.freq_high, userclkCmdLine.freq_high);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to set user clock ");
			goto out_destroy_prop;
		}

		// read user clock
		result = get_userclock(sysfs_path, &userclk_high, &userclk_low);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to get user clock ");
			goto out_destroy_prop;
		}

	}

	printf("Done set and get for user clock values\n");

	/* Destroy properties object */
out_destroy_prop:
	result = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(result, out_exit, "destroying properties object");

out_exit:
	return result;

}
// Get port syfs path
fpga_result get_fpga_port_sysfs(fpga_token token,char* sysfs_port,int portid)
{
	struct _fpga_token  *_token;
	char *p                        = 0;
	int device_id                  = 0;

	if (sysfs_port == NULL) {
		FPGA_ERR("Invalid Input Parameters");
		return FPGA_INVALID_PARAM;
	}

	if (token == NULL) {
		FPGA_ERR("Invalid fpga token");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token*)token;

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL == p)
		return FPGA_INVALID_PARAM;
	p = strrchr(_token->sysfspath, '.');
	if (NULL == p)
		return FPGA_INVALID_PARAM;

	device_id = atoi(p + 1);

	if(device_id < INT_MIN || device_id >INT_MAX) {
		FPGA_ERR("Invalid Input Parameters");
		return FPGA_INVALID_PARAM;
	}

	if(portid <INT_MIN || portid >INT_MAX) {
		FPGA_ERR("Invalid Input Parameters");
		return FPGA_INVALID_PARAM;
	}

	if(device_id + portid <INT_MIN || device_id + portid >INT_MAX) {
		FPGA_ERR("Invalid Input Parameters");
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ii(sysfs_port, SYSFS_PATH_MAX,
		SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT,
		device_id, device_id + portid);

	return FPGA_OK;
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
				('=' == *tmp_optarg)){
			++tmp_optarg;
		}

		if((!optarg) &&
				(NULL != argv[optind]) &&
				('-' != argv[optind][0]) ) {
			tmp_optarg = argv[optind++];
		}

		if(tmp_optarg == NULL )
			break;

		switch(getopt_ret){
		case 'h':
			// Command line help
			UserClkAppShowHelp();
			return -2;
			break;

		case 'B':
			// bus number
			endptr = NULL;
			userclkCmdLine->bus = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'D':
			// Device number
			endptr = NULL;
			userclkCmdLine->device = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'F':
			// Function number
			endptr = NULL;
			userclkCmdLine->function = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'S':
			// Socket number
			endptr = NULL;
			userclkCmdLine->socket = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'P':
			// Port id
			endptr = NULL;
			userclkCmdLine->port = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'H':
			// User clock High
			endptr = NULL;
			userclkCmdLine->freq_high = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'L':
			// User clock low
			endptr = NULL;
			userclkCmdLine->freq_low = strtol(tmp_optarg, &endptr, 0);
			break;

		case '?':
		default:    /* invalid option */
			printf("Invalid cmdline options.\n");
			return -1;
		}
	}
	return 0;
}

