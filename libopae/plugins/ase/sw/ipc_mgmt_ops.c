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

// IPC control list
static char ipclist_filepath[ASE_FILEPATH_LEN];

// Local IPC log
FILE *local_ipc_fp;


// -----------------------------------------------------------------------
// create_ipc_listfile : Creates a log of IPC structures opened by
//                       this ASE session. This will be looked up so a
//                       civilized cleaning can be done
// -----------------------------------------------------------------------
void create_ipc_listfile(void)
{
	FUNC_CALL_ENTRY;

	// Allocate memory, fail if not possible
	ase_memset(ipclist_filepath, 0, ASE_FILEPATH_LEN);

	// Create IPC file path length
	snprintf(ipclist_filepath, ASE_FILEPATH_LEN, "%s/%s",
		 ase_workdir_path, IPC_LOCAL_FILENAME);

	// Open IPC log in write mode
	local_ipc_fp = fopen(ipclist_filepath, "w");
	if (local_ipc_fp == NULL) {
		// Error out gracefully on problem
		ase_error_report("fopen", errno, ASE_OS_FOPEN_ERR);
		ASE_ERR("Local IPC file cannot be opened\n");
		start_simkill_countdown();	// RRS: exit(1);
	}
#ifdef ASE_DEBUG
	else {
		ASE_DBG("IPC Watchdog file %s opened\n",
			IPC_LOCAL_FILENAME);
	}
#endif

	FUNC_CALL_EXIT;
}


// -----------------------------------------------------------------------
// add_to_ipc_list : Add record to IPC list
// -----------------------------------------------------------------------
void add_to_ipc_list(char *ipc_type, char *ipc_name)
{
	FUNC_CALL_ENTRY;
	int ret;

	// Add name to local IPC list
	ret = fprintf(local_ipc_fp, "%s\t%s\n", ipc_type, ipc_name);

	if (ret < 0) {
		ASE_ERR
			("add_to_ipc_list: Failed to update IPC management file, cleanup may be incomplete\n");
		ASE_ERR("                 Simulation will continue\n");
	}

	FUNC_CALL_EXIT;
}

// -----------------------------------------------------------------------
// final_ipc_cleanup : A second level check to see all the blocks are
//                     removed
// -----------------------------------------------------------------------
void final_ipc_cleanup(void)
{
	FUNC_CALL_ENTRY;

	char *ipc_type;
	char *ipc_name;

	char *ipc_line;
	size_t ipc_line_len = 0;
	char *saveptr;

	ipc_line = ase_malloc(ASE_FILEPATH_LEN + 16);

	// Close global/local files
	fclose(local_ipc_fp);

	// Reopen local IPC listfile
	local_ipc_fp = fopen(ipclist_filepath, "r");
	if (local_ipc_fp == NULL) {
		ase_error_report("fopen", errno, ASE_IPCKILL_CATERR);
		start_simkill_countdown();	// RRS: exit(1);
	} else {
		// Parse through list
		ASE_MSG
			("Removing message queues and buffer handles ... \n");
		while (getline(&ipc_line, &ipc_line_len, local_ipc_fp) !=
		       -1) {
			ipc_type = strtok_r(ipc_line, " \t", &saveptr);
			ipc_name = strtok_r(NULL, " \t", &saveptr);
			if ((ipc_type == NULL) || (ipc_name == NULL)) {
				ASE_DBG("Ignoring ipc_line_len\n");
			} else {
				// Compare type of ipc_type
				if (ase_strncmp(ipc_type, "MQ", 2) == 0) {
					ASE_DBG("        Removing MQ  %s ",
						ipc_name);
					if (unlink(ipc_name) == -1) {
						ASE_DBG("\n");
					} else {
						ASE_DBG("DONE\n");
					}
				} else if (ase_strncmp(ipc_type, "SHM", 3)
					   == 0) {
					ASE_DBG("        Removing SHM %s ",
						ipc_name);
					if (shm_unlink(ipc_name) == -1) {
						ASE_DBG("\n");
					} else {
						ASE_DBG("DONE\n");
					}
				}
			}
		}

		// Close both files
		fclose(local_ipc_fp);
	}

	ase_free_buffer(ipc_line);

	FUNC_CALL_EXIT;
}
