// Copyright(c) 2018-2022, Intel Corporation
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

#ifndef __FPGAD_COMMAND_LINE_H__
#define __FPGAD_COMMAND_LINE_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "bitstream.h"
#include "cfg-file.h"

#define MAX_NULL_GBS 32

struct fpgad_config {
	useconds_t poll_interval_usec;

	bool daemon;
	char directory[PATH_MAX];
	char logfile[PATH_MAX];
	char pidfile[PATH_MAX];
	char *cfgfile;
	mode_t filemode;

	bool running;

	const char *api_socket;

	opae_bitstream_info null_gbs[MAX_NULL_GBS];
	unsigned num_null_gbs;

	pthread_t bmc_monitor_thr;
	pthread_t monitor_thr;
	pthread_t event_dispatcher_thr;
	pthread_t events_api_thr;

	fpgad_config_data *supported_devices;
};

extern struct fpgad_config global_config;

/*
** Returns
**  -2 if --help requested
**  -1 on parse error
**   0 on success
*/
int cmd_parse_args(struct fpgad_config *c, int argc, char *argv[]);

void cmd_show_help(FILE *fptr);

// 0 on success
int cmd_canonicalize_paths(struct fpgad_config *c);

void cmd_destroy(struct fpgad_config *c);

bool cmd_path_is_symlink(const char *path);

#endif /* __FPGAD_COMMAND_LINE_H__ */
