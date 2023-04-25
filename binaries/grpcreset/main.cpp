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
#endif  // HAVE_CONFIG_H

#include <getopt.h>
#include <netinet/in.h>

#include <iostream>
#include <sstream>

#include "grpc_client.hpp"

typedef struct _reset_config {
  char server_host[HOST_NAME_MAX];
  in_port_t server_port;
  bool debug;
} reset_config;

STATIC reset_config config = {{
                                  0,
                              },
                              50051,
                              false};

#define OPT_STR ":hgvs:p:"

STATIC struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                                   {"debug", no_argument, NULL, 'g'},
                                   {"version", no_argument, NULL, 'v'},
                                   {"server", required_argument, NULL, 's'},
                                   {"port", required_argument, NULL, 'p'},

                                   {0, 0, 0, 0}};

std::ostream &show_help(std::ostream &os) {
  os << "Usage: grpcreset <options>" << std::endl
     << std::endl
     << "\t-g,--debug                 enable debug logging." << std::endl
     << "\t-s,--server <hostname>     the host name for the server. "
     << std::endl
     << "\t-p,--port    <port>        the port for the server. [50051]"
     << std::endl
     << "\t-h,--help                  display this info and exit." << std::endl
     << "\t-v,--version               display the version and exit."
     << std::endl;

  return os;
}

int parse_args(reset_config *cfg, int argc, char *argv[]) {
  int getopt_ret;
  int option_index = 0;
  size_t len;

  while (-1 != (getopt_ret = getopt_long(argc, argv, OPT_STR, longopts,
                                         &option_index))) {
    const char *tmp_optarg = optarg;

    if (optarg && ('=' == *tmp_optarg)) ++tmp_optarg;

    if (!optarg && (optind < argc) && (NULL != argv[optind]) &&
        ('-' != argv[optind][0]))
      tmp_optarg = argv[optind++];

    switch (getopt_ret) {
      case 'h':
        show_help(std::cout);
        return -2;

      case 'v':
        std::cout << "grpcreset " << OPAE_VERSION << ' ' << OPAE_GIT_COMMIT_HASH
                  << (OPAE_GIT_SRC_TREE_DIRTY ? "*" : "") << std::endl;
        return -2;

      case 'g':
        cfg->debug = true;
        break;

      case 's':
        if (tmp_optarg) {
          len = strnlen(tmp_optarg, HOST_NAME_MAX);
          memcpy(cfg->server_host, tmp_optarg, len);
          cfg->server_host[len] = '\0';
        } else {
          std::cerr << "missing server host name parameter." << std::endl;
          return 1;
        }
        break;

      case 'p':
        if (tmp_optarg) {
          char *endptr = NULL;
          cfg->server_port = (in_port_t)strtoul(tmp_optarg, &endptr, 0);
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

int main(int argc, char *argv[]) {
  int res;

  res = parse_args(&config, argc, argv);
  if (res) {
    if (res != -2) show_help(std::cout);
    return res;
  }

  std::ostringstream oss;
  oss << config.server_host << ':' << config.server_port;
  if (config.debug) std::cout << "Resetting server " << oss.str() << std::endl;

  OPAEClient *client = new OPAEClient(
      grpc::CreateChannel(oss.str(), grpc::InsecureChannelCredentials()),
      config.debug);

  res = (int)client->ServerReset();

  delete client;

  return res;
}
