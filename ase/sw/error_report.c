// Copyright(c) 2014-2018, Intel Corporation
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
// **************************************************************************

#include "ase_common.h"

// -----------------------------------------------------------------------
// ASE error report : Prints a verbose report on catastrophic errors
// -----------------------------------------------------------------------
void ase_error_report(const char *err_func, int err_num, int err_code)
{
	// Report error
	ASE_ERR("@ERROR in %s CODE %d | %s\n", err_func, err_num, strerror(err_num));

	// Corrective actions
	switch (err_code) {
	case ASE_USR_CAPCM_NOINIT:     // CAPCM not initialized
		ASE_ERR
			("QPI-CA private memory has not been initialized.\n");
		break;

	case ASE_OS_MQUEUE_ERR:        // Message queue error
		ASE_ERR
			("There was an error in the POSIX Message Queue subsystem. "
			 "Please look up 'man mq_overview' for more information.\n");
		break;

	case ASE_OS_SHM_ERR:          // Message queue error
		ASE_ERR
			("There was an error in the POSIX Shared Memory subsystem.\n");
		break;

	case ASE_OS_FOPEN_ERR:       // File open error
		ASE_ERR
			("File opening failed. This could be due to several reasons: \n"
				"1. ASE is being run from the wrong relative paths, and causing fstat to fail.\n"
				"2. File system permissions are not optimal\n");
		break;

	case ASE_OS_MEMMAP_ERR:     // Memory map/unmap failed
		ASE_ERR
			("A problem occured when mapping or unmapping a memory region to a virtual base pointer.\n");
		break;

	case ASE_OS_MQTXRX_ERR: 	// MQ send/receive error
		ASE_ERR
			("There was a problem when sending/receiving messages using the POSIX message queues.\n"
			 "This may be due to sub-optimal message queue attributes.\n");
		break;

	case ASE_OS_MALLOC_ERR:		// Malloc error
		ASE_ERR
			("There was a problem with memory allocation system, NULL was returned.\n"
			 "Simulator will attempt to close down.\n");
		break;

	case ASE_IPCKILL_CATERR:	// IPCkill catastrophic error
		ASE_ERR
			("There was an ERROR when trying to open IPC local listfile for cleaning.\n"
			 "fopen failed, see .ase_ipc_local file, and clean them manually.\n");
		break;

	default:					// Default or unknown error
		ASE_ERR
			("ERROR code is not defined, or cause is unknown.\n"
			 "If your agreement allows this, please report detailed steps to recreate the error to the developer.\n");
	}

	END_RED_FONTCOLOR;
}


/*
 * Wrapper for backtrace handler
 * Useful for debugging broken symbols
 */
extern const char *__progname;

void backtrace_handler(int sig)
{
	void *bt_addr[16];
	char **bt_messages = (char **) NULL;
	int ii, trace_depth = 0;
	char sys_cmd[256];
	char app_or_sim[16];
	int cmd_ret;

	ase_memset(app_or_sim, 0, sizeof(app_or_sim));

#ifdef SIM_SIDE
	snprintf(app_or_sim, 16, "Simulator ");
#else
	snprintf(app_or_sim, 16, "Application ");
#endif

	// Identify SIG received
	ASE_ERR("%s received a ", app_or_sim);
	switch (sig) {
	case SIGSEGV:
		ASE_ERR("SIGSEGV\n");
		break;

	case SIGBUS:
		ASE_ERR("SIGBUS\n");
		break;

	case SIGABRT:
		ASE_ERR("SIGABRT\n");
		break;

	default:
		ASE_ERR("Unidentified signal %d\n", sig);
		break;
	}

	trace_depth = backtrace(bt_addr, 16);
	bt_messages = backtrace_symbols(bt_addr, trace_depth);
	ASE_ERR("\n[bt] Execution Backtrace:\n");
	for (ii = 1; ii < trace_depth; ii++) {
		ASE_ERR("[bt] #%d %s\n", ii, bt_messages[ii]);
		snprintf(sys_cmd, 256, "addr2line %p -e %s", bt_addr[ii],
			 __progname);
		cmd_ret = system(sys_cmd);
		// man page for system asks users to check for SIGINT/SIGQUIT
		if (WIFSIGNALED(cmd_ret)
		    && ((WTERMSIG(cmd_ret) == SIGINT)
			|| (WTERMSIG(cmd_ret) == SIGQUIT))) {
			break;
		}
	}

	ase_exit();
}
