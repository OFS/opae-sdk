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

#ifndef _ASE_COMMON_H_
#define _ASE_COMMON_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/eventfd.h>
#include <dirent.h>
#include <execinfo.h>
#include <locale.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <uuid/uuid.h>
#include <pthread.h>

#ifdef SIM_SIDE
#include "svdpi.h"
#endif

#ifndef SIM_SIDE
#define APP_SIDE
#endif

/*
 * ASE Unique ID Check
 */
#define ASE_UNIQUE_ID "SR-6.4.0-7325e31"

#define UNUSED_PARAM(x) ((void)x)

/*
 * Return integers
 */
#define OK     0
#define NOT_OK -1

/* *******************************************************************************
 *
 * SYSTEM FACTS
 *
 * *******************************************************************************/
/* #define FPGA_ADDR_WIDTH       48 */
/* #define PHYS_ADDR_PREFIX_MASK 0x0000FFFFFFE00000 */
#define CL_ALIGN          6
#define MEMBUF_2MB_ALIGN  21

// Width of a cache line in bytes
#define CL_BYTE_WIDTH        64
#define SIZEOF_1GB_BYTES     ((uint64_t)pow(1024, 3))

// Size of page
#define ASE_PAGESIZE         0x1000	// 4096 bytes
#define CCI_CHUNK_SIZE       (2*1024*1024)	// CCI 2 MB physical chunks

//MMIO memory map size
#define MMIO_LENGTH                (512*1024)	// 512 KB MMIO size
#define MMIO_AFU_OFFSET            (256*1024)

// MMIO Tid width
#define MMIO_TID_BITWIDTH          9
#define MMIO_TID_BITMASK           (uint32_t)(pow((uint32_t)2, MMIO_TID_BITWIDTH)-1)
#define MMIO_MAX_OUTSTANDING       64

// Number of UMsgs per AFU
#define NUM_UMSG_PER_AFU           8

// UMAS region
#define UMAS_LENGTH                (NUM_UMSG_PER_AFU * ASE_PAGESIZE)
#define UMAS_REGION_MEMSIZE        (2*1024*1024)

// User clock default
#define DEFAULT_USR_CLK_MHZ        312.500
#define DEFAULT_USR_CLK_TPS        (int)(1E+12/(DEFAULT_USR_CLK_MHZ*pow(1000, 2)));

// Max number of user interrupts
#define MAX_USR_INTRS              4

/*
 * ASE Debug log-level
 * -------------------
 * Two log levels are supported in ASE, controlled by env(ASE_LOG)
 * - ASE_LOG=0  | ASE_LOG_SILENT  : Only INFO, ERR messages are posted
 * - ASE_LOG!=0 | ASE_LOG_MESSAGE : All MSG, INFO, ERR messages are posted
 */
enum ase_loglevel {
	ASE_LOG_ERROR     = -3,
	ASE_LOG_INFO_2    = -2,
	ASE_LOG_INFO      = -1,
	ASE_LOG_SILENT    = 0,
	ASE_LOG_MESSAGE,
	ASE_LOG_DEBUG
};

int get_loglevel(void);
void set_loglevel(int level);

int ase_calc_loglevel(void);
void ase_print(int loglevel, char *fmt, ...);
int generate_sockname(char *);

#ifdef SIM_SIDE
#define LOG_PREFIX "  [SIM]  "
#else
#define LOG_PREFIX "  [APP]  "
#endif

// Shorten filename
#ifdef __SHORTEN_FILE__
#undef __SHORTEN_FILE__
#endif // __SHORT_FILE__
#define __SHORTEN_FILE__             \
({	const char *file = __FILE__; \
	const char *p    = file;     \
	while (*p)                   \
		++p;                 \
	while ((p > file) &&         \
	       ('/' != *p) &&        \
	       ('\\' != *p))         \
		--p;                 \
	if (p > file)                \
		++p;                 \
	p;                           \
})

#ifdef ASE_ERR
#undef ASE_ERR
#endif
#ifdef ASE_DEBUG
#define ASE_ERR(format, ...)					\
	ase_print(ASE_LOG_ERROR, LOG_PREFIX "%s:%u:%s()\t" format, __SHORTEN_FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define ASE_ERR(format, ...)					\
	ase_print(ASE_LOG_ERROR, LOG_PREFIX format, ## __VA_ARGS__)
#endif

#ifdef ASE_INFO
#undef ASE_INFO
#endif
#ifdef ASE_DEBUG
#define ASE_INFO(format, ...)					\
	ase_print(ASE_LOG_INFO, LOG_PREFIX "%s:%u:%s()\t" format, __SHORTEN_FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define ASE_INFO(format, ...)					\
	ase_print(ASE_LOG_INFO, LOG_PREFIX format, ## __VA_ARGS__)
#endif

#ifdef ASE_INFO_2
#undef ASE_INFO_2
#endif
#ifdef ASE_DEBUG
#define ASE_INFO_2(format, ...)					\
	ase_print(ASE_LOG_INFO_2, LOG_PREFIX "%s:%u:%s()\t" format, __SHORTEN_FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define ASE_INFO_2(format, ...)					\
	ase_print(ASE_LOG_INFO_2, LOG_PREFIX format, ## __VA_ARGS__)
#endif

#ifdef ASE_MSG
#undef ASE_MSG
#endif
#ifdef ASE_DEBUG
#define ASE_MSG(format, ...)					\
	ase_print(ASE_LOG_MESSAGE, LOG_PREFIX "%s:%u:%s()\t" format, __SHORTEN_FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define ASE_MSG(format, ...)						\
	ase_print(ASE_LOG_MESSAGE, LOG_PREFIX format, ## __VA_ARGS__)
#endif

#ifdef ASE_DBG
#undef ASE_DBG
#endif
#define ASE_DBG(format, ...)						\
	ase_print(ASE_LOG_DEBUG, LOG_PREFIX "%s:%u:%s()\t" format, __SHORTEN_FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)

/*
 * ASE INTERNAL MACROS
 * -------------------
 * Controls file, path lengths
 *
 */
// SHM memory name length
#define ASE_FILENAME_LEN        40

// ASE filepath length
#define ASE_FILEPATH_LEN        256

// ASE logger len
#define ASE_LOGGER_LEN          1024

// Timestamp session code length
#define ASE_SESSION_CODE_LEN    20

// work Directory location
extern char *ase_workdir_path;

// Timestamp IPC file
#define TSTAMP_FILENAME ".ase_timestamp"
extern char tstamp_filepath[ASE_FILEPATH_LEN];

// READY file name
#define ASE_READY_FILENAME ".ase_ready.pid"
#define APP_LOCK_FILENAME  ".app_lock.pid"

// ASE Mode macros
#define ASE_MODE_DAEMON_NO_SIMKILL   1
#define ASE_MODE_DAEMON_SIMKILL      2
#define ASE_MODE_DAEMON_SW_SIMKILL   3
#define ASE_MODE_REGRESSION          4

// UMAS establishment status
#define NOT_ESTABLISHED 0xC0C0
#define ESTABLISHED     0xBEEF

/*
 * Console colors
 */
// ERROR codes are in RED color
#define BEGIN_RED_FONTCOLOR    printf("\033[1;31m");
#define END_RED_FONTCOLOR      printf("\033[0m");

// INFO or INSTRUCTIONS are in GREEN color
#define BEGIN_GREEN_FONTCOLOR  printf("\033[32;1m");
#define END_GREEN_FONTCOLOR    printf("\033[0m");

// WARNING codes in YELLOW color
#define BEGIN_YELLOW_FONTCOLOR printf("\033[0;33m");
#define END_YELLOW_FONTCOLOR   printf("\033[0m");

// Wipeout current line in printf
#define WIPEOUT_LINE           printf("]\n\033[F\033[J");

/*
 * ASE Error codes
 */
#define ASE_USR_CAPCM_NOINIT           0x1	// CAPCM not initialized
#define ASE_OS_MQUEUE_ERR              0x2	// MQ open error
#define ASE_OS_SHM_ERR                 0x3	// SHM open error
#define ASE_OS_FOPEN_ERR               0x4	// Normal fopen failure
#define ASE_OS_MEMMAP_ERR              0x5	// Memory map/unmap errors
#define ASE_OS_MQTXRX_ERR              0x6	// MQ send receive error
#define ASE_OS_MALLOC_ERR              0x7	// Malloc error
#define ASE_OS_STRING_ERR              0x8	// String operations error
#define ASE_IPCKILL_CATERR             0xA	// Catastropic error when cleaning
// IPCs, manual intervention required
#define ASE_UNDEF_ERROR                0xFF	// Undefined error, pls report

// Simkill message
#define ASE_SIMKILL_MSG      0xDEADDEAD

// ASE PortCtrl Command values
typedef enum {
    AFU_RESET,
    ASE_SIMKILL,
    ASE_INIT,
    UMSG_MODE
} ase_portctrl_cmd;

// Test complete separator
#define TEST_SEPARATOR       "#####################################################"

/* *******************************************************************************
 *
 * Shared buffer structure
 * Fri Mar 11 09:02:18 PST 2016 : Converted to dual-ended linked list
 *
 * ******************************************************************************/
// Buffer information structure --
//   Be careful of alignment within this structure!  The layout must be
//   identical on both 64 bit and 32 bit compilations.
struct buffer_t			//  Descriptiion                    Computed by
{				// --------------------------------------------
	int32_t index;		// Tracking id                     | INTERNAL
	int32_t valid;		// Valid buffer indicator          | INTERNAL
	uint64_t vbase;		// SW virtual address              |   APP
	uint64_t pbase;		// SIM virtual address             |   SIM
	uint64_t fake_paddr;	// Simulated physical address      |   APP
	bool is_privmem;	// Flag memory as a private memory |
	bool is_mmiomap;	// Flag memory as CSR map          |
	bool is_umas;		// Flag memory as UMAS region      |
	bool is_pinned;		// Flag standard pinned region     |   APP
	uint32_t memsize;	// Memory size                     |   APP
	char memname[ASE_FILENAME_LEN];	// Shared memory name      | INTERNAL
	struct buffer_t *next;
};

/*
 * MMIO transaction packet --
 *   Be careful of alignment within this structure!  The layout must be
 *   identical on both 64 bit and 32 bit compilations.
 */
typedef struct mmio_t {
	int32_t tid;
	int32_t write_en;
	int32_t width;
	int32_t addr;
	uint64_t qword[8];
	int32_t resp_en;
	int32_t dummy;        // For 64 bit alignment
} mmio_t;


/*
 * Umsg transaction packet
 */
typedef struct umsgcmd_t {
	int32_t id;
	int32_t hint;
	uint64_t qword[8];
} umsgcmd_t;


// Compute buffer_t size
#define BUFSIZE     sizeof(struct buffer_t)


// Head and tail pointers of DPI side Linked list
extern struct buffer_t *head;	// Head pointer
extern struct buffer_t *end;	// Tail pointer

// DPI side CSR base, offsets updated on CSR writes
extern uint64_t *mmio_afu_vbase;
// UMAS Base Address
extern uint64_t *umsg_umas_vbase;

// ASE buffer valid/invalid indicator
// When a buffer is 'allocated' successfully, it will be valid, when
// it is deallocated, it will become invalid.
#define ASE_BUFFER_VALID        0xFFFF
#define ASE_BUFFER_INVALID      0x0

// Buffer allocate/deallocate message headers
#define HDR_MEM_ALLOC_REQ     0x7F7F
#define HDR_MEM_ALLOC_REPLY   0x77FF
#define HDR_MEM_DEALLOC_REQ   0x00FF
#define HDR_MEM_DEALLOC_REPLY 0x7700

// MMIO widths
#define MMIO_WRITE_REQ       0xAA88
#define MMIO_READ_REQ        0xBB88

#define MMIO_WIDTH_32        32
#define MMIO_WIDTH_64        64

// UMSG info structure
typedef struct {
	int id;
	int hint;
	char data[CL_BYTE_WIDTH];
} umsg_pack_t;

// Size map
#define SIZEOF_UMSG_PACK_T    sizeof(umsg_pack_t)


/* ********************************************************************
 *
 * FUNCTION PROTOTYPES
 *
 * ********************************************************************/
// Linked list functions
void ll_print_info(struct buffer_t *);
void ll_traverse_print(void);
void ll_append_buffer(struct buffer_t *);
void ll_remove_buffer(struct buffer_t *);
struct buffer_t *ll_search_buffer(int);

// Mem-ops functions
int ase_recv_msg(struct buffer_t *);
void ase_shmem_alloc_action(struct buffer_t *);
void ase_shmem_dealloc_action(struct buffer_t *, int);
void ase_shmem_destroy(void);
void ase_dbg_memtest(struct buffer_t *);
void ase_shmem_perror_teardown(char *, int);
#ifdef ASE_DEBUG
void print_mmiopkt(FILE *, char *, struct mmio_t *);
#endif
void ase_free_buffer(char *);

uint32_t generate_ase_seed(void);
bool check_app_lock_file(char *);
void create_new_lock_file(char *);

// ASE operations
#ifdef ASE_DEBUG
void ase_buffer_info(struct buffer_t *);
#endif
void ase_buffer_t_to_str(struct buffer_t *, char *);
void ase_str_to_buffer_t(char *, struct buffer_t *);
int ase_dump_to_file(struct buffer_t *, char *);
void ase_eval_session_directory(void);
int ase_instance_running(void);

void parse_ase_cfg_line(char *, char *, float *);
uint32_t ret_random_in_range(int, int);
void ase_string_copy(char *, const char *, size_t);
// Is environment variable defined?
bool ase_checkenv(const char *);
char *ase_getenv(const char *);
void ase_memcpy(void *, const void *, size_t);
int ase_strncmp(const char *, const char *, size_t);
int ase_memset(void *, int, size_t);
void ase_exit(void);
// Safe string equivalents
int ase_memcpy_s(void *, size_t, const void *, size_t);
int ase_strncpy_s(char *, size_t, const char *, size_t);
int ase_strcmp_s(const char *, size_t, const char *, int *);
int ase_memset_s(void *, size_t, int, size_t);

// Message queue operations
void ipc_init(void);
int mqueue_open(char *, int);
void mqueue_close(int);

void mqueue_send(int, const char *, int);
int mqueue_recv(int, char *, int);

// Timestamp functions
void put_timestamp(void);
// char* get_timestamp(int);
void get_timestamp(char *);
char *generate_tstamp_path(char *);

// IPC management functions
void final_ipc_cleanup(void);
void add_to_ipc_list(char *, char *);
void create_ipc_listfile(void);

// BBS DFH specifics
void initialize_fme_dfh(struct buffer_t *);
void update_fme_dfh(struct buffer_t *);


/*
 * These functions are called by C++ Applications
 */
#ifdef __cplusplus
extern "C" {
#endif				// __cplusplus
	// Session control
	void session_init(void);
	void session_deinit(void);
	void poll_for_session_id(void);
	int ase_read_lock_file(const char *);
	void send_simkill(int);
	void send_swreset(void);
	// Note pinned/unpinned pages
	void note_pinned_page(uint64_t, uint64_t, uint64_t);
	void note_unpinned_page(uint64_t, uint64_t);
	// Shared memory alloc/dealloc operations
	void allocate_buffer(struct buffer_t *, uint64_t *);
	void deallocate_buffer(struct buffer_t *);
	void append_buf(struct buffer_t *);
	void free_buffers(void);
	// MMIO activity
	int find_empty_mmio_scoreboard_slot(void);
	int get_scoreboard_slot_by_tid(int);
	int count_mmio_tid_used(void);
	uint32_t generate_mmio_tid(void);
	int mmio_request_put(struct mmio_t *);
	void mmio_response_get(struct mmio_t *);
	void mmio_write32(int, uint32_t);
	void mmio_write64(int, uint64_t);
	void mmio_read32(int, uint32_t *);
	void mmio_read64(int, uint64_t *);

	// UMSG functions
	// uint64_t *umsg_get_address(int);
	//void umsg_send(int, uint64_t *);
	void umsg_set_attribute(uint32_t);
	// Driver activity
	void ase_portctrl(ase_portctrl_cmd, int);
	// Threaded watch processes
	void *mmio_response_watcher();
	// ASE-special malloc
	void *ase_malloc(size_t);
	void *umsg_watcher();
	// void *intr_request_watcher();
	void register_signal(int, void *);
	void start_simkill_countdown(void);

	void ase_buffer_oneline(struct buffer_t *);
	void remove_spaces(char *);
	void remove_tabs(char *);
	void remove_newline(char *);
	int sscanf_s_ii(const char *, const char *, int *, int *);
	int fscanf_s_i(FILE *, const char *, int *);
	unsigned int parse_format(const char *format, char pformatList[], unsigned int maxFormats);
	// Error report functions
	void ase_error_report(const char *, int, int);
	void backtrace_handler(int);
	void mqueue_create(char *);
	void mqueue_destroy(char *);
	bool remove_existing_lock_file(const char *);
	void delete_lock_file(const char *);
#ifdef __cplusplus
}
#endif				// __cplusplus
#define DUMPSTRVAR(varname) fprintf(DUMPSTRVAR_str, "%s", #varname);
/* ********************************************************************
 *
 * MESSAGING IPC
 *
 * ********************************************************************/// Message queue parameters
#define ASE_MQ_MAXMSG     8
#define ASE_MQ_MSGSIZE    1024
#define ASE_MQ_NAME_LEN   64
#define ASE_MQ_INSTANCES  14
// Message presence setting
#define ASE_MSG_PRESENT 0xD33D
#define ASE_MSG_ABSENT  0xDEAD
#define ASE_MSG_ERROR   -1
// Message queue controls
struct ipc_t {
	char name[ASE_MQ_NAME_LEN];
	char path[ASE_FILEPATH_LEN];
	int perm_flag;
};
struct ipc_t mq_array[ASE_MQ_INSTANCES];


/* ********************************************************************
 *
 * DEBUG STRUCTURES
 *
 * ********************************************************************/
// Enable function call entry/exit
// Extremely noisy debug feature to watch function entry/exit
// #define ENABLE_ENTRY_EXIT_WATCH
#ifdef ENABLE_ENTRY_EXIT_WATCH
#define FUNC_CALL_ENTRY printf("--- ENTER: %s ---\n", __FUNCTION__);
#define FUNC_CALL_EXIT  printf("--- EXIT : %s ---\n", __FUNCTION__);
#else
#define FUNC_CALL_ENTRY
#define FUNC_CALL_EXIT
#endif
#define SOCKNAME "/tmp/ase_event_server_"
enum request_type {
	REGISTER_EVENT = 0,
	UNREGISTER_EVENT = 1
};

struct event_request {
	enum request_type type;
	int flags;
};

int register_event(int event_handle, int flags);
int unregister_event(int event_handle);

// ---------------------------------------------------------------------
// Enable memory test function
// ---------------------------------------------------------------------
// Basic Memory Read/Write test feature (runs on allocate_buffer)
// Leaving this setting ON automatically scrubs memory (sets 0s)
// Read shm_dbg_memtest() and ase_dbg_memtest()
// #define ASE_MEMTEST_ENABLE


// ------------------------------------------------------------------
// Triggers, safety catches and debug information used in the AFU
// simulator environment.
// ------------------------------------------------------------------
// ASE message view #define - Print messages as they go around
// #define ASE_MSG_VIEW

// Enable debug info from linked lists
// #define ASE_LL_VIEW

// Print buffers as they are being alloc/dealloc
// #define ASE_BUFFER_VIEW

// Backtrace data
int bt_j, bt_nptrs;
void *bt_buffer[4096];
char **bt_strings;


/* *********************************************************************
 *
 * SIMULATION-ONLY (SIM_SIDE) declarations
 * - This is available only in simulation side
 * - This compiled in when SIM_SIDE is set to 1
 *
 * *********************************************************************/
#ifdef SIM_SIDE

/*
 * ASE config structure
 * This will reflect ase.cfg
 */
struct ase_cfg_t {
	int ase_mode;
	int ase_timeout;
	int ase_num_tests;
	int enable_reuse_seed;
	int ase_seed;
	int enable_cl_view;
	int usr_tps;
	int phys_memory_available_gb;
};
extern struct ase_cfg_t *cfg;

/*
 * Data-exchange functions and structures
 */
// CCI transaction packet
typedef struct {
	int       mode;
	int       qw_start;
	long      mdata;
	long long cl_addr;
	long long qword[8];
	int       resp_channel;
	int       intr_id;
	int       success;
} cci_pkt;

#define CCIPKT_WRITE_MODE    0x1010
#define CCIPKT_READ_MODE     0x2020
#define CCIPKT_WRFENCE_MODE  0xFFFF
#define CCIPKT_ATOMIC_MODE   0x8080
#define CCIPKT_INTR_MODE     0x4040

/*
 * Function prototypes
 */
// DPI-C export(C to SV) calls
extern void simkill(void);
extern void sw_simkill_request(void);
extern void buffer_messages(char *);
extern void ase_config_dex(struct ase_cfg_t *);

// DPI-C import(SV to C) calls
int ase_init(void);
int ase_ready(void);
void ase_write_lock_file(void);
int ase_listener(void);
void ase_config_parse(char *);

// Simulation control function
void register_signal(int, void *);
void start_simkill_countdown(void);
void run_clocks(int num_clocks);
void afu_softreset_trig(int init, int value);
void ase_reset_trig(void);
void sw_reset_response(void);

// Read system memory line
void rd_memline_req_dex(cci_pkt *pkt);
void rd_memline_rsp_dex(cci_pkt *pkt);

// Write system memory line
void wr_memline_req_dex(cci_pkt *pkt);
void wr_memline_rsp_dex(cci_pkt *pkt);

// MMIO request
void mmio_dispatch(int init, struct mmio_t *mmio_pkt);

// MMIO Read response
void mmio_response(struct mmio_t *mmio_pkt);

// UMSG functions
void umsg_dispatch(int init, struct umsgcmd_t *umsg_pkt);

// Interrupt generator function
void ase_interrupt_generator(int id);

// Buffer message injection
void buffer_msg_inject(int, char *);

// Count error flag dex
extern int count_error_flag_ping(void);
void count_error_flag_pong(int);
void update_glbl_dealloc(int);

// Redeclaring ase_malloc, following maintainer-check issues !!! Do Not Edit !!!
void *ase_malloc(size_t);

// Ready filepath
extern char *ase_ready_filepath;


/*
 * IPC cleanup on catastrophic errors
 */
#define IPC_LOCAL_FILENAME ".ase_ipc_local"
extern FILE *local_ipc_fp;

// ASE PID
extern int ase_pid;

// Memory access debug log
#ifdef ASE_DEBUG
extern FILE *fp_memaccess_log;
extern FILE *fp_pagetable_log;
#endif

// Physical address mask - used to constrain generated addresses
extern uint64_t PHYS_ADDR_PREFIX_MASK;

// '1' indicates that teardown is in progress
extern int self_destruct_in_progress;

#endif


/*
 * IPC MQ fd names
 */
#ifdef SIM_SIDE
extern int sim2app_alloc_tx;		// sim2app mesaage queue in TX mode

extern int sim2app_dealloc_tx;
#endif				// End SIM_SIDE

// There is no global fixes for this
#define DEFEATURE_ATOMICS

/*
 * Platform specific switches are enabled here
 * - ase_magic - ensure that the data is legal (0xFACEF00D is default)
 * - umsg_feature - Enable Umsg feature
 * - intr_feature - Enable Interrupt feature
 * - mmio_512bit  - Enable 512-bit MMIO Write
 *
 */
struct ase_capability_t {
    char magic_word[sizeof(ASE_UNIQUE_ID)];
    int  umsg_feature;
    int  intr_feature;
    int  mmio_512bit;
};

extern struct ase_capability_t ase_capability;

// ------------------------------------------ //
#ifdef FPGA_PLATFORM_INTG_XEON
#define ASE_ENABLE_UMSG_FEATURE
#undef  ASE_ENABLE_INTR_FEATURE
#define ASE_ENABLE_MMIO512
// ------------------------------------------ //
#elif FPGA_PLATFORM_DISCRETE
#undef  ASE_ENABLE_UMSG_FEATURE
#define ASE_ENABLE_INTR_FEATURE
#undef  ASE_ENABLE_MMIO512
#endif
// ------------------------------------------ //

static inline int is_directory(const char *path)
{
	struct stat path_stat;

	/* handle error, follows the symbolic link */
	if (stat(path, &path_stat) != 0) {
		return 0;
	}

	return S_ISDIR(path_stat.st_mode);
}

#endif	// End _ASE_COMMON_H_
