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
#include <opae/log.h>

#include "opae_int.h"
#include "mock/opae_std.h"

#include "action.h"
#include "udsserv.h"
#include "handle_client.h"

volatile bool running = true;

void sig_handler(int sig, siginfo_t *info, void *unused)
{
(void) info;
(void) unused;

	switch (sig) {
	case SIGINT :
	case SIGTERM:
		running = false;
		break;
	}
}

int init_remote_context(uds_server_context *c, nfds_t i)
{
	opae_remote_context *remc =
		opae_malloc(sizeof(opae_remote_context));

	opae_init_remote_context(remc);

	c->remote_context[i] = remc;

	return 0;
}

int release_remote_context(uds_server_context *c, nfds_t i)
{
	opae_remote_context *remc =
		(opae_remote_context *)c->remote_context[i];

	opae_release_remote_context(remc);

	opae_free(remc);

	return 0;
}

int main(int argc, char *argv[])
{
	UNUSED_PARAM(argc);
	UNUSED_PARAM(argv);

	uds_server_context serv;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = sig_handler;

	if ((sigaction(SIGINT, &sa, NULL) < 0) ||
	    (sigaction(SIGTERM, &sa, NULL) < 0)) {
		OPAE_ERR("sigaction() failed");
		return 1;
	}

	if (uds_server_init(&serv, SOCKET_NAME, &running))
		return 2;

	serv.handle_client_message = handle_client;
	serv.init_remote_context = init_remote_context;
	serv.release_remote_context = release_remote_context;

	uds_server_poll_loop(&serv);

	uds_server_release(&serv);

	return 0;
}
