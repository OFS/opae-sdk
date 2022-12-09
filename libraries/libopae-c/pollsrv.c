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

#include <linux/un.h>

#include "mock/opae_std.h"

#include "pollsrv.h"

void opae_poll_server_init(opae_poll_server *psrv, int server_socket)
{
	memset(psrv, 0, sizeof(*psrv));

	psrv->pollfds[OPAE_POLLSRV_SRV_SOCKET].fd = server_socket;
	psrv->pollfds[OPAE_POLLSRV_SRV_SOCKET].events = POLLIN | POLLPRI;
	psrv->num_fds = 1;
	psrv->poll_timeout = OPAE_POLLSRV_POLL_TIMEOUT;
	psrv->running = true;
}

int opae_uds_server_init(opae_uds_server *srv,
			 const char *socket_name)
{
	size_t len;
	int server_socket;
	struct sockaddr_un addr;
	int res;

	memset(srv->socket_name, 0, sizeof(srv->socket_name));

	len = strnlen(socket_name, OPAE_UDS_SOCKET_NAME_MAX);
	memcpy(srv->socket_name, socket_name, len);
	srv->socket_name[len] = '\0';

	unlink(srv->socket_name);

	server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_socket < 0) {
		OPAE_ERR("socket() failed: %s", strerror(errno));
		return 1;
	}

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, srv->socket_name, len);
	addr.sun_path[len] = '\0';

	res = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
	if (res < 0) {
		OPAE_ERR("bind() failed: %s", strerror(errno));
		close(server_socket);
		return 2;
	}

	res = listen(server_socket, OPAE_POLLSRV_LISTEN_BACKLOG);
	if (res < 0) {
		OPAE_ERR("listen() failed: %s", strerror(errno));
		close(server_socket);
		return 3;
	}

	opae_poll_server_init(&srv->psrv, server_socket);

	return 0;
}

void opae_uds_server_release(void *srv)
{
	opae_uds_server *p = (opae_uds_server *)srv;

	opae_poll_server_release(&p->psrv);
	unlink(p->socket_name);
}

void opae_uds_server_release_free(void *srv)
{
	opae_uds_server_release(srv);
	opae_free(srv);
}

int opae_inet_server_init(opae_inet_server *srv,
			  const char *ip_addr,
			  in_port_t port)
{
	int server_socket;
	struct sockaddr_in sin;
	size_t len;
	int res;

	memset(srv->ip_addr, 0, sizeof(srv->ip_addr));
	len = strnlen(ip_addr, INET_ADDRSTRLEN - 1);
	memcpy(srv->ip_addr, ip_addr, len + 1);
	srv->ip_addr[len] = '\0';

	srv->port = port;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		OPAE_ERR("socket() failed: %s", strerror(errno));
		return 1;
	}

	sin.sin_family = AF_INET;

	res = inet_pton(AF_INET, srv->ip_addr, &sin.sin_addr);
	if (res != 1) {
		OPAE_ERR("inet_pton() failed: %s", strerror(errno));
		return 2;
	}

	sin.sin_port = htons(srv->port);

	res = bind(server_socket, (struct sockaddr *)&sin, sizeof(sin));
	if (res < 0) {
		OPAE_ERR("bind() failed: %s", strerror(errno));
		close(server_socket);
		return 3;
	}

	res = listen(server_socket, OPAE_POLLSRV_LISTEN_BACKLOG);
	if (res < 0) {
		OPAE_ERR("listen() failed: %s", strerror(errno));
		close(server_socket);
		return 4;
	}

	opae_poll_server_init(&srv->psrv, server_socket);

	return 0;
}

void opae_inet_server_release(void *srv)
{
	opae_inet_server *p = (opae_inet_server *)srv;

	opae_poll_server_release(&p->psrv);
}

void opae_inet_server_release_free(void *srv)
{
	opae_inet_server_release(srv);
	opae_free(srv);
}

int opae_poll_server_loop(opae_poll_server *psrv)
{
	int res;
	nfds_t i;

	while (psrv->running) {

		res = poll(psrv->pollfds, psrv->num_fds, psrv->poll_timeout);
		if (res < 0) {
			OPAE_ERR("poll() error: %s", strerror(errno));
			continue;
		}

		if (!res) { // timeout
			if (psrv->on_poll_timeout)
				psrv->on_poll_timeout(psrv);
			continue;
		}

		if ((nfds_t)res > psrv->num_fds) { // strange
			OPAE_ERR("something went wrong during poll()");
			continue;
		}

		// Handle requests arriving from existing clients.
		for (i = OPAE_POLLSRV_FIRST_CLIENT_SOCKET ; i < psrv->num_fds ; ++i) {
			if (psrv->pollfds[i].revents) {
				psrv->handle_client_message(psrv,
							    psrv->remote_context[i],
							    psrv->pollfds[i].fd);
			}
		}

		// Handle new client connection requests.
		if (psrv->pollfds[OPAE_POLLSRV_SRV_SOCKET].revents) {
			int client_sock;

			if (psrv->num_fds > OPAE_POLLSRV_MAX_CLIENTS) {
				OPAE_ERR("server exceeded max connections. "
					 "Rejecting new connection request.");
				continue;
			}

			client_sock = accept(
				psrv->pollfds[OPAE_POLLSRV_SRV_SOCKET].fd,
				NULL,
				NULL);

			if (client_sock < 0) {
				OPAE_ERR("accept() failed: %s", strerror(errno));
			} else {
				OPAE_DBG("accepting new connection");

				psrv->pollfds[psrv->num_fds].fd = client_sock;
				psrv->pollfds[psrv->num_fds].events = POLLIN | POLLPRI;

				if (psrv->init_remote_context)
					psrv->init_remote_context(psrv, psrv->num_fds);

				++psrv->num_fds;
			}
		}

	}

	return 0;
}

void opae_poll_server_close_client(opae_poll_server *psrv, int client_sock)
{
	nfds_t i;
	nfds_t j;
	nfds_t removed = 0;

	for (i = j = OPAE_POLLSRV_FIRST_CLIENT_SOCKET ; i < psrv->num_fds ; ++i) {
		if (client_sock != psrv->pollfds[i].fd) {
			if (j != i)
				psrv->pollfds[j] = psrv->pollfds[i];
			++j;
		} else {
			if (psrv->release_remote_context)
				psrv->release_remote_context(psrv, i);
			++removed;
		}

	}

	close(client_sock);
	psrv->num_fds -= removed;
}

void opae_poll_server_release(opae_poll_server *psrv)
{
	nfds_t i;

	for (i = OPAE_POLLSRV_FIRST_CLIENT_SOCKET ; i < psrv->num_fds ; ++i)
		opae_poll_server_close_client(psrv, psrv->pollfds[i].fd);

	close(psrv->pollfds[OPAE_POLLSRV_SRV_SOCKET].fd);
	psrv->num_fds = 0;
}
