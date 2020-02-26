// Copyright(c) 2017-2019, Intel Corporation
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
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <new>
#include <arpa/inet.h>

#include <opae/fpga.h>
#include "mmlink_server.h"
#include "mm_debug_link_interface.h"

#include "safe_string/safe_string.h"

// STP index in AFU
#define FPGA_PORT_INDEX_STP               1
#define FPGA_PORT_STP_DFH_REVBIT         12

#define GETOPT_STRING ":hB:D:F:S:P:Iv"

#define PRINT_ERR(format, ...) \
	printf("%s:%u:%s() : " format "\n", __FILE__, __LINE__, __func__,\
               ## __VA_ARGS__)

struct option longopts[] = {
		{"help",        no_argument,       NULL, 'h'},
		{"segment",     required_argument, NULL, 0xe},
		{"bus",         required_argument, NULL, 'B'},
		{"device",      required_argument, NULL, 'D'},
		{"function",    required_argument, NULL, 'F'},
		{"socket-id",   required_argument, NULL, 'S'},
		{"port",        required_argument, NULL, 'P'},
		{"ip",          required_argument, NULL, 'I'},
    {"version",     no_argument,       NULL, 'v'},
		{0,0,0,0}
};

// mmlink Command line struct
struct  MMLinkCommandLine
{
	int      segment;
	int      bus;
	int      device;
	int      function;
	int      socket;
	int      port;
	char     ip[16];
};

struct MMLinkCommandLine mmlinkCmdLine = { -1, -1, -1, -1, -1, 0, { 0, } };

// mmlink Command line input help
void MMLinkAppShowHelp()
{
	printf("Usage:\n");
	printf("mmlink\n");
	printf("<Segment>             --segment=<SEGMENT NUMBER>\n");
	printf("<Bus>                 --bus=<BUS NUMBER>           "
		"OR  -B <BUS NUMBER>\n");
	printf("<Device>              --device=<DEVICE NUMBER>     "
		"OR  -D <DEVICE NUMBER>\n");
	printf("<Function>            --function=<FUNCTION NUMBER> "
		"OR  -F <FUNCTION NUMBER>\n");
	printf("<Socket-id>           --socket-id=<SOCKET NUMBER>  "
		"OR  -S <SOCKET NUMBER>\n");
	printf("<TCP PORT>            --port=<PORT>                "
		"OR  -P <PORT>\n");
	printf("<IP ADDRESS>          --ip=<IP ADDRESS>            "
		"OR  -I <IP ADDRESS>\n");
  printf("<Version>             -v,--version Print version and exit\n");
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

void mmlink_sig_handler(int sig)
{
	UNUSED_PARAM(sig);
	perror("SIGINT: stopping the server\n");
}

int ParseCmds(struct MMLinkCommandLine *mmlinkCmdLine,
		int argc,
		char *argv[]);
int run_mmlink(fpga_handle  port_handle,
		uint64_t *mmio_ptr,
		struct MMLinkCommandLine *mmlinkCmdLine );

int main( int argc, char** argv )
{
	fpga_properties filter             = NULL;
	uint32_t num_matches               = 1;
	fpga_result result                 = FPGA_OK;
	fpga_token port_token              = NULL;
	fpga_handle  port_handle           = NULL;
	uint64_t *mmio_ptr                 = NULL;
	int res;

	// Parse command line
	if ( argc < 2 ) {
		MMLinkAppShowHelp();
		return 1;
	} else if ( 0 != (res = ParseCmds(&mmlinkCmdLine, argc, argv)) ) {
		if (res != -2)
			PRINT_ERR( "Error scanning command line \n.");
		return 2;
	}

	if ('\0' == mmlinkCmdLine.ip[0]) {
		strncpy_s(mmlinkCmdLine.ip, sizeof(mmlinkCmdLine.ip),
			"0.0.0.0", 9);
	}

	printf(" ------- Command line Input START ----\n\n");

	printf(" Segment               : %d\n", mmlinkCmdLine.segment);
	printf(" Bus                   : %d\n", mmlinkCmdLine.bus);
	printf(" Device                : %d\n", mmlinkCmdLine.device);
	printf(" Function              : %d\n", mmlinkCmdLine.function);
	printf(" Socket-id             : %d\n", mmlinkCmdLine.socket);
	printf(" Port                  : %d\n", mmlinkCmdLine.port);
	printf(" IP address            : %s\n", mmlinkCmdLine.ip);
	printf(" ------- Command line Input END   ----\n\n");

	// Signal Handler
	signal(SIGINT, mmlink_sig_handler);

	// Enum FPGA device
	result = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(result, out_exit, "creating properties object");

	result = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(result, out_destroy_prop, "setting object type");

	if (mmlinkCmdLine.segment > -1){
		result = fpgaPropertiesSetSegment(filter, mmlinkCmdLine.segment);
		ON_ERR_GOTO(result, out_destroy_prop, "setting segment");
	}

	if (mmlinkCmdLine.bus > -1){
		result = fpgaPropertiesSetBus(filter, mmlinkCmdLine.bus);
		ON_ERR_GOTO(result, out_destroy_prop, "setting bus");
	}

	if (mmlinkCmdLine.device > -1) {
		result = fpgaPropertiesSetDevice(filter, mmlinkCmdLine.device);
		ON_ERR_GOTO(result, out_destroy_prop, "setting device");
	}

	if (mmlinkCmdLine.function > -1){
		result = fpgaPropertiesSetFunction(filter, mmlinkCmdLine.function);
		ON_ERR_GOTO(result, out_destroy_prop, "setting function");
	}

	if (mmlinkCmdLine.socket > -1){
		result = fpgaPropertiesSetSocketID(filter, mmlinkCmdLine.socket);
		ON_ERR_GOTO(result, out_destroy_prop, "setting socket");
	}

	result = fpgaEnumerate(&filter, 1, &port_token,1, &num_matches);
	ON_ERR_GOTO(result, out_destroy_prop, "enumerating FPGAs");

	if (num_matches < 1) {
		fprintf(stderr, "PORT  Resource not found.\n");
		result = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}
	fprintf(stderr, "PORT Resource found.\n");

	result = fpgaOpen(port_token, &port_handle, FPGA_OPEN_SHARED);
	ON_ERR_GOTO(result, out_destroy_tok, "opening accelerator");

	result = fpgaMapMMIO(port_handle, FPGA_PORT_INDEX_STP, &mmio_ptr);
	ON_ERR_GOTO(result, out_close, "mapping MMIO space");

	if( run_mmlink(port_handle,mmio_ptr,&mmlinkCmdLine) != 0) {
		PRINT_ERR( "Failed to connect MMLINK  \n.");
		result = FPGA_NOT_SUPPORTED;
	}

	/* Unmap MMIO space */
	result = fpgaUnmapMMIO(port_handle, FPGA_PORT_INDEX_STP);
	ON_ERR_GOTO(result, out_close, "unmapping MMIO space");

	/* Close driver handle */
out_close:
	result = fpgaClose(port_handle);
	ON_ERR_GOTO(result, out_destroy_tok, "closing Port");

	/* Destroy token */
out_destroy_tok:
	result = fpgaDestroyToken(&port_token);
	ON_ERR_GOTO(result, out_destroy_prop, "destroying token");

	/* Destroy properties object */
out_destroy_prop:
	result = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(result, out_exit, "destroying properties object");

out_exit:
	return result;

}

// run MMLink server
int run_mmlink(fpga_handle  port_handle,
		uint64_t *mmio_ptr,
		struct MMLinkCommandLine *mmlinkCmdLine )
{
	mmlink_server *server          = NULL;
	int res                        = 0;
	struct sockaddr_in sock;
	uint64_t value                 = 0;

	if (mmio_ptr == NULL) {
		PRINT_ERR("Invalid input mmio pointer \n");
		return -1;
	}

	if (mmlinkCmdLine == NULL) {
		PRINT_ERR("Invalid input command line \n");
		return -1;
	}

	memset_s(&sock, sizeof(sock), 0);
	sock.sin_family = AF_INET;
	sock.sin_port = htons(mmlinkCmdLine->port);
	if (1 != inet_pton(AF_INET, mmlinkCmdLine->ip, &sock.sin_addr)) {
		PRINT_ERR("Failed to convert IP address: %s\n", mmlinkCmdLine->ip);
		return -1;
	}

	res = fpgaReadMMIO64(port_handle, FPGA_PORT_INDEX_STP, 0, &value);
	if (res != 0) {
		PRINT_ERR("Failed to read STP DFH \n");
		return -1;
	}
	//printf("STP DFH = 0x%lx\n" ,value);

	value &= 0x1000;
	value = value >> FPGA_PORT_STP_DFH_REVBIT;
	if(1 != value){
		PRINT_ERR("Invalid STP revision number \n");
		return -1;
	}
	mm_debug_link_interface *driver = get_mm_debug_link();
	server = new (std::nothrow) mmlink_server(&sock, driver);
	if (!server) {
		PRINT_ERR("Failed to allocate memory \n");
		return -1;
	}

	// Run MMLink server
	res = server->run((unsigned char*)mmio_ptr);

	if (server)
		delete server;

	return res;
}

// parse Input command line
int ParseCmds(struct MMLinkCommandLine *mmlinkCmdLine, int argc, char *argv[])
{
	int getopt_ret     = 0 ;
	int option_index   = 0;
	char *endptr       = NULL;

	while( -1 != ( getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
			longopts, &option_index))){
		const char *tmp_optarg = optarg;

		if((optarg) &&
		   ('=' == *tmp_optarg)){
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
			MMLinkAppShowHelp();
			return -2;
			break;

		case 0xe:
			// segment number
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --segment");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->segment = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'B':
			// bus number
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --bus");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->bus = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'D':
			// Device number
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --device");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->device = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'F':
			// Function number
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --function");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->function = strtol(tmp_optarg,
							&endptr, 0);
			break;

		case 'S':
			// Socket number
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --socket");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->socket = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'P':
			// TCP Port 
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --port");
				return -1;
			}
			endptr = NULL;
			mmlinkCmdLine->port = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'I':
			// Ip address
			if (!tmp_optarg) {
				PRINT_ERR("Missing required argument for --ip");
				return -1;
			}
			strncpy_s(mmlinkCmdLine->ip, sizeof(mmlinkCmdLine->ip),
				tmp_optarg, 16);
			break;

		case 'v':
			// Version
      printf("mmlink %s %s%s\n",
             OPAE_VERSION,
             OPAE_GIT_COMMIT_HASH,
             OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			return -2;

		case '?':
		default: /* invalid option */
			printf("Invalid cmdline options.\n");
			return -1;
		}
	}

	return 0;
}
