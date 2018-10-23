// Copyright(c) 2018, Intel Corporation
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

/**
 * \signal_handler.c
 * \brief FPGA DMA User-mode driver
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

// For signal handler - need to properly handle HUP
static void sig_handler(int sig, siginfo_t *signfo, void *unused);
static volatile uint32_t *CsrControl;

int fpgaDMA_setup_sig_handler(fpga_dma_handle_t *dma_h)
{
	struct sigaction sa;
	int sigres;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = sig_handler;

	sigres = sigaction(SIGHUP, &sa, &dma_h->old_action);
	if (sigres < 0) {
		CsrControl = NULL;
		return sigres;
	}
	CsrControl = HOST_MMIO_32_ADDR(dma_h, CSR_CONTROL(dma_h));

	return 0;
}

void fpgaDMA_restore_sig_handler(fpga_dma_handle_t *dma_h)
{
	int sigres;

	if (CsrControl) {
		sigres = sigaction(SIGHUP, &dma_h->old_action, NULL);
		if (sigres < 0) {
			error_print(
				"Error: failed to unregister signal handler.\n");
		}
		CsrControl = NULL;
	}
}

static void sig_handler(int sig, siginfo_t *info, void *unused)
{
	(void)(info);
	(void)(unused);

	if (CsrControl == NULL) {
		return;
	}

	switch (sig) {
	case SIGHUP: {
		// Driver removed - shut down!
		*CsrControl = DMA_SHUTDOWN_CTL_VAL;
		fprintf(stderr, "Got SIGHUP. Exiting.\n");
		*CsrControl = DMA_SHUTDOWN_CTL_VAL;
		_exit(-1);
	} break;
	default:
		break;
	}
}
