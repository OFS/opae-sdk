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

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <unistd.h>
#include <poll.h>

#include <opae/log.h>

#include "mock/opae_std.h"

#include "uds-ifc.h"

int opae_uds_client_open(void *con)
{
	opae_uds_client_connection *c =
		(opae_uds_client_connection *)con;
	struct sockaddr_un addr;
	size_t len;

	if (c->client_socket >= 0)
		return 0; // already open

	c->client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (c->client_socket < 0) {
		OPAE_ERR("socket() failed: %s", strerror(errno));
		return 1;
	}

	len = strnlen(c->socket_name, OPAE_SOCKET_NAME_MAX);

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, c->socket_name, len);
	addr.sun_path[len] = '\0';

	if (connect(c->client_socket,
		    (struct sockaddr *)&addr,
		    sizeof(addr)) < 0) {
		OPAE_ERR("connect() failed: %s", strerror(errno));
		close(c->client_socket);
		c->client_socket = -1;
		return 2;
	}

	return 0;
}

int opae_uds_client_close(void *con)
{
	opae_uds_client_connection *c =
		(opae_uds_client_connection *)con;

	if (c->client_socket >= 0) {
		close(c->client_socket);
		c->client_socket = -1;
	}

	return 0;
}

int opae_uds_client_release(void *con)
{
	opae_free(con);
	return 0;
}

ssize_t opae_uds_client_send(void *con, const void *buf, size_t len)
{
	opae_uds_client_connection *c =
		(opae_uds_client_connection *)con;

	if (c->client_socket < 0) {
		OPAE_ERR("invalid client_socket");
		return -1;
	}

	return chunked_send(c->client_socket, buf, len, c->send_flags);
}

ssize_t opae_uds_client_receive(void *con, void *buf, size_t len)
{
	opae_uds_client_connection *c =
		(opae_uds_client_connection *)con;
	ssize_t total_bytes;

	if (c->client_socket < 0) {
		OPAE_ERR("invalid client_socket");
		return -1;
	}

	total_bytes = chunked_recv(c->client_socket,
				   buf,
				   len,
				   c->receive_flags);

	if (total_bytes == -2) {
		// Orderly shutdown by peer.
		OPAE_ERR("%s: peer closed connection",
			 c->socket_name);
		opae_uds_client_close(con);
	}

	return total_bytes;
}

int opae_uds_ifc_init(opae_remote_client_ifc *i,
		      const char *socket_name,
		      int send_flags,
		      int receive_flags)
{
	opae_uds_client_connection *con =
		(opae_uds_client_connection *)calloc(1, sizeof(*con));
	size_t len;

	if (!con) {
		OPAE_ERR("calloc failed");
		return 1;
	}

	len = strnlen(socket_name, OPAE_SOCKET_NAME_MAX);

	memcpy(con->socket_name, socket_name, len);
	con->client_socket = -1;
	con->send_flags = send_flags;
	con->receive_flags = receive_flags;

	i->type = OPAE_CLIENT_UDS;
	i->open = opae_uds_client_open;
	i->close = opae_uds_client_close;
	i->release = opae_uds_client_release;
	i->send = opae_uds_client_send;
	i->receive = opae_uds_client_receive;
	i->connection = con;

	return 0;
}
