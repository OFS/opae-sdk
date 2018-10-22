// Copyright(c) 2017-2018, Intel Corporation
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

#include <opae/init.h>
#include <opae/utils.h>
#include "pluginmgr.h"
#include "opae_int.h"
#undef __USE_GNU

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#include <pthread.h>

/* global loglevel */
static int g_loglevel = OPAE_DEFAULT_LOGLEVEL;
static FILE *g_logfile;
/* mutex to protect against garbled log output */
static pthread_mutex_t log_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void /* __FIXME_MAKE_VISIBLE__ */ opae_print(int loglevel, const char *fmt, ...)
{
	FILE *fp;
	int err;
	va_list argp;

	if (loglevel > g_loglevel)
		return;

	if (loglevel == OPAE_LOG_ERROR)
		fp = stderr;
	else
		fp = g_logfile == NULL ? stdout : g_logfile;

	va_start(argp, fmt);
	err = pthread_mutex_lock(
		&log_lock); /* ignore failure and print anyway */
	if (err)
		fprintf(stderr, "pthread_mutex_lock() failed: %s",
			strerror(err));
	vfprintf(fp, fmt, argp);
	err = pthread_mutex_unlock(&log_lock);
	if (err)
		fprintf(stderr, "pthread_mutex_unlock() failed: %s",
			strerror(err));
	va_end(argp);
}


__attribute__((constructor)) STATIC void opae_init(void)
{
	g_logfile = NULL;

	/* try to read loglevel from environment */
	char *s = getenv("LIBOPAE_LOG");
	if (s) {
		g_loglevel = atoi(s);
#ifndef LIBOPAE_DEBUG
		if (g_loglevel >= OPAE_LOG_DEBUG)
			fprintf(stderr,
				"WARNING: Environment variable LIBOPAE_LOG is "
				"set to output debug\nmessages, "
				"but libopae-c was not built with debug "
				"information.\n");
#endif
	}

	s = getenv("LIBOPAE_LOGFILE");
	if (s) {
		g_logfile = fopen(s, "w");
		if (g_logfile == NULL) {
			fprintf(stderr,
				"Could not open log file for writing: %s. ", s);
			fprintf(stderr, "Error is: %s\n", strerror(errno));
		}
	}

	if (g_logfile == NULL)
		g_logfile = stdout;

	// If the environment hasn't requested explicit initialization,
	// perform the initialization implicitly here.
	if (getenv("OPAE_EXPLICIT_INITIALIZE") == NULL)
		fpgaInitialize(NULL);
}

__attribute__((destructor)) STATIC void opae_release(void)
{
	fpga_result res;

	res = fpgaFinalize();
	if (res != FPGA_OK)
		OPAE_ERR("fpgaFinalize: %s", fpgaErrStr(res));

	if (g_logfile != NULL && g_logfile != stdout) {
		fclose(g_logfile);
	}
	g_logfile = NULL;
}
