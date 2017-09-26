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
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "opae/fpga.h"
#include "types_int.h"
#include "common_int.h"

// SYSFS FME Errors
#define FME_SYSFS_FME_ERRORS                   "errors/fme-errors/errors"
#define FME_SYSFS_PCIE0_ERRORS                 "errors/pcie0_errors"
#define FME_SYSFS_PCIE1_ERRORS                 "errors/pcie1_errors"

#define FME_SYSFS_BBS_ERRORS                   "errors/bbs_errors"
#define FME_SYSFS_GBS_ERRORS                   "errors/gbs_errors"
#define FME_SYSFS_WARNING_ERRORS               "errors/warning_errors"

#define FME_SYSFS_NONFATAL_ERRORS              "errors/nonfatal_errors"
#define FME_SYSFS_CATFATAL_ERRORS              "errors/catfatal_errors"
#define FME_SYSFS_INJECT_ERROR                 "errors/inject_error"

#define FME_SYSFS_ERR_REVISION                 "errors/revision"

#define PORT_SYSFS_ERR                         "errors/errors"
#define PORT_SYSFS_ERR_CLEAR                   "errors/clear"

// SYFS Thermal
#define FME_SYSFS_THERMAL_MGMT_TEMP            "thermal_mgmt/temperature"
#define FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP  "thermal_mgmt/threshold_trip"

// SYSFS Power
#define FME_SYSFS_POWER_MGMT_CONSUMED          "power_mgmt/consumed"

// MMIO scratchpad
#define PORT_SCRATCHPAD0                       0x0028
#define NLB_CSR_SCRATCHPAD                     (0x40000 + 0x0104 )
#define PORT_MMIO_LEN                          (0x40000 + 0x0512 )

#define MMO_WRITE64_VALUE                      0xF1F1F1F1F1F1F1F1
#define MMO_WRITE32_VALUE                      0xF1F1F1

#define FPGA_CSR_LEN                           64

#define DEVICEID_PATH        "/sys/bus/pci/devices/%04x:%02x:%02x.%d/device"
#define FPGA_PORT_RES_PATH   "/sys/bus/pci/devices/%04x:%02x:%02x.%d/resource2"


#define FPGA_SET_BIT(val, index) val |= (1 << index)
#define FPGA_CLEAR_BIT(val, index) val &= ~(1 << index)
#define FPGA_TOGGLE_BIT(val, index) val ^= (1 << index)
#define FPGA_BIT_IS_SET(val, index) (val & (1 << index))

/* Type definitions */
typedef struct {
	uint32_t uint[16];
} cache_line;


int usleep(unsigned);

#ifndef CL
# define CL(x)                       ((x) * 64)
#endif // CL
#ifndef LOG2_CL
# define LOG2_CL                     6
#endif // LOG2_CL
#ifndef MB
# define MB(x)                       ((x) * 1024 * 1024)
#endif // MB

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

#define LPBK1_BUFFER_SIZE            MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE MB(2)
#define LPBK1_DSM_SIZE               MB(2)
#define CSR_SRC_ADDR                 0x0120
#define CSR_DST_ADDR                 0x0128
#define CSR_CTL                      0x0138
#define CSR_CFG                      0x0140
#define CSR_NUM_LINES                0x0130
#define DSM_STATUS_TEST_COMPLETE     0x40
#define CSR_AFU_DSM_BASEL            0x0110
#define CSR_AFU_DSM_BASEH            0x0114

/* SKX-P NLB0 AFU_ID */
#define SKX_P_NLB0_AFUID "D8424DC4-A4A3-C413-F89E-433683F9040B"

static const char * const FME_ERROR[] = {
		"Fabric error detected", \
		"Fabric fifo under / overflow error detected", \
		"KTI CDC Parity Error detected", \
		"KTI CDC Parity Error detected", \
		"IOMMU Parity error detected", \
		"AFU PF/VF access mismatch detected", \
		"Indicates an MBP event error detected", \
};

static const char * const PCIE0_ERROR[] = {
		"TLP format/type error detected", \
		"TTLP MW address error detected", \
		"TLP MW length error detected", \
		"TLP MR address error detected", \
		"TLP MR length error detected", \
		"TLP CPL tag error detected", \
		"TLP CPL status error detected", \
		"TLP CPL timeout error detected", \
		"CCI bridge parity error detected", \
		"TLP with EP  error  detected", \
};

static const char * const PCIE1_ERROR[] = {
		"TLP format/type error detected", \
		"TTLP MW address error detected", \
		"TLP MW length error detected", \
		"TLP MR address error detected", \
		"TLP MR length error detected", \
		"TLP CPL tag error detected", \
		"TLP CPL status error detected", \
		"TLP CPL timeout error detected", \
		"CCI bridge parity error detected", \
		"TLP with EP  error  detected", \
};

static const char * const RAS_NONFATAL_ERROR [] = {
		"Temperature threshold triggered AP1 detected", \
		"Temperature threshold triggered AP2 detected", \
		"PCIe error detected", \
		"AFU port Fatal error detected", \
		"ProcHot event error detected", \
		"AFU PF/VF access mismatch error detected", \
		"Injected Warning Error detected", \
		"Reserved", \
		"Reserved", \
		"Temperature threshold triggered AP6 detected", \
		"Power threshold triggered AP1 error detected", \
		"Power threshold triggered AP2 error detected", \
		"MBP event error detected", \
};

static const char * const RAS_CATFATAL_ERROR[] = {
		"KTI link layer error detected.", \
		"tag-n-cache error detected.", \
		"CCI error detected.", \
		"KTI protocol error detected.", \
		"Fatal DRAM error detected", \
		"IOMMU fatal parity error detected.", \
		"Fabric fatal error detected", \
		"Poison error from any of PCIe ports detected", \
		"Injected Fatal Error detected", \
		"Catastrophic CRC error detected", \
		"Catastrophic thermal runaway event detected", \
		"Injected Catastrophic Error detected", \
};

static const char * const RAS_INJECT_ERROR[] = {
		"Set Catastrophic  error .", \
		"Set Fatal error.", \
		"Ser Non-fatal error .", \
};

static const char * const RAS_GBS_ERROR [] = {
		"Temperature threshold triggered AP1 detected", \
		"Temperature threshold triggered AP2 detected", \
		"PCIe error detected", \
		"AFU port Fatal error detected", \
		"ProcHot event error detected", \
		"AFU PF/VF access mismatch error detected", \
		"Injected Warning Error detected", \
		"Poison error from any of PCIe ports detected", \
		"GBS CRC errordetected ", \
		"Temperature threshold triggered AP6 detected", \
		"Power threshold triggered AP1 error detected", \
		"Power threshold triggered AP2 error detected", \
		"MBP event error detected", \
};

static const char * const RAS_BBS_ERROR[] = {
		"KTI link layer error detected.", \
		"tag-n-cache error detected.", \
		"CCI error detected.", \
		"KTI protocol error detected.", \
		"Fatal DRAM error detected", \
		"IOMMU fatal parity error detected.", \
		"Fabric fatal error detected", \
		"Poison error from any of PCIe ports detected", \
		"Injected Fatal Error detected", \
		"Catastrophic CRC error detected", \
		"Catastrophic thermal runaway event detected", \
		"Injected Catastrophic Error detected", \
};

static const char * const RAS_WARNING_ERROR[] = {
		"Green bitstream fatal event error detected.", \
};

static const char * const PORT_ERROR[] = {
		"Tx Channel 0 overflow error detected.", \
		"Tx Channel 0 invalid request encodingr error detected.", \
		"Tx Channel 0 cl_len=3 not supported error detected.", \
		"Tx Channel 0 request with cl_len=2 does NOT have a 2CL aligned address error detected.", \
		"RSVD.", "RSVD.", "RSVD.","RSVD.",\
		"AFU MMIO RD received while PORT is in reset error detected", \
		"AFU MMIO WR received while PORT is in reset error detected", \
		"RSVD.", "RSVD.", "RSVD.", "RSVD.", "RSVD.",\
		"Tx Channel 1 invalid request encoding error detected", \
		"Tx Channel 1 cl_len=3 not supported error detected.", \
		"Tx Channel 1 request with cl_len=2 does NOT have a 2CL aligned address error detected", \
		"Tx Channel 1 request with cl_len=4 does NOT have a 4CL aligned address error detected", \
		"Tx Channel 1 insufficient data payload Error detected", \
		"Tx Channel 1 data payload overrun error detected", \
		"Tx Channel 1 incorrect address on subsequent payloads error detected", \
		"Tx Channel 1 Non-zero SOP detected for requests!=WrLine_* error detected", \
		"Tx Channel 1 Illegal VC_SEL. Atomic request is only supported on VL0 error detected", \
		"RSVD.", "RSVD.", "RSVD.", "RSVD.", "RSVD.", "RSVD.", "RSVD.",\
		"MMIO TimedOut error detected", \
		"Tx Channel 2 fifo overflo error detected", \
		"MMIO Read response received, with no matching request pending error detected", \
		"RSVD.", "RSVD.", "RSVD.", "RSVD.", "RSVD.", \
		"Number of pending requests: counter overflow error detected", \
		"Request with Address violating SMM range error detected", \
		"Request with Address violating second SMM range error detected", \
		"Request with Address violating ME stolen range", \
		"Request with Address violating Generic protected range error detected ", \
		"Request with Address violating Legacy Range Low error detected", \
		"Request with Address violating Legacy Range High error detected", \
		"Request with Address violating VGA memory range error detected", \
		"Page Fault error detected", \
		"PMR Erro error detected", \
		"AP6 event detected ", \
		"VF FLR detected on port when PORT configured in PF access mode error detected ", \
};

// RAS Error Inject CSR
struct ras_inject_error {
	union {
		uint64_t csr;
		struct {
			/* Catastrophic  error */
			uint64_t  catastrophicr_error : 1;
			/* Fatal error */
			uint64_t  fatal_error : 1;
			/* Non-fatal error */
			uint64_t  nonfatal_error : 1;
			/* Reserved */
			uint64_t  rsvd : 61;
		};
	};
};

#define GETOPT_STRING ":hB:D:F:S:PQRNTCEGHIO"

struct option longopts[] = {
		{"help",                no_argument,       NULL, 'h'},
		{"bus-number",          required_argument, NULL, 'B'},
		{"device-number",       required_argument, NULL, 'D'},
		{"function-number",     required_argument, NULL, 'F'},
		{"socket-number",       required_argument, NULL, 'S'},
		{"print-error",         no_argument,       NULL, 'P'},
		{"catast-error",        no_argument,       NULL, 'Q'},
		{"fatal-error",         no_argument,       NULL, 'R'},
		{"nofatal-error",       no_argument,       NULL, 'N'},
		{"thermal-trip",        no_argument,       NULL, 'T'},
		{"clearinj-error",      no_argument,       NULL, 'C'},
		{"mwaddress-error",     no_argument,       NULL, 'E'},
		{"mraddress-error",     no_argument,       NULL, 'G'},
		{"mwlength-error",      no_argument,       NULL, 'H'},
		{"mrlength-error",      no_argument,       NULL, 'I'},
		{"pagefault-error",     no_argument,       NULL, 'O'},
		{0,0,0,0}
};

// RAS Command line struct
struct  RASCommandLine
{
	uint32_t          flags;
#define RASAPP_CMD_FLAG_HELP      0x00000001
#define RASAPP_CMD_FLAG_VERSION   0x00000002
#define RASAPP_CMD_PARSE_ERROR    0x00000003

#define RASAPP_CMD_FLAG_BUS       0x00000008
#define RASAPP_CMD_FLAG_DEV       0x00000010
#define RASAPP_CMD_FLAG_FUNC      0x00000020
#define RASAPP_CMD_FLAG_SOCKET    0x00000040

	int      bus;
	int      device;
	int      function;
	int      socket;
	bool     print_error;
	bool     catast_error;
	bool     fatal_error;
	bool     nonfatal_error;
	bool     clear_injerror;
	bool     mwaddress_error;
	bool     mraddress_error;
	bool     mwlength_error;
	bool     mrlength_error;
	bool     pagefault_error;
};

struct RASCommandLine rasCmdLine = { 0, -1, -1, -1, -1, false,
				false, false, false,false,
				false, false, false, false, false};

// RAS Command line input help
void RASAppShowHelp()
{
	printf("Usage:\n");
	printf("./ras \n");
	printf("<Bus>              --bus=<BUS NUMBER>           "
		"OR  -B=<BUS NUMBER>\n");
	printf("<Device>           --device=<DEVICE NUMBER>     "
		"OR  -D=<DEVICE NUMBER>\n");
	printf("<Function>         --function=<FUNCTION NUMBER> "
		"OR  -F=<FUNCTION NUMBER>\n");
	printf("<Socket>           --socket=<socket NUMBER>    "
		" OR  -S=<SOCKET NUMBER>\n");
	printf("<Print Error>      --print-error                OR  -P \n");
	printf("<Catast Error>     --catast-error               OR  -Q \n");
	printf("<Fatal Error>      --fatal-error                OR  -R \n");
	printf("<NoFatal Error>    --nofatal-error              OR  -N \n");
	printf("<Clear Inj Error>  --clearinj-error             OR  -C \n");
	printf("<MW Address error> --mwaddress-error            OR  -E \n");
	printf("<MR Address error> --mwaddress-error            OR  -G \n");
	printf("<MW Length error>  --mwlength-error             OR  -H \n");
	printf("<MR Length error>  --mrlength-error             OR  -I \n");
	printf("<Page Fault Error> --pagefault-error            OR  -O \n");
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

fpga_result print_ras_errors(fpga_token token);
fpga_result print_pwr_temp(fpga_token token);
fpga_result clear_inject_ras_errors(fpga_token token,
					struct RASCommandLine *rasCmdLine);
fpga_result inject_ras_errors(fpga_token token,
				struct RASCommandLine *rasCmdLine);
fpga_result mmio_error(struct RASCommandLine *rasCmdLine);
fpga_result print_port_errors(fpga_token token);
fpga_result clear_port_errors(fpga_token token);
fpga_result page_fault_errors();
int ParseCmds(struct RASCommandLine *rasCmdLine, int argc, char *argv[]);

int main( int argc, char** argv )
{
	fpga_result result          = 0;
	fpga_properties filter      = NULL;
	fpga_token fme_token ;
	uint32_t num_matches        = 1;

	// Parse command line
	if ( argc < 2 ) {
		RASAppShowHelp();
	return 1;
	} else if ( 0!= ParseCmds(&rasCmdLine, argc, argv) ) {
		FPGA_ERR( "Error scanning command line \n.");
	return 2;
	}

	printf(" ------- Command line Input Start ---- \n \n");

	printf(" Bus                   : %d\n", rasCmdLine.bus);
	printf(" Device                : %d \n", rasCmdLine.device);
	printf(" Function              : %d \n", rasCmdLine.function);
	printf(" Socket                : %d \n", rasCmdLine.socket);
	printf(" Print Error           : %d \n", rasCmdLine.print_error);
	printf(" Catas Error           : %d \n", rasCmdLine.catast_error);
	printf(" Fatal Error           : %d \n", rasCmdLine.fatal_error);
	printf(" NonFatal Error        : %d \n", rasCmdLine.nonfatal_error);
	printf(" Clear Error           : %d \n", rasCmdLine.clear_injerror);
 	printf(" MW Address Error      : %d \n", rasCmdLine.mwaddress_error);
	printf(" MR Address Error      : %d \n", rasCmdLine.mraddress_error);
	printf(" MW Length Error       : %d \n", rasCmdLine.mwlength_error);
	printf(" MR Length Error       : %d \n", rasCmdLine.mrlength_error);
	printf(" Page Fault Error      : %d \n", rasCmdLine.pagefault_error);

	printf(" ------- Command line Input END ---- \n\n");

	// Enum FPGA device
	result = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(result, out_exit, "creating properties object");

	result = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(result, out_destroy_prop, "setting object type");

	if (rasCmdLine.bus >0){
		result = fpgaPropertiesSetBus(filter, rasCmdLine.bus);
		ON_ERR_GOTO(result, out_destroy_prop, "setting bus");
	}

	if (rasCmdLine.device >0) {
		result = fpgaPropertiesSetDevice(filter, rasCmdLine.device);
		ON_ERR_GOTO(result, out_destroy_prop, "setting device");
	}

	if (rasCmdLine.function >0){
		result = fpgaPropertiesSetFunction(filter, rasCmdLine.function);
		ON_ERR_GOTO(result, out_destroy_prop, "setting function");
	}

	if (rasCmdLine.socket >0){
		result = fpgaPropertiesSetSocketID(filter, rasCmdLine.socket);
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

	// Inject error
	if (rasCmdLine.catast_error ||
		rasCmdLine.fatal_error ||
		rasCmdLine.nonfatal_error) {

		// Inject RAS ERROR
		result = inject_ras_errors(fme_token,&rasCmdLine);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to print fme errors");
			goto out_destroy_prop;
		}
	}

	// inject MMIO error
	if ( (rasCmdLine.mwaddress_error == true) ||
		(rasCmdLine.mraddress_error == true) ||
		(rasCmdLine.mwlength_error == true) ||
		(rasCmdLine.mrlength_error == true) ) {

		result = mmio_error(&rasCmdLine);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed set MMIO errors");
			goto out_destroy_prop;
		}
	}

	// Clear Inject Error
	if (rasCmdLine.clear_injerror ) {

		// clear RAS ERROR
		result = clear_inject_ras_errors(fme_token,&rasCmdLine);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to clear inject errors");
			goto out_destroy_prop;
		}

		// clear Port ERROR
		result = clear_port_errors(fme_token);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to clear port errors");
			goto out_destroy_prop;
		}
	}

	sleep(1);

	if (rasCmdLine.print_error) {
	// Print RAS Error
		result = print_ras_errors(fme_token);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to print fme errors");
			goto out_destroy_prop;
		}

		// Print port Error
		result = print_port_errors(fme_token);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to print port errors");
			goto out_destroy_prop;
		}

		// Print power and temp
		result = print_pwr_temp(fme_token);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get power and temp");
			goto out_destroy_prop;
		}
	}

	if (rasCmdLine.pagefault_error) {
		// Page fault error
		result = page_fault_errors();
		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to trigger page fault errors");
			goto out_destroy_prop;
		}
	}

	/* Destroy properties object */
out_destroy_prop:
	result = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(result, out_exit, "destroying properties object");

out_exit:
	return result;

}

// Print Error
fpga_result print_errors(fpga_token token,
			const char * err_path,
			const char * const* err_strings,
			int size)
{
	struct _fpga_token  *_token       = 0;
	int i                             = 0;
	uint64_t value                    = 0;
	char syfs_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                = FPGA_OK;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	if(err_path == NULL ||
		err_strings == NULL) {
		FPGA_ERR("Invalid input sting");
		return FPGA_INVALID_PARAM;
	}

	snprintf(syfs_path, sizeof(syfs_path), "%s/%s",
			_token->sysfspath,
			err_path );

	// Read error.
	result = sysfs_read_u64(syfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get errors");
		return result;
	}

	printf(" CSR : 0x%lx \n", value);
	for (i = 0; i < FPGA_CSR_LEN; i++) {
		if ((i < size) && FPGA_BIT_IS_SET(value, i)) {
			printf("\t %s \n", err_strings[i]);
		}
	}
	return result;
}

// prints RAS errors
fpga_result print_ras_errors(fpga_token token)
{
	struct _fpga_token  *_token       = 0;
	uint64_t revision                 = 0;
	char syfs_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                = FPGA_OK;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	printf("\n ==========================================\n");
	printf(" ----------- PRINT FME ERROR  START-------- \n \n");

	// get revision
	snprintf(syfs_path, sizeof(syfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_ERR_REVISION );

	// Read revision.
	result = sysfs_read_u64(syfs_path, &revision);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get fme revison");
		return result;
	}
	printf(" fme error revison : %ld \n", revision);

	// Revision 0
	if( revision == 1 ) {

		// Non Fatal Error
		printf("\n ------- Non Fatal error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_NONFATAL_ERRORS,
					RAS_NONFATAL_ERROR,
					sizeof(RAS_NONFATAL_ERROR) /sizeof(RAS_NONFATAL_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme non fatal errors");
			return result;
		}

		// Fatal Error
		printf("\n ------- Fatal error ------------ \n");
		result = print_errors(token,
				FME_SYSFS_CATFATAL_ERRORS,
				RAS_CATFATAL_ERROR,
				sizeof(RAS_CATFATAL_ERROR) /sizeof(RAS_CATFATAL_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme fatal errors");
			return result;
		}

		// Injected error
		printf("\n ------- Injected error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_INJECT_ERROR,
					RAS_INJECT_ERROR,
					sizeof(RAS_INJECT_ERROR) /sizeof(RAS_INJECT_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme Injected errors");
			return result;
		}

		// FME error
		printf("\n ------- FME error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_FME_ERRORS,
					FME_ERROR,
					sizeof(FME_ERROR) /sizeof(FME_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme  errors");
			return result;
		}

		// PCIe0 error
		printf("\n ------- PCIe0 error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_PCIE0_ERRORS,
					PCIE0_ERROR,
					sizeof(PCIE0_ERROR) /sizeof(PCIE0_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get pcie0  errors");
			return result;
		}

		// PCIe1 error
		printf("\n ------- PCIe1 error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_PCIE1_ERRORS,
					PCIE1_ERROR,
					sizeof(PCIE1_ERROR) /sizeof(PCIE1_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get pcie1  errors");
			return result;
		}

	// Revision 0
	} else if( revision == 0){

		// GBS Error
		printf("\n ------- GBS error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_GBS_ERRORS,
					RAS_GBS_ERROR,
					sizeof(RAS_GBS_ERROR) /sizeof(RAS_GBS_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme gbs errors");
			return result;
		}

		// BBS Error
		printf("\n ------- BBS error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_BBS_ERRORS,
					RAS_BBS_ERROR,
					sizeof(RAS_BBS_ERROR) /sizeof(RAS_BBS_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme bbs errors");
			return result;
		}

		// Injected error
		printf("\n ------- Injected error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_INJECT_ERROR,
					RAS_INJECT_ERROR,
					sizeof(RAS_INJECT_ERROR) /sizeof(RAS_INJECT_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme Injected errors");
			return result;
		}

		// FME error
		printf("\n ------- FME error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_FME_ERRORS,
					FME_ERROR,
					sizeof(FME_ERROR) /sizeof(FME_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get fme  errors");
			return result;
		}

		// PCIe0 error
		printf("\n ------- PCIe0 error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_PCIE0_ERRORS,
					PCIE0_ERROR,
					sizeof(PCIE0_ERROR) /sizeof(PCIE0_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get pcie0  errors");
			return result;
		}

		// PCIe1 error
		printf("\n ------- PCIe1 error ------------ \n");
		result = print_errors(token,
					FME_SYSFS_PCIE1_ERRORS,
					PCIE1_ERROR,
					sizeof(PCIE1_ERROR) /sizeof(PCIE1_ERROR[0]));

		if (result != FPGA_OK) {
			FPGA_ERR("Failed  to get pcie1  errors");
			return result;
		}

	} else {
		printf("\n Invalid FME Error Revision \n");
	}

	printf("\n ----------- PRINT FME ERROR  END----------\n");
	printf(" ========================================== \n \n");
	return result;
}
// prints PORT errors
fpga_result print_port_errors(fpga_token token)
{
	struct _fpga_token  *_token       = 0;
	int i                             = 0;
	uint64_t value                    = 0;
	int size                          = 0;
	char sysfs_port[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;
	char *p                           = 0;
	int device_id                     = 0;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	printf("\n ==========================================\n");
	printf(" ----------- PRINT PORT ERROR  START-------- \n \n");

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL == p)
		return FPGA_INVALID_PARAM;
	p = strrchr(_token->sysfspath, '.');
	if (NULL == p)
		return FPGA_INVALID_PARAM;

	device_id = atoi(p + 1);

	snprintf(sysfs_port, SYSFS_PATH_MAX,
		SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT"/%s",
		device_id, device_id,PORT_SYSFS_ERR);

	// Read port error.
	result = sysfs_read_u64(sysfs_port, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get fme errors");
		return result;
	}

	printf("\n \n Port error CSR : 0x%lx \n", value);

	size = sizeof(PORT_ERROR) /sizeof(PORT_ERROR[0]);

	for (i = 0; i < 64; i++) {
		if ( FPGA_BIT_IS_SET(value, i) && (i < size)) {
			printf("\t %s \n", PORT_ERROR[i]);
		}
	}

	printf("\n ----------- PRINT PORT ERROR  END----------\n");
	printf(" ========================================== \n \n");

	return result;

}

// clear PORT errors
fpga_result clear_port_errors(fpga_token token)
{
	struct _fpga_token  *_token       = 0;
	uint64_t value                    = 0;
	char sysfs_port[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;
 	char *p                           = 0;
	int device_id                     = 0;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	printf(" ----------- Clear port error-------- \n \n");

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL == p)
		return FPGA_INVALID_PARAM;
	p = strrchr(_token->sysfspath, '.');
	if (NULL == p)
		return FPGA_INVALID_PARAM;

	device_id = atoi(p + 1);

	snprintf(sysfs_port, SYSFS_PATH_MAX,
		SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT"/%s",
		device_id, device_id,PORT_SYSFS_ERR);

	// Read port error.
	result = sysfs_read_u64(sysfs_port, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get port errors");
		return result;
	}

	printf("\n \n Port error CSR : 0x%lx \n", value);

	snprintf(sysfs_port, SYSFS_PATH_MAX,
		SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT"/%s",
		device_id, device_id,PORT_SYSFS_ERR_CLEAR);

	result = sysfs_write_u64(sysfs_port, value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to write errors");
	}

	return result;
}

// Inject RAS errors
fpga_result inject_ras_errors(fpga_token token,
				struct RASCommandLine *rasCmdLine)
{
	struct _fpga_token  *_token           = NULL;
	struct ras_inject_error  inj_error    = {0};
	char sysfs_path[SYSFS_PATH_MAX]       = {0};
	fpga_result result                    = FPGA_OK;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	printf("----------- INJECT ERROR START -------- \n \n");

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_INJECT_ERROR);
	result = sysfs_read_u64(sysfs_path, &inj_error.csr);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to get fme errors");
		return result;
	}

	printf("inj_error.csr: %ld \n", inj_error.csr);

	if (rasCmdLine->catast_error ) {
		inj_error.catastrophicr_error = 1;
	}

	if (rasCmdLine->fatal_error ) {
		inj_error.fatal_error = 1;
	}

	if (rasCmdLine->nonfatal_error ) {
		inj_error.nonfatal_error = 1;
	}

	printf("inj_error.csr: %ld \n", inj_error.csr);

	result = sysfs_write_u64(sysfs_path ,inj_error.csr);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to write RAS inject errors");
		return result;
	}

	printf("----------- INJECT ERROR  END-------- \n \n");
	return result;
}

// Clear Inject RAS errors
fpga_result clear_inject_ras_errors(fpga_token token,
					struct RASCommandLine *rasCmdLine)
{
	struct _fpga_token  *_token           = NULL;
	struct ras_inject_error  inj_error    = {0};
	char sysfs_path[SYSFS_PATH_MAX]       = {0};
	fpga_result result                    = FPGA_OK;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_INJECT_ERROR);
	result = sysfs_read_u64(sysfs_path, &inj_error.csr);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed  to read inject error");
		return result;
	}

	printf(" Clear inj_error.csr: 0x%lx \n", inj_error.csr);

	result = sysfs_write_u64(sysfs_path ,0x0);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to clear inject errors");
		return result;
	}
	return result;
}

// Print FPGA power and temperature
fpga_result print_pwr_temp(fpga_token token)
{
	struct _fpga_token  *_token       = 0;
	uint64_t value                    = 0;
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;

	_token = (struct _fpga_token*)token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	printf("\n ----------- POWER & THERMAL -------------\n");
	printf(" ========================================== \n \n");

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_POWER_MGMT_CONSUMED);
	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get power consumed");
		return result;
	}
	printf(" Power consumed       : %lu watts \n",value);

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_THERMAL_MGMT_TEMP);
	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get temperature");
		return result;
	}
	printf(" Temperature          : %lu Centigrade \n",value );

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_THERMAL_MGMT_THRESHOLD_TRIP);
	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get temperature");
		return result;
	}
	printf(" Thermal Trip         : %lu Centigrade \n",value );

	printf("\n ----------- POWER & THERMAL -------------\n");
	printf(" ========================================== \n \n");

	return result;
}

// MMIO erros
fpga_result mmio_error(struct RASCommandLine *rasCmdLine)
{
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;
	int bus                           = 0;
	int device                        = 0;
	int function                      = 0;
	uint64_t value                    = 0;
	int fd                            = 0;
	uint8_t *ptr                      = 0;

	if (rasCmdLine  == NULL ) {
		FPGA_ERR("Invalid input ");
		return FPGA_INVALID_PARAM;
	}

	if ( rasCmdLine->bus >0 )
		bus = rasCmdLine->bus;

	if ( rasCmdLine->device >0 )
		device = rasCmdLine->bus;

	if ( rasCmdLine->function >0 )
		function = rasCmdLine->bus;

	snprintf(sysfs_path, sizeof(sysfs_path),
			DEVICEID_PATH,0,bus,device,function);

	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read Device id");
		return result;
	}

	if(value != FPGA_INTEGRATED_DEVICEID) {
		FPGA_ERR("Failed  to read Device id");
		return FPGA_NOT_SUPPORTED;
	}

	snprintf(sysfs_path, sizeof(sysfs_path),
			FPGA_PORT_RES_PATH,0,bus,device,function);

	fd = open(sysfs_path, O_RDWR);
	if (fd < 0) {
		FPGA_ERR("Failed to open FPGA PCIE BAR2");
		return FPGA_EXCEPTION;
	}

	ptr =  mmap(NULL, PORT_MMIO_LEN,
			PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED ) {
		FPGA_ERR("Failed to map FPGA PCIE BAR2");
		result = FPGA_EXCEPTION;
		goto out_close ;
	}

	// Memory Write length error
	if(rasCmdLine->mwlength_error) {

		FPGA_DBG("Memory Write length error \n");
		*((volatile uint64_t *) (ptr + PORT_SCRATCHPAD0+3))
				= (uint16_t)MMO_WRITE64_VALUE;
	}

	// Memory Read length error
	if(rasCmdLine->mrlength_error) {

		FPGA_DBG(" Memory Read length error \n");
		value = *((volatile uint64_t *) (ptr + PORT_SCRATCHPAD0+3));
		FPGA_DBG(" Memory Read length value %lx\n",value);
	}

	// Memory Read addresss error
	if(rasCmdLine->mraddress_error) {

		FPGA_DBG("Memory Read addresss error \n");
		value = *((volatile uint16_t *) (ptr + NLB_CSR_SCRATCHPAD +3));
		FPGA_DBG("Memory Read addresss value  %lx\n",value);

		value = *((volatile uint64_t *) (ptr + PORT_SCRATCHPAD0+3));
		FPGA_DBG("Memory Read addresss value  %lx\n",value);
	}

	// Memory Write addresss error
	if(rasCmdLine->mwaddress_error) {

		FPGA_DBG("Memory Write addresss error \n");
		*((volatile uint16_t *) (ptr + NLB_CSR_SCRATCHPAD +3))
				= (uint16_t)MMO_WRITE32_VALUE;
	}

	if(ptr)
		munmap(ptr, PORT_MMIO_LEN);
out_close:
	if(fd >=0)
		close(fd);

	return result;
}
// page fault errors
fpga_result page_fault_errors()
{
	fpga_properties    filter = NULL;
	fpga_token         accelerator_token;
	fpga_handle        accelerator_handle;
	fpga_guid          guid;
	uint32_t           num_matches;

	volatile uint64_t *dsm_ptr    = NULL;
	volatile uint64_t *status_ptr = NULL;
	volatile uint64_t *input_ptr  = NULL;
	volatile uint64_t *output_ptr = NULL;

	uint64_t        dsm_wsid;
	uint64_t        input_wsid;
	uint64_t        output_wsid;
	fpga_result     res = FPGA_OK;


	if (uuid_parse(SKX_P_NLB0_AFUID, guid) < 0) {
		fprintf(stderr, "Error parsing guid '%s'\n", SKX_P_NLB0_AFUID);
		goto out_exit;
	}

	/* Look for accelerator with MY_ACCELERATOR_ID */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_exit, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

	res = fpgaPropertiesSetGUID(filter, guid);
	ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");

	if (rasCmdLine.bus >0){
		res = fpgaPropertiesSetBus(filter, rasCmdLine.bus);
		ON_ERR_GOTO(res, out_destroy_prop, "setting bus");
	}

	if (rasCmdLine.device >0) {
		res = fpgaPropertiesSetDevice(filter, rasCmdLine.device);
		ON_ERR_GOTO(res, out_destroy_prop, "setting device");
	}

	if (rasCmdLine.function >0){
		res = fpgaPropertiesSetFunction(filter, rasCmdLine.function);
		ON_ERR_GOTO(res, out_destroy_prop, "setting function");
	}

	res = fpgaEnumerate(&filter, 1, &accelerator_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "enumerating accelerators");

	if (num_matches < 1) {
		fprintf(stderr, "accelerator not found.\n");
		res = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}

	/* Open accelerator and map MMIO */
	res = fpgaOpen(accelerator_token, &accelerator_handle, FPGA_OPEN_SHARED);
	ON_ERR_GOTO(res, out_destroy_tok, "opening accelerator");

	res = fpgaMapMMIO(accelerator_handle, 0, NULL);
	ON_ERR_GOTO(res, out_close, "mapping MMIO space");

	/* Allocate buffers */
	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_DSM_SIZE,
				(void **)&dsm_ptr, &dsm_wsid, 0);
	ON_ERR_GOTO(res, out_close, "allocating DSM buffer");

	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&input_ptr, &input_wsid, 0);
	ON_ERR_GOTO(res, out_free_dsm, "allocating input buffer");

	res = fpgaPrepareBuffer(accelerator_handle, LPBK1_BUFFER_ALLOCATION_SIZE,
			   (void **)&output_ptr, &output_wsid, 0);
	ON_ERR_GOTO(res, out_free_input, "allocating output buffer");

	printf("Running Test\n");

	/* Initialize buffers */
	memset((void *)dsm_ptr,    0,    LPBK1_DSM_SIZE);
	memset((void *)input_ptr,  0xAF, LPBK1_BUFFER_SIZE);
	memset((void *)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

	cache_line *cl_ptr = (cache_line *)input_ptr;
	for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
		cl_ptr[i].uint[15] = i+1; /* set the last uint in every cacheline */
	}

	/* Reset accelerator */
	res = fpgaReset(accelerator_handle);
	ON_ERR_GOTO(res, out_free_output, "resetting accelerator");

	/* Program DMA addresses */
	uint64_t iova;
	res = fpgaGetIOAddress(accelerator_handle, dsm_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting DSM IOVA");

	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_AFU_DSM_BASEL, iova);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_AFU_DSM_BASEL");

	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 0);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 1);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	// Free Input buffer
	res = fpgaReleaseBuffer(accelerator_handle, input_wsid);

	res = fpgaGetIOAddress(accelerator_handle, input_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting input IOVA");
	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_SRC_ADDR");

	res = fpgaGetIOAddress(accelerator_handle, output_wsid, &iova);
	ON_ERR_GOTO(res, out_free_output, "getting output IOVA");
	res = fpgaWriteMMIO64(accelerator_handle, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_DST_ADDR");


	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1));
	ON_ERR_GOTO(res, out_free_output, "writing CSR_NUM_LINES");
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CFG, 0x42000);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE/8;

	/* Start the test */
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 3);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	/* Wait for test completion */
	usleep(10000);


	/* Stop the device */
	res = fpgaWriteMMIO32(accelerator_handle, 0, CSR_CTL, 7);
	ON_ERR_GOTO(res, out_free_output, "writing CSR_CFG");

	printf("Done Running Test\n");

	/* Release buffers */
out_free_output:
	res = fpgaReleaseBuffer(accelerator_handle, output_wsid);
	ON_ERR_GOTO(res, out_free_input, "releasing output buffer");
out_free_input:
//	res = fpgaReleaseBuffer(accelerator_handle, input_wsid);
//	ON_ERR_GOTO(res, out_free_dsm, "releasing input buffer");
out_free_dsm:
	res = fpgaReleaseBuffer(accelerator_handle, dsm_wsid);
	ON_ERR_GOTO(res, out_unmap, "releasing DSM buffer");

	/* Unmap MMIO space */
out_unmap:
	res = fpgaUnmapMMIO(accelerator_handle, 0);
	ON_ERR_GOTO(res, out_close, "unmapping MMIO space");

	/* Release accelerator */
out_close:
	res = fpgaClose(accelerator_handle);
	ON_ERR_GOTO(res, out_destroy_tok, "closing accelerator");

	/* Destroy token */
out_destroy_tok:
	res = fpgaDestroyToken(&accelerator_token);
	ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

	/* Destroy properties object */
out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
	return res;

}
// parse Input command line
int ParseCmds(struct RASCommandLine *rasCmdLine, int argc, char *argv[])
{
	int getopt_ret     = 0;
	int option_index   = 0;
	char *endptr       = NULL;

	while( -1 != ( getopt_ret = getopt_long(argc, argv,
						GETOPT_STRING,
						longopts,
						&option_index))){
		const char *tmp_optarg = optarg;

		if ((optarg) &&
				('=' == *tmp_optarg)){
			++tmp_optarg;
		}

		switch(getopt_ret){
		case 'h':
			// Command line help
			RASAppShowHelp();
			return -2;
			break;

		case 'B':
			// bus number
			if (tmp_optarg == NULL ) break;
			endptr = NULL;
			rasCmdLine->bus = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'D':
			// Device number
			if (tmp_optarg == NULL ) break;
			endptr = NULL;
			rasCmdLine->device = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'F':
			// Function number
			if (tmp_optarg == NULL ) break;
			endptr = NULL;
			rasCmdLine->function = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'S':
			// Socket number
			if (tmp_optarg == NULL ) break;
			endptr = NULL;
			rasCmdLine->socket = strtol(tmp_optarg, &endptr, 0);
			break;

		case 'P':
			// Print Errors
			rasCmdLine->print_error = true;
			break;

		case 'Q':
			// Set Cast error
			rasCmdLine->catast_error = true;
			break;

		case 'R':
			// Set Fatal error
			rasCmdLine->fatal_error = true;
			break;

		case 'O':
			// Set page fault error
			rasCmdLine->pagefault_error = true;
			break;

		case 'N':
			// Set Non Fatal error
			rasCmdLine->nonfatal_error = true;
			break;

		case 'C':
			// Clear Injected Error
			rasCmdLine->clear_injerror = true;
			break;

		case 'E':
			// Set MW Address error
			rasCmdLine->mwaddress_error = true;
			break;

		case 'G':
			// Set MR Address error
			rasCmdLine->mraddress_error = true;
			break;

		case 'H':
			// Set MW Length error
			rasCmdLine->mwlength_error = true;
			break;

		case 'I':
			// Set MR Length error
			rasCmdLine->mrlength_error = true;
			break;

		case ':': /* missing option argument */
			printf("Missing option argument.\n");
			return -1;

		case '?':
		default: /* invalid option */
			printf("Invalid cmdline options.\n");
			return -1;
		}
	}
	return 0;
}
