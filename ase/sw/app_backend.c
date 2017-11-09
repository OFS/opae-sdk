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
/*
 * Module Info: ASE native SW application interface (bare-bones ASE access)
 * Language   : C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 */

#define _GNU_SOURCE

#include "ase_common.h"

// MMIO Mutex Lock, initilize it here
pthread_mutex_t mmio_port_lock = PTHREAD_MUTEX_INITIALIZER;

// CSR map storage
struct buffer_t *mmio_region;

// UMAS region
struct buffer_t *umas_region;

// Workspace metadata table
struct wsmeta_t *wsmeta_head = (struct wsmeta_t *) NULL;
struct wsmeta_t *wsmeta_end = (struct wsmeta_t *) NULL;

// Buffer index count
int asebuf_index_count;   // global count/index
int userbuf_index_count;  // User count/index

// Timestamp char array
char *tstamp_string;

// Application lock file path
char app_ready_lockpath[ASE_FILEPATH_LEN];

// Session file path
char tstamp_filepath[ASE_FILEPATH_LEN] = {0,};

// Base addresses of required regions
uint64_t *mmio_afu_vbase;
uint64_t *umsg_umas_vbase;

// work Directory location
char *ase_workdir_path;

// ASE Capability register
struct ase_capability_t ase_capability;

// MMIO Scoreboard (used in APP-side only)
struct mmio_scoreboard_line_t {
    uint64_t data;
    int tid;
    bool tx_flag;
    bool rx_flag;
};
volatile struct mmio_scoreboard_line_t mmio_table[MMIO_MAX_OUTSTANDING];

// Debug logs
#ifdef ASE_DEBUG
FILE *fp_pagetable_log = (FILE *) NULL;
FILE *fp_mmioaccess_log = (FILE *) NULL;
#endif

/*
 * MMIO Read response watcher
 */
// MMIO Tid
int glbl_mmio_tid;

// Tracker thread Id
pthread_t mmio_watch_tid;

// MMIO Response packet handoff control
mmio_t *mmio_rsp_pkt;

/*
 * UMsg listener/packet
 */
// UMsg Watch TID
pthread_t umsg_watch_tid;

// UMsg byte offset
const int umsg_byteindex_arr[] = {
    0x0, 0x1040, 0x2080, 0x30C0, 0x4100, 0x5140, 0x6180, 0x71C0
};

// UMsg address array
char *umsg_addr_array[NUM_UMSG_PER_AFU];

// UMAS initialized flag
volatile int umas_init_flag;

/*
 * Existance check flags
 */
static uint32_t session_exist_status;
static uint32_t mq_exist_status;
static uint32_t mmio_exist_status;
static uint32_t umas_exist_status;


// Time taken calc
struct timespec start_time_snapshot, end_time_snapshot;
unsigned long long runtime_nsec;

// Exist status flags
static uint32_t session_exist_status;
static uint32_t mq_exist_status;
static uint32_t mmio_exist_status;
static uint32_t umas_exist_status;


/*
 * MMIO Generate TID
 * - Creation of TID must be atomic
 */
uint32_t generate_mmio_tid(void)
{
    // Return value
    uint32_t ret_mmio_tid;

    while (count_mmio_tid_used() == MMIO_MAX_OUTSTANDING) {
#ifdef ASE_DEBUG
	ASE_INFO("MMIO TIDs have run out --- waiting !\n");
#endif
	usleep(10000);
    }

    // Increment and mask
    ret_mmio_tid = glbl_mmio_tid & MMIO_TID_BITMASK;
    glbl_mmio_tid++;

    // Return ID
    return ret_mmio_tid;
}


/*
 * THREAD: MMIO Read thread watcher
 */
void *mmio_response_watcher(void *arg)
{
    // Mark as thread that can be cancelled anytime
    pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

    mmio_rsp_pkt = (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));
    int ret;
    int slot_idx;

#ifdef ASE_DEBUG
    char mmio_type[3];
#endif

    // start watching for messages
    while (mmio_exist_status == ESTABLISHED) {
	memset((void *) mmio_rsp_pkt, 0xbc, sizeof(mmio_t));

	// If received, update global message
	ret =
	    mqueue_recv(sim2app_mmiorsp_rx, (char *) mmio_rsp_pkt,
			sizeof(mmio_t));
	if (ret == ASE_MSG_PRESENT) {
#ifdef ASE_DEBUG
	    // Logging event
	    print_mmiopkt(fp_mmioaccess_log, "Got ",
			  mmio_rsp_pkt);
	    if (mmio_rsp_pkt->write_en == MMIO_WRITE_REQ) {
		ase_string_copy(mmio_type, "WR\0", 3);
	    } else if (mmio_rsp_pkt->write_en == MMIO_READ_REQ) {
		ase_string_copy(mmio_type, "RD\0", 3);
	    }

	    ASE_DBG
		("mmio_watcher => %03x, %s, %d, %x, %016llx\n",
		 mmio_rsp_pkt->tid, mmio_type,
		 mmio_rsp_pkt->width, mmio_rsp_pkt->addr,
		 mmio_rsp_pkt->qword[0]);

#endif

	    // Find scoreboard slot number to update
	    slot_idx =
		get_scoreboard_slot_by_tid(mmio_rsp_pkt->tid);

	    if (slot_idx == 0xFFFF) {
		ASE_ERR
		    ("get_scoreboard_slot_by_tid() found a bad slot !");
		raise(SIGABRT);
	    } else {
		// MMIO Read response (for credit count only)
		if (mmio_rsp_pkt->write_en ==
		    MMIO_READ_REQ) {
		    mmio_table[slot_idx].tid =
			mmio_rsp_pkt->tid;
		    mmio_table[slot_idx].data =
			mmio_rsp_pkt->qword[0];
		    mmio_table[slot_idx].tx_flag =
			true;
		    mmio_table[slot_idx].rx_flag =
			true;
		}
		// MMIO Write response (for credit count only)
		else if (mmio_rsp_pkt->write_en ==
			 MMIO_WRITE_REQ) {
		    mmio_table[slot_idx].tx_flag =
			false;
		    mmio_table[slot_idx].rx_flag =
			false;
		}
#ifdef ASE_DEBUG
		else {
		    ASE_ERR
			("Illegal MMIO request found -- must not happen !\n");
		}
#endif
	    }
	}
    }

    return 0;
}


/*
 * Interrupt request (FPGA->CPU) watcher
 */
// void *intr_request_watcher()
// {

// }


/*
 * Send SW Reset
 */
void send_swreset(void)
{
    ASE_MSG("\n");
    ASE_MSG("Issuing Soft Reset... \n");
    while (count_mmio_tid_used() != 0) {
	sleep(1);
    }

    // Reset High
    ase_portctrl(AFU_RESET, 1);

    // Wait
    usleep(1);

    // Reset Low
    ase_portctrl(AFU_RESET, 0);
}


/*
 * Send SIMKILL
 */
void send_simkill(int n)
{
    // Issue Simkill
    ase_portctrl(ASE_SIMKILL, 0);

    ASE_ERR("CTRL-C was seen... SW application will exit\n");

    // Deinitialize session
    session_exist_status = NOT_ESTABLISHED;
    session_deinit();

    exit(0);
}


/*
 * Session Initialize
 * Open the message queues to ASE simulator
 */
void session_init(void)
{
    FUNC_CALL_ENTRY;

    // Start clock
    clock_gettime(CLOCK_MONOTONIC, &start_time_snapshot);

    // Current APP PID
    pid_t lockread_app_pid;
    FILE *fp_app_lockfile;

    // Session setup
    if (session_exist_status != ESTABLISHED) {

	// Set loglevel
	set_loglevel(ase_calc_loglevel());

	setvbuf(stdout, NULL, (int) _IONBF, (size_t) 0);

	ipc_init();

	// Initialize ase_workdir_path
	ASE_MSG("ASE Session Directory located at =>\n");
	ASE_MSG("%s\n", ase_workdir_path);

		// Generate Timestamp filepath
		snprintf(tstamp_filepath, ASE_FILEPATH_LEN, "%s/%s", ase_workdir_path, TSTAMP_FILENAME);

		// Craft a .app_lock.pid lock filepath string
		memset(app_ready_lockpath, 0, ASE_FILEPATH_LEN);
		snprintf(app_ready_lockpath, ASE_FILEPATH_LEN, "%s/%s",
			 ase_workdir_path, APP_LOCK_FILENAME);

	// Check if .app_lock_pid lock already exists or not.
	if (check_app_lock_file()) {
	    //If .app_lock.pid exists but pid doesnt exist.
	    if (!remove_existing_lock_file()) {
		ASE_MSG("Application Exiting \n");
		exit(1);
	    }
	}

	create_new_lock_file();

	// Register kill signals to issue simkill
	signal(SIGTERM, send_simkill);
	signal(SIGINT, send_simkill);
	signal(SIGQUIT, send_simkill);
	signal(SIGHUP, send_simkill);

	// When bad stuff happens, print backtrace
	signal(SIGSEGV, backtrace_handler);
	signal(SIGBUS, backtrace_handler);
	signal(SIGABRT, backtrace_handler);

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);


	ASE_INFO("Initializing simulation session ... \n");


	app2sim_alloc_tx =
	    mqueue_open(mq_array[0].name, mq_array[0].perm_flag);
	app2sim_mmioreq_tx =
	    mqueue_open(mq_array[1].name, mq_array[1].perm_flag);
	app2sim_umsg_tx =
	    mqueue_open(mq_array[2].name, mq_array[2].perm_flag);
	sim2app_alloc_rx =
	    mqueue_open(mq_array[3].name, mq_array[3].perm_flag);
	sim2app_mmiorsp_rx =
	    mqueue_open(mq_array[4].name, mq_array[4].perm_flag);
	app2sim_portctrl_req_tx =
	    mqueue_open(mq_array[5].name, mq_array[5].perm_flag);
	app2sim_dealloc_tx =
	    mqueue_open(mq_array[6].name, mq_array[6].perm_flag);
	sim2app_dealloc_rx =
	    mqueue_open(mq_array[7].name, mq_array[7].perm_flag);
	sim2app_portctrl_rsp_rx =
	    mqueue_open(mq_array[8].name, mq_array[8].perm_flag);
	sim2app_intr_request_rx =
	    mqueue_open(mq_array[9].name, mq_array[9].perm_flag);

	// Message queues have been established
	mq_exist_status = ESTABLISHED;

	// Issue soft reset
	send_swreset();

	// Page table tracker (optional logger)
#ifdef ASE_DEBUG
	// Create debug log of page table
	fp_pagetable_log = fopen("app_pagetable.log", "w");
	if (fp_pagetable_log == NULL) {
	    ASE_ERR
		("APP pagetable logger initialization failed !\n");
	} else {
	    ASE_MSG("APP pagetable logger initialized\n");
	}

	// Debug log of MMIO
	fp_mmioaccess_log = fopen("app_mmioaccess.log", "w");
	if (fp_mmioaccess_log == NULL) {
	    ASE_ERR
		("APP MMIO access logger initialization failed !\n");
	} else {
	    ASE_MSG("APP MMIO access logger initialized\n");
	}
#endif

		// Set MMIO Tid to 0
		glbl_mmio_tid = 0;

		// Thread error integer
		int thr_err;

		// Start MSI-X watcher thread
		// ASE_MSG("Starting Interrupt watcher ... ");

		// Session start
		ASE_MSG("Session started\n");

		// Initialize session with PID
		ase_portctrl(ASE_INIT, getpid());

		// Wait till session file is created
		poll_for_session_id();

		tstamp_string = (char *) ase_malloc(20);
		get_timestamp(tstamp_string);

		// Creating CSR map
		ASE_MSG("Creating MMIO ...\n");

		mmio_region = (struct buffer_t *)
			ase_malloc(sizeof(struct buffer_t));
		mmio_region->memsize = MMIO_LENGTH;
		mmio_region->is_mmiomap = 1;
		allocate_buffer(mmio_region, NULL);
		mmio_afu_vbase =
			(uint64_t *) ((uint64_t) mmio_region->vbase +
				      MMIO_AFU_OFFSET);
		mmio_exist_status = ESTABLISHED;

		ASE_MSG("AFU MMIO Virtual Base Address = %p\n",
			(void *) mmio_afu_vbase);


		// Create UMSG region
		umas_init_flag = 0;
		ASE_MSG("Creating UMAS ... \n");

		umas_region = (struct buffer_t *)
			ase_malloc(sizeof(struct buffer_t));
		umas_region->memsize = UMAS_REGION_MEMSIZE;	//UMAS_LENGTH;
		umas_region->is_umas = 1;
		allocate_buffer(umas_region, NULL);
		umsg_umas_vbase =
			(uint64_t *) ((uint64_t) umas_region->vbase);
		umas_exist_status = ESTABLISHED;
		umsg_set_attribute(0x0);
		ASE_MSG("UMAS Virtual Base address = %p\n",
			(void *) umsg_umas_vbase);

		// Start MMIO read response watcher watcher thread
		ASE_MSG("Starting MMIO Read Response watcher ... \n");
		thr_err =
			pthread_create(&mmio_watch_tid, NULL,
				       &mmio_response_watcher, NULL);
		if (thr_err != 0) {
			ASE_ERR("FAILED\n");
			BEGIN_RED_FONTCOLOR;
			perror("pthread_create");
			END_RED_FONTCOLOR;
			exit(1);
		} else {
			ASE_MSG("SUCCESS\n");
		}

	ASE_MSG("Starting UMsg watcher ... \n");

	// Initiate UMsg watcher
	thr_err = pthread_create(&umsg_watch_tid, NULL, &umsg_watcher,
				 NULL);
	if (thr_err != 0) {
	    ASE_ERR("FAILED\n");
	    BEGIN_RED_FONTCOLOR;
	    perror("pthread_create");
	    END_RED_FONTCOLOR;
	    exit(1);
	} else {
	    ASE_MSG("SUCCESS\n");
	}

	while (umas_init_flag != 1)
	    ;

	// MMIO Scoreboard setup
	int ii;
	for (ii = 0; ii < MMIO_MAX_OUTSTANDING; ii = ii + 1) {
	    mmio_table[ii].tid = 0;
	    mmio_table[ii].data = 0;
	    mmio_table[ii].tx_flag = false;
	    mmio_table[ii].rx_flag = false;
	}

	// Session status
	session_exist_status = ESTABLISHED;

    } else {
#ifdef ASE_DEBUG
	ASE_DBG("Session already exists\n");
#endif
    }

    FUNC_CALL_EXIT;
}


// Create New Lock File
void create_new_lock_file(void)
{
    FILE *fp_app_lockfile;
    // Open lock file for writing
    fp_app_lockfile = fopen(app_ready_lockpath, "w");
    if (fp_app_lockfile == NULL) {
	ASE_ERR
	    ("Application lockfile could not opened for writing in env(ASE_WORKDIR) !");
	exit(1);
    } else {
	// Write PID into lockfile
	fprintf(fp_app_lockfile, "%d\n", getpid());
    }

    // close lockfile
    fclose(fp_app_lockfile);
}


// Check for access to .app_lock_pid
bool check_app_lock_file(void)
{
    if (access(app_ready_lockpath, F_OK) == 0)
	return true;
    else
	return false;
};


// Remove Lock File
bool remove_existing_lock_file(void)
{
    pid_t lock;
    FILE *fp_app_lockfile;

    // Read the PID of the running application
    fp_app_lockfile = fopen(app_ready_lockpath, "r");

    if (fp_app_lockfile == NULL) {
	ASE_ERR
	    ("Error opening Application lock file path, EXITING\n");
	return false;
    } else {
	if (fscanf
	    (fp_app_lockfile, "%d\n",
	     &lock) != 0) {
	    // Check if PID exists
	    kill(lock, 0);
	    if (errno == ESRCH) {
				ASE_MSG
				("ASE found a stale Application lock with PID = %d -- this will be removed\n",
				lock);
		fclose(fp_app_lockfile);
		// Delete lock file
		delete_lock_file();
		return true;
	    } else if (errno == EPERM) {
		ASE_ERR ("Application does not have permission to remove $ASE_WORKDIR/.app_lock.pid \n");
			} else
				ASE_ERR
					("ASE session in env(ASE_WORKDIR) is currently used by PID=%d\n",
					 lock);
	} else {
	    ASE_ERR
		("Error reading PID of application using ASE, EXITING\n");
	    ASE_ERR
		("ASE was found to be running with another application !\n");
	    ASE_ERR("\n");
	    ASE_ERR("If you think this is in error:\n");
	    ASE_ERR
		(" - Manually delete $ASE_WORKDIR/.app_lock.pid file\n");
	    ASE_ERR
		(" - Close any ASE simulator is running from the $ASE_WORKDIR directory\n");
	}
    }
    fclose(fp_app_lockfile);
    return false;
}


// Delete app_lock file for non-existent processes.
void delete_lock_file(void)
{
    if (unlink(app_ready_lockpath) == 0)
	ASE_INFO("Deleted the existing app_lock.pid with Stale pid \n");
    else {
	ASE_ERR("Application Lock file could not be removed, please remove manually from $ASE_WORKDIR/.app_lock.pid \n");
	exit(1);
    }
}

/*
 * Session deninitialize
 * Close down message queues to ASE simulator
 */
void session_deinit(void)
{
    FUNC_CALL_ENTRY;

    if (session_exist_status == ESTABLISHED) {

	ASE_INFO("Deinitializing simulation session \n");

	// Mark session as destroyed
	session_exist_status = NOT_ESTABLISHED;

	// Unmap UMAS region
	if (umas_exist_status == ESTABLISHED) {

	    ASE_MSG("Closing Watcher threads\n");

	    // Update status
	    umas_exist_status = NOT_ESTABLISHED;

	    // Close UMsg thread
	    pthread_cancel(umsg_watch_tid);

	    // Deallocate the region
	    ASE_MSG("Deallocating UMAS\n");
	    deallocate_buffer(umas_region);
	}
#ifdef ASE_DEBUG
	else {
	    ASE_MSG("No UMAS established\n");
	}
#endif

	// Um-mapping CSR region
	ASE_MSG("Deallocating MMIO map\n");
	if (mmio_exist_status == ESTABLISHED) {

	    deallocate_buffer(mmio_region);
	    mmio_exist_status = NOT_ESTABLISHED;

			// Close MMIO Response tracker thread
			if (pthread_cancel(mmio_watch_tid) != 0) {
				printf
					("MMIO pthread_cancel failed -- Ignoring\n");
			}
		}

		// Send SIMKILL
		ase_portctrl(ASE_SIMKILL, 0);

#ifdef ASE_DEBUG
	fclose(fp_pagetable_log);
	fclose(fp_mmioaccess_log);
#endif
    } else {
	ASE_MSG("Session already deinitialized, call ignored !\n");
    }

    // close message queue
    mqueue_close(app2sim_mmioreq_tx);
    mqueue_close(sim2app_mmiorsp_rx);
    mqueue_close(app2sim_alloc_tx);
    mqueue_close(sim2app_alloc_rx);
    mqueue_close(app2sim_umsg_tx);
    mqueue_close(app2sim_portctrl_req_tx);
    mqueue_close(app2sim_dealloc_tx);
    mqueue_close(sim2app_dealloc_rx);
    mqueue_close(sim2app_portctrl_rsp_rx);

    // Lock deinit
    if (pthread_mutex_unlock(&mmio_port_lock) != 0) {
	ASE_MSG("Trying to shutdown mutex unlock\n");
    }
    // Stop running threads
    pthread_cancel(umsg_watch_tid);
    pthread_cancel(mmio_watch_tid);

    // End Clock snapshot
    clock_gettime(CLOCK_MONOTONIC, &end_time_snapshot);
    runtime_nsec =
	1e9 * (end_time_snapshot.tv_sec -
	       start_time_snapshot.tv_sec) +
	(end_time_snapshot.tv_nsec - start_time_snapshot.tv_nsec);

    // Set locale, inherit locale, and reset back
    char *oldLocale = setlocale(LC_NUMERIC, NULL);
    setlocale(LC_NUMERIC, "");
    ASE_INFO("\tTook %'llu nsec \n", runtime_nsec);
    setlocale(LC_NUMERIC, oldLocale);

    // Delete the .app_lock.pid file
    if (access(app_ready_lockpath, F_OK) == 0) {
	if (unlink(app_ready_lockpath) == 0) {
	    // Session end, set locale
	    ASE_INFO("Session ended \n");
	}
    }

    FUNC_CALL_EXIT;
}


/*
 * Get a scoreboard slot
 */
int find_empty_mmio_scoreboard_slot(void)
{
    int ii;
    int idx;
    for (ii = 0; ii < MMIO_MAX_OUTSTANDING; ii = ii + 1) {
	idx = ii % MMIO_MAX_OUTSTANDING;
	if ((mmio_table[idx].tx_flag == false)
	    && (mmio_table[idx].rx_flag == false))
	    return idx;
    }
    return 0xFFFF;
}


/*
 * Get MMIO Slot by TID
 */
int get_scoreboard_slot_by_tid(int in_tid)
{
    int ii;
    for (ii = 0; ii < MMIO_MAX_OUTSTANDING; ii = ii + 1) {
	if ((mmio_table[ii].tx_flag == true)
	    && (mmio_table[ii].tid == in_tid))
	    return ii;
    }
    return 0xFFFF;
}


/*
 * Count MMIO TIDs in use
 */
int count_mmio_tid_used(void)
{
    int ii;
    int cnt = 0;

    for (ii = 0; ii < MMIO_MAX_OUTSTANDING; ii = ii + 1)
	if (mmio_table[ii].tx_flag == true)
	    cnt++;

    return cnt;
}


/*
 * MMIO Request call
 * - Return index value
 */
int mmio_request_put(struct mmio_t *pkt)
{
    FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
    print_mmiopkt(fp_mmioaccess_log, "Sent", pkt);
#endif

    // Update scoreboard
    int mmiotable_idx;
    mmiotable_idx = find_empty_mmio_scoreboard_slot();
    if (mmiotable_idx != 0xFFFF) {
	mmio_table[mmiotable_idx].tx_flag = true;
	mmio_table[mmiotable_idx].rx_flag = false;
	mmio_table[mmiotable_idx].tid = pkt->tid;
	mmio_table[mmiotable_idx].data = pkt->qword[0];
    }
    /* #ifdef ASE_DEBUG */
    else {
	ASE_ERR
	    ("ASE Error generating MMIO TID, simulation cannot proceed !\n");
	raise(SIGABRT);
    }
    /* #endif */

    // Send packet
    mqueue_send(app2sim_mmioreq_tx, (char *) pkt, sizeof(mmio_t));

    FUNC_CALL_EXIT;

#ifdef ASE_DEBUG
    if (pkt->write_en == MMIO_READ_REQ) {
	ASE_DBG("mmiotable_idx = %d\n", mmiotable_idx);
    }
#endif

    return mmiotable_idx;
}


/*
 * MMIO Write 32-bit
 */
void mmio_write32(int offset, uint32_t data)
{
    FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
    int slot_idx;
#endif

    if (offset < 0) {
	ASE_ERR("Requested offset is not in AFU MMIO region\n");
	ASE_ERR("MMIO Write Error\n");
	raise(SIGABRT);
    } else {
	mmio_t *mmio_pkt;
	mmio_pkt =
	    (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));

	mmio_pkt->write_en = MMIO_WRITE_REQ;
	mmio_pkt->width = MMIO_WIDTH_32;
	mmio_pkt->addr = offset;
	ase_memcpy(mmio_pkt->qword, &data, sizeof(uint32_t));
	mmio_pkt->resp_en = 0;

	// Critical Section
	{
	    if (pthread_mutex_lock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("pthread_mutex_lock could not attain lock !\n");
		exit(1);
	    }

	    mmio_pkt->tid = generate_mmio_tid();
#ifdef ASE_DEBUG
	    slot_idx = mmio_request_put(mmio_pkt);
#else
	    mmio_request_put(mmio_pkt);
#endif

	    if (pthread_mutex_unlock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("Mutex unlock failure ... Application Exit here\n");
		exit(1);
	    }
	}

	// Write to MMIO map
	uint32_t *mmio_vaddr;
	mmio_vaddr =
	    (uint32_t *) ((uint64_t) mmio_afu_vbase + offset);
	ase_memcpy(mmio_vaddr, (char *) &data, sizeof(uint32_t));

		// Display
		ASE_MSG
			("MMIO Write     : tid = 0x%03x, offset = 0x%x, data = 0x%08x\n",
			 mmio_pkt->tid, mmio_pkt->addr, data);

	free(mmio_pkt);
    }

    FUNC_CALL_EXIT;
}


/*
 * MMIO Write 64-bit
 */
void mmio_write64(int offset, uint64_t data)
{
    FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
    int slot_idx;
#endif

    if (offset < 0) {
	ASE_ERR("Requested offset is not in AFU MMIO region\n");
	ASE_ERR("MMIO Write Error\n");
	raise(SIGABRT);
    } else {
	mmio_t *mmio_pkt;
	mmio_pkt =
	    (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));

	mmio_pkt->write_en = MMIO_WRITE_REQ;
	mmio_pkt->width = MMIO_WIDTH_64;
	mmio_pkt->addr = offset;
	ase_memcpy(mmio_pkt->qword, &data, sizeof(uint64_t));
	mmio_pkt->resp_en = 0;

	// Critical section
	{
	    if (pthread_mutex_lock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("pthread_mutex_lock could not attain lock !\n");
		exit(1);
	    }

	    mmio_pkt->tid = generate_mmio_tid();
#ifdef ASE_DEBUG
	    slot_idx = mmio_request_put(mmio_pkt);
#else
	    mmio_request_put(mmio_pkt);
#endif

	    if (pthread_mutex_unlock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("Mutex unlock failure ... Application Exit here\n");
		exit(1);
	    }
	}

	// Write to MMIO Map
	uint64_t *mmio_vaddr;
	mmio_vaddr =
	    (uint64_t *) ((uint64_t) mmio_afu_vbase + offset);
	*mmio_vaddr = data;


	ASE_MSG
	    ("MMIO Write     : tid = 0x%03x, offset = 0x%x, data = 0x%llx\n",
	     mmio_pkt->tid, mmio_pkt->addr,
	     (unsigned long long) data);


	free(mmio_pkt);
    }

    FUNC_CALL_EXIT;
}



/* *********************************************************************
 * MMIO Read
 * *********************************************************************
 *
 * Request packet
 * ---------------------------------------
 * | MMIO_READ_REQ | MMIO_WIDTH | Offset |
 * ---------------------------------------
 *
 * Response packet
 * -------------------------------------
 * | MMIO_READ_RSP | MMIO_WIDTH | Data |
 * -------------------------------------
 *
 */
/*
 * MMIO Read 32-bit
 */
void mmio_read32(int offset, uint32_t *data32)
{
    FUNC_CALL_ENTRY;
    int slot_idx;

    if (offset < 0) {
	ASE_ERR("Requested offset is not in AFU MMIO region\n");
	ASE_ERR("MMIO Read Error\n");
	raise(SIGABRT);
    } else {
	mmio_t *mmio_pkt;
	mmio_pkt =
	    (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));

	mmio_pkt->write_en = MMIO_READ_REQ;
	mmio_pkt->width = MMIO_WIDTH_32;
	mmio_pkt->addr = offset;
	mmio_pkt->resp_en = 0;

	// Critical section
	{
	    if (pthread_mutex_lock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("pthread_mutex_lock could not attain lock !\n");
		exit(1);
	    }

	    mmio_pkt->tid = generate_mmio_tid();
	    slot_idx = mmio_request_put(mmio_pkt);

	    if (pthread_mutex_unlock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("Mutex unlock failure ... Application Exit here\n");
		exit(1);
	    }
	}

	ASE_MSG("MMIO Read      : tid = 0x%03x, offset = 0x%x\n",
		mmio_pkt->tid, mmio_pkt->addr);

#ifdef ASE_DEBUG
	ASE_DBG("slot_idx = %d\n", slot_idx);
#endif

	// Wait until correct response found
	while (mmio_table[slot_idx].rx_flag != true) {
	    usleep(1);
	}

	// Write data
	*data32 = (uint32_t) mmio_table[slot_idx].data;

	// Display

	ASE_MSG("MMIO Read Resp : tid = 0x%03x, %08x\n",
		mmio_table[slot_idx].tid, (uint32_t) *data32);


	// Reset scoreboard flags
	mmio_table[slot_idx].tx_flag = false;
	mmio_table[slot_idx].rx_flag = false;

	free(mmio_pkt);
    }

    FUNC_CALL_EXIT;
}


/*
 * MMIO Read 64-bit
 */
void mmio_read64(int offset, uint64_t *data64)
{
    FUNC_CALL_ENTRY;
    int slot_idx;

#ifdef ASE_DEBUG
    void *retptr;
#endif

    if (offset < 0) {
	ASE_ERR("Requested offset is not in AFU MMIO region\n");
	ASE_ERR("MMIO Read Error\n");
	raise(SIGABRT);
    } else {
	mmio_t *mmio_pkt;
	mmio_pkt =
	    (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));

	mmio_pkt->write_en = MMIO_READ_REQ;
	mmio_pkt->width = MMIO_WIDTH_64;
	mmio_pkt->addr = offset;
	mmio_pkt->resp_en = 0;

	// Critical section
	{
	    if (pthread_mutex_lock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("pthread_mutex_lock could not attain lock !\n");
		exit(1);
	    }

	    mmio_pkt->tid = generate_mmio_tid();
	    slot_idx = mmio_request_put(mmio_pkt);

	    if (pthread_mutex_unlock(&mmio_port_lock) != 0) {
		ASE_ERR
		    ("Mutex unlock failure ... Application Exit here\n");
		exit(1);
	    }
	}

	ASE_MSG
	    ("MMIO Read      : tid = 0x%03x, offset = 0x%x\n",
	     mmio_pkt->tid, mmio_pkt->addr);

#ifdef ASE_DEBUG
	ASE_DBG("slot_idx = %d\n", slot_idx);
#endif

	// Wait for correct response to be back
	while (mmio_table[slot_idx].rx_flag != true) {
	    usleep(1);
	};

	// Write data
	*data64 = mmio_table[slot_idx].data;

	// Display
	ASE_MSG
	    ("MMIO Read Resp : tid = 0x%03x, data = %llx\n",
	     mmio_table[slot_idx].tid,
	     (unsigned long long) *data64);

	// Reset scoreboard flags
	mmio_table[slot_idx].tx_flag = false;
	mmio_table[slot_idx].rx_flag = false;

	free(mmio_pkt);
    }

    FUNC_CALL_EXIT;
}


/*
 * allocate_buffer: Shared memory allocation and vbase exchange
 * Instantiate a buffer_t structure with given parameters
 * Must be called by ASE_APP
 */
void allocate_buffer(struct buffer_t *mem, uint64_t *suggested_vaddr)
{
    FUNC_CALL_ENTRY;

    // pthread_mutex_trylock (&app_lock);
    int fd_alloc;
    char tmp_msg[ASE_MQ_MSGSIZE] = { 0, };


    ASE_MSG("Attempting to open a shared memory... \n");


    // Buffer is invalid until successfully allocated
    mem->valid = ASE_BUFFER_INVALID;

    // If memory size is not set, then exit !!
    if (mem->memsize <= 0) {
	ASE_ERR
	    ("Memory requested must be larger than 0 bytes... exiting...\n");
	exit(1);
    }
    // Autogenerate a memname, by defualt the first region id=0 will be
    // called "/mmio", subsequent regions will be called strcat("/buf", id)
    // Initially set all characters to NULL
    memset(mem->memname, 0, sizeof(mem->memname));
    if (mem->is_mmiomap == 1) {
	snprintf(mem->memname, ASE_FILENAME_LEN, "/mmio.%s",
		 tstamp_string);
    } else if (mem->is_umas == 1) {
	snprintf(mem->memname, ASE_FILENAME_LEN, "/umas.%s",
		 tstamp_string);
    } else {
	snprintf(mem->memname, ASE_FILENAME_LEN, "/buf%d.%s",
		 userbuf_index_count, tstamp_string);
	userbuf_index_count++;
    }

    // Disable private memory flag
    mem->is_privmem = 0;

    // Obtain a file descriptor for the shared memory region
    // Tue May  5 19:24:21 PDT 2015
    // https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html
    // S_IREAD | S_IWRITE are obselete
    fd_alloc =
	shm_open(mem->memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_alloc < 0) {
	BEGIN_RED_FONTCOLOR;
	perror("shm_open");
	END_RED_FONTCOLOR;
	exit(1);
    }
    // Mmap shared memory region
    if (suggested_vaddr == (uint64_t *) NULL) {
	mem->vbase =
	    (uint64_t) mmap(NULL, mem->memsize,
			    PROT_READ | PROT_WRITE, MAP_SHARED,
			    fd_alloc, 0);
    } else {
	mem->vbase =
	    (uint64_t) mmap(suggested_vaddr, mem->memsize,
			    PROT_READ | PROT_WRITE,
			    MAP_SHARED, fd_alloc, 0);
    }

    // Check
    if (mem->vbase == (uint64_t) MAP_FAILED) {
	BEGIN_RED_FONTCOLOR;
	perror("mmap");
	END_RED_FONTCOLOR;
	ASE_ERR("error string %s", strerror(errno));
	exit(1);
    }
    // Extend memory to required size
    int ret;
    ret = ftruncate(fd_alloc, (off_t) mem->memsize);
#ifdef ASE_DEBUG
    if (ret != 0) {
	ASE_DBG("ftruncate failed");
	BEGIN_RED_FONTCOLOR;
	perror("ftruncate");
	END_RED_FONTCOLOR;

    }
#endif

    // Autogenerate buffer index
    mem->index = asebuf_index_count;
    asebuf_index_count++;
    ASE_MSG("SUCCESS\n");

    // Set buffer as valid
    mem->valid = ASE_BUFFER_VALID;

    // Send an allocate command to DPI, metadata = ASE_MEM_ALLOC
    // mem->metadata = HDR_MEM_ALLOC_REQ;
    mem->next = NULL;

    // Message queue must be enabled when using DPI (else debug purposes only)
    if (mq_exist_status == NOT_ESTABLISHED) {

	ASE_MSG("Session not started --- STARTING now\n");

	session_init();
    }
    // Form message and transmit to DPI
    ase_buffer_t_to_str(mem, tmp_msg);
    mqueue_send(app2sim_alloc_tx, tmp_msg, ASE_MQ_MSGSIZE);

    // Receive message from DPI with pbase populated
    while (mqueue_recv(sim2app_alloc_rx, tmp_msg, ASE_MQ_MSGSIZE) == 0) {	/* wait */
    }
    ase_str_to_buffer_t(tmp_msg, mem);

    // Print out the buffer
#ifdef ASE_DEBUG
    ase_buffer_info(mem);
#endif

    // book-keeping WSmeta // used by ASEALIAFU
    struct wsmeta_t *ws;
    ws = (struct wsmeta_t *) ase_malloc(sizeof(struct wsmeta_t));
    ws->index = mem->index;
    ws->buf_structaddr = (uint64_t *) mem;
    append_wsmeta(ws);

#ifdef ASE_DEBUG
    if (fp_pagetable_log != NULL) {
	if (mem->index % 20 == 0) {
	    fprintf(fp_pagetable_log,
		    "Index\tAppVBase\tASEVBase\tBufsize\tBufname\t\tPhysBase\n");
	}
	fprintf(fp_pagetable_log,
		"%d\t%p\t%p\t%x\t%s\t\t%p\n",
		mem->index,
		(void *) mem->vbase,
		(void *) mem->pbase,
		mem->memsize,
		mem->memname, (void *) mem->fake_paddr);
    }
#endif

    close(fd_alloc);

    FUNC_CALL_EXIT;
}


/*
 * deallocate_buffer : Deallocate a memory region
 * Destroy shared memory regions
 * Called by ASE APP only
 */
void deallocate_buffer(struct buffer_t *mem)
{
    FUNC_CALL_ENTRY;

    int ret;
    char tmp_msg[ASE_MQ_MSGSIZE] = { 0, };

    ASE_MSG("Deallocating memory %s ... \n", mem->memname);

    // Send a one way message to request a deallocate
    ase_buffer_t_to_str(mem, tmp_msg);
    mqueue_send(app2sim_dealloc_tx, tmp_msg, ASE_MQ_MSGSIZE);
    // Wait for response to deallocate
    mqueue_recv(sim2app_dealloc_rx, tmp_msg, ASE_MQ_MSGSIZE);
    ase_str_to_buffer_t(tmp_msg, mem);

    // Unmap the memory accordingly
    ret = munmap((void *) mem->vbase, (size_t) mem->memsize);
    if (0 != ret) {
	BEGIN_RED_FONTCOLOR;
	perror("munmap");
	END_RED_FONTCOLOR;
	exit(1);
    }
    // Print if successful
    ASE_MSG("SUCCESS\n");

    FUNC_CALL_EXIT;
}


/*
 * Appends and maintains a Workspace Meta Linked List (wsmeta_t)
 * <index, buffer_t<vaddr>> linkedlist
 */
void append_wsmeta(struct wsmeta_t *new)
{
    FUNC_CALL_ENTRY;

    if (wsmeta_head == NULL) {
	wsmeta_head = new;
	wsmeta_end = new;
    }

    wsmeta_end->next = new;
    new->next = NULL;
    wsmeta_end = new;
    wsmeta_end->valid = 1;

#ifdef ASE_DEBUG

    struct wsmeta_t *wsptr;
    ASE_DBG("WSMeta traversal START =>\n");
    wsptr = wsmeta_head;
    while (wsptr != NULL) {
	ASE_DBG("\t%d %p %d\n", wsptr->index,
		wsptr->buf_structaddr, wsptr->valid);
	wsptr = wsptr->next;
    }
    ASE_DBG("WSMeta traversal END\n");

#endif

    FUNC_CALL_EXIT;
}


/*
 * deallocate_buffer_by_index:
 * Find a workspace by ID and then call deallocate_buffer
 */
bool deallocate_buffer_by_index(int search_index)
{
    FUNC_CALL_ENTRY;
    bool value;
    uint64_t *bufptr = (uint64_t *) NULL;
    struct wsmeta_t *wsptr;

    ASE_MSG("Deallocate request index = %d ... \n", search_index);

    // Traverse wsmeta_t
    wsptr = wsmeta_head;
    while (wsptr != NULL) {
	if (wsptr->index == search_index) {
	    bufptr = wsptr->buf_structaddr;
	    ASE_DBG("FOUND\n");
	    break;
	} else {
	    wsptr = wsptr->next;
	}
    }


    // Call deallocate
    if ((bufptr != NULL) && (wsptr->valid == 1)) {
	deallocate_buffer((struct buffer_t *) bufptr);
	wsptr->valid = 0;
	value = true;
    } else {
	ASE_MSG("Buffer pointer was returned as NULL\n");
	value = false;
    }

    FUNC_CALL_EXIT;
    return value;
}


/*
 * Traverse WSmeta array and find buffer by WSID
 */
struct buffer_t *find_buffer_by_index(uint64_t wsid)
{
    struct buffer_t *bufptr = (struct buffer_t *) NULL;
    struct wsmeta_t *trav_ptr;

    trav_ptr = wsmeta_head;
    while (trav_ptr != NULL) {
	if (trav_ptr->index == wsid) {
	    bufptr =
		(struct buffer_t *) trav_ptr->buf_structaddr;
	    break;
	} else {
	    trav_ptr = trav_ptr->next;
	}
    }

    if (bufptr == (struct buffer_t *) NULL) {
	ASE_ERR
	    ("find_buffer_by_index: Couldn't find buffer by WSID\n");
    }

    return bufptr;
}


/*
 * UMSG Get Address
 * umsg_get_address: Takes in umsg_id, and returns App virtual address
 */
uint64_t *umsg_get_address(int umsg_id)
{
    uint64_t *ret_vaddr;
    if ((umsg_id >= 0) && (umsg_id < NUM_UMSG_PER_AFU)) {
	ret_vaddr =
	    (uint64_t *) ((uint64_t) umsg_umas_vbase +
			  (uint64_t) ((uint64_t) umsg_id *
				      (ASE_PAGESIZE + 64)));
    } else {
	ret_vaddr = NULL;
	ASE_ERR
	    ("**ERROR** Requested umsg_id out of range... EXITING\n");
	exit(1);
    }
    return ret_vaddr;
}


/*
 * umsg_send: Write data to umsg region
 */
void umsg_send(int umsg_id, uint64_t *umsg_data)
{
    ase_memcpy((char *) umsg_addr_array[umsg_id], (char *) umsg_data,
	       sizeof(uint64_t));
}


/*
 * umsg_set_attribute: Set UMSG Hint setting
 */
void umsg_set_attribute(uint32_t hint_mask)
{
    // Send hint transaction
    ase_portctrl(UMSG_MODE, hint_mask);
}


/*
 * Umsg watcher thread
 * Setup UMSG tracker addresses, and watch for activity
 */
void *umsg_watcher(void *arg)
{
    // Mark as thread that can be cancelled anytime
    pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

    // Generic index
    int cl_index;

    // UMsg old data
    char umsg_old_data[NUM_UMSG_PER_AFU][CL_BYTE_WIDTH];

    // Declare and Allocate umsgcmd_t packet
    umsgcmd_t *umsg_pkt;
    umsg_pkt =
	(struct umsgcmd_t *) ase_malloc(sizeof(struct umsgcmd_t));

    // Patrol each UMSG line
    for (cl_index = 0; cl_index < NUM_UMSG_PER_AFU; cl_index++) {
	// Original copy
	ase_memcpy((char *) umsg_old_data[cl_index],
		   (char *) ((uint64_t) umas_region->vbase +
			     umsg_byteindex_arr[cl_index]),
		   CL_BYTE_WIDTH);

	// Calculate addres
	umsg_addr_array[cl_index] =
	    (char *) ((uint64_t) umas_region->vbase +
		      umsg_byteindex_arr[cl_index]);
#ifdef ASE_DEBUG

	ASE_DBG("umsg_addr_array[%d] = %p\n", cl_index,
		umsg_addr_array[cl_index]);

#endif
    }

    // Set UMsg initialized flag
    umas_init_flag = 1;

    // While application is running
    while (umas_exist_status == ESTABLISHED) {
	// Walk through each line
	for (cl_index = 0; cl_index < NUM_UMSG_PER_AFU; cl_index++) {
	    if (memcmp
		(umsg_addr_array[cl_index],
		 umsg_old_data[cl_index],
		 CL_BYTE_WIDTH) != 0) {
		// Construct UMsg packet
		umsg_pkt->id = cl_index;
		ase_memcpy((char *) umsg_pkt->qword,
			   (char *)
			   umsg_addr_array[cl_index],
			   CL_BYTE_WIDTH);

		// Send UMsg
		mqueue_send(app2sim_umsg_tx,
			    (char *) umsg_pkt,
			    sizeof(struct umsgcmd_t));

		// Update local mirror
		ase_memcpy((char *)
			   umsg_old_data[cl_index],
			   (char *) umsg_pkt->qword,
			   CL_BYTE_WIDTH);
	    }
	}
	usleep(1);
    }

    // Free memory
    ase_free_buffer((char *) umsg_pkt);

    return 0;
}

/*
 * Helper for sending a file descriptor over a socket
 */
static int send_fd(int sock_fd, int fd, struct event_request *req)
{
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];

    memset(buf, 0x0, sizeof(buf));
    struct iovec io = { .iov_base = req, .iov_len = sizeof(req) };

    cmsg = (struct cmsghdr *)buf;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    msg.msg_flags = 0;
    int *fd_ptr = (int *)CMSG_DATA((struct cmsghdr *)buf);
    *fd_ptr = fd;
    if (sendmsg(sock_fd, &msg, 0) == -1) {
	ASE_ERR("error sending message. errno = %s\n", strerror(errno));
	close(sock_fd);
	return 1;
    }
    return 0;
}

/*
 * Register event handle
 */
int register_event(int event_handle, int flags)
{
    struct sockaddr_un saddr;
    errno_t err;
    int res;
    int sock_fd;

    saddr.sun_family = AF_UNIX;
	err = generate_sockname(saddr.sun_path);
	if (err != EOK)
	return 1;
    /* open socket */
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
	ASE_ERR("Error opening socket: %s\n", strerror(errno));
	return 1;
    }
    res = connect(sock_fd, (struct sockaddr *) &saddr,
		  sizeof(struct sockaddr_un));
    if (res < 0) {
	ASE_ERR("%s: Error connecting to stream socket: %s\n",
		__func__, strerror(errno));
	goto out;
    }

    struct event_request req;

    req.type = REGISTER_EVENT;
	req.flags = flags;
    res = send_fd(sock_fd, event_handle, &req);
 out:
    close(sock_fd);
    return res;
}

/*
 * Unregister event
 */
int unregister_event(int event_handle)
{
    struct sockaddr_un saddr;
    int res;
    struct event_request req;
    errno_t err;
    int sock_fd;

	err = generate_sockname(saddr.sun_path);
	if (err != EOK)
	return 1;

    /* open socket */
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
	ASE_ERR("Error opening socket: %s\n", strerror(errno));
	return 1;
    }

    saddr.sun_family = AF_UNIX;
    res =  connect(sock_fd, (struct sockaddr *) &saddr,
		   sizeof(struct sockaddr_un));
    if (res < 0) {
	ASE_ERR("%s: Error connecting to stream socket: %s\n",
		__func__, strerror(errno));
	goto out;
    }

    req.type = UNREGISTER_EVENT;
	res = send_fd(sock_fd, event_handle, &req);

 out:
    close(sock_fd);
    return res;
}

/*
 * ase_portctrl: Send port control message to simulator
 *
 * AFU_RESET   <setting>            | (0,1)
 * UMSG_MODE   <mode_nibbles>[31:0] | (0xF0FF0FF0)
 * ASE_INIT    <dummy number>       | (X)
 * ASE_SIMKILL <dummy number>       | (X)
 *
 */
void __attribute__ ((optimize("O0"))) ase_portctrl(ase_portctrl_cmd command, int value)
{
    char ctrl_msg[ASE_MQ_MSGSIZE] = {0, };
    char rx_msg[ASE_MQ_MSGSIZE] = {0, };

    struct ase_capability_t tmp_cap;

    // construct message
    snprintf(ctrl_msg, ASE_MQ_MSGSIZE, "%d %d", (int)command, value);

    // Send message
    mqueue_send(app2sim_portctrl_req_tx, ctrl_msg, ASE_MQ_MSGSIZE);

    // Receive message
    mqueue_recv(sim2app_portctrl_rsp_rx, rx_msg, ASE_MQ_MSGSIZE);

    // Copy to ase_capability
    ase_memcpy(&tmp_cap, rx_msg, sizeof(struct ase_capability_t));

    // Check capability register integrity
    if (command == ASE_INIT) {
	    // Set Capability register only when ASE_INIT is used
	    ase_memcpy(&ase_capability, &tmp_cap, sizeof(struct ase_capability_t));

	    // Make a check for the magic word
	    if (memcmp(ase_capability.magic_word, ASE_UNIQUE_ID, sizeof(ASE_UNIQUE_ID)) != 0) {
		    // Restore defaults
		    ASE_MSG("ASE Capability register was corrupted, loading defaults\n");
		    ase_capability.umsg_feature = 0;
		    ase_capability.intr_feature = 0;
		    ase_capability.mmio_512bit = 0;
		}

	    // Print ASE Capabilities on console
	    ASE_MSG("ASE Capabilities: Base %s %s %s\n",
		    ase_capability.umsg_feature ? "UMsg" : "",
		    ase_capability.intr_feature ? "Intr" : "",
		    ase_capability.mmio_512bit  ? "MMIO512" : "");
	}
}
