// Copyright(c) 2021-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <ofs/ofs_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#include <pthread.h>

#include "mock/opae_std.h"

static int log_level = OFS_DEFAULT_LOG_LEVEL;
static FILE *log_file;
static pthread_mutex_t log_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void ofs_print(int level, const char *fmt, ...)
{
	FILE *fp;
	int err;
	va_list argp;

	if (level > log_level)
		return;

	if (level == OFS_LOG_ERROR)
		fp = stderr;
	else
		fp = log_file ? log_file : stdout;

	va_start(argp, fmt);
	err = pthread_mutex_lock(&log_lock);
	if (err)
		fprintf(stderr, "ofs_print(): pthread_mutex_lock() failed: %s",
			strerror(err));
	vfprintf(fp, fmt, argp);
	err = pthread_mutex_unlock(&log_lock);
	if (err)
		fprintf(stderr, "ofs_print(): pthread_mutex_unlock() failed: %s",
			strerror(err));
	va_end(argp);
}

__attribute__((constructor)) STATIC void ofs_init(void)
{
	char *s;

	log_file = NULL;

	s = getenv("LIBOFS_LOG");
	if (s) {
		log_level = atoi(s);
#ifndef LIBOFS_DEBUG
		if (log_level >= OFS_LOG_DEBUG)
			fprintf(stderr,
				"WARNING: LIBOFS_DEBUG Env variable LIBOFS_LOG is "
				"set to output debug\nmessages, "
				"but libofs was not built with debug logging "
				"enabled.\n");
#endif // LIBOFS_DEBUG
	}

	s = getenv("LIBOFS_LOGFILE");
	if (s) {
		if (s[0] != '/' || !strncmp(s, "/tmp/", 5)) {
			log_file = opae_fopen(s, "w");
			if (!log_file) {
				fprintf(stderr,
					"Could not open log file for writing: %s. ", s);
				fprintf(stderr, "Error is: %s\n", strerror(errno));
			}
		}
	}

	if (!log_file)
		log_file = stdout;
}

__attribute__((destructor)) STATIC void ofs_release(void)
{
	if (log_file && log_file != stdout)
		opae_fclose(log_file);
	log_file = NULL;
}
