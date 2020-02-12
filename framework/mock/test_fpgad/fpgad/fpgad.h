// Copyright(c) 2018-2019, Intel Corporation
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

#ifndef __FPGAD_FPGAD_H__
#define __FPGAD_FPGAD_H__

#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/limits.h>
#include <opae/fpga.h>

#include "libbitstream/bitstream.h"

//#include "opae_int.h"

//#include "safe_string/safe_string.h"

//#include "api/logging.h"
//#include "command_line.h"

//int daemonize(void (*hndlr)(int, siginfo_t *, void *),
//	      mode_t mask,
//	      const char *dir);

/*
typedef void *fpga_token;
typedef void *fpga_objtype;
typedef int fpga_result;
*/

#define fpgad_mutex_lock(__res, __mtx_ptr)                                     \
	({                                                                     \
		(__res) = pthread_mutex_lock(__mtx_ptr);                       \
		if (__res)                                                     \
			LOG("pthread_mutex_lock failed: %s",                   \
				strerror(errno));                              \
		__res;                                                         \
	})

#define fpgad_mutex_unlock(__res, __mtx_ptr)                                   \
	({                                                                     \
		(__res) = pthread_mutex_unlock(__mtx_ptr);                     \
		if (__res)                                                     \
			LOG("pthread_mutex_unlock failed: %s",                 \
					strerror(errno));                      \
		__res;                                                         \
	})

typedef struct _fpgad_supported_device fpgad_supported_device;

struct fpgad_config {
	useconds_t poll_interval_usec;

	bool daemon;
	char directory[PATH_MAX];
	char logfile[PATH_MAX];
	char pidfile[PATH_MAX];
	char cfgfile[PATH_MAX];
	mode_t filemode;

	bool running;

	const char *api_socket;

#define MAX_NULL_GBS 32
	opae_bitstream_info null_gbs[MAX_NULL_GBS];
	unsigned num_null_gbs;

	pthread_t bmc_monitor_thr;
	pthread_t monitor_thr;
	pthread_t event_dispatcher_thr;
	pthread_t events_api_thr;

	fpgad_supported_device *supported_devices;
};

extern struct fpgad_config global_config;

fpga_result fpgaDestroyToken(fpga_token *token);

#endif /* __FPGAD_FPGAD_H__ */
