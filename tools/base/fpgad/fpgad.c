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

#include <signal.h>
#include "fpgad.h"
#include "monitor_thread.h"
#include "event_dispatcher_thread.h"
#include "events_api_thread.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("main: " format, ##__VA_ARGS__)

struct fpgad_config global_config;

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	UNUSED_PARAM(info);
	UNUSED_PARAM(unused);
	switch (sig) {
	case SIGINT:
		// Process interrupted.
		LOG("Got SIGINT. Exiting.\n");
		global_config.running = false;
		break;
	case SIGTERM:
		// Process terminated.
		LOG("Got SIGTERM. Exiting.\n");
		global_config.running = false;
		break;
	}
}

int main(int argc, char *argv[])
{
	int res;

	memset_s(&global_config, sizeof(global_config), 0);

	global_config.poll_interval_usec = 100 * 1000;
	global_config.running = true;
	global_config.api_socket = "/tmp/fpga_event_socket";
	global_config.num_null_gbs = 0;

	log_set(stdout);

	res = cmd_parse_args(&global_config, argc, argv);
	if (res != 0) {
		if (res == -2)
			res = 0;
		else
			LOG("error parsing command line.\n");
		return res;
	}

	cmd_canonicalize_paths(&global_config);

	if (global_config.daemon) {
		FILE *fp;

		res = daemonize(sig_handler,
				global_config.filemode,
				global_config.directory);
		if (res != 0) {
			LOG("daemonize failed: %s\n", strerror(res));
			goto out_destroy;
		}

		fp = fopen(global_config.pidfile, "w");
		if (NULL == fp) {
			LOG("failed to open pid file\n");
			res = 1;
			goto out_destroy;
		}
		fprintf(fp, "%d\n", getpid());
		fclose(fp);

		if (log_open(global_config.logfile) < 0) {
			LOG("failed to open log file\n");
			res = 1;
			goto out_destroy;
		}

	} else {
		struct sigaction sa;

		memset_s(&sa, sizeof(sa), 0);
		sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
		sa.sa_sigaction = sig_handler;

		res = sigaction(SIGINT, &sa, NULL);
		if (res < 0) {
			LOG("failed to register SIGINT handler.\n");
			goto out_destroy;
		}

		res = sigaction(SIGTERM, &sa, NULL);
		if (res < 0) {
			LOG("failed to register SIGTERM handler.\n");
			goto out_destroy;
		}
	}

	res = mon_enumerate(&global_config);
	if (res) {
		LOG("OPAE device enumeration failed\n");
		goto out_destroy;
	}

	res = pthread_create(&global_config.event_dispatcher_thr,
			     NULL,
			     event_dispatcher_thread,
			     &event_dispatcher_config);
	if (res) {
		LOG("failed to create event_dispatcher_thread\n");
		global_config.running = false;
		goto out_destroy;
	}

	while (!evt_dispatcher_is_ready())
		usleep(1);

	res = pthread_create(&global_config.monitor_thr,
			     NULL,
			     monitor_thread,
			     &monitor_config);
	if (res) {
		LOG("failed to create monitor_thread\n");
		global_config.running = false;
		goto out_stop_event_dispatcher;
	}

	res = pthread_create(&global_config.events_api_thr,
			     NULL,
			     events_api_thread,
			     &events_api_config);
	if (res) {
		LOG("failed to create events_api_thread\n");
		global_config.running = false;
		goto out_stop_monitor;
	}

	if (pthread_join(global_config.events_api_thr, NULL)) {
		LOG("failed to join events_api_thread\n");
	}
out_stop_monitor:
	if (pthread_join(global_config.monitor_thr, NULL)) {
		LOG("failed to join monitor_thread\n");
	}
out_stop_event_dispatcher:
	if (pthread_join(global_config.event_dispatcher_thr, NULL)) {
		LOG("failed to join event_dispatcher_thread\n");
	}
out_destroy:
	mon_destroy();
	cmd_destroy(&global_config);
	log_close();
	return res;
}
