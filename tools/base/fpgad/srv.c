// Copyright(c) 2017-2018, Intel Corporation
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
 * srv.c : server portion of the daemon
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <inttypes.h>

#include "safe_string/safe_string.h"
#undef _GNU_SOURCE

#include "errtable.h"
#include "srv.h"
#include "config_int.h"
#include "log.h"

#define MAX_PATH_LEN           256
#define MAX_CLIENT_CONNECTIONS 41

pthread_mutex_t list_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
struct client_event_registry *event_registry_list;

enum request_type {
	REGISTER_EVENT = 0,
	UNREGISTER_EVENT = 1
};

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};

struct client_event_registry *register_event(int conn_socket, int fd,
					fpga_event_type e, uint64_t object_id)
{
	struct client_event_registry *r =
		(struct client_event_registry *) malloc(sizeof(*r));
	errno_t err;

	if (!r)
		return NULL;

	r->conn_socket = conn_socket;
	r->fd = fd;
	r->data = 1;
	r->event = e;
	r->object_id = object_id;

	fpgad_mutex_lock(err, &list_lock);

	r->next = event_registry_list;
	event_registry_list = r;

	fpgad_mutex_unlock(err, &list_lock);

	return r;
}

void release_event_registry(struct client_event_registry *r)
{
	close(r->fd);
	free(r);
}

void unregister_event(int conn_socket, fpga_event_type e, uint64_t object_id)
{
	struct client_event_registry *trash;
	struct client_event_registry *save;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	trash = event_registry_list;

	if (!trash) // empty list
		goto out_unlock;

	if ((conn_socket == trash->conn_socket) &&
		(e == trash->event) &&
		(object_id == trash->object_id)) {

		// found at head of list

		event_registry_list = event_registry_list->next;
		release_event_registry(trash);
		goto out_unlock;
	}

	save = trash;
	trash = trash->next;
	while (trash) {

		if ((conn_socket == trash->conn_socket) &&
			(e == trash->event) &&
			(object_id == trash->object_id))
			break;

		save = trash;
		trash = trash->next;
	}

	if (!trash) // not found
		goto out_unlock;

	// found at trash
	save->next = trash->next;
	release_event_registry(trash);

out_unlock:
	fpgad_mutex_unlock(err, &list_lock);
}

struct client_event_registry *
find_event_for(int conn_socket)
{
	struct client_event_registry *r;

	for (r = event_registry_list ; r ; r = r->next)
		if (conn_socket == r->conn_socket)
			break;

	return r;
}

void unregister_all_events_for(int conn_socket)
{
	struct client_event_registry *r;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	r = find_event_for(conn_socket);
	while (r) {
		unregister_event(conn_socket, r->event, r->object_id);
		r = find_event_for(conn_socket);
	}

	fpgad_mutex_unlock(err, &list_lock);
}

void unregister_all_events(void)
{
	struct client_event_registry *r;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	for (r = event_registry_list ; r != NULL ; ) {
		struct client_event_registry *trash;
		trash = r;
		r = r->next;
		release_event_registry(trash);
	}

	event_registry_list = NULL;

	fpgad_mutex_unlock(err, &list_lock);
}

void for_each_registered_event(void (*cb)(struct client_event_registry *,
					  uint64_t object_id,
					  const struct fpga_err *),
			       uint64_t object_id,
			       const struct fpga_err *e) {
	struct client_event_registry *r;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	for (r = event_registry_list; r != NULL; r = r->next) {
		cb(r, object_id, e);
	}

	fpgad_mutex_unlock(err, &list_lock);
}

#define SRV_SOCKET          0
#define FIRST_CLIENT_SOCKET 1
/* array keeping track of all connection file descriptors (plus server socket) */
static struct pollfd pollfds[MAX_CLIENT_CONNECTIONS+1];
static nfds_t num_fds = 1;

void remove_client(int conn_socket)
{
	struct pollfd tmp_pollfds[MAX_CLIENT_CONNECTIONS+1];
	unsigned i, j;

	unregister_all_events_for(conn_socket);
	dlog("server: closing connection %d.\n", conn_socket);
	close(conn_socket);

	for (i = 0 ; i < num_fds ; ++i) {
		tmp_pollfds[i] = pollfds[i];
	}

	for (i = j = 1 ; i < num_fds ; ++i) {
		if (conn_socket == tmp_pollfds[i].fd)
			continue;
		pollfds[j++] = tmp_pollfds[i];
	}

	num_fds = j;
}

int handle_message(int conn_socket)
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
	memset_s(buf, sizeof(buf), 0);
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
		dlog("server: recvmsg() failed: %s\n", strerror(errno));
		return (int)n;
	}

	if (!n) { // socket closed by peer
		remove_client(conn_socket);
		return (int)n;
	}

	switch (req.type) {

	case REGISTER_EVENT:
		fd_ptr = (int *)CMSG_DATA(cmh);

		if (!register_event(conn_socket, *fd_ptr,
				    req.event, req.object_id)) {
			dlog("server: failed to register event\n");
			return -1;
		}

		dlog("server: registered event sock=%d:fd=%d"
		     "(event=%d object_id=%" PRIx64  ")\n",
			conn_socket, *fd_ptr, req.event, req.object_id);

		break;

	case UNREGISTER_EVENT:
		unregister_event(conn_socket, req.event, req.object_id);

		dlog("server: unregistered event sock=%d:"
		     "(event=%d object_id=%" PRIx64  ")\n",
			conn_socket, req.event, req.object_id);

		break;

	default:
		dlog("server: unknown request type %d\n", req.type);
		return -1;
	}

	return 0;
}

void *server_thread(void *thread_context)
{
	struct config *c = (struct config *)thread_context;

	nfds_t i;

	struct sockaddr_un addr;
	int server_socket;
	int conn_socket;

	int res;
	errno_t e;

	unlink(c->socket);

	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0) {
		dlog("server: failed to create server socket.\n");
		return NULL;
	}
	dlog("server: created server socket.\n");

	addr.sun_family = AF_UNIX;

	e = strncpy_s(addr.sun_path, sizeof(addr.sun_path),
			c->socket, MAX_PATH_LEN);
	if (EOK != e) {
		dlog("server: strncpy_s failed\n");
		goto out_close_server;
	}

	if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		dlog("server: failed to bind socket.\n");
		goto out_close_server;
	}
	dlog("server: bind success.\n");

	if (listen(server_socket, 20) < 0) {
		dlog("server: failed to listen on socket.\n");
		goto out_close_server;
	}
	dlog("server: listening for connections.\n");

	pollfds[SRV_SOCKET].fd = server_socket;
	pollfds[SRV_SOCKET].events = POLLIN | POLLPRI;
	num_fds = 1;

	while (c->running) {

		res = poll(pollfds, num_fds, 100);
		if (res < 0) {
			dlog("server: poll error!\n");
			continue;
		}

		if (0 == res) // timeout
			continue;

		if ((nfds_t)res > num_fds) { // weird
			dlog("server: something bad happened during poll!\n");
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
				dlog("server: exceeded max connections!\n");
				continue;
			}

			conn_socket = accept(server_socket, NULL, NULL);

			if (conn_socket < 0) {
				dlog("server: failed to accept new connection!\n");
			} else {
				dlog("server: accepting connection %d.\n", conn_socket);

				pollfds[num_fds].fd = conn_socket;
				pollfds[num_fds].events = POLLIN | POLLPRI;
				++num_fds;
			}

		}

	}

	unregister_all_events();

	// close any active client sockets
	for (i = FIRST_CLIENT_SOCKET ; i < num_fds ; ++i) {
		close(pollfds[i].fd);
	}

out_close_server:
	close(server_socket);
	return NULL;
}

