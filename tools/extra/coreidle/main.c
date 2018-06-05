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
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <uuid/uuid.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "safe_string/safe_string.h"
#include "opae/fpga.h"
#include "bitstream_int.h"


#define GETOPT_STRING ":hB:D:F:S:G"

struct option longopts[] = {
		{"help",                no_argument,       NULL, 'h'},
		{"bus-number",          required_argument, NULL, 'B'},
		{"device-number",       required_argument, NULL, 'D'},
		{"function-number",     required_argument, NULL, 'F'},
		{"socket-number",       required_argument, NULL, 'S'},
		{"gbs",                 required_argument, NULL, 'G'},
		{0,0,0,0}
};

// coreidle Command line struct
struct  CoreIdleCommandLine
{
	int      bus;
	int      device;
	int      function;
	int      socket;
	char     filename[512];
	uint8_t *gbs_data;
	size_t   gbs_len;

};

struct CoreIdleCommandLine coreidleCmdLine = { -1, -1, -1, -1, "",NULL, 0};

// core idle Command line input help
void CoreidleAppShowHelp()
{
	printf("Usage:\n");
	printf("./coreidle \n");
	printf("<Bus>                 --bus=<BUS NUMBER>          "
			" OR  -B=<BUS NUMBER>\n");
	printf("<Device>              --device=<DEVICE NUMBER>    "
			" OR  -D=<DEVICE NUMBER>\n");
	printf("<Function>            --function=<FUNCTION NUMBER> "
			"OR  -F=<FUNCTION NUMBER>\n");
	printf("<Socket>              --socket=<socket NUMBER>    "
			" OR  -S=<SOCKET NUMBER>\n");
	printf("<GBS bitstream>       --gbs                      "
			"  OR  -G \n");
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

int read_bitstream(struct CoreIdleCommandLine *coreidleCmdLine);
int ParseCmds(struct CoreIdleCommandLine *coreidleCmdLine, int argc, char *argv[]);
extern fpga_result set_cpu_core_idle(fpga_handle handle,uint64_t gbs_power);

int main( int argc, char** argv )
{
	fpga_properties filter             = NULL;
	uint32_t num_matches               = 1;
	fpga_result result                 = FPGA_OK;
	fpga_token fme_token               = NULL;
	fpga_handle  fme_handle            = NULL;
	struct gbs_metadata  metadata;
	fpga_result res                    = FPGA_OK;

	// Parse command line
	if ( argc < 2 ) {
		CoreidleAppShowHelp();
	return 1;
	} else if ( 0!= ParseCmds(&coreidleCmdLine, argc, argv) ) {
		fprintf(stderr, "Error scanning command line \n.");
	return 2;
	}

	printf(" ------- Command line Input START ---- \n \n");

	printf(" Bus                   : %d \n",  coreidleCmdLine.bus);
	printf(" Device                : %d \n", coreidleCmdLine.device);
	printf(" Function              : %d \n", coreidleCmdLine.function);
	printf(" Socket                : %d \n", coreidleCmdLine.socket);
	printf(" filename              : %s \n", coreidleCmdLine.filename);


	printf(" ------- Command line Input END ---- \n\n");

	if(read_bitstream(&coreidleCmdLine) !=0) {
		ON_ERR_GOTO(FPGA_INVALID_PARAM, out_exit, "Invalid Input bitstream");
	}

	// Enum FPGA device
	result = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(result, out_exit, "creating properties object");

	result = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(result, out_destroy_prop, "setting object type");

	if (coreidleCmdLine.bus >0){
		result = fpgaPropertiesSetBus(filter, coreidleCmdLine.bus);
		ON_ERR_GOTO(result, out_destroy_prop, "setting bus");
	}

	if (coreidleCmdLine.device >0) {
		result = fpgaPropertiesSetDevice(filter, coreidleCmdLine.device);
		ON_ERR_GOTO(result, out_destroy_prop, "setting device");
	}

	if (coreidleCmdLine.function >0){
		result = fpgaPropertiesSetFunction(filter, coreidleCmdLine.function);
		ON_ERR_GOTO(result, out_destroy_prop, "setting function");
	}

	if (coreidleCmdLine.socket >0){
		result = fpgaPropertiesSetSocketID(filter, coreidleCmdLine.socket);
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

	// Open FME device
	result = fpgaOpen(fme_token, &fme_handle, 0);
	ON_ERR_GOTO(result, out_destroy_tok, "opening FME");

	// Verify bitstream GUID
	result = check_bitstream_guid(coreidleCmdLine.gbs_data);
	ON_ERR_GOTO(result, out_close, "closing FME");

	// Verify bitstream metadata
	result = validate_bitstream_metadata(fme_handle,
					coreidleCmdLine.gbs_data) ;
	ON_ERR_GOTO(result, out_close, "closing FME");

	// Verify bitstream length
	memset_s(&metadata, sizeof(metadata), 0);
	if (get_bitstream_json_len(coreidleCmdLine.gbs_data) > 0) {

		// Read GBS json metadata
		result = read_gbs_metadata(coreidleCmdLine.gbs_data, &metadata);
		ON_ERR_GOTO(result, out_close, "closing FME");

		printf(" GBS Power :%d watts \n", metadata.afu_image.power);
	}

	// Idle CPU cores
	if ( metadata.afu_image.power >=0 ) {
		 res = set_cpu_core_idle(fme_handle, metadata.afu_image.power);
	}


out_close:
	/* Close file handle */
	result = fpgaClose(fme_handle);
	ON_ERR_GOTO(result, out_destroy_tok, "closing FME");

	/* Destroy token */
out_destroy_tok:
	result = fpgaDestroyToken(&fme_token);
	ON_ERR_GOTO(result, out_destroy_prop, "destroying token");


	/* Destroy properties object */
out_destroy_prop:
	result = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(result, out_exit, "destroying properties object");

out_exit:
	return res;

}

// Read file name
int read_bitstream(struct CoreIdleCommandLine *coreidleCmdLine)
{
	FILE *f         = NULL;
	long len        = 0;
	int ret         = 0;

	if ( !coreidleCmdLine || !coreidleCmdLine->filename )
		return -EINVAL;

	/* open file */
	f = fopen(coreidleCmdLine->filename, "rb");
	if (!f) {
		perror(coreidleCmdLine->filename);
		return -1;
	}

	/* get filesize */
	ret = fseek(f, 0, SEEK_END);
	if (ret < 0) {
		perror(coreidleCmdLine->filename);
		goto out_close;
	}
	len = ftell(f);
	if (len < 0) {
		perror(coreidleCmdLine->filename);
		goto out_close;
	}

	/* allocate memory */
	coreidleCmdLine->gbs_data = (uint8_t *)malloc(len);
	if (!coreidleCmdLine->gbs_data) {
		perror("malloc");
		goto out_close;
	}

	/* read bistream data */
	ret = fseek(f, 0, SEEK_SET);
	if (ret < 0) {
		perror(coreidleCmdLine->filename);
		goto out_free;
	}

	coreidleCmdLine->gbs_len = fread(coreidleCmdLine->gbs_data, 1, len, f);
	if (ferror(f)) {
		perror(coreidleCmdLine->filename);
		goto out_free;
	}
	if (coreidleCmdLine->gbs_len != (size_t)len) {
		fprintf(stderr,
		     "Filesize and number of bytes read don't match\n");
		goto out_free;
	}

	fclose(f);
	return 0;

out_free:
	free((void*)coreidleCmdLine->gbs_data);
out_close:
	fclose(f);
	return -1;
}

#define MAX_CMD_OPT 256
// parse Input command line
int ParseCmds(struct CoreIdleCommandLine *coreidleCmdLine,
		int argc,
		char *argv[])
{
	int getopt_ret     = 0 ;
	int option_index   = 0;
	char *endptr       = NULL;

	while( -1 != ( getopt_ret = getopt_long(argc, argv,
			GETOPT_STRING, longopts, &option_index))){
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
			CoreidleAppShowHelp();
			return -2;
			break;

		case 'B':
			// bus number
			endptr = NULL;
			coreidleCmdLine->bus = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'D':
			// Device number
			endptr = NULL;
			coreidleCmdLine->device = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'F':
			// Function number
			endptr = NULL;
			coreidleCmdLine->function = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'S':
			// Socket number
			endptr = NULL;
			coreidleCmdLine->socket = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'G': {
			errno_t e;
			// Bitstream GBS
			e = strncpy_s(coreidleCmdLine->filename,
					sizeof(coreidleCmdLine->filename),
					tmp_optarg,
					MAX_CMD_OPT);
			if (EOK != e) {
				printf("strncpy_s failed.\n");
				return -1;
			}
			coreidleCmdLine->filename[sizeof(coreidleCmdLine->filename)-1] = 0;
			} break;

		case '?':
		default:    /* invalid option */
			printf("Invalid cmdline options.\n");
			return -1;
		}
	}
	return 0;
}
