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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <inttypes.h>
#include "events_api_thread.h"
#include "api/opae_events_api.h"

#include "logging.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("events_api_thread: " format, ##__VA_ARGS__)

events_api_thread_config events_api_config = {
	.global = &global_config,
	.sched_policy = SCHED_RR,
	.sched_priority = 10,
};

#define MAX_CLIENT_CONNECTIONS 1023
#define SRV_SOCKET             0
#define FIRST_CLIENT_SOCKET    1

/* array keeping track of all connection file descriptors (plus server socket) */
STATIC struct pollfd pollfds[MAX_CLIENT_CONNECTIONS+1];
STATIC nfds_t num_fds = 1;

STATIC void remove_client(int conn_socket)
{
	nfds_t i, j;
	nfds_t removed = 0;

	opae_api_unregister_all_events_for(conn_socket);
	LOG("closing connection conn_socket=%d.\n", conn_socket);
	close(conn_socket);

	for (i = j = FIRST_CLIENT_SOCKET ; i < num_fds ; ++i) {
		if (conn_socket != pollfds[i].fd) {
			if (j != i)
				pollfds[j] = pollfds[i];
			++j;
		} else {
			++removed;
		}
	}

	num_fds -= removed;
}

STATIC int handle_message(int conn_socket)
{
	struct msghdr mh;
	struct cmsghdr *cmh;
	struct iovec iov[1];
	struct event_request req;
	char buf[CMSG_SPACE(sizeof(int))];
	ssize_t n;
	int *fd_ptr;

	/* set up ancillary data message header */
	iov[0].iov_base = &req;
	iov[0].iov_len = sizeof(req);
	//memset_s(buf, sizeof(buf), 0);
	memset(buf, 0, sizeof(buf));
	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_iov = iov;
	mh.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	mh.msg_control = buf;
	mh.msg_controllen = CMSG_LEN(sizeof(int));
	mh.msg_flags = 0;
	cmh = CMSG_FIRSTHDR(&mh);
	cmh->cmsg_len = CMSG_LEN(sizeof(int));
	cmh->cmsg_level = SOL_SOCKET;
	cmh->cmsg_type = SCM_RIGHTS;

	n = recvmsg(conn_socket, &mh, 0);
	if (n < 0) {
		LOG("recvmsg() failed: %s\n", strerror(errno));
		return (int)n;
	}

	if (!n) { // socket closed by peer
		remove_client(conn_socket);
		return (int)n;
	}

	switch (req.type) {

	case REGISTER_EVENT:
		fd_ptr = (int *)CMSG_DATA(cmh);

		if (opae_api_register_event(conn_socket, *fd_ptr,
				    req.event, req.object_id)) {
			LOG("failed to register event\n");
			return -1;
		}

		LOG("registered event sock=%d:fd=%d"
		     "(event=%d object_id=0x%" PRIx64  ")\n",
			conn_socket, *fd_ptr, req.event, req.object_id);

		break;

	case UNREGISTER_EVENT:

		if (opae_api_unregister_event(conn_socket,
					      req.event,
					      req.object_id)) {
			LOG("failed to unregister event\n");
			return -1;
		}

		LOG("unregistered event sock=%d:"
		     "(event=%d object_id=0x%" PRIx64  ")\n",
			conn_socket, req.event, req.object_id);

		break;

	default:
		LOG("unknown request type %d\n", req.type);
		return -1;
	}

	return 0;
}

STATIC volatile bool evt_api_is_ready = false;

bool events_api_is_ready(void)
{
	return evt_api_is_ready;
}

void *events_api_thread(void *thread_context)
{
	events_api_thread_config *c =
		(events_api_thread_config *)thread_context;
	struct sched_param sched_param;
	int policy = 0;
	int res;

	nfds_t i;
	struct sockaddr_un addr;
	int server_socket;
	int conn_socket;
	//errno_t e;

	LOG("starting\n");

	res = pthread_getschedparam(pthread_self(), &policy, &sched_param);
	if (res) {
		LOG("error getting scheduler params: %s\n", strerror(res));
	} else {
		policy = c->sched_policy;
		sched_param.sched_priority = c->sched_priority;

		res = pthread_setschedparam(pthread_self(),
					    policy,
					    &sched_param);
		if (res) {
			LOG("error setting scheduler params"
			    " (got root?): %s\n", strerror(res));
		}
	}

	unlink(c->global->api_socket);

	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0) {
		LOG("failed to create server socket.\n");
		goto out_exit;
	}
	LOG("created server socket.\n");

	addr.sun_family = AF_UNIX;

/*
	e = strncpy_s(addr.sun_path, sizeof(addr.sun_path),
			c->global->api_socket, PATH_MAX);
	if (EOK != e) {
		LOG("strncpy_s failed\n");
		goto out_close_server;
	}
*/
    strncpy(addr.sun_path, c->global->api_socket, sizeof(addr.sun_path));

	if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG("failed to bind server socket.\n");
		goto out_close_server;
	}
	LOG("server socket bind success.\n");

	if (listen(server_socket, 20) < 0) {
		LOG("failed to listen on socket.\n");
		goto out_close_server;
	}
	LOG("listening for connections.\n");

	evt_api_is_ready = true;

	pollfds[SRV_SOCKET].fd = server_socket;
	pollfds[SRV_SOCKET].events = POLLIN | POLLPRI;
	num_fds = 1;

	while (c->global->running) {

		res = poll(pollfds, num_fds, 100);
		if (res < 0) {
			LOG("poll error\n");
			continue;
		}

		if (0 == res) // timeout
			continue;

		if ((nfds_t)res > num_fds) { // weird
			LOG("something bad happened during poll!\n");
			continue;
		}

		// handle requests on existing sockets
		for (i = FIRST_CLIENT_SOCKET ; i < num_fds ; ++i) {
			if (pollfds[i].revents) {
				handle_message(pollfds[i].fd);
			}
		}

		// handle new connection requests
		if (pollfds[SRV_SOCKET].revents) {

			if (num_fds == MAX_CLIENT_CONNECTIONS+1) {
				LOG("exceeded max connections!\n");
				continue;
			}

			conn_socket = accept(server_socket, NULL, NULL);

			if (conn_socket < 0) {
				LOG("failed to accept new connection!\n");
			} else {
				LOG("accepting connection %d.\n", conn_socket);

				pollfds[num_fds].fd = conn_socket;
				pollfds[num_fds].events = POLLIN | POLLPRI;
				++num_fds;
			}

		}

	}

	opae_api_unregister_all_events();

	// close any active client sockets
	for (i = FIRST_CLIENT_SOCKET ; i < num_fds ; ++i) {
		close(pollfds[i].fd);
	}

out_close_server:
	evt_api_is_ready = false;
	close(server_socket);
out_exit:
	LOG("exiting\n");
	return NULL;
}
