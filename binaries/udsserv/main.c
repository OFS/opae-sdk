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

opae_uds_server srv;

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	UNUSED_PARAM(info);
	UNUSED_PARAM(unused);

	switch (sig) {
	case SIGINT :
	case SIGTERM:
		srv.psrv.running = false;
		break;
	}
}

int main(int argc, char *argv[])
{
	UNUSED_PARAM(argc);
	UNUSED_PARAM(argv);

	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = sig_handler;

	if ((sigaction(SIGINT, &sa, NULL) < 0) ||
	    (sigaction(SIGTERM, &sa, NULL) < 0)) {
		OPAE_ERR("sigaction() failed");
		return 1;
	}

	if (opae_uds_server_init(&srv, "/tmp/opaeuds"))
		return 2;

	srv.psrv.handle_client_message = opae_poll_server_handle_client;
	srv.psrv.init_remote_context = opae_poll_server_init_remote_context;
	srv.psrv.release_remote_context = opae_poll_server_release_remote_context;

	opae_poll_server_loop(&srv.psrv);

	opae_uds_server_release(&srv);

	return 0;
}
