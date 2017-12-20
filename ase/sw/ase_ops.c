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

// Ready filepath
char ase_ready_filepath[ASE_FILEPATH_LEN];

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

#ifdef __linux__
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
#endif

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
		(mem->valid == ASE_BUFFER_VALID) ? "VALID" : "INVALID");
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
#ifdef _WIN32
	MemoryBarrier();
#elif defined __linux__
	__asm__ __volatile__("":::"memory");
#endif
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

	// Evaluate location of simulator or own location
#ifdef SIM_SIDE
	ase_workdir_path = getenv("PWD");
#else
	ase_workdir_path = getenv("ASE_WORKDIR");
#endif
	ASE_DBG("env(ASE_WORKDIR) = %s\n", ase_workdir_path);

	if (ase_workdir_path == NULL) {
		ASE_ERR
		    ("**ERROR** Environment variable ASE_WORKDIR could not be evaluated !!\n");
		ASE_ERR("**ERROR** ASE will exit now !!\n");
		perror("getenv");
		exit(1);
	} else {
		// Check if directory exists here
#ifdef _WIN32
	  HANDLE WINAPI dirhandle = CreateFile(
		  ase_workdir_path,
		  GENERIC_READ,
		  0,
		  NULL,
		  OPEN_EXISTING,
		  FILE_ATTRIBUTE_NORMAL,
		  NULL
	  );

	  if (dirhandle == INVALID_HANDLE_VALUE)
	  {
		  if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			  BEGIN_RED_FONTCOLOR;
			  printf("  [APP]  ASE workdir path pointed by env(ASE_WORKDIR) does not exist !\n");
			  printf("         Cannot continue execution... exiting !");
			  END_RED_FONTCOLOR;
			  exit(1);
		  }

	  }
	  else {
		  CloseHandle(dirhandle);
	  }
#elif defined __linux__
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
#endif
	}

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
	//   File pointer
	static FILE *fp_ase_ready;
	// ASE hostname
	char ase_hostname[ASE_FILENAME_LEN];
	int ret_err;

	// Create a filepath string
	snprintf(ase_ready_filepath, ASE_FILEPATH_LEN, "%s/%s",
		 ase_workdir_path, ASE_READY_FILENAME);

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
	char exp_ready_filepath[ASE_FILEPATH_LEN];
	char line[256];
	size_t len = 256;

	char *parameter;
	char *value;

	char readback_workdir_path[ASE_FILEPATH_LEN];
	char readback_hostname[ASE_FILENAME_LEN];
	char curr_hostname[ASE_FILENAME_LEN];
	char readback_uid[ASE_FILEPATH_LEN];
	char curr_uid[ASE_FILENAME_LEN];
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
		snprintf(exp_ready_filepath, ASE_FILEPATH_LEN, "%s\\%s",
			 workdir, ASE_READY_FILENAME);

		// Check if file exists
#ifdef _WIN32
	  DWORD file_attr;
	  file_attr = GetFileAttributes(exp_ready_filepath);
	  if (file_attr != 0xFFFFFFFF) {	//File exists
#elif defined __linux__
		if (access(exp_ready_filepath, F_OK) != -1) {	// File exists
#endif
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
			  while (ase_fgets(line, &len, fp_exp_ready))
			  {
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
#ifdef _WIN32
		  WSADATA data;

		  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			  printf("Initialization failed with %d\n", WSAGetLastError());
			  exit(1);
		  }
#endif
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
				}
			}

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

#ifdef __linux__
	struct sigaction cfg;

	// Configure signal handler
	cfg.sa_handler = handler;
	sigemptyset(&cfg.sa_mask);
	cfg.sa_flags = SA_RESTART;

	// Declare signal action
	sigaction(sig, &cfg, 0);
#endif

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
		if ((unsigned)dest_strlen < num_bytes) {
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
#ifdef _WIN32
	// Insecure implementation
	memcpy(dest, src, n);
#elif defined __linux__
	// Secure implementation
	memcpy_s(dest, n, src, n);
#endif
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

#ifdef _WIN32
	indicator = strncmp(s2, s1, n);
	return indicator;
#elif defined __linux__
	// Run secure compare
	ret = strcmp_s(s2, n, s1, &indicator);

	if (ret != EOK) {
		ASE_DBG("Problem with ase_strncmp - code %d\n", ret);
		return -1;
	} else {
		return indicator;
	}
#endif
}


/*
 * ASE sleep function
 */
void ase_sleep(int time, char unit) 
{
#ifdef _WIN32
	if (unit == 'u') {
		if ((time / 10) == 0) {
			Sleep(1);
		}
		else {
			Sleep(time / 10);
		}
	}
	else if (unit == 's') {
		Sleep(time * 1000);
	}

#elif defined __linux__
	if (unit == 'u') {
		usleep(time);
	}
	else if (unit == 's') {
		sleep(time);
	}
#endif
}

/*
 * ASE timer function
 */
void ase_timer(time_snapshot * timer) 
{
#ifdef _WIN32
	QueryPerformanceCounter(timer);
#elif defined __linux__
	clock_gettime(CLOCK_MONOTONIC, timer);
#endif
}

/*
 * ASE timestamp function
 */
unsigned long long ase_timestamp(time_snapshot start, time_snapshot end) 
{
#ifdef _WIN32
	time_snapshot frequency;
	QueryPerformanceFrequency(&frequency);
	return(1e9 * (end.QuadPart - start.QuadPart) / (long)frequency.QuadPart);
#elif defined __linux__
	return(1e9 * (end.tv_sec -
			start.tv_sec) +
			(end.tv_nsec -
				start.tv_nsec));
#endif
}

/*
 * ASE create thread function
 */
void ase_create_thread(thread_handle * thread_id,  thread_routine thread_callback) 
{
#ifdef _WIN32
	*thread_id = CreateThread(
		NULL,
		0,
		thread_callback,
		NULL,
		0,
		NULL
	);

	if ((*thread_id) == NULL) {
		BEGIN_RED_FONTCOLOR;
		printf("FAILED\n");
		printf("thread_create with %d", GetLastError());
		exit(1);
		END_RED_FONTCOLOR;
	}
	else {
		BEGIN_YELLOW_FONTCOLOR;
		printf("SUCCESS\n");
		END_YELLOW_FONTCOLOR;
	}
#elif defined __linux__
	// Thread error integer
	/* to-do*/
	int thr_err;
	thr_err =
		pthread_create(thread_id, NULL,
			(thread_callback), NULL);

	if (thr_err != 0) {
		BEGIN_RED_FONTCOLOR;
		printf("FAILED\n");
		perror("pthread_create");
		exit(1);
		END_RED_FONTCOLOR;
	}
	else {
		BEGIN_YELLOW_FONTCOLOR;
		printf("SUCCESS\n");
		END_YELLOW_FONTCOLOR;
	}
#endif
}

/*
 * ASE stop thread function
 */
void ase_stop_thread(thread_handle thread_id) 
{
#ifdef _WIN32
	TerminateThread(
		thread_id,
		0
	);
	CloseHandle(thread_id);
#elif defined __linux__
	pthread_cancel(thread_id);
	pthread_join(thread_id, NULL);
#endif
}


/*
 * ASE thread cancel function
 */
#ifndef SIM_SIDE
void ase_thread_cancel() 
{
#ifdef __linux__
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
}

/*
 * ASE Mutex creation function
 */
void ase_create_mutex(mutex_handle * handle) 
{
#ifdef _WIN32
	(*handle) = CreateMutex(
		NULL,
		FALSE,
		NULL
	);
	if (*handle == NULL) {
		BEGIN_RED_FONTCOLOR;
		printf
		("  [APP]  MMIO Lock initialization failed, EXIT\n");
		END_RED_FONTCOLOR;
		exit(1);
	}
#elif defined __linux__
	if (pthread_mutex_init(handle, NULL) != 0) {
		BEGIN_RED_FONTCOLOR;
		printf
		("  [APP]  MMIO Lock initialization failed, EXIT\n");
		END_RED_FONTCOLOR;
		exit(EXIT_FAILURE);
	}
#endif
}

/*
 * ASE Mutex lock function
 */
void ase_lock_mutex(mutex_handle * handle) 
{
#ifdef _WIN32
	WaitForSingleObject(
		(*handle),
		INFINITE
	);
#elif defined __linux__
	pthread_mutex_lock(handle);
#endif
}

/*
 * ASE Mutex release function
 */
void ase_release_mutex(mutex_handle * handle) 
{
#ifdef _WIN32
	ReleaseMutex(*handle);
	CloseHandle(*handle);
#elif defined __linux__
	pthread_mutex_unlock(handle);
	pthread_mutex_destroy(handle);
#endif
}

#endif   //#ifndef SIM_SIDE
/*
 * ASE get pid function
 */
pid ase_get_pid() 
{
#ifdef _WIN32
	return GetCurrentProcessId();
#elif defined __linux__
	return getpid();
#endif
}

/*
 * ASE truncate function
 */
void ase_truncate(file_desc fd_alloc, uint32_t memsize) 
{
#ifdef __linux__
	int res = ftruncate(fd_alloc, (off_t)memsize);
#ifdef ASE_DEBUG
	if (res != 0) {
		BEGIN_YELLOW_FONTCOLOR;
		printf("  [DEBUG]  ftruncate failed");
		perror("ftruncate");
		END_YELLOW_FONTCOLOR;
	}
#endif
#endif
}

/*
 * ASE close handle function
 */
void ase_close_handle(file_desc handle) 
{
#ifdef _WIN32
	CloseHandle(handle);
#elif defined __linux__
	close(handle);
#endif
}

/*
 * ASE check file exist function
 */
int ase_check_file_exists(char * filename) 
{
#ifdef _WIN32
	DWORD file_attr;
	file_attr = GetFileAttributes(filename);
	if (file_attr == 0xFFFFFFFF) {	//File doesnot exist
		return 0;
	}
	else {
		return 1;
	}
#elif defined __linux__
	if (access(filename, F_OK) == -1) {
		return 0;
	}
	else {
		return 1;
	}
#endif
}

/*
 * ASE getcwd function
 */
void ase_getcwd(char * cwd, int length) 
{
#ifdef _WIN32
	GetCurrentDirectory(ASE_FILEPATH_LEN, cwd);
#elif defined __linux__
	cwd = getcwd(cwd, ASE_FILEPATH_LEN);
#endif
}

/*
 * ASE create shared memory function
 */
file_desc ase_create_shm(uint32_t memsize, char * memname) 
{
	file_desc fd_alloc;
#ifdef _WIN32
	fd_alloc = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		memsize,
		memname
	);

	if (fd_alloc == NULL) {
		printf("CreateFilemapping error %d\n", GetLastError());
		exit(1);
	}
#elif defined __linux__
	fd_alloc =
		shm_open(memname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd_alloc < 0) {
		perror("shm_open");
		exit(1);
	}
#endif
	return fd_alloc;
}

/*
 * ASE open shared memory function
 */
int ase_shm_open(char * memname, file_desc * fd_alloc, int * error) 
{
#ifdef _WIN32
	*fd_alloc = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		memname
	);
	if (*fd_alloc == NULL) {
		*error = GetLastError();
		return 0;
	}
	else {
		return 1;
	}
#elif defined __linux__
	*fd_alloc = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);
	if (*fd_alloc < 0) {
		*error = errno;
		return 0;
	}
	else {
		return 1;
	}
#endif
}

/*
 * ASE memory map function
 */
uint64_t ase_mmap(uint64_t * addr, file_desc fd_alloc, int flags, uint32_t memsize, int *error) 
{
#ifdef _WIN32
	DWORDLONG start = 0;
	if (addr != NULL) {
		start = (DWORDLONG)addr;
	}
	LPVOID WINAPI map = MapViewOfFile(
		fd_alloc,
		FILE_MAP_ALL_ACCESS,
		start,
		0,
		memsize
	);
	
	if (map == NULL) {
		*error = GetLastError();
	}
#elif defined __linux__
	int shared = MAP_SHARED;
	if (flags == 1) {
		shared = MAP_SHARED | MAP_FIXED;
	}
	void * map = mmap(addr, memsize,
					PROT_READ | PROT_WRITE, shared,
					fd_alloc, 0);
	
	if ((uint64_t)map == (uint64_t)MAP_FAILED) {
		*error = errno;
	}
#endif
	return (uint64_t)map;
}

/*
 * ASE unmap shared memory function
 */
void ase_unmap(uint64_t addr, uint32_t memsize) 
{
	int ret;
#ifdef _WIN32
	ret = UnmapViewOfFile((LPCVOID)addr);
	if (ret == 0) {
		printf("UnmapViewOfFile error %d\n", GetLastError());
		exit(1);
	}
#elif defined __linux__
	ret = munmap((void *)addr, (size_t)memsize);
	if (0 != ret) {
		perror("munmap");
		exit(1);
	}
#endif
}

/*
 * ASE shared memory unlink
 */
void ase_shm_unlink(char * memname) 
{
#ifdef __linux__
	shm_unlink(memname);
#endif
}

/*
 * ASE fgets function
 */
int ase_fgets(char * line, size_t * len, FILE * fp) 
{
#ifdef _WIN32
	if (fgets(line, *len, fp)) return 1;
	else return 0;
#elif defined __linux__
	if (getline(&line, len, fp) != -1) return 1;
	else return 0;
#endif
}


