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
#ifndef __OPAE_UDSSERV_H__
#define __OPAE_UDSSERV_H__
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <unistd.h>
#include <poll.h>

#include <opae/log.h>

#define SOCKET_NAME "/tmp/opaeuds"
#define SOCKET_NAME_MAX        256

#define MAX_CLIENT_CONNECTIONS 511
#define SRV_SOCKET               0
#define FIRST_CLIENT_SOCKET      1

#define LISTEN_BACKLOG          16
#define POLL_TIMEOUT           100
typedef struct _uds_server_context {
	char socket_name[SOCKET_NAME_MAX];

	void *remote_context[MAX_CLIENT_CONNECTIONS + 1];
	struct pollfd pollfds[MAX_CLIENT_CONNECTIONS + 1];
	nfds_t num_fds;
	int poll_timeout;

	volatile bool *running;

	int (*handle_client_message)(struct _uds_server_context * , void * , int );
	int (*on_poll_timeout)(struct _uds_server_context *);
	int (*init_remote_context)(struct _uds_server_context * , nfds_t );
	int (*release_remote_context)(struct _uds_server_context * , nfds_t );
} uds_server_context;

int uds_server_init(uds_server_context *c,
		    const char *socket_name,
		    volatile bool *running);

int uds_server_poll_loop(uds_server_context *c);

void uds_server_close_client(uds_server_context *c, int client);

void uds_server_release(uds_server_context *c);

#endif // __OPAE_UDSSERV_H__
