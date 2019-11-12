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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define _GNU_SOURCE

#include <unistd.h>
#include <sys/mman.h>

#include "ase_common.h"
#include "ase_host_memory.h"

const int TID_DELAY = 10000;   // Wait time for generating TID

// UMsg byte offset
const int umsg_byteindex_arr[] = {
	0x0, 0x1040, 0x2080, 0x30C0, 0x4100, 0x5140, 0x6180, 0x71C0
};

typedef struct mmio_s {
	// MMIO Mutex Lock, initilize it here
	pthread_mutex_t mmio_port_lock;

	struct buffer_t *mmio_region;      // CSR map storage

	// MMIO Read response watcher
	int glbl_mmio_tid;       	       // MMIO Tid

	pthread_t mmio_watch_tid;	       // Tracker thread Id
	mmio_t *mmio_rsp_pkt;              // MMIO Response packet handoff control
} MMIO_S;

typedef struct umas_s {
	pthread_t umsg_watch_tid;          // UMsg Watch TID

	struct buffer_t *umas_region;	   // UMAS region

	char *umsg_addr_array[NUM_UMSG_PER_AFU];  // UMsg address array
} UMAS_S;

typedef struct membus_s {
	pthread_t membus_rd_watch_tid;     // Memory bus watch TID
	pthread_t membus_wr_watch_tid;
} MEMBUS_S;

// MMIO Scoreboard (used in APP-side only)
struct mmio_scoreboard_line_t {
	uint64_t   data;
	int        tid;
	bool       tx_flag;
	bool       rx_flag;
};

volatile struct mmio_scoreboard_line_t mmio_table[MMIO_MAX_OUTSTANDING];

// Timestamp char array
char tstamp_string[20];

// Application lock file path
char app_ready_lockpath[ASE_FILEPATH_LEN];

// Session file path
char tstamp_filepath[ASE_FILEPATH_LEN];

// work Directory location
char *ase_workdir_path;

// Time taken calc
struct timespec start_time_snapshot, end_time_snapshot;
unsigned long long runtime_nsec;

static int app2sim_alloc_tx;           // app2sim mesaage queue in RX mode
static int sim2app_alloc_rx;           // sim2app mesaage queue in TX mode
static int app2sim_mmioreq_tx;         // MMIO Request path
static int sim2app_mmiorsp_rx;         // MMIO Response path
static int app2sim_umsg_tx;            // UMSG    message queue in RX mode
static int app2sim_portctrl_req_tx;    // Port Control message in TX mode
static int app2sim_dealloc_tx;
static int sim2app_dealloc_rx;
static int sim2app_portctrl_rsp_rx;
static int sim2app_intr_request_rx;
static int sim2app_membus_rd_req_rx;
static int app2sim_membus_rd_rsp_tx;
static int sim2app_membus_wr_req_rx;
static int app2sim_membus_wr_rsp_tx;

MMIO_S io_s;
UMAS_S umas_s;
MEMBUS_S membus_s;

uint64_t *mmio_afu_vbase;
uint64_t *umsg_umas_vbase;            // Base addresses of required regions

volatile int umas_init_flag;	      // UMAS initialized flag

// Record of MMIO and UMAS memory
struct buffer_t *buf_head = (struct buffer_t *) NULL;
struct buffer_t *buf_end = (struct buffer_t *) NULL;

// Buffer index count
int asebuf_index_count;               // global count/index
int userbuf_index_count;              // User count/index

// ASE Capability register
struct ase_capability_t ase_capability;
static uint32_t session_exist_status;
static uint32_t mq_exist_status;
static uint32_t mmio_exist_status;
static uint32_t umas_exist_status;
static uint32_t membus_exist_status;

static void *membus_rd_watcher(void *arg);
static void *membus_wr_watcher(void *arg);

// Debug logs
#ifdef ASE_DEBUG
FILE *fp_pagetable_log = (FILE *) NULL;
FILE *fp_mmioaccess_log = (FILE *) NULL;
#endif

/*
 * Unmap umas region
 */
void cleanup_umas(void)
{
	// Deallocate the region
	ASE_MSG("Deallocating UMAS\n");
	deallocate_buffer(umas_s.umas_region);
}

/*
 * Unmap mmio regrion
 */
void cleanup_mmio(void)
{
	// Un-mapping CSR region
	ASE_MSG("Deallocating MMIO map\n");

	deallocate_buffer(io_s.mmio_region);
}

/*
 * close message queues
 */
void close_mq(void)
{
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
	mqueue_close(sim2app_intr_request_rx);
	mqueue_close(sim2app_membus_rd_req_rx);
	mqueue_close(app2sim_membus_rd_rsp_tx);
	mqueue_close(sim2app_membus_wr_req_rx);
	mqueue_close(app2sim_membus_wr_rsp_tx);
}

/*
 * Clean up before exit
 */
void failure_cleanup(void)
{
  ASE_ERR("FAILED\n");
  BEGIN_RED_FONTCOLOR;
  perror("pthread_create");
  END_RED_FONTCOLOR;
  cleanup_umas();
  cleanup_mmio();
  close_mq();
  free_buffers();
  ase_exit();
}

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
		usleep(1);
	}

	// Increment and mask
	ret_mmio_tid = io_s.glbl_mmio_tid & MMIO_TID_BITMASK;
	io_s.glbl_mmio_tid++;

	// Return ID
	return ret_mmio_tid;
}

/*
 * THREAD: MMIO Read thread watcher
 */
void *mmio_response_watcher(void *arg)
{
	UNUSED_PARAM(arg);
	// Mark as thread that can be cancelled anytime
	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

	io_s.mmio_rsp_pkt = (struct mmio_t *) ase_malloc(sizeof(struct mmio_t));
	int ret;
	int slot_idx;

#ifdef ASE_DEBUG
	char mmio_type[3];
#endif

	// start watching for messages
	while (mmio_exist_status == ESTABLISHED) {
		ase_memset((void *) io_s.mmio_rsp_pkt, 0xbc, sizeof(mmio_t));

		// If received, update global message
		ret = mqueue_recv(sim2app_mmiorsp_rx, (char *) io_s.mmio_rsp_pkt,
				sizeof(mmio_t));
		if (ret == ASE_MSG_PRESENT) {
#ifdef ASE_DEBUG
			// Logging event
			print_mmiopkt(fp_mmioaccess_log, "Got ",
						io_s.mmio_rsp_pkt);
			if (io_s.mmio_rsp_pkt->write_en == MMIO_WRITE_REQ) {
				ase_string_copy(mmio_type, "WR\0", 3);
			} else if (io_s.mmio_rsp_pkt->write_en == MMIO_READ_REQ) {
				ase_string_copy(mmio_type, "RD\0", 3);
			}

			ASE_DBG("mmio_watcher => %03x, %s, %d, %x, %016llx\n",
				io_s.mmio_rsp_pkt->tid, mmio_type,
				io_s.mmio_rsp_pkt->width, io_s.mmio_rsp_pkt->addr,
				io_s.mmio_rsp_pkt->qword[0]);

#endif

			// Find scoreboard slot number to update
			slot_idx =
				get_scoreboard_slot_by_tid(io_s.mmio_rsp_pkt->tid);

			if (slot_idx == 0xFFFF) {
				ASE_ERR
					("get_scoreboard_slot_by_tid() found a bad slot !");
				raise(SIGABRT);
			} else {
				// MMIO Read response (for credit count only)
				if (io_s.mmio_rsp_pkt->write_en == MMIO_READ_REQ) {
					mmio_table[slot_idx].tid =
						io_s.mmio_rsp_pkt->tid;
					mmio_table[slot_idx].data =
						io_s.mmio_rsp_pkt->qword[0];
					mmio_table[slot_idx].tx_flag =
						true;
					mmio_table[slot_idx].rx_flag =
						true;
				} else if (io_s.mmio_rsp_pkt->write_en == MMIO_WRITE_REQ) {
					// MMIO Write response (for credit count only)
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
	UNUSED_PARAM(n);
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

	int rc = 0;

	// Start clock
	clock_gettime(CLOCK_MONOTONIC, &start_time_snapshot);

	// Session setup
	if (session_exist_status != ESTABLISHED) {
		set_loglevel(ase_calc_loglevel());
		setvbuf(stdout, NULL, (int)_IONBF, (size_t)0);
		ase_eval_session_directory();
		ipc_init();
		// Initialize ase_workdir_path
		ASE_MSG("ASE Session Directory located at =>\n");
		ASE_MSG("%s\n", ase_workdir_path);

		// Generate Timestamp filepath
		snprintf(tstamp_filepath, ASE_FILEPATH_LEN, "%s/%s", ase_workdir_path, TSTAMP_FILENAME);

		// Craft a .app_lock.pid lock filepath string
		ase_memset(app_ready_lockpath, 0, ASE_FILEPATH_LEN);
		snprintf(app_ready_lockpath, ASE_FILEPATH_LEN, "%s/%s",
			ase_workdir_path, APP_LOCK_FILENAME);

		// Check if .app_lock.pid lock already exists or not.
		if (check_app_lock_file(app_ready_lockpath)) {
			//If .app_lock.pid exists but pid doesnt exist.
			if (!remove_existing_lock_file(app_ready_lockpath)) {
				ASE_MSG("Application Exiting \n");
				exit(1);
			}
		}
		create_new_lock_file(app_ready_lockpath);

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

		// Memory read/write requests.	All simulated references to shared memory are
		// handled by the application.
		sim2app_membus_rd_req_rx =
			mqueue_open(mq_array[10].name, mq_array[10].perm_flag);
		app2sim_membus_rd_rsp_tx =
			mqueue_open(mq_array[11].name, mq_array[11].perm_flag);
		sim2app_membus_wr_req_rx =
			mqueue_open(mq_array[12].name, mq_array[12].perm_flag);
		app2sim_membus_wr_rsp_tx =
			mqueue_open(mq_array[13].name, mq_array[13].perm_flag);

		// Message queues have been established
		mq_exist_status = ESTABLISHED;

		// Issue soft reset
		send_swreset();

		// Page table tracker (optional logger)
		if (ase_host_memory_initialize()) {
			ASE_ERR("Error initializing simulated host memory.\n");
		}
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
		io_s.glbl_mmio_tid = 0;

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

		get_timestamp(tstamp_string);

		// Creating CSR map
		ASE_MSG("Creating MMIO ...\n");

		io_s.mmio_region = (struct buffer_t *)
			ase_malloc(sizeof(struct buffer_t));
		io_s.mmio_region->memsize = MMIO_LENGTH;
		io_s.mmio_region->is_mmiomap = 1;
		allocate_buffer(io_s.mmio_region, NULL);
		mmio_afu_vbase = (uint64_t *)((uint64_t)io_s.mmio_region->vbase +
			MMIO_AFU_OFFSET);
		mmio_exist_status = ESTABLISHED;

		ASE_MSG("AFU MMIO Virtual Base Address = %p\n",
			(void *)mmio_afu_vbase);


		// Create UMSG region
		umas_init_flag = 0;
		ASE_MSG("Creating UMAS ... \n");

		umas_s.umas_region = (struct buffer_t *)ase_malloc(sizeof(struct buffer_t));
		umas_s.umas_region->memsize = UMAS_REGION_MEMSIZE;    //UMAS_LENGTH;
		umas_s.umas_region->is_umas = 1;
		allocate_buffer(umas_s.umas_region, NULL);
		umsg_umas_vbase = (uint64_t *)((uint64_t)umas_s.umas_region->vbase);
		umas_exist_status = ESTABLISHED;
		umsg_set_attribute(0x0);
		ASE_MSG("UMAS Virtual Base address = %p\n",
			(void *)umsg_umas_vbase);

		// Start MMIO read response watcher watcher thread
		ASE_MSG("Starting MMIO Read Response watcher ... \n");
		rc = pthread_mutex_init(&io_s.mmio_port_lock, NULL);
		if (rc != 0)
			ASE_ERR("Failed to initialize the pthread_mutex_lock\n");
		thr_err = pthread_create(&io_s.mmio_watch_tid, NULL,
			&mmio_response_watcher, NULL);
		if (thr_err != 0) {
			failure_cleanup();
		} else {
			ASE_MSG("SUCCESS\n");
		}

		ASE_MSG("Starting UMsg watcher ... \n");

		// Initiate UMsg watcher
		thr_err = pthread_create(&umas_s.umsg_watch_tid, NULL, &umsg_watcher, NULL);
		if (thr_err != 0) {
			failure_cleanup();
		} else {
			ASE_MSG("SUCCESS\n");
		}

		// Initiate memory bus watcher
		ASE_MSG("Starting memory bus watcher ... \n");
		membus_exist_status = ESTABLISHED;
		thr_err = pthread_create(&membus_s.membus_rd_watch_tid, NULL, &membus_rd_watcher, NULL);
		if (thr_err != 0) {
			failure_cleanup();
		} else {
			ASE_MSG("RD SUCCESS\n");
		}
		thr_err = pthread_create(&membus_s.membus_wr_watch_tid, NULL, &membus_wr_watcher, NULL);
		if (thr_err != 0) {
			failure_cleanup();
		} else {
			ASE_MSG("WR SUCCESS\n");
		}

		while (umas_init_flag != 1)
			usleep(1);

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

/*
 * Create New Lock File
 */
void create_new_lock_file(char *filename)
{
	FILE *fp_app_lockfile;

	// Open lock file for writing
	fp_app_lockfile = fopen(filename, "w");
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

/*
 * Check for access to .app_lock_pid
 */
bool check_app_lock_file(char *filename)
{
	if (access(filename, F_OK) == 0)
		return true;
	else
		return false;
};

/*
 * Remove Lock File
 */
bool remove_existing_lock_file(const char *filename)
{
	pid_t lock;
	FILE *fp_app_lockfile;

	// Read the PID of the running application
	fp_app_lockfile = fopen(filename, "r");

	if (fp_app_lockfile == NULL) {
		ASE_ERR("Error opening Application lock file path, EXITING\n");
		return false;
	}

	if (fscanf_s_i(fp_app_lockfile, "%d\n", &lock) != 0) {
		// Check if PID exists
		kill(lock, 0);
		if (errno == ESRCH) {
			ASE_MSG
				("ASE found a stale Application lock with PID = %d -- this will be removed\n",
					lock);
			fclose(fp_app_lockfile);
			// Delete lock file
			delete_lock_file(filename);
			return true;
		} else if (errno == EPERM) {
			ASE_ERR ("Application does not have permission to remove $ASE_WORKDIR/.app_lock.pid \n");
		} else
				ASE_ERR("ASE session in env(ASE_WORKDIR) is currently used by PID=%d\n", lock);
	} else {
		ASE_ERR("Error reading PID of application using ASE, EXITING\n"
				"ASE was found to be running with another application !\n\n"
				"If you think this is in error:\n"
				" - Manually delete $ASE_WORKDIR/.app_lock.pid file\n"
				" - Close any ASE simulator is running from the $ASE_WORKDIR directory\n");
	}
	fclose(fp_app_lockfile);
	return false;
}

/*
 * Delete app_lock file for non-existent processes.
 */
void delete_lock_file(const char *filename)
{
	if (unlink(filename) == 0)
		ASE_INFO("Deleted the existing app_lock.pid with Stale pid \n");
	else {
		ASE_ERR("Application Lock file could not be removed, please remove manually from $ASE_WORKDIR/.app_lock.pid \n");
		ase_exit();
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
			pthread_cancel(umas_s.umsg_watch_tid);
			cleanup_umas();
		}
#ifdef ASE_DEBUG
		else {
			ASE_MSG("No UMAS established\n");
		}
#endif
		// Um-mapping CSR region
		ASE_MSG("Deallocating MMIO map\n");
		if (mmio_exist_status == ESTABLISHED) {
			cleanup_mmio();
			mmio_exist_status = NOT_ESTABLISHED;

			// Close MMIO Response tracker thread
			if (pthread_cancel(io_s.mmio_watch_tid) != 0) {
				fprintf(stderr, "MMIO pthread_cancel failed -- Ignoring\n");
			}
		}

		// Stop memory bus watcher
		ASE_MSG("Stopping memory bus watcher\n");
		if (membus_exist_status == ESTABLISHED) {
			membus_exist_status = NOT_ESTABLISHED;

			if (pthread_cancel(membus_s.membus_rd_watch_tid) != 0) {
				fprintf(stderr, "Memory bus pthread_cancel failed -- Ignoring\n");
			} else {
				pthread_join(membus_s.membus_rd_watch_tid, NULL);
			}

			if (pthread_cancel(membus_s.membus_wr_watch_tid) != 0) {
				fprintf(stderr, "Memory bus pthread_cancel failed -- Ignoring\n");
			} else {
				pthread_join(membus_s.membus_wr_watch_tid, NULL);
			}
		}

		//free memory
		free_buffers();

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
	close_mq();

	// Lock deinit
	if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
		ASE_MSG("Trying to shutdown mutex unlock\n");
	}

	// Stop running threads
	pthread_cancel(umas_s.umsg_watch_tid);
	pthread_join(umas_s.umsg_watch_tid, NULL);
	pthread_cancel(io_s.mmio_watch_tid);
	pthread_join(io_s.mmio_watch_tid, NULL);

	if (io_s.mmio_rsp_pkt) {
		free(io_s.mmio_rsp_pkt);
		io_s.mmio_rsp_pkt = NULL;
	}

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
	} else {
		ASE_ERR
			("ASE Error generating MMIO TID, simulation cannot proceed !\n");
		raise(SIGABRT);
	}

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
 * Deinitialize before exit
 */
void exit_cleanup(void)
{
  session_deinit();
  ase_exit();
}


/*
 * MMIO Write 32-bit
 */
void mmio_write32(int offset, uint32_t data)
{
	FUNC_CALL_ENTRY;

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
		if (pthread_mutex_lock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("pthread_mutex_lock could not attain lock !\n");
			exit_cleanup();
		}

		mmio_pkt->tid = generate_mmio_tid();
		mmio_request_put(mmio_pkt);

		if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR
				("Mutex unlock failure ... Application Exit here\n");
			session_deinit();
			exit(1);
		}

		// Write to MMIO map
		uint32_t *mmio_vaddr;
		mmio_vaddr =
			(uint32_t *) ((uint64_t) mmio_afu_vbase + offset);
		ase_memcpy(mmio_vaddr, (char *) &data, sizeof(uint32_t));

		// Display
		ASE_MSG("MMIO Write     : tid = 0x%03x, offset = 0x%x, data = 0x%08x\n",
			mmio_pkt->tid, mmio_pkt->addr, data);

		free(mmio_pkt);
		mmio_pkt = NULL;
	}

	FUNC_CALL_EXIT;
}


/*
 * MMIO Write 64-bit
 */
void mmio_write64(int offset, uint64_t data)
{
	FUNC_CALL_ENTRY;

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
		if (pthread_mutex_lock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("pthread_mutex_lock could not attain lock !\n");
			exit_cleanup();
		}

		mmio_pkt->tid = generate_mmio_tid();
		mmio_request_put(mmio_pkt);

		if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("Mutex unlock failure ... Application Exit here\n");
			exit_cleanup();
		}

		// Write to MMIO Map
		uint64_t *mmio_vaddr;
		mmio_vaddr =
		(uint64_t *) ((uint64_t) mmio_afu_vbase + offset);
		*mmio_vaddr = data;


		ASE_MSG("MMIO Write     : tid = 0x%03x, offset = 0x%x, data = 0x%llx\n",
			mmio_pkt->tid, mmio_pkt->addr,
			(unsigned long long) data);

		free(mmio_pkt);
		mmio_pkt = NULL;
	}

	FUNC_CALL_EXIT;
}


/*
 * MMIO Write 512-bit
 */
void mmio_write512(int offset, const void *data)
{
	FUNC_CALL_ENTRY;

	if (offset < 0) {
		ASE_ERR("Requested offset is not in AFU MMIO region\n");
		ASE_ERR("MMIO Write Error\n");
		raise(SIGABRT);
	} else {
		mmio_t *mmio_pkt;
		mmio_pkt =
			(struct mmio_t *) ase_malloc(sizeof(struct mmio_t));

		mmio_pkt->write_en = MMIO_WRITE_REQ;
		mmio_pkt->width = MMIO_WIDTH_512;
		mmio_pkt->addr = offset;
		ase_memcpy(mmio_pkt->qword, data, 64);
		mmio_pkt->resp_en = 0;

		// Critical Section
		if (pthread_mutex_lock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("pthread_mutex_lock could not attain lock !\n");
			exit_cleanup();
		}

		mmio_pkt->tid = generate_mmio_tid();
		mmio_request_put(mmio_pkt);

		if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR
				("Mutex unlock failure ... Application Exit here\n");
			session_deinit();
			exit(1);
		}

		// Write to MMIO map
		void *mmio_vaddr;
		mmio_vaddr =
			(void *) ((uint64_t) mmio_afu_vbase + offset);
		ase_memcpy(mmio_vaddr, (char *) &data, 64);

		// Display
		ASE_MSG("MMIO Write     : tid = 0x%03x, offset = 0x%x, data = 0x%08x\n",
			mmio_pkt->tid, mmio_pkt->addr, data);

		free(mmio_pkt);
		mmio_pkt = NULL;
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
		if (pthread_mutex_lock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("pthread_mutex_lock could not attain lock !\n");
			exit_cleanup();
		}

		mmio_pkt->tid = generate_mmio_tid();
		slot_idx = mmio_request_put(mmio_pkt);

		if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("Mutex unlock failure ... Application Exit here\n");
			exit_cleanup();
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
		mmio_pkt = NULL;
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
		if (pthread_mutex_lock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("pthread_mutex_lock could not attain lock !\n");
			exit_cleanup();
		}

		mmio_pkt->tid = generate_mmio_tid();
		slot_idx = mmio_request_put(mmio_pkt);

		if (pthread_mutex_unlock(&io_s.mmio_port_lock) != 0) {
			ASE_ERR("Mutex unlock failure ... Application Exit here\n");
			exit_cleanup();
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
		mmio_pkt = NULL;
	}

	FUNC_CALL_EXIT;
}

/*
 * Shared memory mapping error handling
 */
void shm_error(const char *msg)
{
  BEGIN_RED_FONTCOLOR;
  perror(msg);
  END_RED_FONTCOLOR;
  session_deinit();
  ase_exit();
}

/*
 * Note pinned pages. This is used only for logging in the simulator.
 */
void note_pinned_page(uint64_t va, uint64_t iova, uint64_t length)
{
	struct buffer_t mem;
	ase_memset(&mem, 0, sizeof(mem));
	mem.is_pinned = true;
	mem.valid = true;
	mem.vbase = va;
	mem.fake_paddr = iova;
	mem.memsize = length;

	char tmp_msg[ASE_MQ_MSGSIZE] = { 0, };
	ase_buffer_t_to_str(&mem, tmp_msg);
	mqueue_send(app2sim_alloc_tx, tmp_msg, ASE_MQ_MSGSIZE);
}

void note_unpinned_page(uint64_t iova, uint64_t length)
{
	struct buffer_t mem;
	ase_memset(&mem, 0, sizeof(mem));
	mem.is_pinned = true;
	mem.fake_paddr = iova;
	mem.memsize = length;

	char tmp_msg[ASE_MQ_MSGSIZE] = { 0, };
	ase_buffer_t_to_str(&mem, tmp_msg);
	mqueue_send(app2sim_dealloc_tx, tmp_msg, ASE_MQ_MSGSIZE);
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
		session_deinit();
		exit(1);
	}
	// Autogenerate a memname, by defualt the first region id=0 will be
	// called "/mmio", subsequent regions will be called strcat("/buf", id)
	// Initially set all characters to NULL
	ase_memset(mem->memname, 0, sizeof(mem->memname));
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
	fd_alloc = shm_open(mem->memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd_alloc < 0) {
		shm_error("shm_open");
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
		ASE_ERR("error string %s", strerror(errno));
		shm_error("mmap");
	}
#ifdef ASE_DEBUG
	// Extend memory to required size
	int ret;
	ret = ftruncate(fd_alloc, (off_t) mem->memsize);
	if (ret != 0) {
		ASE_DBG("ftruncate failed");
		BEGIN_RED_FONTCOLOR;
		perror("ftruncate");
		END_RED_FONTCOLOR;

	}
#endif

	// Assign a simulated physical address
	mem->fake_paddr = ase_host_memory_va_to_pa(mem->vbase, mem->memsize);

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
	while (mqueue_recv(sim2app_alloc_rx, tmp_msg, ASE_MQ_MSGSIZE) == 0)
		usleep(1);

	ase_str_to_buffer_t(tmp_msg, mem);

	// Print out the buffer
#ifdef ASE_DEBUG
	ase_buffer_info(mem);
#endif

	append_buf(mem); // keep records of allocated buffers

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
	struct buffer_t *mem_next;  // Keep current buffer's pointer to next

	ASE_MSG("Deallocating memory %s ... \n", mem->memname);

	// Send a one way message to request a deallocate
	mem_next = mem->next;
	ase_buffer_t_to_str(mem, tmp_msg);
	mqueue_send(app2sim_dealloc_tx, tmp_msg, ASE_MQ_MSGSIZE);
	// Wait for response to deallocate
	mqueue_recv(sim2app_dealloc_rx, tmp_msg, ASE_MQ_MSGSIZE);
	ase_str_to_buffer_t(tmp_msg, mem);
	mem->next = mem_next;

	// Unmap the memory accordingly
	ret = munmap((void *) mem->vbase, (size_t) mem->memsize);
	if (0 != ret) {
		shm_error("munmap");
	}
	mem->valid = ASE_BUFFER_INVALID;

	// Print if successful
	ASE_MSG("SUCCESS\n");

	FUNC_CALL_EXIT;
}

/*
 * Appends and maintains a buffer Linked List (buffer_t)
 * <index, buffer_t<vaddr>> linkedlist
 */
void append_buf(struct buffer_t *buf)
{
	FUNC_CALL_ENTRY;

	if (buf_head == NULL) {
		buf_head = buf;
		buf_end = buf;
		buf->next = NULL;
	} else {

		buf_end->next = buf;
		buf->next = NULL;
		buf_end = buf;
	}
#ifdef ASE_DEBUG

	struct buffer_t *ptr;
	ASE_DBG("Buffer traversal START =>\n");
	ptr = buf_head;
	while (ptr != NULL) {
		ASE_DBG("\t%d %p %d\n", ptr->index,
			ptr, ptr->valid);
		ptr = ptr->next;
	}
	ASE_DBG("Buffer traversal END\n");

#endif

	FUNC_CALL_EXIT;
}

/*
 * Clean up the memory allocated for buffer_t
 */
void free_buffers(void)
{
	FUNC_CALL_ENTRY;

	struct buffer_t *bufptr = (struct buffer_t *) NULL;
	struct buffer_t *ptr;

	ASE_MSG("Deallocate all buffers ... \n");

	// Traverse buffer list
	ptr = buf_head;
	while (ptr != NULL) {
		bufptr = ptr;
		ptr = ptr->next;
		free(bufptr);
	}

	buf_head = NULL;
	buf_end = NULL;

	ase_host_memory_terminate();

	asebuf_index_count = 0;
	userbuf_index_count = 0;

	FUNC_CALL_EXIT;
}

/*
 * UMSG Get Address
 * umsg_get_address: Takes in umsg_id, and returns App virtual address
 */
/* uint64_t *umsg_get_address(int umsg_id)
{
	uint64_t *ret_vaddr;
	if ((umsg_id >= 0) && (umsg_id < NUM_UMSG_PER_AFU)) {
		ret_vaddr = (uint64_t *) ((uint64_t) umsg_umas_vbase +
					(uint64_t) ((uint64_t) umsg_id *
							(ASE_PAGESIZE + 64)));
	} else {
		ret_vaddr = NULL;
		ASE_ERR
			("**ERROR** Requested umsg_id out of range... EXITING\n");
		session_deinit();
		exit(1);
	}
	return ret_vaddr;
} */


/*
 * umsg_send: Write data to umsg region
 */
/*void umsg_send(int umsg_id, uint64_t *umsg_data)
{
	ase_memcpy((char *) umsg_addr_array[umsg_id], (char *) umsg_data,
		   sizeof(uint64_t));
}*/


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
	UNUSED_PARAM(arg);
	// Mark as thread that can be cancelled anytime
	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

	// Generic index
	int cl_index;

	// UMsg old data
	char umsg_old_data[NUM_UMSG_PER_AFU][CL_BYTE_WIDTH];

	// Declare and Allocate umsgcmd_t packet
	umsgcmd_t *umsg_pkt;
	umsg_pkt = (struct umsgcmd_t *) ase_malloc(sizeof(struct umsgcmd_t));

	// Patrol each UMSG line
	for (cl_index = 0; cl_index < NUM_UMSG_PER_AFU; cl_index++) {
		// Original copy
		ase_memcpy((char *) umsg_old_data[cl_index],
				(char *) ((uint64_t) umas_s.umas_region->vbase +
					umsg_byteindex_arr[cl_index]),
					CL_BYTE_WIDTH);

		// Calculate addres
		umas_s.umsg_addr_array[cl_index] =
			(char *) ((uint64_t) umas_s.umas_region->vbase +
				umsg_byteindex_arr[cl_index]);
#ifdef ASE_DEBUG

		ASE_DBG("umas_s.umsg_addr_array[%d] = %p\n", cl_index,
			umas_s.umsg_addr_array[cl_index]);

#endif
	}

	// Set UMsg initialized flag
	umas_init_flag = 1;

	// While application is running
	while (umas_exist_status == ESTABLISHED) {
		// Walk through each line
		for (cl_index = 0; cl_index < NUM_UMSG_PER_AFU; cl_index++) {
			if (memcmp
				(umas_s.umsg_addr_array[cl_index],
				 umsg_old_data[cl_index],
				 CL_BYTE_WIDTH) != 0) {
				// Construct UMsg packet
				umsg_pkt->id = cl_index;
				ase_memcpy((char *) umsg_pkt->qword,
						(char *)
						umas_s.umsg_addr_array[cl_index],
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

STATIC ase_host_memory_status membus_op_status(uint64_t va, uint64_t pa)
{
	ase_host_memory_status st;

	static uint64_t page_mask;
	if (!page_mask) {
		page_mask = sysconf(_SC_PAGESIZE);
		page_mask = ~(page_mask - 1);
	}

	if (pa & 0x3f) {
		// Not line-aligned address
		st = HOST_MEM_STATUS_ILLEGAL;
	} else if (va == 0) {
		st = HOST_MEM_STATUS_NOT_PINNED;
	} else {
		// We use mincore to detect whether the virtual address is mapped.
		// mincore returns an error when it isn't. This will detect most cases
		// where the user code pins a page and subsequently unmaps the
		// page without unpinnning it with fpgaReleaseBuffer() first.
		//
		// There is still a race here. The user code could unmap the page
		// after this check and before the simulator reads or writes the
		// location. The race is short and there isn't much we can do other
		// than raise a SEGV.
		unsigned char vec[8];
		if (mincore((void *)(va & page_mask), CL_BYTE_WIDTH, vec)) {
			st = HOST_MEM_STATUS_NOT_MAPPED;
		} else {
			st = HOST_MEM_STATUS_VALID;
		}
	}

	return st;
}


/*
 * Service simulator memory read/write requests.
 */
static void *membus_rd_watcher(void *arg)
{
	UNUSED_PARAM(arg);
	// Mark as thread that can be cancelled anytime
	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

	ase_host_memory_read_req rd_req;
	ase_host_memory_read_rsp rd_rsp;
	ase_memset(&rd_rsp, 0, sizeof(rd_rsp));

	// While application is running
	while (membus_exist_status == ESTABLISHED) {
		if (mqueue_recv(sim2app_membus_rd_req_rx, (char *) &rd_req, sizeof(rd_req)) == ASE_MSG_PRESENT) {
			rd_rsp.pa = rd_req.addr;
			rd_rsp.va = ase_host_memory_pa_to_va(rd_req.addr, true);
			rd_rsp.status = membus_op_status(rd_rsp.va, rd_rsp.pa);
			if (rd_rsp.status == HOST_MEM_STATUS_VALID) {
				ase_memcpy(rd_rsp.data, (char *) rd_rsp.va, CL_BYTE_WIDTH);
			}
			ase_host_memory_unlock();

			mqueue_send(app2sim_membus_rd_rsp_tx, (char *) &rd_rsp, sizeof(rd_rsp));
		}
	}

	printf("Stop MEMBUS RD REQ\n");

	return 0;
}

static void *membus_wr_watcher(void *arg)
{
	UNUSED_PARAM(arg);
	// Mark as thread that can be cancelled anytime
	pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);

	ase_host_memory_write_req wr_req;
	ase_host_memory_write_rsp wr_rsp;
	ase_memset(&wr_rsp, 0, sizeof(wr_rsp));

	// While application is running
	while (membus_exist_status == ESTABLISHED) {
		if (mqueue_recv(sim2app_membus_wr_req_rx, (char *) &wr_req, sizeof(wr_req)) == ASE_MSG_PRESENT) {
			wr_rsp.pa = wr_req.addr;
			wr_rsp.va = ase_host_memory_pa_to_va(wr_req.addr, true);
			wr_rsp.status = membus_op_status(wr_rsp.va, wr_rsp.pa);
			if (wr_rsp.status == HOST_MEM_STATUS_VALID) {
				char *dst = (char *) wr_rsp.va;
				char *src = (char *) wr_req.data;
				size_t len = CL_BYTE_WIDTH;

				// Partial write? Figure out the region to copy.
				if (wr_req.byte_en) {
					dst += wr_req.byte_start;
					src += wr_req.byte_start;
					len = wr_req.byte_len;
					if ((src + len - (char *) wr_req.data) > CL_BYTE_WIDTH) {
						ASE_ERR("Byte range is outside of cache line!");
						ase_exit();
					}
				}

				ase_memcpy(dst, src, len);
			}
			ase_host_memory_unlock();

			mqueue_send(app2sim_membus_wr_rsp_tx, (char *) &wr_rsp, sizeof(wr_rsp));
		}
	}

	printf("Stop MEMBUS WR REQ\n");

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

	ase_memset(buf, 0x0, sizeof(buf));
	struct iovec io = { .iov_base = req, .iov_len = sizeof(struct event_request *) };

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
	int *fd_ptr = (int *)CMSG_DATA(cmsg);
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
	int res;
	int sock_fd;

	saddr.sun_family = AF_UNIX;
	res = generate_sockname(saddr.sun_path);
	if (res < 0) {
		return 1;
	}
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
	} else {
		struct event_request req;

		req.type = REGISTER_EVENT;
		req.flags = flags;
		res = send_fd(sock_fd, event_handle, &req);
	}

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
	int sock_fd;

	res = generate_sockname(saddr.sun_path);
	if (res < 0) {
		return 1;
	}

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
	} else {
		ase_memset(&req, 0, sizeof(req));
		req.type = UNREGISTER_EVENT;
		res = send_fd(sock_fd, event_handle, &req);
	}

	close(sock_fd);
	return res;
}

/*
 * ase_portctrl: Send port control message to simulator
 *
 * AFU_RESET   <setting>			| (0,1)
 * UMSG_MODE   <mode_nibbles>[31:0] | (0xF0FF0FF0)
 * ASE_INIT	<dummy number>	   | (X)
 * ASE_SIMKILL <dummy number>	   | (X)
 *
 */
void __attribute__ ((optimize("O0"))) ase_portctrl(ase_portctrl_cmd command, int value)
{
	char ctrl_msg[ASE_MQ_MSGSIZE]  = { 0 };
	char rx_msg[ASE_MQ_MSGSIZE] = { 0 };

	// ASE Capability register
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
