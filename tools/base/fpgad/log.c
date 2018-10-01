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

/*
 * log.c : logging routines
 */

#include "log.h"

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
FILE *fLog;
int open_log(const char *filename)
{
	int res;
	int err;

	opae_mutex_lock(err, &log_lock);

	fLog = fopen(filename, "a");
	if (fLog) {
		res = fprintf(fLog, "----- MARK -----\n");
		fflush(fLog);
	} else {
		res = -1;
	}

	opae_mutex_unlock(err, &log_lock);

	return res;
}

int dlog(const char *fmt, ...)
{
	va_list l;
	int res;
	int err;

	va_start(l, fmt);

	opae_mutex_lock(err, &log_lock);

	res = vfprintf(fLog, fmt, l);
	fflush(fLog);

	opae_mutex_unlock(err, &log_lock);

	va_end(l);

	return res;
}

void close_log(void)
{
	fclose(fLog);
}
