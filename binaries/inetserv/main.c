// Copyright(c) 2022, Intel Corporation
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
#include <getopt.h>

#include <opae/log.h>

#include "opae_int.h"
#include "mock/opae_std.h"

#include "action.h"

#include "cfg.h"
#include "inetserv.h"
#include "handle_client.h"

STATIC inet_server_config the_config = {
	true,
	{ '0', '.', '0', '.', '0', '.', '0', 0 },
	0
};

#define OPT_STR ":hdl:P:va:p:"

STATIC struct option longopts[] = {
	{ "help",    no_argument,       NULL, 'h' },
	{ "daemon",  no_argument,       NULL, 'd' },
	{ "logfile", required_argument, NULL, 'l' },
	{ "pidfile", required_argument, NULL, 'P' },
	{ "version", no_argument,       NULL, 'v' },
	{ "address", required_argument, NULL, 'a' },
	{ "port",    required_argument, NULL, 'p' },

	{ 0,         0,                 0,    0   }
};

void show_help(FILE *fptr)
{
	fprintf(fptr, "Usage: inetserv <options>\n");
	fprintf(fptr, "\n");
	//fprintf(fptr, "\t-d,--daemon                 run as daemon process.\n");
	//fprintf(fptr, "\t-l,--logfile <file>         the log file for daemon mode [%s].\n", DEFAULT_LOG);
	//fprintf(fptr, "\t-p,--pidfile <file>         the pid file for daemon mode [%s].\n", DEFAULT_PID);
	fprintf(fptr, "\t-a,--address <ip address>   the IPv4 address for the server. [0.0.0.0]\n");
	fprintf(fptr, "\t-p,--port    <port>         the port for the server. [0]\n");
	fprintf(fptr, "\t-h,--help                   display this info and exit.\n");
	fprintf(fptr, "\t-v,--version                display the version and exit.\n");
}

int parse_args(inet_server_config *cfg, int argc, char *argv[])
{
	int getopt_ret;
	int option_index = 0;
	size_t len;

	while (-1 != (getopt_ret = getopt_long(argc, argv, OPT_STR, longopts, &option_index))) {
	        const char *tmp_optarg = optarg;

	        if (optarg && ('=' == *tmp_optarg))
	                ++tmp_optarg;

	        if (!optarg && (optind < argc) &&
	                (NULL != argv[optind]) &&
	                ('-' != argv[optind][0]))
	                tmp_optarg = argv[optind++];


	        switch (getopt_ret) {
	        case 'h':
	                show_help(stdout);
	                return -2;

		case 'v':
			fprintf(stdout, "inetserv %s %s%s\n",
	                                OPAE_VERSION,
	                                OPAE_GIT_COMMIT_HASH,
	                                OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
	                return -2;

		case 'a':
			if (tmp_optarg) {
	                        len = strnlen(tmp_optarg, 16);
	                        memcpy(cfg->ip_addr, tmp_optarg, len);
	                        cfg->ip_addr[len] = '\0';
			} else {
	                        fprintf(stderr, "missing IPv4 address parameter.\n");
	                        return 1;
	                }
	                break;

		case 'p':
			if (tmp_optarg) {
				char *endptr = NULL;
				cfg->port = (in_port_t)strtoul(tmp_optarg, &endptr, 0);
				if (endptr != tmp_optarg + strlen(tmp_optarg)) {
					fprintf(stderr, "%s is not a valid port.\n", tmp_optarg);
					return 1;
				}
			} else {
	                        fprintf(stderr, "missing port parameter.\n");
	                        return 1;
	                }
	                break;

	        case ':':
	                fprintf(stderr, "Missing option argument.\n");
	                return 1;

	        case '?':
	                fprintf(stderr, "Invalid command option.\n");
	                return 1;

	        default:
	                fprintf(stderr, "Invalid command option.\n");
	                return 1;
		}
	}

	return 0;
}

void sig_handler(int sig, siginfo_t *info, void *unused)
{
(void) info;
(void) unused;

	switch (sig) {
	case SIGINT :
	case SIGTERM:
		the_config.running = false;
		break;
	}
}

int init_remote_context(inet_server_context *c, nfds_t i)
{
	opae_remote_context *remc =
		opae_malloc(sizeof(opae_remote_context));

	opae_init_remote_context(remc);

	c->remote_context[i] = remc;

	return 0;
}

int release_remote_context(inet_server_context *c, nfds_t i)
{
	opae_remote_context *remc =
		(opae_remote_context *)c->remote_context[i];

	opae_release_remote_context(remc);

	opae_free(remc);

	return 0;
}

int main(int argc, char *argv[])
{
	inet_server_context serv;
	struct sigaction sa;
	int res;

	res = parse_args(&the_config, argc, argv);
	if (res) {
		if (res != -2)
			show_help(stdout);
		return res;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = sig_handler;

	if ((sigaction(SIGINT, &sa, NULL) < 0) ||
	    (sigaction(SIGTERM, &sa, NULL) < 0)) {
		OPAE_ERR("sigaction() failed");
		return 2;
	}

	if (inet_server_init(&serv, &the_config))
		return 3;

	serv.handle_client_message = handle_client;
	serv.init_remote_context = init_remote_context;
	serv.release_remote_context = release_remote_context;

	inet_server_poll_loop(&serv);

	inet_server_release(&serv);

	return 0;
}
