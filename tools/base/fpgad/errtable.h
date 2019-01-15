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

#ifndef __FPGAD_ERRTABLE_H__
#define __FPGAD_ERRTABLE_H__

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
#include <opae/fpga.h>
#undef _GNU_SOURCE

struct fpga_err {
	const char *sysfsfile;
	const char *reg_field;
	int lowbit;
	int highbit;
	void (*callback)(uint8_t socket_id,
			 uint64_t object_id,
			 const struct fpga_err *);
};
#define TABLE_TERMINATOR { NULL, NULL, 0, 0, NULL }

typedef struct _supported_device {
	uint16_t vendor_id;
	uint16_t device_id;
	uint64_t error_revision;
	struct fpga_err *error_table;
} supported_device;

#define MAX_ERROR_COUNT 64

typedef struct _monitored_device {
	struct _monitored_device *next;
	fpga_token token;
	uint8_t socket_id;
	uint64_t object_id;
	supported_device *device;
	struct fpga_err *error_occurrences[MAX_ERROR_COUNT];
	uint32_t num_error_occurrences;
} monitored_device;

bool error_already_occurred(monitored_device *d, struct fpga_err *e);
void error_just_occurred(monitored_device *d, struct fpga_err *e);
void clear_occurrences_of(monitored_device *d, struct fpga_err *e);

int daemonize(void (*hndlr)(int, siginfo_t *, void *), mode_t, const char *);

void *logger_thread(void *);

#endif

