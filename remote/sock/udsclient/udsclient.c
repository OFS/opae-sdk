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

#include <opae/log.h>

#include "udsclient.h"

int uds_client_init(uds_client_context *c,
		    const char *socket_name)
{
	size_t len;
	struct sockaddr_un addr;

	memset(c, 0, sizeof(*c));

	len = strnlen(socket_name, SOCKET_NAME_MAX);
	memcpy(c->socket_name, socket_name, len);
	c->socket_name[len] = '\0';

	c->client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (c->client_socket < 0) {
		OPAE_ERR("socket() failed: %s", strerror(errno));
		return 1;
	}

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, c->socket_name, len);
	addr.sun_path[len] = '\0';

	if (connect(c->client_socket,
		    (struct sockaddr *)&addr,
		    sizeof(addr)) < 0) {
		OPAE_ERR("connect() failed: %s", strerror(errno));
		close(c->client_socket);
		return 2;
	}

	return 0;
}

void uds_client_release(uds_client_context *c)
{
	close(c->client_socket);
}
