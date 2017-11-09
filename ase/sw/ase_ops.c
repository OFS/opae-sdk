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
 * Module Info: ASE operations functions
 * Language   : C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 */

#include "ase_common.h"

struct ase_cfg_t *cfg;

inline struct ase_cfg_t * get_cfg() { return cfg; }

struct buffer_t *head;
struct buffer_t *end;

//   File pointer
static FILE *fp_ase_ready;

// Ready filepath
char *ase_ready_filepath;

// ASE hostname
static char *ase_hostname;

// Log-level
int glbl_loglevel = ASE_LOG_MESSAGE;

inline int get_loglevel(void)
{
	return glbl_loglevel;
}

inline void set_loglevel(int level)
{
	if ((level >= ASE_LOG_ERROR)
		&& (level <= ASE_LOG_DEBUG))
		glbl_loglevel = level;
	else
		ASE_MSG("%s: Illlegal loglevel value.\n", __func__);
}

/*
 * Generate unique socket server name
 * Generated name is populated in "name"
 */
errno_t generate_sockname(char *name)
{
	errno_t err = EOK;

	err = strncpy_s(name, strlen(SOCKNAME)+1, SOCKNAME,
							strlen(SOCKNAME)+1);
	if (err != EOK) {
		ASE_ERR("%s: Error strncpy_s\n", __func__);
		return err;
	}

	char *tstamp = ase_malloc(100);

	if (!tstamp)
		return ENOMEM;

	get_timestamp(tstamp);
	err = strcat_s(name, strlen(SOCKNAME)+strlen(tstamp)+1, tstamp);
	ase_free_buffer((char *) tstamp);
	return err;
}
/*
 * Parse strings and remove unnecessary characters
 */
// Remove spaces
void remove_spaces(char *in_str)
{
	if (in_str == NULL) {
		ASE_ERR("remove_spaces : Input string is NULL\n");
	} else {
		char *i;
		char *j;
		i = in_str;
		j = in_str;
		while (*j != 0) {
			*i = *j++;
			if (*i != ' ')
				i++;
		}
		*i = 0;
	}
}


// Remove tabs
void remove_tabs(char *in_str)
{
	if (in_str == NULL) {
		ASE_ERR("remove_tabs : Input string is NULL\n");
	} else {
		char *i = in_str;
		char *j = in_str;
		while (*j != 0) {
			*i = *j++;
			if (*i != '\t')
				i++;
		}
		*i = 0;
	}
}

// Remove newline
void remove_newline(char *in_str)
{
	if (in_str == NULL) {
		ASE_ERR("remove_newline : Input string is NULL\n");
	} else {
		char *i = in_str;
		char *j = in_str;
		while (*j != 0) {
			*i = *j++;
			if (*i != '\n')
				i++;
		}
		*i = 0;
	}
}


// -------------------------------------------------------------
// ase_buffer_info : Print out information about the buffer
// -------------------------------------------------------------
#ifdef ASE_DEBUG
void ase_buffer_info(struct buffer_t *mem)
{
	FUNC_CALL_ENTRY;

	ASE_MSG("\tindex       = %d \n", mem->index);
	ASE_MSG("\tvalid       = %s \n",
		(mem->valid == 0xffff) ? "VALID" : "INVALID");
	ASE_MSG("\tAPPVirtBase = 0x%" PRIx64 "\n", mem->vbase);
	ASE_MSG("\tSIMVirtBase = 0x%" PRIx64 "\n", mem->pbase);
	ASE_MSG("\tBufferSize  = 0x%" PRIx32 " \n", mem->memsize);
	ASE_MSG("\tBufferName  = \"%s\"\n", mem->memname);
	ASE_MSG("\tPhysAddr LO = 0x%" PRIx64 "\n", mem->fake_paddr);
	ASE_MSG("\tPhysAddr HI = 0x%" PRIx64 "\n", mem->fake_paddr_hi);
	ASE_MSG("\tisMMIOMap   = %s\n",
		(mem->is_mmiomap == 1) ? "YES" : "NO");
	ASE_MSG("\tisUMAS      = %s\n",
		(mem->is_umas == 1) ? "YES" : "NO");

	FUNC_CALL_EXIT;
}
#endif


/*
 * ase_buffer_oneline : Print one line info about buffer
 */
void ase_buffer_oneline(struct buffer_t *mem)
{
	if (mem->valid == ASE_BUFFER_VALID) {
		ASE_MSG("%d\tADDED   \t%5s\n", mem->index, mem->memname);
	} else {
		ASE_MSG("%d\tREMOVED \t%5s\n", mem->index, mem->memname);
	}
}


// -------------------------------------------------------------------
// buffer_t_to_str : buffer_t to string conversion
// Converts buffer_t to string
// -------------------------------------------------------------------
void ase_buffer_t_to_str(struct buffer_t *buf, char *str)
{
	FUNC_CALL_ENTRY;

	ase_memcpy(str, (char *) buf, sizeof(struct buffer_t));

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------
// ase_str_to_buffer_t : string to buffer_t conversion
// All fields are space separated, use strtok to decode
// --------------------------------------------------------------
void ase_str_to_buffer_t(char *str, struct buffer_t *buf)
{
	FUNC_CALL_ENTRY;

	ase_memcpy((char *) buf, str, sizeof(struct buffer_t));

	FUNC_CALL_EXIT;
}


/*
 * ASE memory barrier
 */
void ase_memory_barrier(void)
{
	// asm volatile("" ::: "memory");
	__asm__ __volatile__("":::"memory");
}


/*
 * Evaluate Session directory
 * If SIM_SIDE is set, Return "$ASE_WORKDIR/work/"
 *               else, Return "$PWD/work/"
 *               Both must be the same location
 *
 * PROCEDURE:
 * - Check if PWD/ASE_WORKDIR exists:
 *   - Most cases, it will exist, created by Makefile
 *     - Check if "work" directory already exists, if not create one
 *   - If not Error out
 */
void ase_eval_session_directory(void)
{
	FUNC_CALL_ENTRY;

	// ase_workdir_path = ase_malloc (ASE_FILEPATH_LEN);

	// Evaluate location of simulator or own location
#ifdef SIM_SIDE
	ase_workdir_path = getenv("PWD");
#else
	ase_workdir_path = getenv("ASE_WORKDIR");

#ifdef ASE_DEBUG
	ASE_DBG("env(ASE_WORKDIR) = %s\n", ase_workdir_path);
#endif

	if (ase_workdir_path == NULL) {
		ASE_ERR
		    ("**ERROR** Environment variable ASE_WORKDIR could not be evaluated !!\n");
		ASE_ERR("**ERROR** ASE will exit now !!\n");
		perror("getenv");
		exit(1);
	} else {
		// Check if directory exists here
		DIR *ase_dir;
		ase_dir = opendir(ase_workdir_path);
		if (!ase_dir) {
			ASE_ERR
			    ("ASE workdir path pointed by env(ASE_WORKDIR) does not exist !\n");
			ASE_ERR("Cannot continue execution... exiting !");
			perror("opendir");
			exit(1);
		} else {
			closedir(ase_dir);
		}
	}
#endif
}


/*
 * ASE malloc
 * Malloc wrapped with ASE closedown if failure accures
 */
char *ase_malloc(size_t size)
{
	FUNC_CALL_ENTRY;

	char *buffer;

	buffer = malloc(size);
	// posix_memalign((void**)&buffer, (size_t)getpagesize(), size);
	if (buffer == NULL) {
		ase_error_report("malloc", errno, ASE_OS_MALLOC_ERR);

		ASE_ERR("Malloc failed\n");
		ase_free_buffer(buffer);
		FUNC_CALL_EXIT;
#ifdef SIM_SIDE
		start_simkill_countdown();
		exit(1);	// Klocwork fix
#else
		exit(1);
#endif
	} else {
		memset(buffer, 0, size);
		FUNC_CALL_EXIT;
		return buffer;
	}
}


/*
 * ase_write_lock_file : Write ASE Lock file
 * To be used only by simulator, NOT application
 *
 * Writes a lock file about session specific items like this
 * ------------------------------
 * | pid = <pid>
 * | host = <hostname>
 * | dir = <$PWD>
 * | uid = <ASE Unique ID>
 * ------------------------------
 *
 */
#ifdef SIM_SIDE
void ase_write_lock_file(void)
{
	FUNC_CALL_ENTRY;

	int ret_err;

	// Create a filepath string
	ase_ready_filepath = ase_malloc(ASE_FILEPATH_LEN);
	snprintf(ase_ready_filepath, ASE_FILEPATH_LEN, "%s/%s",
		 ase_workdir_path, ASE_READY_FILENAME);

	// Line 2
	ase_hostname = ase_malloc(ASE_FILENAME_LEN);

	// Open file
	fp_ase_ready = fopen(ase_ready_filepath, "w");
	if (fp_ase_ready == (FILE *) NULL) {
		ASE_ERR
		    ("**ERROR** => ASE lock file could not be written, Exiting\n");
		start_simkill_countdown();
	} else {
		// ///////// Write specifics ////////////////
		// Line 1
		fprintf(fp_ase_ready, "pid  = %d\n", ase_pid);

		// Get hostname for comparison
		ret_err = gethostname(ase_hostname, ASE_FILENAME_LEN);
		if (ret_err != 0) {
			ASE_ERR
			    ("**ERROR** => Hostname could not be calculated, Exiting\n");

			// Close file
			fclose(fp_ase_ready);

			// Remove buffers
			free(ase_hostname);

			// Issue Simkill
			start_simkill_countdown();
		} else {
			fprintf(fp_ase_ready, "host = %s\n", ase_hostname);

			// Line 3
			fprintf(fp_ase_ready, "dir  = %s\n",
				ase_workdir_path);

			// Line 4
			fprintf(fp_ase_ready, "uid  = %s\n",
				ASE_UNIQUE_ID);

			////////////////////////////////////////////
			// Close file
			fclose(fp_ase_ready);
			free(ase_hostname);

			// Notice on stdout
			ASE_MSG
			    ("ASE lock file .ase_ready.pid written in work directory\n");
		}
	}


	FUNC_CALL_EXIT;
}
#endif


/*
 * ase_read_lock_file() : Read an existing lock file and decipher contents
 */
int ase_read_lock_file(const char *workdir)
{
	// Allocate string
	FILE *fp_exp_ready;
	char *exp_ready_filepath;
	char *line;
	size_t len;

	char *parameter;
	char *value;

	char *readback_workdir_path;
	char *readback_hostname;
	char *curr_hostname;
	char *readback_uid;
	char *curr_uid;
	int readback_pid = 0;
	int ret_err;
	char *saveptr;

	// Null check and exit
	if (workdir == NULL) {
		ASE_ERR
		    ("ase_read_lock_file : Input ASE workdir path is NULL \n");
#ifdef SIM_SIDE
		start_simkill_countdown();
#else
		exit(1);
#endif
	} else {
		// Calculate ready file path
		exp_ready_filepath = ase_malloc(ASE_FILEPATH_LEN);
		snprintf(exp_ready_filepath, ASE_FILEPATH_LEN, "%s/%s",
			 workdir, ASE_READY_FILENAME);

		// Check if file exists
		if (access(exp_ready_filepath, F_OK) != -1) {	// File exists
			// Malloc/memset
			line = ase_malloc(256);
			readback_hostname = ase_malloc(ASE_FILENAME_LEN);
			readback_uid = ase_malloc(ASE_FILEPATH_LEN);
			readback_workdir_path =
			    ase_malloc(ASE_FILEPATH_LEN);
			curr_hostname = ase_malloc(ASE_FILENAME_LEN);

			// Open file
			fp_exp_ready = fopen(exp_ready_filepath, "r");
			if (fp_exp_ready == NULL) {
				ASE_ERR
				    ("Ready file couldn't be opened for reading, Exiting !\n");
#ifdef SIM_SIDE
				start_simkill_countdown();
#else
				exit(1);
#endif
			} else {

				// Read file line by line
				while (getline(&line, &len, fp_exp_ready)
				       != -1) {
					// LHS/RHS tokenizing
					parameter = strtok_r(line, "=", &saveptr);
					value = strtok_r(NULL, "", &saveptr);
					// Check for parameter being recorded as NULL
					if ((parameter == NULL)
					    || (value == NULL)) {
						ASE_ERR
						    ("** Error tokenizing paramter in lock file, EXIT !\n");
#ifdef SIM_SIDE
						start_simkill_countdown();
#else
						exit(1);
#endif
					} else {
						// Trim contents
						remove_spaces(parameter);
						remove_tabs(parameter);
						remove_newline(parameter);
						remove_spaces(value);
						remove_tabs(value);
						remove_newline(value);
						// Line 1/2/3/4 check
						if (ase_strncmp
						    (parameter, "pid",
						     3) == 0) {
							readback_pid =
							    atoi(value);
						} else
						    if (ase_strncmp
							(parameter, "host",
							 4) == 0) {
							ase_string_copy
							    (readback_hostname,
							     value,
							     ASE_FILENAME_LEN);
						} else
						    if (ase_strncmp
							(parameter, "dir",
							 3) == 0) {
							ase_string_copy
							    (readback_workdir_path,
							     value,
							     ASE_FILEPATH_LEN);
						} else
						    if (ase_strncmp
							(parameter, "uid",
							 3) == 0) {
							ase_string_copy
							    (readback_uid,
							     value,
							     ASE_FILEPATH_LEN);
						} else {
							ASE_ERR
							    ("** ERROR **: Session parameter could not be deciphered !\n");
						}
					}
				}
				fclose(fp_exp_ready);
			}

			////////////////// Error checks //////////////////
			// If hostname does not match
			ret_err =
			    gethostname(curr_hostname, ASE_FILENAME_LEN);
			if (ret_err != 0) {
				ASE_ERR
				    ("**ERROR** => Hostname could not be calculated, Exiting\n");
				exit(1);
			} else {
				// Check here
				if (ase_strncmp
				    (curr_hostname, readback_hostname,
				     ASE_FILENAME_LEN) != 0) {
					ASE_ERR
					    ("** ERROR ** => Hostname specified in ASE lock file (%s) is different as current hostname (%s)\n",
					     readback_hostname,
					     curr_hostname);
					ASE_ERR
					    ("** ERROR ** => Ensure that Simulator process and OPAE SW application are running on the same host !\n");
#ifdef SIM_SIDE
					start_simkill_countdown();
#else
					exit(1);
#endif
				} else {
					// If readback_uid (Readback unique ID from lock file) doesnt match ase_common.h
					curr_uid =
					    ase_malloc(ASE_FILENAME_LEN);
					ase_string_copy(curr_uid,
							ASE_UNIQUE_ID,
							ASE_FILENAME_LEN);

					// Check
					if (ase_strncmp
					    (curr_uid, readback_uid,
					     ASE_FILENAME_LEN) != 0) {
						ASE_ERR
						    ("** ERROR ** => Application UID does not match known release UID\n");
						ASE_ERR
						    ("** ERROR ** => Simulator built with UID=%s, Application built with UID=%s\n",
						     readback_uid,
						     curr_uid);
						ASE_ERR
						    ("** ERROR ** => Ensure that Simulator process and OPAE SW application are compiled from the same System Release version !\n");
						ASE_ERR
						    ("** ERROR ** => Also, check if env(LD_LIBRARY_PATH) is set to appropriate <prefix> or <DESTDIR> library paths \n");
						ASE_ERR
						    ("** ERROR ** => Simulation cannot proceed ... EXITING\n");
#ifdef SIM_SIDE
						start_simkill_countdown();
#else
						exit(1);
#endif
					}
					// Free curr_uid
					free(curr_uid);
				}
			}

			// Free all buffers
			ase_free_buffer(line);
			ase_free_buffer(readback_hostname);
			ase_free_buffer(curr_hostname);
			ase_free_buffer(readback_workdir_path);
			ase_free_buffer(readback_uid);
		} else {         // File does not exist
			ASE_ERR
			    ("ASE Ready file was not found at env(ASE_WORKDIR) !\n");
			ASE_ERR
			    ("This could be for one of two reasons =>\n");
			ASE_ERR(" - Simualtor is not running yet  \n");
			ASE_ERR
			    (" - env(ASE_WORKDIR) is set to the wrong location \n");
			// Shutdown process
#ifdef SIM_SIDE
			start_simkill_countdown();
#else
			exit(1);
#endif
		}

		// Free expected filepath buffer
		free(exp_ready_filepath);
	}

	// Return PID of Simulator instance
	return readback_pid;
}


/*
 * Pretty print function - print_mmiopkt
 */
#ifdef ASE_DEBUG
void print_mmiopkt(FILE *fp, char *activity, struct mmio_t *pkt)
{
	FUNC_CALL_ENTRY;

	char mmio_action_type[20];
	memset(mmio_action_type, 0, 20);

	snprintf(mmio_action_type, 20,
		 "MMIO-%s-%d-%s",
		 (pkt->write_en == MMIO_WRITE_REQ ? "Write" : "Read"),
		 pkt->width, (pkt->resp_en == 0 ? "Req " : "Resp"));

	fprintf(fp, "%s\t%03x\t%s\t%x\t%" PRIx64 "\n", activity,
		pkt->tid, mmio_action_type, pkt->addr, pkt->qword[0]);

	FUNC_CALL_EXIT;
}
#endif


/*
 * ase_free_buffer : Free memory if not NULL
 */
void ase_free_buffer(char *ptr)
{
	if (ptr != (char *) NULL) {
		free(ptr);
	}
}


/*
 * register_signal : Register Signal Handler
 */
void register_signal(int sig, void *handler)
{
	FUNC_CALL_ENTRY;

	struct sigaction cfg;

	// Configure signal handler
	cfg.sa_handler = handler;
	sigemptyset(&cfg.sa_mask);
	cfg.sa_flags = SA_RESTART;

	// Declare signal action
	sigaction(sig, &cfg, 0);

	FUNC_CALL_EXIT;
}


/*
 * ret_random_in_range : Return random number in a range
 */
uint32_t ret_random_in_range(int low, int high)
{
	return (rand() % (high + 1 - low) + low);
}


/*
 * ase_string_copy
 * ASE's own safe string copy insures a null-termination
 * NOTE: dest must be malloc'ed before use (use ase_malloc)
 */
void ase_string_copy(char *dest, const char *src, size_t num_bytes)
{
	FUNC_CALL_ENTRY;

	int dest_strlen;

	// Allocate memory if not already done
	if (dest == NULL) {
		ASE_ERR
		    ("** ERROR ** => String copy destination not allocated.. Exiting\n");
#ifdef SIM_SIDE
		start_simkill_countdown();
#else
		exit(1);
#endif
	} else {
		// Use snprintf as a copy mechanism
		snprintf(dest, num_bytes, "%s", src);

		// Find length
		dest_strlen = strlen(dest);

		// Terminate length, or kill
		if (dest_strlen < num_bytes) {
			dest[dest_strlen] = '\0';
		} else {
			ASE_ERR
			    ("** Internal Error ** => Invalid null termination during string copy [%d]\n",
			     dest_strlen);
#ifdef SIM_SIDE
			start_simkill_countdown();
#else
			exit(1);
#endif
		}
	}

	FUNC_CALL_EXIT;
}


/*
 * ase_getenv : Secure getenv abstraction
 */
char *ase_getenv(const char *name)
{
	char *env;

	if (name == NULL) {
		ASE_ERR
		    ("** ERROR **: Input Environment variable is NULL... EXITING");
#ifdef SIM_SIDE
		start_simkill_countdown();
#else
		exit(1);
#endif
	} else {
		// GLIBC check before getenv call (check if GLIBC >= 2.17)
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ > 17)
		env = secure_getenv(name);
#else
		env = getenv(name);
#endif

		if (env == NULL) {
			ASE_ERR
			    ("** ERROR **: Environment variable env(%s) could not be evaluated... EXITING",
			     name);
#ifdef SIM_SIDE
			start_simkill_countdown();
#else
			exit(1);
#endif
		} else {
			return env;
		}
	}

	return NULL;
}


/*
 * ase_memcpy - Secure memcpy abstraction
 */
void ase_memcpy(void *dest, const void *src, size_t n)
{
	// Insecure implementation
	// memcpy(dest, src, n);

	// Secure implementation
	memcpy_s(dest, n, src, n);
}


/*
 * Print messages
 */
int ase_calc_loglevel(void)
{
	int ret_loglevel;

	// Evaluate env(ASE_LOG)
	char *str_env;
	str_env = getenv("ASE_LOG");
	if (str_env) {
		ret_loglevel = atoi(str_env);
	} else {
		ret_loglevel = ASE_LOG_MESSAGE;
	}

	// Clean up if illegal (can be 0 or higher only
	if (ret_loglevel < ASE_LOG_SILENT) {
		ret_loglevel = ASE_LOG_MESSAGE;
	}
	// If ASE_DEBUG is set, return ASE_LOG_DEBUG
#ifdef ASE_DEBUG
	ASE_MSG("Started in DEBUG mode\n");
	ret_loglevel = ASE_LOG_DEBUG;
#endif

	return ret_loglevel;
}


/*
 * Print level function
 */
void ase_print(int loglevel, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	// glbl_loglevel is sanitized to Either 0, 1, or 2

	if (loglevel == ASE_LOG_ERROR) {
		BEGIN_RED_FONTCOLOR;
		vprintf(fmt, args);
		END_RED_FONTCOLOR;
	} else if (loglevel == ASE_LOG_INFO) {
#ifdef SIM_SIDE
		BEGIN_GREEN_FONTCOLOR;
		vprintf(fmt, args);
		END_GREEN_FONTCOLOR;
#else
		BEGIN_YELLOW_FONTCOLOR;
		vprintf(fmt, args);
		END_YELLOW_FONTCOLOR;
#endif
	}
#ifdef SIM_SIDE
	else if (loglevel == ASE_LOG_INFO_2) {
		BEGIN_YELLOW_FONTCOLOR;
		vprintf(fmt, args);
		END_YELLOW_FONTCOLOR;
	}
#endif
	else if (loglevel == ASE_LOG_DEBUG) {
#ifdef ASE_DEBUG
		BEGIN_YELLOW_FONTCOLOR;
		vprintf(fmt, args);
		END_YELLOW_FONTCOLOR;
#endif
	} else {
		if (glbl_loglevel != ASE_LOG_SILENT) {
			BEGIN_YELLOW_FONTCOLOR;
			vprintf(fmt, args);
			END_YELLOW_FONTCOLOR;
		}
	}

	va_end(args);
}


/*
 * ASE String Compare
 */
int ase_strncmp(const char *s1, const char *s2, size_t n)
{
	errno_t ret;
	int indicator;

	// Run secure compare
	ret = strcmp_s(s2, n, s1, &indicator);
	if (ret != EOK) {
		ASE_DBG("Problem with ase_strncmp - code %d\n", ret);
		return -1;
	} else {
		return indicator;
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

	FILE *fp = (FILE *)NULL;
	char *line;
	size_t len = 0;
	char *parameter;
	int value;
	char *pch;
	char *saveptr;

	float f_usrclk;    // User clock frequency


					   // Allocate space to store ASE config
	cfg = (struct ase_cfg_t *) ase_malloc(sizeof(struct ase_cfg_t));

	// Allocate memory to store a line
	line = ase_malloc(sizeof(char) * 80);

	// Default values
	cfg->ase_mode = ASE_MODE_DAEMON_NO_SIMKILL;
	cfg->ase_timeout = 50000;
	cfg->ase_num_tests = 1;
	cfg->enable_reuse_seed = 0;
	cfg->ase_seed = 9876;
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
		}
		else {
			ASE_INFO_2("Reading %s configuration file \n",
				filename);
			// Parse file line by line
			while (getline(&line, &len, fp) != -1) {
				// Remove all invalid characters
				remove_spaces(line);
				remove_tabs(line);
				remove_newline(line);
				// Ignore strings begining with '#' OR NULL (compound NOR)
				if ((line[0] != '#') && (line[0] != '\0')) {
					parameter = strtok_r(line, "=\n", &saveptr);
					if (parameter != NULL) {
						if (ase_strncmp
						(parameter, "ASE_MODE",
							8) == 0) {
							pch =
								strtok_r(NULL,
									"", &saveptr);
							if (pch != NULL) {
								cfg->
									ase_mode
									=
									atoi
									(pch);
							}
						}
						else
							if (ase_strncmp
							(parameter,
								"ASE_TIMEOUT",
								11) == 0) {
								pch =
									strtok_r(NULL,
										"", &saveptr);
								if (pch != NULL) {
									cfg->
										ase_timeout
										=
										atoi
										(pch);
								}
							}
							else
								if (ase_strncmp
								(parameter,
									"ASE_NUM_TESTS",
									13) == 0) {
									pch =
										strtok_r(NULL,
											"", &saveptr);
									if (pch != NULL) {
										cfg->
											ase_num_tests
											=
											atoi
											(pch);
									}
								}
								else
									if (ase_strncmp
									(parameter,
										"ENABLE_REUSE_SEED",
										17) == 0) {
										pch =
											strtok_r(NULL,
												"", &saveptr);
										if (pch != NULL) {
											cfg->
												enable_reuse_seed
												=
												atoi
												(pch);
										}
									}
									else
										if (ase_strncmp
										(parameter,
											"ASE_SEED",
											8) == 0) {
											pch =
												strtok_r(NULL,
													"", &saveptr);
											if (pch != NULL) {
												cfg->
													ase_seed
													=
													atoi
													(pch);
											}
										}
										else
											if (ase_strncmp
											(parameter,
												"ENABLE_CL_VIEW",
												14) == 0) {
												pch =
													strtok_r(NULL,
														"", &saveptr);
												if (pch != NULL) {
													cfg->
														enable_cl_view
														=
														atoi
														(pch);
												}
											}
											else
												if (ase_strncmp
												(parameter,
													"USR_CLK_MHZ",
													11) == 0) {
													pch =
														strtok_r(NULL,
															"", &saveptr);
													if (pch != NULL) {
														f_usrclk =
															atof
															(pch);
														if (f_usrclk == 0.000000) {
															ASE_ERR
															("User Clock Frequency cannot be 0.000 MHz\n");
															ASE_ERR
															("        Reverting to %f MHz\n",
																DEFAULT_USR_CLK_MHZ);
															f_usrclk
																=
																DEFAULT_USR_CLK_MHZ;
															cfg->
																usr_tps
																=
																DEFAULT_USR_CLK_TPS;
														}
														else
															if
																(f_usrclk
																	==
																	DEFAULT_USR_CLK_MHZ)
															{
																cfg->
																	usr_tps
																	=
																	DEFAULT_USR_CLK_TPS;
															}
															else {
																cfg->
																	usr_tps
																	=
																	(int)
																	(1E+12
																		/
																		(f_usrclk
																			*
																			pow
																			(1000,
																				2)));
#ifdef ASE_DEBUG
																ASE_DBG
																("usr_tps = %d\n",
																	cfg->
																	usr_tps);
#endif
																if (f_usrclk != DEFAULT_USR_CLK_MHZ) {
																	ASE_INFO_2
																	("User clock Frequency was modified from %f to %f MHz\n",
																		DEFAULT_USR_CLK_MHZ,
																		f_usrclk);
																}
															}
													}
												}
												else
													if (ase_strncmp
													(parameter,
														"PHYS_MEMORY_AVAILABLE_GB",
														24) == 0) {
														pch =
															strtok_r(NULL,
																"", &saveptr);
														if (pch != NULL) {
															value =
																atoi
																(pch);
															if (value <
																0) {
																ASE_ERR
																("Physical memory size is negative in %s\n",
																	filename);
																ASE_ERR
																("        Reverting to default 256 GB\n");
															}
															else {
																cfg->
																	phys_memory_available_gb
																	=
																	value;
															}
														}
													}
													else {
														ASE_INFO_2
														("In config file %s, Parameter type %s is unidentified \n",
															filename,
															parameter);
													}
					}
				}
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

	}
	else {
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
		("Reuse simulation seed      ... DISABLED (will create one at $ASE_WORKDIR/ase_seed.txt) \n");
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

void config_free(void)
{
	if (cfg)
		free(cfg);
}


