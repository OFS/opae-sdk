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
 * fpgad: FPGA system daemon
 *
 * System deamon to
 *
 * - monitor FPGA status (errors, power, thermal)
 * - relay error/power/thermal events to FPGA API applications
 */

#include "errtable.h"
#include "srv.h"
#include "ap6.h"
#include "config_int.h"
#include "log.h"
#include "ap_event.h"
#include <getopt.h>

#define OPT_STR ":hdD:l:p:m:s:n:"

struct option longopts[] = {
	{ "help",           no_argument,       NULL, 'h' },
	{ "daemon",         no_argument,       NULL, 'd' },
	{ "directory",      required_argument, NULL, 'D' },
	{ "logfile",        required_argument, NULL, 'l' },
	{ "pidfile",        required_argument, NULL, 'p' },
	{ "umask",          required_argument, NULL, 'm' },
	{ "socket",         required_argument, NULL, 's' },
	{ "null-bitstream", required_argument, NULL, 'n' },

	{ 0, 0, 0, 0 }
};

void show_help(void)
{
	FILE *fp = stdout;

	fprintf(fp, "Usage: fpgad <options>\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t-d,--daemon                 run as daemon process.\n");
	fprintf(fp, "\t-D,--directory <dir>        the working directory for daemon mode [/tmp].\n");
	fprintf(fp, "\t-l,--logfile <file>         the log file for daemon mode [/tmp/fpgad.log].\n");
	fprintf(fp, "\t-p,--pidfile <file>         the pid file for daemon mode [/tmp/fpgad.pid].\n");
	fprintf(fp, "\t-m,--umask <mode>           the file mode for daemon mode [0].\n");
	fprintf(fp, "\t-s,--socket <sock>          the unix domain socket [/tmp/fpga_event_socket].\n");
	fprintf(fp, "\t-n,--null-bitstream <file>  NULL bitstream (for AP6 handling, may be\n"
		    "\t                            given multiple times).\n");
}

struct config config = {
	.verbosity = 0,
	.poll_interval_usec = 100 * 1000,
	.daemon = 0,
	.directory = "/tmp",
	.logfile = "/tmp/fpgad.log",
	.pidfile = "/tmp/fpgad.pid",
	.filemode = 0,
	.running = true,
	.socket = "/tmp/fpga_event_socket",
	.null_gbs = {0},
	.num_null_gbs = 0,
};

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	UNUSED_PARAM(info);
	UNUSED_PARAM(unused);
	switch (sig) {
	case SIGINT:
		// Process terminated.
		dlog("Got SIGINT. Exiting.\n");
		config.running = false;
		break;
	}
}

int main(int argc, char *argv[])
{
	int getopt_ret;
	int option_index;
	int res;
	int i;
	int j;
	pthread_t logger;
	pthread_t server;
	pthread_t apevent;
	pthread_t ap6[MAX_SOCKETS]; /* one per socket */
	struct ap6_context context[MAX_SOCKETS];

	fLog = stdout;

	while (-1 != (getopt_ret = getopt_long(argc, argv, OPT_STR, longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg))
			++tmp_optarg;

		if (!optarg && (NULL != argv[optind]) &&
						('-' != argv[optind][0]))
			tmp_optarg = argv[optind++];

		switch (getopt_ret) {
		case 'h':
			show_help();
			return 0;
			break;

		case 'd':
			config.daemon = 1;
			dlog("daemon requested\n");
			break;

		case 'D':
			if (tmp_optarg) {
				config.directory = tmp_optarg;
				dlog("daemon directory is %s\n", config.directory);
			} else {
				fprintf(stderr, "missing directory parameter.\n");
				return 1;
			}
			break;

		case 'l':
			if (tmp_optarg) {
				config.logfile = tmp_optarg;
				dlog("daemon log file is %s\n", config.logfile);
			} else {
				fprintf(stderr, "missing logfile parameter.\n");
				return 1;
			}
			break;

		case 'p':
			if (tmp_optarg) {
				config.pidfile = tmp_optarg;
				dlog("daemon pid file is %s\n", config.pidfile);
			} else {
				fprintf(stderr, "missing pidfile parameter.\n");
				return 1;
			}
			break;

		case 'm':
			if (tmp_optarg) {
				config.filemode = (mode_t) strtoul(tmp_optarg, NULL, 0);
				dlog("daemon umask is 0x%x\n", config.filemode);
			} else {
				fprintf(stderr, "missing filemode parameter.\n");
				return 1;
			}
			break;

		case 'n':
			if (tmp_optarg) {
				if (config.num_null_gbs < MAX_NULL_GBS) {
					config.null_gbs[config.num_null_gbs++] = tmp_optarg;
					dlog("registering NULL bitstream \"%s\"\n", tmp_optarg);
					/* TODO: check NULL bitstream
					 * compatibility */
				} else {
					dlog("maximum number of NULL bitstreams exceeded, ignoring -n option\n");
				}
			} else {
				fprintf(stderr, "missing bitstream parameter.\n");
				return 1;
			}
			break;

		case 's':
			if (tmp_optarg) {
				config.socket = tmp_optarg;
				dlog("daemon socket is %s\n", config.socket);
			} else {
				fprintf(stderr, "missing socket parameter.\n");
				return 1;
			}
			break;

		case ':':
			dlog("Missing option argument.\n");
			return 1;
			break;

		case '?':
		default:
			dlog("Invalid command option.\n");
			return 1;
			break;

		}

	}

	if (config.daemon) {
		FILE *fp;

		/* daemonize also registers sig_handler for SIGINT */
		res = daemonize(sig_handler, config.filemode, config.directory);
		if (res != 0) {
			dlog("daemonize failed: %s\n", strerror(res));
			return 1;
		}

		fp = fopen(config.pidfile, "w");
		if (NULL == fp) {
			return 1;
		}
		fprintf(fp, "%d\n", getpid());
		fclose(fp);

		if (open_log(config.logfile) != 0)
			fprintf(stderr, "failed to open logfile\n");


	} else {
		struct sigaction sa;

		memset(&sa, 0, sizeof(sa));
		sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
		sa.sa_sigaction = sig_handler;

		res = sigaction(SIGINT, &sa, NULL);
		if (res < 0) {
			dlog("failed to register signal handler.\n");
			return 1;
		}

	}

	for (i = 0; i < MAX_SOCKETS; i++) {
		sem_init(&ap6_sem[i], 0, 0);

		context[i].config = &config;
		context[i].socket = i;
		res = pthread_create(&ap6[i], NULL, ap6_thread, &context[i]);
		if (res) {
			dlog("failed to create AP6 thread #%i.\n", i);
			config.running = false;
			for (j = 0; j < i; j++)
				pthread_join(ap6[j], NULL);
			return 1;
		}
	}

	res = pthread_create(&logger, NULL, logger_thread, &config);
	if (res) {
		dlog("failed to create logger thread.\n");
		config.running = false;
		for (i = 0; i < MAX_SOCKETS; i++)
			pthread_join(ap6[i], NULL);
		return 1;
	}

	res = pthread_create(&server, NULL, server_thread, &config);
	if (res) {
		dlog("failed to create server thread.\n");
		config.running = false;
		for (i = 0; i < MAX_SOCKETS; i++)
			pthread_join(ap6[i], NULL);
		pthread_join(logger, NULL);
		return 1;
	}

	// AP event
	res = pthread_create(&apevent, NULL, apevent_thread, &config);
	if (res) {
		dlog("failed to create apevent thread.\n");
		config.running = false;
		for (i = 0; i < MAX_SOCKETS; i++)\
			pthread_join(ap6[i], NULL);
		pthread_join(logger, NULL);
		pthread_join(server, NULL);
		return 1;
	}

	pthread_join(logger, NULL);
	pthread_join(server, NULL);
	pthread_join(apevent, NULL);

	for (i = 0; i < MAX_SOCKETS; i++)
		pthread_join(ap6[i], NULL);

	if (stdout != fLog)
		close_log();

	if (config.daemon)
		remove(config.pidfile);

	return 0;
}

