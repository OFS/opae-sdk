// Copyright(c) 2023, Intel Corporation
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

#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <signal.h>
#include <getopt.h>
#include <netinet/in.h>

#include <opae/log.h>

#include "grpc_server.hpp"

using grpc::Server;
using grpc::ServerBuilder;

typedef struct _inet_server_config {
  char ip_addr[INET_ADDRSTRLEN];
  in_port_t port;
} inet_server_config;

STATIC inet_server_config config = {
	{ '0', '.', '0', '.', '0', '.', '0', 0 },
	50051
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

std::ostream & show_help(std::ostream &os)
{
  os << "Usage: grpcserver <options>" << std::endl
		 << std::endl
		 //<< "\t-d,--daemon  run as a daemon process." << std::endl
	   //<< "\t-l,--logfile <file> the log file for daemon mode [" << DEFAULT_LOG << "]" << std::endl
		 //<< "\t-p,--pidfile <file> the pid file for daemon mode [" << DEFAULT_PID << "]" << std::endl
		 << "\t-a,--address <ip address>  the IPv4 address for the server. [0.0.0.0]" << std::endl
		 << "\t-p,--port    <port>        the port for the server. [50051]" << std::endl
		 << "\t-h,--help                  display this info and exit." << std::endl
		 << "\t-v,--version               display the version and exit." << std::endl;

	return os;
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
	      show_help(std::cout);
	      return -2;

		case 'v':
				std::cout << "grpcserver " << OPAE_VERSION << ' '
						      << OPAE_GIT_COMMIT_HASH
					        << (OPAE_GIT_SRC_TREE_DIRTY ? "*" : "") << std::endl;
	      return -2;

		case 'a':
			if (tmp_optarg) {
	        len = strnlen(tmp_optarg, 16);
	        memcpy(cfg->ip_addr, tmp_optarg, len);
	        cfg->ip_addr[len] = '\0';
			} else {
					std::cerr << "missing IPv4 address parameter." << std::endl;
	        return 1;
	    }
	    break;

		case 'p':
			if (tmp_optarg) {
				char *endptr = NULL;
				cfg->port = (in_port_t)strtoul(tmp_optarg, &endptr, 0);
				if (endptr != tmp_optarg + strlen(tmp_optarg)) {
          std::cerr << tmp_optarg << " is not a valid port." << std::endl;
					return 1;
				}
			} else {
        std::cerr << "missing port parameter." << std::endl;
	      return 1;
	    }
	    break;

	   case ':':
       std::cerr << "Missing option argument." << std::endl;
	     return 1;

	   case '?':
       std::cerr << "Invalid command option." << std::endl;
	     return 1;

	   default:
       std::cerr << "Invalid command option." << std::endl;
	     return 1;
		}
	}

	return 0;
}

std::unique_ptr<Server> server;
std::mutex m;
std::condition_variable cv;
volatile bool shutting_down = false;

void shutdown_thread()
{
  std::unique_lock<std::mutex> l(m);
  cv.wait(l, []{ return shutting_down; });

  std::cout << "Server shutting down." << std::endl;
  server->Shutdown();

  l.unlock();
}

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	UNUSED_PARAM(info);
	UNUSED_PARAM(unused);

	switch (sig) {
	case SIGINT :
	case SIGTERM: {
			std::lock_guard<std::mutex> l(m);
			shutting_down = true;
	  }
		cv.notify_one();
		break;
	}
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	int res;

	res = parse_args(&config, argc, argv);
	if (res) {
		if (res != -2)
			show_help(std::cout);
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

  std::thread shutdown(shutdown_thread);

  OPAEServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  std::ostringstream oss;
	oss << config.ip_addr << ':' << config.port;

  ServerBuilder builder;
  builder.AddListeningPort(oss.str(), grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  server = builder.BuildAndStart();
  std::cout << "Server listening on " << oss.str() << std::endl;

  server->Wait();
	shutdown.join();

	return 0;
}
