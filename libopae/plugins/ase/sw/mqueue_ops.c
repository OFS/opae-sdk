// Copyright(c) 2014-2017, Intel Corporation
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

/*
 * Named pipe string array
 */
const char *mq_name_arr[] = {
	"app2sim_alloc_ping_smq\0",
	"app2sim_mmioreq_smq\0",
	"app2sim_umsg_smq\0",
	"sim2app_alloc_pong_smq\0",
	"sim2app_mmiorsp_smq\0",
	"app2sim_portctrl_req_smq\0",
	"app2sim_dealloc_ping_smq\0",
	"sim2app_dealloc_pong_smq\0",
	"sim2app_portctrl_rsp_smq\0",
	"sim2app_intr_request_smq\0",
	"sim2app_membus_rd_req_smq\0",
	"app2sim_membus_rd_rsp_smq\0",
	"sim2app_membus_wr_req_smq\0",
	"app2sim_membus_wr_rsp_smq\0"
};


/*
 * get_smq_perm_flag : Calculate perm_flag based on string name
 *      Automates the assignment of MQ flags
 *
 */
int get_smq_perm_flag(const char *mq_name_str)
{
	char *mq_str;
	int ret = -1;

	// Length controlled string copy
	mq_str = ase_malloc(ASE_MQ_NAME_LEN);

	// Length terminated string copy
	ase_string_copy(mq_str, mq_name_str, ASE_MQ_NAME_LEN);

	// Tokenize string and get first phrase --- "app2sim" OR "sim2app"
	char *token;
	char *saveptr = NULL;
	token = strtok_r(mq_str, "_", &saveptr);

	// If name looks weird, throw an error, crash gracefully
	if (token == NULL) {
		ase_free_buffer(mq_str);
#ifdef SIM_SIDE
		start_simkill_countdown();
#else
		exit(1);
#endif
	} else {
		if ((ase_strncmp(token, "sim2app", 7) != 0)
		    && (ase_strncmp(token, "app2sim", 7) != 0)) {
			ASE_ERR
			    ("  ** ERROR **: Named pipe name is neither app2sim nor sim2app!\n");
			ase_free_buffer(mq_str);
#ifdef SIM_SIDE
			start_simkill_countdown();
#else
			exit(1);
#endif
		} else {
#ifdef SIM_SIDE
			if (ase_strncmp(token, "sim2app", 7) == 0) {
				ret = O_WRONLY;
			} else if (ase_strncmp(token, "app2sim", 7) == 0) {
				ret = O_RDONLY | O_NONBLOCK;
			}
#else
			if (ase_strncmp(token, "sim2app", 7) == 0) {
				ret = O_RDONLY;
			} else if (ase_strncmp(token, "app2sim", 7) == 0) {
				ret = O_WRONLY;
			}
#endif

			// Free memory
			ase_free_buffer(mq_str);
		}
	}

	return ret;
}


/*
 * ipc_init: Initialize IPC messaging structure
 *           DOES not create or open the IPC, simply initializes the structures
 */
void ipc_init(void)
{
	FUNC_CALL_ENTRY;

	int ipc_iter;

	// Initialize named pipe array
	for (ipc_iter = 0; ipc_iter < ASE_MQ_INSTANCES; ipc_iter++) {
		// Set name
		ase_string_copy(mq_array[ipc_iter].name,
				mq_name_arr[ipc_iter], ASE_MQ_NAME_LEN);

		// Compute path
		snprintf(mq_array[ipc_iter].path, ASE_FILEPATH_LEN,
			 "%s/%s", ase_workdir_path,
			 mq_array[ipc_iter].name);
		// Set permission flag
		mq_array[ipc_iter].perm_flag =
		    get_smq_perm_flag(mq_name_arr[ipc_iter]);
		if (mq_array[ipc_iter].perm_flag == -1) {
			ASE_ERR
			    ("Message pipes opened up with wrong permissions --- unexpected error");
#ifdef SIM_SIDE
			start_simkill_countdown();
#else
			exit(1);
#endif
		}
	}

	// Remove IPCs if already there
#ifdef SIM_SIDE
	for (ipc_iter = 0; ipc_iter < ASE_MQ_INSTANCES; ipc_iter++)
		unlink(mq_array[ipc_iter].path);
#endif

	FUNC_CALL_EXIT;
}


/*
 * mqueue_create: Create a simplex mesaage queue by passing a name
 */
void mqueue_create(char *mq_name_suffix)
{
	FUNC_CALL_ENTRY;

	char *mq_path;
	int ret;

	mq_path = ase_malloc(ASE_FILEPATH_LEN);
	snprintf(mq_path, ASE_FILEPATH_LEN, "%s/%s", ase_workdir_path,
		 mq_name_suffix);

	ret = mkfifo(mq_path, S_IRUSR | S_IWUSR);
	if (ret == -1) {
		ASE_ERR("Error creating IPC %s\n", mq_path);
		ASE_ERR("Consider re-compiling OPAE libraries !\n");
	}
	// Add IPC to list
#ifdef SIM_SIDE
	add_to_ipc_list("MQ", mq_path);
	fflush(local_ipc_fp);
#endif

	// Free memories
	free(mq_path);
	mq_path = NULL;

	FUNC_CALL_EXIT;
}


/*
 * mqueue_open : Open IPC messaging device
 *
 * NOTES:
 * - Named pipes require reader to be ready for non-blocking open to
 *   proceed. This may not be possible in an ASE environment.
 * - This may be solved by having a dummy reader the WRONLY fifo, then
 *   close it after ASE's real fd is created successfully.
 *
 */
int mqueue_open(char *mq_name, int perm_flag)
{
	FUNC_CALL_ENTRY;

	int mq;
	char *mq_path;

	mq_path = ase_malloc(ASE_FILEPATH_LEN);
	snprintf(mq_path, ASE_FILEPATH_LEN, "%s/%s", ase_workdir_path,
		 mq_name);

	// Dummy function to open WRITE only MQs
	// Named pipe requires non-blocking write-only move on from here
	// only when reader is ready.
#ifdef SIM_SIDE
	int dummy_fd = -1;
	if (perm_flag == O_WRONLY) {
		ASE_DBG("Opening IPC in write-only mode with dummy fd\n");
		dummy_fd = open(mq_path, O_RDONLY | O_NONBLOCK);
	}
#endif

	mq = open(mq_path, perm_flag);
	if (mq == -1) {
		ASE_ERR("Error opening IPC %s\n", mq_path);
#ifdef SIM_SIDE
		ase_error_report("open", errno, ASE_OS_FOPEN_ERR);
		start_simkill_countdown();
#else
		perror("open");
		exit(1);
#endif
	}
#ifdef SIM_SIDE
	if (dummy_fd != -1) {
		close(dummy_fd);
	}
#endif

	FUNC_CALL_EXIT;

	// Free temp variables
	free(mq_path);
	mq_path = NULL;

	return mq;
}


// -------------------------------------------------------
// mqueue_close(int): close MQ by descriptor
// -------------------------------------------------------
void mqueue_close(int mq)
{
	FUNC_CALL_ENTRY;

	int ret;
	ret = close(mq);
	if (ret == -1) {
#ifdef SIM_SIDE
		ASE_DBG("Error closing IPC\n");
#endif
	}

	FUNC_CALL_EXIT;
}


// -----------------------------------------------------------
// mqueue_destroy(): Unlink message queue, must be called from API
// -----------------------------------------------------------
void mqueue_destroy(char *mq_name_suffix)
{
	FUNC_CALL_ENTRY;

	char *mq_path;
	int ret;

	// ASE malloc will allocate buffer, mq_path will be set correctly
	mq_path = ase_malloc(ASE_FILEPATH_LEN);

	// Generate mq_path variable from given and $ASE_WORKDIR
	snprintf(mq_path, ASE_FILEPATH_LEN, "%s/%s", ase_workdir_path,
		 mq_name_suffix);

	// Attempt to unlink the pipe
	ret = unlink(mq_path);
	if (ret == -1) {
		ASE_ERR
		    ("Message queue %s could not be removed, please remove manually\n",
		     mq_name_suffix);
	}
	// Free memory
	free(mq_path);
	mq_path = NULL;

	FUNC_CALL_EXIT;
}


// ------------------------------------------------------------
// mqueue_send(): Easy send function
// - Typecast any message as a character array and ram it in.
// ------------------------------------------------------------
void mqueue_send(int mq, const char *str, int size)
{
	FUNC_CALL_ENTRY;

	int ret_tx;
	ret_tx = write(mq, (void *) str, size);

	if ((ret_tx == 0) || (ret_tx != size)) {
		perror("write");
#ifdef ASE_DEBUG
		ASE_DBG("write() returned wrong data size.");
#endif
	}

	FUNC_CALL_EXIT;
}


// ------------------------------------------------------------------
// mqueue_recv(): Easy receive function
// - Typecast message back to a required type
// ------------------------------------------------------------------
int mqueue_recv(int mq, char *str, int size)
{
	FUNC_CALL_ENTRY;

	int ret;

	ret = read(mq, str, size);
	FUNC_CALL_EXIT;
	if (ret > 0) {
		return ASE_MSG_PRESENT;
	} else {
		return ((ret == 0) || (errno == EAGAIN)) ? ASE_MSG_ABSENT : ASE_MSG_ERROR;
	}
}
