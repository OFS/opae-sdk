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

#ifndef __FPGAD_CONFIG_H__
#define __FPGAD_CONFIG_H__
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <inttypes.h>

#define MAX_NULL_GBS 16
#define MAX_SOCKETS 2

#define UNUSED_PARAM(x) ((void)x)

/*
 * Global configuration, set during parse_args()
 */
struct config {
	unsigned int verbosity;
	useconds_t   poll_interval_usec;

	int daemon;               // whether to daemonize
	char directory[PATH_MAX]; // working directory when daemonizing
	char logfile[PATH_MAX];   // location of log file
	char pidfile[PATH_MAX];   // where to write fpgad.pid
	mode_t filemode;          // argument for umask

	bool running;

	const char *socket;

	char *null_gbs[MAX_NULL_GBS];
	unsigned int num_null_gbs;
};

#endif
