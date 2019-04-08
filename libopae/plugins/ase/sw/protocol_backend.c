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
/*
 * Module Info:
 * - Protocol backend for keeping IPCs alive
 * - Interfacing with DPI-C, messaging
 * - Interface to page table
 */
#include "ase_common.h"
#include "ase_host_memory.h"

//const int NUM_DS = 10;

struct ase_cfg_t *cfg;

static int app2sim_alloc_rx;		// app2sim mesaage queue in RX mode
int sim2app_alloc_tx;		        // sim2app mesaage queue in TX mode
static int app2sim_mmioreq_rx;		// MMIO Request path
static int sim2app_mmiorsp_tx;		// MMIO Response path
static int app2sim_umsg_rx;		// UMSG    message queue in RX mode
static int app2sim_portctrl_req_rx;	// Port Control messages in Rx mode
static int app2sim_dealloc_rx;
int sim2app_dealloc_tx;
static int sim2app_portctrl_rsp_tx;
static int sim2app_intr_request_tx;
static int sim2app_membus_rd_req_tx;
static int app2sim_membus_rd_rsp_rx;
static int sim2app_membus_wr_req_tx;
static int app2sim_membus_wr_rsp_rx;
static int intr_event_fds[MAX_USR_INTRS];

int glbl_test_cmplt_cnt;                // Keeps the number of session_deinits received

volatile int sockserver_kill;
static pthread_t socket_srv_tid;

// MMIO Respons lock
// pthread_mutex_t mmio_resp_lock;

// Variable declarations
char tstamp_filepath[ASE_FILEPATH_LEN];
char ccip_sniffer_file_statpath[ASE_FILEPATH_LEN];

// CONFIG,SCRIPT parameter paths received from SV (initial)
char sv2c_config_filepath[ASE_FILEPATH_LEN];
char sv2c_script_filepath[ASE_FILEPATH_LEN];

// ASE PID
int ase_pid;

// Workspace information log (information dump of
static FILE *fp_workspace_log;

// Memory access debug log
#ifdef ASE_DEBUG
FILE *fp_memaccess_log;
FILE *fp_pagetable_log;
#endif

uint64_t PHYS_ADDR_PREFIX_MASK;
int self_destruct_in_progress;

// work Directory location
char *ase_workdir_path;

// Incoming UMSG packet (allocated in ase_init, deallocated in start_simkill_countdown)
static struct umsgcmd_t *incoming_umsg_pkt;

// Incoming MMIO packet (allocated in ase_init, deallocated in start_simkill_countdown)
static struct mmio_t *incoming_mmio_pkt;

/*
 * ASE capability register
 * Purpose: This is the response for portctrl_cmd requests (as an ACK)
 */
struct ase_capability_t ase_capability = {
	ASE_UNIQUE_ID,
	/* UMsg feature interrupt */
#ifdef ASE_ENABLE_UMSG_FEATURE
	1,
#else
	0,
#endif
	/* Interrupt feature interrupt */
#ifdef ASE_ENABLE_INTR_FEATURE
	1,
#else
	0,
#endif
	/* 512-bit MMIO support */
#ifdef ASE_ENABLE_MMIO512
	1
#else
	0
#endif
};

const char *completed_str_msg = (char *)&ase_capability;

/*
 * Generate scope data
 */
svScope scope;
void scope_function(void)
{
	scope = svGetScope();
}


/*
 * ASE instance already running
 * - If instance is found, return its process ID, else return 0
 */
int ase_instance_running(void)
{
	FUNC_CALL_ENTRY;

	int ase_simv_pid;

	// If Ready file does not exist
	if (access(ASE_READY_FILENAME, F_OK) == -1) {
		ase_simv_pid = 0;
	}
	// If ready file exists
	else {
		char *pwd_str;
		pwd_str = ase_malloc(ASE_FILEPATH_LEN);
		ase_simv_pid =
			ase_read_lock_file(getcwd(pwd_str, ASE_FILEPATH_LEN));
		free(pwd_str);
	}

	FUNC_CALL_EXIT;
	return ase_simv_pid;
}


/*
 * DPI: CONFIG path data exchange
 */
void sv2c_config_dex(const char *str)
{
	// Allocate memory
	ase_memset(sv2c_config_filepath, 0, ASE_FILEPATH_LEN);

	// Check that input string is not NULL
	if (str == NULL) {
		ASE_MSG("sv2c_config_dex => Input string is null\n");
	} else {
		// Attempt string copy and keep safe
		ase_string_copy(sv2c_config_filepath, str,
				ASE_FILEPATH_LEN);
#ifdef ASE_DEBUG
		ASE_DBG("sv2c_config_filepath = %s\n",
			sv2c_config_filepath);
#endif

		// Check if file exists
		if (access(sv2c_config_filepath, F_OK) == 0) {
			ASE_MSG("+CONFIG %s file found !\n",
				sv2c_config_filepath);
		} else {
			ASE_ERR
				("** WARNING ** +CONFIG file was not found, will revert to DEFAULTS\n");
			ase_memset(sv2c_config_filepath, 0,
				      ASE_FILEPATH_LEN);
		}
	}
}


/*
 * DPI: SCRIPT path data exchange
 */
void sv2c_script_dex(const char *str)
{
	if (str == NULL) {
		ASE_MSG("sv2c_script_dex => Input string is null\n");
	} else {
		ase_memset(sv2c_script_filepath, 0, ASE_FILEPATH_LEN);
		ase_string_copy(sv2c_script_filepath, str,
				ASE_FILEPATH_LEN);
#ifdef ASE_DEBUG
		ASE_DBG("sv2c_script_filepath = %s\n",
			sv2c_script_filepath);
#endif

		// Check for existance of file
		if (access(sv2c_script_filepath, F_OK) == 0) {
			ASE_MSG("+SCRIPT %s file found !\n",
				sv2c_script_filepath);
		} else {
			ASE_MSG
				("** WARNING ** +SCRIPT file was not found, will revert to DEFAULTS\n");
			ase_memset(sv2c_script_filepath, 0,
				      ASE_FILEPATH_LEN);
		}
  }
}


/*
 * Print an error for accesses to illegal (unpinned) addresses.
 */
static void memline_addr_error(const char *access_type,
			       ase_host_memory_status status,
			       uint64_t pa, uint64_t va)
{
	FUNC_CALL_ENTRY;

	#define MEMLINE_ADDR_ILLEGAL_MSG \
		"@ERROR: ASE has detected a memory %s to an ILLEGAL memory address.\n" \
		"        Simulation cannot continue, please check the code.\n" \
		"          Failure @ byte-level phys_addr = 0x%" PRIx64 "\n" \
		"                    line-level phys_addr = 0x%" PRIx64 "\n" \
		"        See ERROR log file ase_memory_error.log and timestamped\n" \
		"        transactions in ccip_transactions.tsv.\n"

	#define MEMLINE_ADDR_UNPINNED_MSG \
		"@ERROR: ASE has detected a memory %s to an UNPINNED memory region.\n" \
		"        Simulation cannot continue, please check the code.\n" \
		"          Failure @ byte-level phys_addr = 0x%" PRIx64 "\n" \
		"                    line-level phys_addr = 0x%" PRIx64 "\n" \
		"        See ERROR log file ase_memory_error.log and timestamped\n" \
		"        transactions in ccip_transactions.tsv.\n" \
		"@ERROR: Check that previously requested memories have not been deallocated\n" \
		"        before an AFU transaction could access them.\n" \
		"        NOTE: If your application polls for an AFU completion message,\n" \
		"              and you deallocate after that, consider using a\n" \
		"              write fence before AFU status message. The simulator may\n" \
		"              be committing AFU transactions out of order.\n"

	#define MEMLINE_ADDR_UNMAPPED_MSG \
		"@ERROR: ASE has detected a memory %s to an UNMAPPED memory region.\n" \
		"        Simulation cannot continue, please check the code.\n" \
		"          Failure @ byte-level phys_addr = 0x%" PRIx64 "\n" \
		"                    line-level phys_addr = 0x%" PRIx64 "\n" \
		"                    line-level virtual_addr = 0x%" PRIx64 "\n" \
		"        See ERROR log file ase_memory_error.log and timestamped\n" \
		"        transactions in ccip_transactions.tsv.\n" \
		"@ERROR: This most often happens when the application munmaps or frees\n" \
		"        a region before calling fpgaReleaseBuffer().\n"

	FILE *error_fp = fopen("ase_memory_error.log", "w");

	if (status == HOST_MEM_STATUS_ILLEGAL) {
		ASE_ERR("\n" MEMLINE_ADDR_ILLEGAL_MSG, access_type, pa, pa >> 6);
		if (error_fp != NULL) {
			fprintf(error_fp, MEMLINE_ADDR_ILLEGAL_MSG, access_type, pa, pa >> 6);
		}
	} else if (status == HOST_MEM_STATUS_NOT_PINNED) {
		ASE_ERR("\n" MEMLINE_ADDR_UNPINNED_MSG, access_type, pa, pa >> 6);
		if (error_fp != NULL) {
			fprintf(error_fp, MEMLINE_ADDR_UNPINNED_MSG, access_type, pa, pa >> 6);
		}
	} else if (status == HOST_MEM_STATUS_NOT_MAPPED) {
		ASE_ERR("\n" MEMLINE_ADDR_UNMAPPED_MSG, access_type, pa, pa >> 6, va);
		if (error_fp != NULL) {
			fprintf(error_fp, MEMLINE_ADDR_UNMAPPED_MSG, access_type, pa, pa >> 6, va);
		}
	} else {
		ASE_ERR("\n @ERROR: Unknown memory reference error.\n");
	}

	if (error_fp != NULL) {
		fclose(error_fp);
	}

	#undef MEMLINE_ADDR_ILLEGAL_MSG
	#undef MEMLINE_ADDR_UNPINNED_MSG
	#undef MEMLINE_ADDR_UNMAPPED_MSG

	// Request SIMKILL
	start_simkill_countdown();

	FUNC_CALL_EXIT;
}


/*
 * DPI: Write line request (sent to application)
 */
void wr_memline_req_dex(cci_pkt *pkt)
{
	FUNC_CALL_ENTRY;

	uint64_t phys_addr;
	int intr_id;

	ase_host_memory_write_req wr_req;

	if (pkt->mode == CCIPKT_WRITE_MODE) {
		/*
		 * Normal write operation
		 * Takes Write request and performs verbatim
		 */
		phys_addr = (uint64_t) pkt->cl_addr << 6;

		// Write to memory
		wr_req.req = HOST_MEM_REQ_WRITE_LINE;
		wr_req.addr = phys_addr;
		ase_memcpy(wr_req.data, (char *) pkt->qword, CL_BYTE_WIDTH);

		mqueue_send(sim2app_membus_wr_req_tx, (char *) &wr_req, sizeof(wr_req));

		// Success
		pkt->success = 1;
	} else if (pkt->mode == CCIPKT_INTR_MODE) {
		/*
		 * Interrupt operation
		 */
		// Trigger interrupt action
		intr_id = pkt->intr_id;
		ase_interrupt_generator(intr_id);

		// Success
		pkt->success = 1;
	}

	FUNC_CALL_EXIT;
}


/*
 * DPI: Write line response (from application)
 */
void wr_memline_rsp_dex(cci_pkt *pkt)
{
	FUNC_CALL_ENTRY;

	ase_host_memory_write_rsp wr_rsp;
	int status;

	// The only task required here is to consume the response from the application.
	// triggered by wr_memline_req_dex. The response indicates whether the address
	// was valid.  Raise an error for invalid addresses.

	if (pkt->mode == CCIPKT_WRITE_MODE) {
		while (true) {
			status = mqueue_recv(app2sim_membus_wr_rsp_rx, (char *) &wr_rsp, sizeof(wr_rsp));

			if (status == ASE_MSG_PRESENT) {
				if (wr_rsp.status != HOST_MEM_STATUS_VALID) {
					memline_addr_error("WRITE", wr_rsp.status, wr_rsp.pa, wr_rsp.va);
					pkt->success = 0;
				}
				break;
			}

			// Error?  Probably channel closed and the simulator will be closing
			// soon.
			if (status == ASE_MSG_ERROR) {
				pkt->success = 0;
				break;
			}
		}
	}

	FUNC_CALL_EXIT;
}


/*
 * DPI: Read line request
 */
void rd_memline_req_dex(cci_pkt *pkt)
{
	FUNC_CALL_ENTRY;

	uint64_t phys_addr;
	ase_host_memory_read_req rd_req;

	phys_addr = (uint64_t) pkt->cl_addr << 6;

	rd_req.req = HOST_MEM_REQ_READ_LINE;
	rd_req.addr = phys_addr;
	mqueue_send(sim2app_membus_rd_req_tx, (char *) &rd_req, sizeof(rd_req));

	FUNC_CALL_EXIT;
}


/*
 * DPI: Read line data response
 */
void rd_memline_rsp_dex(cci_pkt *pkt)
{
	FUNC_CALL_ENTRY;

	ase_host_memory_read_rsp rd_rsp;
	int status;

	while (true) {
		status = mqueue_recv(app2sim_membus_rd_rsp_rx, (char *) &rd_rsp, sizeof(rd_rsp));

		if (status == ASE_MSG_PRESENT) {
			if (rd_rsp.status != HOST_MEM_STATUS_VALID) {
				memline_addr_error("READ", rd_rsp.status, rd_rsp.pa, rd_rsp.va);
			}

			// Read from memory
			ase_memcpy((char *) pkt->qword, rd_rsp.data, CL_BYTE_WIDTH);
			break;
		}

		// Error?  Probably channel closed and the simulator will be closing
		// soon.
		if (status == ASE_MSG_ERROR) {
			pkt->success = 0;
			break;
		}
	}

	FUNC_CALL_EXIT;
}


/*
 * DPI: MMIO response
 */
void mmio_response(struct mmio_t *mmio_pkt)
{
	FUNC_CALL_ENTRY;

	// Lock channel
	// pthread_mutex_lock (&mmio_resp_lock);

#ifdef ASE_DEBUG
	print_mmiopkt(fp_memaccess_log, "MMIO Got ", mmio_pkt);
#endif

	// Send MMIO Response
	mqueue_send(sim2app_mmiorsp_tx, (char *) mmio_pkt, sizeof(mmio_t));

	// Unlock channel
	// pthread_mutex_unlock (&mmio_resp_lock);

	FUNC_CALL_EXIT;
}


/*
 * ASE Interrupt generator handle
 */
void ase_interrupt_generator(int id)
{
	int cnt;

	if (id >= MAX_USR_INTRS) {
		ASE_ERR("SIM-C : Interrupt #%d > avail. interrupts (%d)!\n",
			id, MAX_USR_INTRS);
		return;
	}

	if (intr_event_fds[id] < 0) {
		ASE_ERR("SIM-C : No valid event for AFU interrupt %d!\n", id);
	} else {
		uint64_t val = 1;

		cnt = write(intr_event_fds[id], &val, sizeof(uint64_t));
		if (cnt < 0) {
			ASE_ERR("SIM-C : Error writing fd %d errno = %s\n",
				intr_event_fds[id], strerror(errno));
		} else {
			ASE_MSG("SIM-C : AFU Interrupt event %d\n", id);
		}
	}
}


/*
 * DPI: Reset response
 */
void sw_reset_response(void)
{
	FUNC_CALL_ENTRY;

	// Send portctrl_rsp message
	mqueue_send(sim2app_portctrl_rsp_tx, completed_str_msg,
		    ASE_MQ_MSGSIZE);

	FUNC_CALL_EXIT;
}


/*
 * Count error flag ping/pong
 */
volatile int count_error_flag;
void count_error_flag_pong(int flag)
{
	count_error_flag = flag;
}


/*
 * Update global disable/enable
 */
int glbl_dealloc_allowed;
void update_glbl_dealloc(int flag)
{
	glbl_dealloc_allowed = flag;
}


/*
 * Populating required DFH in BBS
 */
// UMSG CSRs
uint64_t *csr_umsg_base_address;

/*
 * Initialize: Populate FME DFH block
 * When initialized, this is called
 * update*function is called when UMSG is to be set up
 */
void initialize_fme_dfh(struct buffer_t *buf)
{
	FUNC_CALL_ENTRY;

	// Capability CSRs
	uint64_t *csr_port_capability;
	uint64_t *csr_port_umsg;

	uint64_t *csr_umsg_capability;
	uint64_t *csr_umsg_mode;

	uint8_t *port_vbase = (uint8_t *) (uintptr_t) buf->pbase;

	/*
	 * PORT CSRs
	 */
	// PORT_CAPABILITY
	csr_port_capability = (uint64_t *) (port_vbase + 0x0030);
	*csr_port_capability = (0x100 << 23) + (0x0 << 0);

	// PORT_UMSG DFH
	csr_port_umsg = (uint64_t *) (port_vbase + 0x2000);
	*csr_port_umsg =
		((uint64_t) 0x3 << 60) | ((uint64_t) 0x1000 << 39) | (0x11 <<
								      0);

	/*
	 * UMSG settings
	 */
	// UMSG_CAPABILITY
	csr_umsg_capability = (uint64_t *) (port_vbase + 0x2008);
	*csr_umsg_capability = (0x0 << 9) + (0x0 << 8) + (0x8 << 0);

	// UMSG_BASE_ADDRESS (only initalize address, update function will update CSR)
	csr_umsg_base_address = (uint64_t *) (port_vbase + 0x2010);

	// UMSG_MODE
	csr_umsg_mode = (uint64_t *) (port_vbase + 0x2018);
	*csr_umsg_mode = 0x0;

	FUNC_CALL_EXIT;
}


// Update FME DFH after UMAS becomes known
void update_fme_dfh(struct buffer_t *umas)
{
	// Write UMAS address
	*csr_umsg_base_address = (uint64_t) umas->pbase;
}

int read_fd(int sock_fd)
{
	struct msghdr msg = {0};
	char buf[CMSG_SPACE(sizeof(int))];
	struct event_request req = { .type = 0, .flags = 0 };
	struct iovec io = { .iov_base = &req, .iov_len = sizeof(req) };
	struct cmsghdr *cmsg;
	int *fdptr;

	ase_memset(buf, '\0', sizeof(buf));
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	cmsg = (struct cmsghdr *)buf;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	msg.msg_flags = 0;

	if (recvmsg(sock_fd, &msg, 0) < 0) {
		ASE_ERR("SIM-C : Unable to rcvmsg from socket\n");
		return 1;
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		ASE_ERR("SIM-C : Null pointer from rcvmsg socket\n");
		return 1;
	}

	int vector_id = 0;

	fdptr = (int *)CMSG_DATA(cmsg);

	if (req.type == REGISTER_EVENT) {
		vector_id = req.flags;
		intr_event_fds[vector_id] = *fdptr;
	}
	if (req.type == UNREGISTER_EVENT) {
		int i;
		// locate the interrupt vector to unregister
		// from the event handle
		for (i = 0; i < MAX_USR_INTRS; i++) {
			if (intr_event_fds[vector_id] == *fdptr)
				intr_event_fds[vector_id] = -1;
		}
	}
	return 0;
}

static void *start_socket_srv(void *args)
{
	int res = 0;
	int err_cnt = 0;
	int sock_msg = 0;
	int sock_fd;
	struct sockaddr_un saddr;
	socklen_t addrlen;
	struct timeval tv;
	fd_set readfds;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		ASE_ERR("SIM-C : Error opening event socket: %s",
			strerror(errno));
		err_cnt++;
		return args;
	}

	// set socket type to non blocking
	fcntl(sock_fd, F_SETFL, O_NONBLOCK);
	fcntl(sock_fd, F_SETFL, O_ASYNC);
	saddr.sun_family = AF_UNIX;

	res = generate_sockname(saddr.sun_path);
	if (res < 0) {
		ASE_ERR("%s: Error strncpy_s\n", __func__);
		err_cnt++;
		goto err;
	}

	// unlink previous addresses in use (if any)
	unlink(saddr.sun_path);
	addrlen = sizeof(struct sockaddr_un);
	if (bind(sock_fd, (struct sockaddr *)&saddr, addrlen) < 0) {
		ASE_ERR("SIM-C : Error binding event socket: %s\n",
			strerror(errno));
		err_cnt++;
		goto err;
	}

	ASE_MSG("SIM-C : Creating Socket Server@%s...\n", saddr.sun_path);
	if (listen(sock_fd, 5) < 0) {
		ASE_ERR("SIM-C : Socket server listen failed with error:%s\n",
			strerror(errno));
		err_cnt++;
		goto err;
	}

	ASE_MSG("SIM-C : Started listening on server %s\n", saddr.sun_path);
	tv.tv_usec = 0;
	FD_ZERO(&readfds);

	do {
		FD_SET(sock_fd, &readfds);
		res = TEMP_FAILURE_RETRY(select(sock_fd+1, &readfds, NULL, NULL, &tv));
		if (res < 0) {
			ASE_ERR("SIM-C : select error=%s\n", strerror(errno));
			err_cnt++;
			break;
		}
		if (FD_ISSET(sock_fd, &readfds)) {
			sock_msg = accept(sock_fd, (struct sockaddr *)&saddr,
					  &addrlen);
			if (sock_msg == -1) {
				ASE_ERR("SIM-C : accept error=%s\n",
					strerror(errno));
				err_cnt++;
				break;
			}
			if (read_fd(sock_msg) != 0) {
				err_cnt++;
				break;
			}
#ifdef ASE_DEBUG
			ASE_MSG("SIM-C : accept success\n");
#endif
		}
		if (sockserver_kill)
			break;
	} while (res >= 0);

	ASE_MSG("SIM-C : Exiting event socket server@%s...\n", saddr.sun_path);

err:
	close(sock_msg);
	close(sock_fd);
	unlink(saddr.sun_path);
	sockserver_kill = 0;
	return args;
}


/* ********************************************************************
 * ASE Listener thread
 * --------------------------------------------------------------------
 * vbase/pbase exchange THREAD
 * when an allocate request is received, the buffer is copied into a
 * linked list. The reply consists of the pbase, fakeaddr and fd_ase.
 * When a deallocate message is received, the buffer is invalidated.
 *
 * MMIO Request
 * Calls MMIO Dispatch task in ccip_emulator
 *
 * *******************************************************************/
int ase_listener(void)
{
	// Buffer management variables
	static struct buffer_t ase_buffer;
	static char incoming_alloc_msgstr[ASE_MQ_MSGSIZE];
	static char incoming_dealloc_msgstr[ASE_MQ_MSGSIZE];
	int  rx_portctrl_cmd;
	int  portctrl_value;

	// Portctrl variables
	static char portctrl_msgstr[ASE_MQ_MSGSIZE];
	static char logger_str[ASE_LOGGER_LEN];
	static char umsg_mapstr[ASE_MQ_MSGSIZE];

	// Session status
	static int   session_empty;
	static char *glbl_session_id;

	//umsg, lookup before issuing UMSG
	static int   glbl_umsgmode;
	char umsg_mode_msg[ASE_LOGGER_LEN];

	//   FUNC_CALL_ENTRY;

	// ---------------------------------------------------------------------- //
	/*
	 * Port Control message
	 * Format: <cmd> <value>
	 * -----------------------------------------------------------------
	 * Supported commands       |
	 * ASE_INIT   <APP_PID>     | Session control - sends PID to
	 *                          |
	 * AFU_RESET  <0,1>         | AFU reset handle
	 * UMSG_MODE  <8-bit mask>  | UMSG mode control
	 *
	 * ASE responds with "COMPLETED" as a string, there is no
	 * expectation of a string check
	 *
	 */
	// Simulator is not in lockdown mode (simkill not in progress)
	if (self_destruct_in_progress == 0) {
		if (mqueue_recv(app2sim_portctrl_req_rx, (char *)portctrl_msgstr, ASE_MQ_MSGSIZE) == ASE_MSG_PRESENT) {
			sscanf_s_ii(portctrl_msgstr, "%d %d", &rx_portctrl_cmd, &portctrl_value);
			if (rx_portctrl_cmd == AFU_RESET) {
				// AFU Reset control
				portctrl_value = (portctrl_value != 0) ? 1 : 0 ;

				// Wait until transactions clear
				// AFU Reset trigger function will wait until channels clear up
				afu_softreset_trig (0, portctrl_value);

				// Reset response is returned from simulator once queues are cleared
				// Simulator cannot be held up here.
			} else if (rx_portctrl_cmd == UMSG_MODE) {
				// Umsg mode setting here
				glbl_umsgmode = portctrl_value & 0xFFFFFFFF;
				snprintf(umsg_mode_msg, ASE_LOGGER_LEN, "UMSG Mode mask set to 0x%x", glbl_umsgmode);
				buffer_msg_inject(1, umsg_mode_msg);

				// Send portctrl_rsp message
				mqueue_send(sim2app_portctrl_rsp_tx, completed_str_msg, ASE_MQ_MSGSIZE);
			} else if (rx_portctrl_cmd == ASE_INIT) {
				ASE_INFO("Session requested by PID = %d\n", portctrl_value);
				// Generate new timestamp
				put_timestamp();

				// Generate session ID path
				snprintf(tstamp_filepath, ASE_FILEPATH_LEN,
					 "%s/%s", ase_workdir_path,
					 TSTAMP_FILENAME);

				// Print timestamp
				glbl_session_id = ase_malloc(20);
				get_timestamp(glbl_session_id);
				ASE_MSG("Session ID => %s\n",
					glbl_session_id);

				session_empty = 0;

				// Send portctrl_rsp message
				mqueue_send(sim2app_portctrl_rsp_tx, completed_str_msg, ASE_MQ_MSGSIZE);

				int thr_err = pthread_create(&socket_srv_tid,
							     NULL, &start_socket_srv, NULL);

				if (thr_err != 0) {
					ASE_ERR("FAILED Event server \
					failed to start\n");
					exit(1);
				}
				ASE_MSG("Event socket server started\n");
			} else if (rx_portctrl_cmd == ASE_SIMKILL) {
#ifdef ASE_DEBUG
				ASE_MSG("ASE_SIMKILL requested, processing options... \n");
#endif

				sockserver_kill = 1;
				// ------------------------------------------------------------- //
				// Update regression counter
				glbl_test_cmplt_cnt = glbl_test_cmplt_cnt + 1;
				// Mode specific exit behaviour
				if ((cfg->ase_mode == ASE_MODE_DAEMON_NO_SIMKILL) && (session_empty == 0)) {
					ASE_MSG("ASE running in daemon mode (see ase.cfg)\n");
					ASE_MSG("Reseting buffers ... Simulator RUNNING\n");
					ase_reset_trig();
					ase_shmem_destroy();
					ASE_INFO("Ready to run next test\n");
					session_empty = 1;
					buffer_msg_inject(0, TEST_SEPARATOR);
				} else if (cfg->ase_mode == ASE_MODE_DAEMON_SIMKILL) {
					ASE_INFO("ASE Timeout SIMKILL will happen soon\n");
				} else if (cfg->ase_mode == ASE_MODE_DAEMON_SW_SIMKILL) {
					ASE_INFO("ASE recognized a SW simkill (see ase.cfg)... Simulator will EXIT\n");
					run_clocks (500);
					ase_shmem_perror_teardown(NULL, 0);
				} else if (cfg->ase_mode == ASE_MODE_REGRESSION) {
					if (cfg->ase_num_tests == glbl_test_cmplt_cnt) {
						ASE_INFO("ASE completed %d tests (see supplied ASE config file)... Simulator will EXIT\n", cfg->ase_num_tests);
						run_clocks (500);
						ase_shmem_perror_teardown(NULL, 0);
					} else {
						ase_reset_trig();
					}
				}

				// Check for simulator sanity -- if transaction counts dont match
				// Kill the simulation ASAP -- DEBUG feature only
#ifdef ASE_DEBUG
				count_error_flag_ping();
				if (count_error_flag != 0) {
					ASE_ERR
						("** ERROR ** Transaction counts do not match, something got lost\n");
					run_clocks(500);
					self_destruct_in_progress = 1;
					ase_shmem_destroy();
					start_simkill_countdown();
				}
#endif

				// Send portctrl_rsp message
				mqueue_send(sim2app_portctrl_rsp_tx,
					    completed_str_msg,
					    ASE_MQ_MSGSIZE);

				// Clean up session OD
				ase_free_buffer(glbl_session_id);
			} else {
				ASE_ERR
					("Undefined Port Control function ... IGNORING\n");

				// Send portctrl_rsp message
				mqueue_send(sim2app_portctrl_rsp_tx,
					    completed_str_msg,
					    ASE_MQ_MSGSIZE);
			}
		}

		// ------------------------------------------------------------------------------- //
		/*
		 * Buffer Allocation Replicator
		 */
		// Receive a DPI message and get information from replicated buffer
		if (mqueue_recv
		    (app2sim_alloc_rx, (char *) incoming_alloc_msgstr,
		     ASE_MQ_MSGSIZE) == ASE_MSG_PRESENT) {
			// Typecast string to buffer_t
			ase_memcpy((char *) &ase_buffer,
				   incoming_alloc_msgstr,
				   sizeof(struct buffer_t));

			if (ase_buffer.is_pinned) {
				// Pinned buffer (host memory) messages are sent only for
				// logging. The actual buffer is not shared with the simulator.
				// The simulator will send read/write memory requests to the
				// application.
				snprintf(logger_str,
						 ASE_LOGGER_LEN,
						 "Pinned host memory page =>\n"
						 "\t\tHost app virtual addr   = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (byte) = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (line) = 0x%" PRIx64 "\n"
						 "\t\tPage size (bytes)       = %" PRId32 "\n",
						 ase_buffer.vbase,
						 ase_buffer.fake_paddr,
						 ase_buffer.fake_paddr >> 6,
						 ase_buffer.memsize);
			} else {
				// Allocate action
				ase_shmem_alloc_action(&ase_buffer);
				ase_buffer.is_privmem = 0;
				if (ase_buffer.index == 0) {
					ase_buffer.is_mmiomap = 1;
				} else {
					ase_buffer.is_mmiomap = 0;
				}

				char *buffer_class = "Buffer";
				if (ase_buffer.is_mmiomap) {
					buffer_class = "MMIO map";
					initialize_fme_dfh(&ase_buffer);
				} else if (ase_buffer.is_umas) {
					buffer_class = "UMAS";
					update_fme_dfh(&ase_buffer);
				}

				snprintf(logger_str,
						 ASE_LOGGER_LEN,
						 "%s allocated, index %d (located /dev/shm%s) =>\n"
						 "\t\tHost app virtual addr   = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (byte) = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (line) = 0x%" PRIx64 "\n"
						 "\t\tWorkspace size (bytes)  = %" PRId32 "\n",
						 buffer_class,
						 ase_buffer.index,
						 ase_buffer.memname,
						 ase_buffer.vbase,
						 ase_buffer.fake_paddr,
						 ase_buffer.fake_paddr >> 6,
						 ase_buffer.memsize);
			}

			// Inject buffer message
			buffer_msg_inject(1, logger_str);

			// Standard oneline message ---> Hides internal info
			ase_buffer_oneline(&ase_buffer);

			// Write buffer information to file
			if (fp_workspace_log != NULL) {
				fprintf(fp_workspace_log, "%s", logger_str);
				fflush(fp_workspace_log);
			}

#ifdef ASE_DEBUG
			ase_buffer_info(&ase_buffer);
#endif
		}

		// ------------------------------------------------------------------------------- //
		if (mqueue_recv
		    (app2sim_dealloc_rx, (char *) incoming_dealloc_msgstr,
		     ASE_MQ_MSGSIZE) == ASE_MSG_PRESENT) {
			// Typecast string to buffer_t
			ase_memcpy((char *) &ase_buffer,
				   incoming_dealloc_msgstr,
				   sizeof(struct buffer_t));

			if (ase_buffer.is_pinned) {
				// Pinned buffer (host memory) messages are sent only for
				// logging.
				snprintf(logger_str,
						 ASE_LOGGER_LEN,
						 "Unpinned host memory page =>\n"
						 "\t\tHW physical addr (byte) = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (line) = 0x%" PRIx64 "\n"
						 "\t\tPage size (bytes)       = %" PRId32 "\n",
						 ase_buffer.fake_paddr,
						 ase_buffer.fake_paddr >> 6,
						 ase_buffer.memsize);
			} else {
				// Format workspace info string
				snprintf(logger_str,
						 ASE_LOGGER_LEN,
						 "Buffer deallocated, index %d (located /dev/shm%s) =>\n"
						 "\t\tHost app virtual addr   = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (byte) = 0x%" PRIx64 "\n"
						 "\t\tHW physical addr (line) = 0x%" PRIx64 "\n"
						 "\t\tWorkspace size (bytes)  = %" PRId32 "\n",
						 ase_buffer.index,
						 ase_buffer.memname,
						 ase_buffer.vbase,
						 ase_buffer.fake_paddr,
						 ase_buffer.fake_paddr >> 6,
						 ase_buffer.memsize);

				// Deallocate action
				ase_shmem_dealloc_action(&ase_buffer, 1);
			}

			// Inject buffer message
			buffer_msg_inject(1, logger_str);

			// Standard oneline message ---> Hides internal info
			ase_buffer.valid = ASE_BUFFER_INVALID;
			ase_buffer_oneline(&ase_buffer);

			// Write buffer information to file
			if (fp_workspace_log != NULL) {
				fprintf(fp_workspace_log, "%s", logger_str);
				fflush(fp_workspace_log);
			}

			// Debug only
#ifdef ASE_DEBUG
			ase_buffer_info(&ase_buffer);
#endif
		}

		// ------------------------------------------------------------------------------- //
		/*
		 * MMIO request listener
		 */
		// Receive csr_write packet
		if (mqueue_recv
		    (app2sim_mmioreq_rx, (char *) incoming_mmio_pkt,
		     sizeof(struct mmio_t)) == ASE_MSG_PRESENT) {
			// ase_memcpy(incoming_mmio_pkt, (mmio_t *)mmio_mapstr, sizeof(struct mmio_t));

#ifdef ASE_DEBUG
			print_mmiopkt(fp_memaccess_log, "MMIO Sent",
				      incoming_mmio_pkt);
#endif
			mmio_dispatch(0, incoming_mmio_pkt);
		}
		// ------------------------------------------------------------------------------- //
		/*
		 * UMSG engine
		 */
		// cleanse string before reading
		if (mqueue_recv
		    (app2sim_umsg_rx, (char *) umsg_mapstr,
		     sizeof(struct umsgcmd_t)) == ASE_MSG_PRESENT) {
			ase_memcpy(incoming_umsg_pkt,
				   (umsgcmd_t *) umsg_mapstr,
				   sizeof(struct umsgcmd_t));

			// Hint trigger
			incoming_umsg_pkt->hint =
				(glbl_umsgmode >> (4 * incoming_umsg_pkt->id))
				& 0xF;

			// dispatch to event processing
#ifdef ASE_ENABLE_UMSG_FEATURE
			umsg_dispatch(0, incoming_umsg_pkt);
#else
			ASE_ERR
				("UMsg is only supported in the integrated configuration!\n");
			ASE_ERR
				("         Simulator will shut down now.\n");
			start_simkill_countdown();
#endif
		}
		// ------------------------------------------------------------------------------- //
	} else {
#ifdef ASE_DEBUG
		ASE_DBG
			("Simulator is in Lockdown mode, Simkill in progress\n");
		sleep(1);
#endif
	}


	//  FUNC_CALL_EXIT;
	return 0;
}

// -----------------------------------------------------------------------
// DPI Initialize routine
// - Setup message queues
// - Start buffer replicator, csr_write listener thread
// -----------------------------------------------------------------------
int ase_init(void)
{
	FUNC_CALL_ENTRY;

	// Set loglevel
	set_loglevel(ase_calc_loglevel());

	// Set stdout bufsize to empty immediately
	// setvbuf(stdout, NULL, _IONBF, 0);
	setbuf(stdout, NULL);

	// Set self_destruct flag = 0, SIMulator is not in lockdown
	self_destruct_in_progress = 0;

	// Graceful kill handlers
	register_signal(SIGTERM, start_simkill_countdown);
	register_signal(SIGINT, start_simkill_countdown);
	register_signal(SIGQUIT, start_simkill_countdown);
	register_signal(SIGHUP, start_simkill_countdown);

	// Runtime error handler (print backtrace)
	register_signal(SIGSEGV, backtrace_handler);
	register_signal(SIGBUS, backtrace_handler);
	register_signal(SIGABRT, backtrace_handler);

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	// Get PID
	ase_pid = getpid();
	ASE_MSG("PID of simulator is %d\n", ase_pid);

	// Allocate incoming_mmio_pkt
	incoming_mmio_pkt = (struct mmio_t *) ase_malloc(sizeof(mmio_t));

	// Allocate incoming_umsg_pkt
	incoming_umsg_pkt =
		(struct umsgcmd_t *) ase_malloc(sizeof(struct umsgcmd_t));

	// ASE configuration management
	ase_config_parse(sv2c_config_filepath);

	// Evaluate ase_workdir_path
	ase_eval_session_directory();
	// Evaluate IPCs
	ipc_init();

	ASE_MSG("Current Directory located at =>\n");
	ASE_MSG("%s\n", ase_workdir_path);

	// Create IPC cleanup setup
	create_ipc_listfile();

	// Sniffer file stat path
	ase_memset(ccip_sniffer_file_statpath, 0, ASE_FILEPATH_LEN);
	snprintf(ccip_sniffer_file_statpath, ASE_FILEPATH_LEN,
		 "%s/ccip_warning_and_errors.txt", ase_workdir_path);

	// Remove existing error log files from previous run
	if (access(ccip_sniffer_file_statpath, F_OK) == 0) {
		if (unlink(ccip_sniffer_file_statpath) == 0) {
			ASE_MSG
				("Removed sniffer log file from previous run\n");
		}
	}

	/*
	 * Debug logs
	 */
#ifdef ASE_DEBUG
	// Create a memory access log
	fp_memaccess_log = fopen("aseafu_access.log", "w");
	if (fp_memaccess_log == NULL) {
		ASE_ERR
			("  [DEBUG]  Memory access debug logger initialization failed !\n");
	} else {
		ASE_DBG("Memory access debug logger initialized\n");
	}

	// Page table tracker
	fp_pagetable_log = fopen("ase_pagetable.log", "w");
	if (fp_pagetable_log == NULL) {
		ASE_ERR
			("  [DEBUG]  ASE pagetable logger initialization failed !\n");
	} else {
		ASE_DBG("ASE pagetable logger initialized\n");
	}
#endif

	// Set up message queues
	ASE_MSG("Creating Messaging IPCs...\n");
	int ipc_iter;
	for (ipc_iter = 0; ipc_iter < ASE_MQ_INSTANCES; ipc_iter++)
		mqueue_create(mq_array[ipc_iter].name);

	// Open message queues
	app2sim_alloc_rx =
		mqueue_open(mq_array[0].name, mq_array[0].perm_flag);
	app2sim_mmioreq_rx =
		mqueue_open(mq_array[1].name, mq_array[1].perm_flag);
	app2sim_umsg_rx =
		mqueue_open(mq_array[2].name, mq_array[2].perm_flag);
	sim2app_alloc_tx =
		mqueue_open(mq_array[3].name, mq_array[3].perm_flag);
	sim2app_mmiorsp_tx =
		mqueue_open(mq_array[4].name, mq_array[4].perm_flag);
	app2sim_portctrl_req_rx =
		mqueue_open(mq_array[5].name, mq_array[5].perm_flag);
	app2sim_dealloc_rx =
		mqueue_open(mq_array[6].name, mq_array[6].perm_flag);
	sim2app_dealloc_tx =
		mqueue_open(mq_array[7].name, mq_array[7].perm_flag);
	sim2app_portctrl_rsp_tx =
		mqueue_open(mq_array[8].name, mq_array[8].perm_flag);
	sim2app_intr_request_tx =
		mqueue_open(mq_array[9].name, mq_array[9].perm_flag);

	// Memory read/write requests.	All simulated references to shared memory are
	// handled by the application.
	sim2app_membus_rd_req_tx =
		mqueue_open(mq_array[10].name, mq_array[10].perm_flag);
	app2sim_membus_rd_rsp_rx =
		mqueue_open(mq_array[11].name, mq_array[11].perm_flag);
	sim2app_membus_wr_req_tx =
		mqueue_open(mq_array[12].name, mq_array[12].perm_flag);
	app2sim_membus_wr_rsp_rx =
		mqueue_open(mq_array[13].name, mq_array[13].perm_flag);

	int i;

	for (i = 0; i < MAX_USR_INTRS; i++)
		intr_event_fds[i] = -1;

	sockserver_kill = 0;

	srand(cfg->ase_seed);

	// Open Buffer info log
	fp_workspace_log = fopen("workspace_info.log", "wb");
	if (fp_workspace_log == (FILE *) NULL) {
		ase_error_report("fopen", errno, ASE_OS_FOPEN_ERR);
	} else {
		ASE_INFO_2
			("Information about allocated buffers => workspace_info.log \n");
	}

	fflush(stdout);

	FUNC_CALL_EXIT;
	return 0;
}


// -----------------------------------------------------------------------
// ASE ready indicator:  Print a message that ASE is ready to go.
// Controls run-modes
// -----------------------------------------------------------------------
int ase_ready(void)
{
	FUNC_CALL_ENTRY;

	// ASE-APP run command
	char app_run_cmd[ASE_FILEPATH_LEN];

	// Set test_cnt to 0
	glbl_test_cmplt_cnt = 0;

	// Write lock file
	ase_write_lock_file();

	// Display "Ready for simulation"
	ASE_INFO
		("** ATTENTION : BEFORE running the software application **\n");
	ASE_INFO
		("Set env(ASE_WORKDIR) in terminal where application will run (copy-and-paste) =>\n");
	ASE_INFO("$SHELL   | Run:\n");
	ASE_INFO
		("---------+---------------------------------------------------\n");
	ASE_INFO("bash/zsh | export ASE_WORKDIR=%s\n", ase_workdir_path);
	ASE_INFO("tcsh/csh | setenv ASE_WORKDIR %s\n", ase_workdir_path);
	ASE_INFO
		("For any other $SHELL, consult your Linux administrator\n");
	ASE_INFO("\n");

	// Run ase_regress.sh here
	if (cfg->ase_mode == ASE_MODE_REGRESSION) {
		ASE_INFO("Starting ase_regress.sh script...\n");
		if ((sv2c_script_filepath != NULL)
		    && (strlen(sv2c_script_filepath) != 0)) {
			snprintf(app_run_cmd, ASE_FILEPATH_LEN, "%s &",
				 sv2c_script_filepath);
		} else {
			ase_string_copy(app_run_cmd, "./ase_regress.sh &",
					ASE_FILEPATH_LEN);
		}

		// Run the regress application
		if (system(app_run_cmd) == -1) {
			ASE_INFO_2
				("ASE had some problem starting script pointed by ASE_SCRIPT\n");
			ASE_INFO_2("Tests may be run manually instead\n");
		}
	} else {
		ASE_INFO("Ready for simulation...\n");
		ASE_INFO("Press CTRL-C to close simulator...\n");
	}

	fflush(stdout);

	FUNC_CALL_EXIT;
	return 0;
}


/*
 * DPI simulation timeout counter
 * - When CTRL-C is pressed, start teardown sequence
 * - TEARDOWN SEQUENCE:
 *   - Close and unlink message queues
 *   - Close and unlink shared memories
 *   - Destroy linked list
 *   - Delete .ase_ready
 *   - Send $finish to VCS
 */
void start_simkill_countdown(void)
{
	FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
	ASE_DBG("Caught a SIG\n");
#endif

	// Close and unlink message queue
	ASE_MSG("Closing message queue and unlinking...\n");

	// Close message queues
	mqueue_close(app2sim_alloc_rx);
	mqueue_close(sim2app_alloc_tx);
	mqueue_close(app2sim_mmioreq_rx);
	mqueue_close(sim2app_mmiorsp_tx);
	mqueue_close(app2sim_umsg_rx);
	mqueue_close(app2sim_portctrl_req_rx);
	mqueue_close(app2sim_dealloc_rx);
	mqueue_close(sim2app_dealloc_tx);
	mqueue_close(sim2app_portctrl_rsp_tx);
	mqueue_close(sim2app_intr_request_tx);
	mqueue_close(sim2app_membus_rd_req_tx);
	mqueue_close(app2sim_membus_rd_rsp_rx);
	mqueue_close(sim2app_membus_wr_req_tx);
	mqueue_close(app2sim_membus_wr_rsp_rx);

	int ipc_iter;
	for (ipc_iter = 0; ipc_iter < ASE_MQ_INSTANCES; ipc_iter++)
		mqueue_destroy(mq_array[ipc_iter].name);

	if (unlink(tstamp_filepath) == -1) {
		ASE_MSG
			("$ASE_WORKDIR/.ase_ready could not be deleted, please delete manually... \n");
	} else {
		ASE_MSG("Session code file removed\n");
	}

	// Final clean of IPC
	final_ipc_cleanup();

	// wait for server shutdown
	pthread_cancel(socket_srv_tid);
	pthread_join(socket_srv_tid, NULL);

	// Remove session files
	ASE_MSG("Cleaning session files...\n");
	if (unlink(ase_ready_filepath) == -1) {
		ASE_ERR
			("Session file %s could not be removed, please remove manually !!\n",
			 ASE_READY_FILENAME);
	}
	// Print location of log files
	ASE_INFO("Simulation generated log files\n");
	ASE_INFO
		("        Transactions file       | $ASE_WORKDIR/ccip_transactions.tsv\n");
	ASE_INFO
		("        Workspaces info         | $ASE_WORKDIR/workspace_info.log\n");
	if (access(ccip_sniffer_file_statpath, F_OK) != -1) {
		ASE_INFO
			("        Protocol warning/errors | $ASE_WORKDIR/ccip_warning_and_errors.txt\n");
	}

	// Display test count
	ASE_INFO("\n");
	ASE_INFO("Tests run     => %d\n", glbl_test_cmplt_cnt);
	ASE_INFO("\n");

	// Send a simulation kill command
	ASE_INFO_2("Sending kill command...\n");
	usleep(1000);

	// Set scope
	svSetScope(scope);

	// Free memories
	free(cfg);
	free(ase_ready_filepath);
	ase_free_buffer((char *) incoming_mmio_pkt);
	ase_free_buffer((char *) incoming_umsg_pkt);

	// Issue Simulation kill
	simkill();

	// Close workspace log
	if (fp_workspace_log != NULL) {
		fclose(fp_workspace_log);
	}
#ifdef ASE_DEBUG
	if (fp_memaccess_log != NULL) {
		fclose(fp_memaccess_log);
	}
	if (fp_pagetable_log != NULL) {
		fclose(fp_pagetable_log);
	}
#endif

	FUNC_CALL_EXIT;
}

/*
 * Parse ase config lines
 */
void parse_ase_cfg_line(char *filename, char *line, float *f_usrclk)
{
	char *parameter;
	int value;
	char *pch;
	char *saveptr;
	// User clock frequency
	float usrclk;

	// Ignore strings beginning with '#' OR NULL (compound NOR)
	if ((line[0] != '#') && (line[0] != '\0')) {
		parameter = strtok_r(line, "=\n", &saveptr);
		if (parameter != NULL) {
			if (ase_strncmp(parameter, "ASE_MODE", 8) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->ase_mode = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "ASE_TIMEOUT", 11) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->ase_timeout = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "ASE_NUM_TESTS", 13) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->ase_num_tests = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "ENABLE_REUSE_SEED", 17) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->enable_reuse_seed = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "ASE_SEED", 8) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->ase_seed = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "ENABLE_CL_VIEW", 14) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					cfg->enable_cl_view = strtol(pch, NULL, 10);
				}
			} else if (ase_strncmp(parameter, "USR_CLK_MHZ", 11) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					usrclk = atof(pch);
					if (usrclk == 0.000000) {
						ASE_ERR("User Clock Frequency cannot be 0.000 MHz\n");
						ASE_ERR("        Reverting to %f MHz\n", DEFAULT_USR_CLK_MHZ);
						usrclk = DEFAULT_USR_CLK_MHZ;
						cfg->usr_tps = DEFAULT_USR_CLK_TPS;
					} else if (usrclk == DEFAULT_USR_CLK_MHZ) {
						cfg->usr_tps = DEFAULT_USR_CLK_TPS;
					} else {
						cfg->usr_tps = (int)(1E+12 /(usrclk * pow(1000, 2)));
#ifdef ASE_DEBUG
						ASE_DBG("usr_tps = %d\n", cfg->usr_tps);
#endif
						if (usrclk != DEFAULT_USR_CLK_MHZ) {
							ASE_INFO_2("User clock Frequency was modified from %f to %f MHz\n",
										DEFAULT_USR_CLK_MHZ, usrclk);
						}
					}
					*f_usrclk = usrclk;
				}
			} else if (ase_strncmp(parameter, "PHYS_MEMORY_AVAILABLE_GB", 24) == 0) {
				pch = strtok_r(NULL, "", &saveptr);
				if (pch != NULL) {
					value = strtol(pch, NULL, 10);
					if (value < 0) {
						ASE_ERR("Physical memory size is negative in %s\n", filename);
						ASE_ERR("        Reverting to default 256 GB\n");
					} else {
						cfg->phys_memory_available_gb = value;
					}
				}
			} else {
				ASE_INFO_2("In config file %s, Parameter type %s is unidentified \n",
							 filename, parameter);
			}
		}
	}
}

/*
 * ASE config parsing
 * - Set default values for ASE configuration
 * - See if a ase.cfg is available for overriding global values
 *   - If YES, parse and configure the cfg (ase_cfg_t) structure
 */
void ase_config_parse(char *filename)
{
	FUNC_CALL_ENTRY;

	FILE *fp = (FILE *) NULL;
	char *line;
	size_t len = 0;

	// User clock frequency
	float f_usrclk;


	// Allocate space to store ASE config
	cfg = (struct ase_cfg_t *) ase_malloc(sizeof(struct ase_cfg_t));

	// Allocate memory to store a line
	line = ase_malloc(sizeof(char) * 80);

	// Default values
	cfg->ase_mode = ASE_MODE_DAEMON_NO_SIMKILL;
	cfg->ase_timeout = 50000;
	cfg->ase_num_tests = 1;
	cfg->enable_reuse_seed = 0;
	cfg->ase_seed = generate_ase_seed();
	cfg->enable_cl_view = 1;
	cfg->usr_tps = DEFAULT_USR_CLK_TPS;
	cfg->phys_memory_available_gb = 256;

	// Fclk Mhz
	f_usrclk = DEFAULT_USR_CLK_MHZ;

	// Find ase.cfg OR not
	if (access(filename, F_OK) != -1) {
		// FILE exists, overwrite
		fp = fopen(filename, "r");
		if (fp == NULL) {
			ASE_ERR
				("%s supplied by +CONFIG could not be opened, IGNORED\n",
				 filename);
		} else {
			ASE_INFO_2("Reading %s configuration file \n",
				   filename);
			// Parse file line by line
			while (getline(&line, &len, fp) != -1) {
				// Remove all invalid characters
				remove_spaces(line);
				remove_tabs(line);
				remove_newline(line);
				parse_ase_cfg_line(filename, line, &f_usrclk);
			}
		}

		/*
		 * ASE mode control
		 */
		switch (cfg->ase_mode) {
			// Classic Server client mode
		case ASE_MODE_DAEMON_NO_SIMKILL:
			ASE_INFO_2
				("ASE was started in Mode 1 (Server-Client without SIMKILL)\n");
			cfg->ase_timeout = 0;
			cfg->ase_num_tests = 0;
			break;

			// Server Client mode with SIMKILL
		case ASE_MODE_DAEMON_SIMKILL:
			ASE_INFO_2
				("ASE was started in Mode 2 (Server-Client with SIMKILL)\n");
			cfg->ase_num_tests = 0;
			break;

			// Long runtime mode (SW kills SIM)
		case ASE_MODE_DAEMON_SW_SIMKILL:
			ASE_INFO_2
				("ASE was started in Mode 3 (Server-Client with Sw SIMKILL (long runs)\n");
			cfg->ase_timeout = 0;
			cfg->ase_num_tests = 0;
			break;

			// Regression mode (lets an SH file with
		case ASE_MODE_REGRESSION:
			ASE_INFO_2
				("ASE was started in Mode 4 (Regression mode)\n");
			cfg->ase_timeout = 0;
			break;

			// Illegal modes
		default:
			ASE_INFO_2
				("ASE mode could not be identified, will revert to ASE_MODE = 1 (Server client w/o SIMKILL)\n");
			cfg->ase_mode = ASE_MODE_DAEMON_NO_SIMKILL;
			cfg->ase_timeout = 0;
			cfg->ase_num_tests = 0;
		}

		// Close file
		if (fp != NULL) {
			fclose(fp);
		}

	} else {
		// FILE does not exist
		ASE_INFO_2("%s not found, using default values\n",
			   filename);
	}

	// Mode configuration
	switch (cfg->ase_mode) {
	case ASE_MODE_DAEMON_NO_SIMKILL:
		ASE_INFO_2
			("ASE Mode: Server-Client mode without SIMKILL\n");
		break;
	case ASE_MODE_DAEMON_SIMKILL:
		ASE_INFO_2("ASE Mode: Server-Client mode with SIMKILL\n");
		break;
	case ASE_MODE_DAEMON_SW_SIMKILL:
		ASE_INFO_2
			("ASE Mode: Server-Client mode with SW SIMKILL (long runs)\n");
		break;
	case ASE_MODE_REGRESSION:
		ASE_INFO_2("ASE Mode: ASE Regression mode\n");
		break;
	}

	// Inactivity
	if (cfg->ase_mode == ASE_MODE_DAEMON_SIMKILL)
		ASE_INFO_2
			("Inactivity kill-switch     ... ENABLED after %d clocks \n",
			 cfg->ase_timeout);
	else
		ASE_INFO_2("Inactivity kill-switch     ... DISABLED \n");

	// Reuse seed
	if (cfg->enable_reuse_seed != 0)
		ASE_INFO_2("Reuse simulation seed      ... ENABLED \n");
	else {
		ASE_INFO_2
			("Reuse simulation seed      ... DISABLED \n");
		cfg->ase_seed = generate_ase_seed();
	}

	// ASE will be run with this seed
	ASE_INFO_2("ASE Seed                   ... %d \n", cfg->ase_seed);

	// CL view
	if (cfg->enable_cl_view != 0)
		ASE_INFO_2("ASE Transaction view       ... ENABLED\n");
	else
		ASE_INFO_2("ASE Transaction view       ... DISABLED\n");

	// User clock frequency
	ASE_INFO_2
		("User Clock Frequency       ... %.6f MHz, T_uclk = %d ps \n",
		 f_usrclk, cfg->usr_tps);
	if (f_usrclk != DEFAULT_USR_CLK_MHZ) {
		ASE_INFO_2
			("** NOTE **: User Clock Frequency was changed from default %f MHz !\n",
			 DEFAULT_USR_CLK_MHZ);
	}
	// GBs of physical memory available
	ASE_INFO_2("Amount of physical memory  ... %d GB\n",
		   cfg->phys_memory_available_gb);

	// Transfer data to hardware (for simulation only)
	ase_config_dex(cfg);

	// free memory
	free(line);

	FUNC_CALL_EXIT;
}
