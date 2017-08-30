// Copyright(c) 2017, Intel Corporation
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

#include "common_int.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* global loglevel */
static int g_loglevel = FPGA_LOG_UNDEFINED;
/* mutex to protect against garbled log output */
pthread_mutex_t log_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

void __FIXME_MAKE_VISIBLE__ fpga_print(int loglevel, char *fmt, ...)
{
	FILE *fp = stdout;
	pthread_mutexattr_t mattr;

	/* TODO: how to make this lazy initializer thread-safe? */
	if (g_loglevel < 0) { /* loglevel not yet set? */

		if (pthread_mutexattr_init(&mattr)) {
			fprintf(stderr, "Failed to create log mutex attributes\n");
			return;
		}

		if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
		    pthread_mutex_init(&log_lock, &mattr)) {
			fprintf(stderr, "Failed to create log mutex\n");
			goto out_attr_destroy;
		}

		pthread_mutexattr_destroy(&mattr);

		/* try to read loglevel from environment */
		char *s = getenv("LIBOPAE_LOG");
		if (s)
			g_loglevel = atoi(s);
#ifndef LIBFGPA_DEBUG
		if (g_loglevel >= FPGA_LOG_DEBUG)
			fprintf(stderr,
				"WARNING: Environment variable LIBOPAE_LOG is "
				"set to output debug\nmessages, "
				"but libopae-c was not built with debug "
				"information.\n");
#endif
	}

	if (g_loglevel < 0) /* loglevel still not set? */
		g_loglevel = FPGA_DEFAULT_LOGLEVEL;

	if (loglevel > g_loglevel)
		return;

	if (loglevel == FPGA_LOG_ERROR)
		fp = stderr;

	va_list argp;
	va_start(argp, fmt);
	pthread_mutex_lock(&log_lock); /* ignore failure and print anyway */
	vfprintf(fp, fmt, argp);
	pthread_mutex_unlock(&log_lock);
	va_end(argp);

	return;

out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);
}

