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

#ifndef __FPGAD_ERRTABLE_H__
#define __FPGAD_ERRTABLE_H__

#define _GNU_SOURCE
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
#undef _GNU_SOURCE

#include "common_int.h"

/* TODO: support variable number of FMEs and PORTs */
#define SYSFS_FME0  SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-fme.0"
#define SYSFS_FME1  SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.1/intel-fpga-fme.1"

#define SYSFS_PORT0 SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-port.0"
#define SYSFS_PORT1 SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.1/intel-fpga-port.1"

struct fpga_err {
	int socket;
	const char *sysfsfile;
	const char *reg_field;
	int lowbit;
	int highbit;
	bool occurred;
	void (*callback)(const struct fpga_err *);
};

int daemonize(void (*hndlr)(int, siginfo_t *, void *), mode_t, const char *);

fpga_result sysfs_read_u64(const char *path, uint64_t *u);

void *logger_thread(void *);

#endif

