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
#ifndef __OPAE_POLLSRV_H__
#define __OPAE_POLLSRV_H__
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <opae/log.h>

#define OPAE_POLLSRV_MAX_CLIENTS         511
#define OPAE_POLLSRV_SRV_SOCKET            0
#define OPAE_POLLSRV_FIRST_CLIENT_SOCKET   1

#define OPAE_POLLSRV_LISTEN_BACKLOG       16
#define OPAE_POLLSRV_POLL_TIMEOUT        100

typedef struct _opae_poll_server {
	volatile bool running;

	void *remote_context[OPAE_POLLSRV_MAX_CLIENTS + 1];
	struct pollfd pollfds[OPAE_POLLSRV_MAX_CLIENTS + 1];
	nfds_t num_fds;
	int poll_timeout;

	int (*on_poll_timeout)(struct _opae_poll_server *);
	int (*handle_client_message)(struct _opae_poll_server *,
				     void *,
				     int);
	int (*init_remote_context)(struct _opae_poll_server *, nfds_t);
	int (*release_remote_context)(struct _opae_poll_server *, nfds_t);
	void (*release_server)(void *);
} opae_poll_server;

void opae_poll_server_init(opae_poll_server *psrv, int server_socket);

int opae_poll_server_loop(opae_poll_server *psrv);

void opae_poll_server_close_client(opae_poll_server *psrv, int client_sock);

void opae_poll_server_release(opae_poll_server *psrv);


#define OPAE_UDS_SOCKET_NAME_MAX         108
typedef struct _opae_uds_server {
	opae_poll_server psrv;
	char socket_name[OPAE_UDS_SOCKET_NAME_MAX];
} opae_uds_server;

int opae_uds_server_init(opae_uds_server *srv,
			 const char *socket_name);

void opae_uds_server_release(void *srv);
void opae_uds_server_release_free(void *srv);


typedef struct _opae_inet_server {
	opae_poll_server psrv;
	char ip_addr[INET_ADDRSTRLEN];
	in_port_t port;
} opae_inet_server;

int opae_inet_server_init(opae_inet_server *srv,
			  const char *ip_addr,
			  in_port_t port);

void opae_inet_server_release(void *srv);
void opae_inet_server_release_free(void *srv);

#endif // __OPAE_POLLSRV_H__
