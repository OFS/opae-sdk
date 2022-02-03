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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <time.h>
#include "logging.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("logging: " format, ##__VA_ARGS__)

STATIC pthread_mutex_t log_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
STATIC FILE *log_file;

#define BUF_TIME_LEN    256

int log_open(const char *filename)
{
	int res;
	int err;

	fpgad_mutex_lock(err, &log_lock);

	log_file = fopen(filename, "a");
	if (log_file) {
		time_t raw;
		struct tm tm;
		char timebuf[BUF_TIME_LEN];
		size_t len;

		time(&raw);
		localtime_r(&raw, &tm);
		asctime_r(&tm, timebuf);

		//len = strnlen_s(timebuf, sizeof(timebuf));
		len = strlen(timebuf);
		if (len < BUF_TIME_LEN) {
			timebuf[len - 1] = '\0'; /* erase \n */
		} else {
			printf(" Invalid time stamp buffer size \n");
			fpgad_mutex_unlock(err, &log_lock);
			return -1;
		}

		res = fprintf(log_file, "----- %s -----\n", timebuf);
		fflush(log_file);
	} else {
		res = -1;
	}

	fpgad_mutex_unlock(err, &log_lock);

	return res;
}

int log_printf(const char *fmt, ...)
{
	va_list l;
	int res = -1;
	int err;

	va_start(l, fmt);

	fpgad_mutex_lock(err, &log_lock);

	if (log_file) {
		res = vfprintf(log_file, fmt, l);
		fflush(log_file);
	}

	fpgad_mutex_unlock(err, &log_lock);

	va_end(l);

	return res;
}

void log_set(FILE *fptr)
{
	int err;

	fpgad_mutex_lock(err, &log_lock);

	log_close();
	log_file = fptr;

	fpgad_mutex_unlock(err, &log_lock);
}

void log_close(void)
{
	int err;

	fpgad_mutex_lock(err, &log_lock);

	if (log_file) {
		if (log_file != stdout &&
		    log_file != stderr) {
			fclose(log_file);
		}
		log_file = NULL;
	}

	fpgad_mutex_unlock(err, &log_lock);
}
