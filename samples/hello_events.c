// Copyright(c) 2017, Intel Corporation
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <errno.h>

#include "opae/fpga.h"
#include "types_int.h"
#include "common_int.h"

int usleep(unsigned);

#define FME_SYSFS_INJECT_ERROR "errors/inject_error"

#define ON_ERR_GOTO(res, label, desc)                    \
	do {                                       \
		if ((res) != FPGA_OK) {            \
			print_err((desc), (res));  \
			goto label;                \
		}                                  \
	} while (0)

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

// RAS Error Inject CSR
struct ras_inject_error {
	union {
		uint64_t csr;
		struct {
			/* Catastrophic  error */
			uint64_t  catastrophicr_error : 1;
			/* Fatal error */
			uint64_t  fatal_error : 1;
			/* Non-fatal error */
			uint64_t  nonfatal_error : 1;
			/* Reserved */
			uint64_t  rsvd : 61;
		};
	};
};

static fpga_result inject_ras_fatal_error(fpga_token token, uint8_t err)
{
	struct _fpga_token  *_token           = NULL;
	struct ras_inject_error  inj_error    = { {0} };
	char sysfs_path[SYSFS_PATH_MAX]       = {0};
	fpga_result result                    = FPGA_OK;

	_token = (struct _fpga_token *)token;
	if (_token == NULL) {
		printf("Token not found\n");
		return FPGA_INVALID_PARAM;
	}

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
			_token->sysfspath,
			FME_SYSFS_INJECT_ERROR);

	inj_error.fatal_error = err;

	result = sysfs_write_u64(sysfs_path, inj_error.csr);
	if (result != FPGA_OK) {
		printf("Failed to write RAS inject errors\n");
		return result;
	}

	return result;
}

int main(int argc, char *argv[])
{
	fpga_properties    filter = NULL;
	fpga_token         fpga_device_token;
	fpga_handle        fpga_device_handle;
	uint32_t           num_matches;
	fpga_result res;
	fpga_event_handle eh;
	uint64_t count = 0;
	pid_t pid;
	struct pollfd pfd;
	int timeout = 10000;

	UNUSED_PARAM(argc);
	UNUSED_PARAM(argv);

	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_destroy_prop, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

	res = fpgaEnumerate(&filter, 1, &fpga_device_token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_tok, "enumerating accelerators");

	if (num_matches < 1) {
		fprintf(stderr, "accelerator not found.\n");
		res = fpgaDestroyProperties(&filter);
		ON_ERR_GOTO(res, out_destroy_tok, "injecting error");
	}

	pid = fork();
	if (pid == -1) {
		printf("Could not create a thread to inject error");
		goto out_destroy_tok;
	}
	if (pid == 0) {
		usleep(5000000);
		res = inject_ras_fatal_error(fpga_device_token, 1);
		ON_ERR_GOTO(res, out_destroy_tok, "setting inject error register");

		res = inject_ras_fatal_error(fpga_device_token, 0);
		ON_ERR_GOTO(res, out_destroy_tok, "unsetting inject error register");
	} else {
		res = fpgaOpen(fpga_device_token, &fpga_device_handle, FPGA_OPEN_SHARED);
		ON_ERR_GOTO(res, out_close, "opening accelerator");

		res = fpgaCreateEventHandle(&eh);
		ON_ERR_GOTO(res, out_close, "creating event handle");

		res = fpgaRegisterEvent(fpga_device_handle, FPGA_EVENT_ERROR, eh, 0);
		ON_ERR_GOTO(res, out_destroy_eh, "registering an FME event");

		printf("Waiting for interrupts now...\n");

		res = fpgaGetOSObjectFromEventHandle(eh, &pfd.fd);
		ON_ERR_GOTO(res, out_destroy_eh, "getting file descriptor");

		pfd.events = POLLIN;
		res = poll(&pfd, 1, timeout);

		if (res < 0) {
			printf("Poll error errno = %s\n", strerror(errno));
			res = FPGA_EXCEPTION;
			goto out_destroy_eh;
		} else if (res == 0) {
			 printf("Poll timeout occured\n");
			 res = FPGA_EXCEPTION;
			 goto out_destroy_eh;
		} else {
			 printf("FME Interrupt occured\n");
			 read(pfd.fd, &count, sizeof(count));
		}

		res = fpgaUnregisterEvent(fpga_device_handle, FPGA_EVENT_ERROR, eh);
		ON_ERR_GOTO(res, out_destroy_eh, "unregistering an FME event");

		printf("Successfully tested Register/Unregister for FME events!\n");
	}

out_destroy_eh:
	res = fpgaDestroyEventHandle(&eh);
	ON_ERR_GOTO(res, out_close, "deleting event handle");

out_close:
	res = fpgaClose(fpga_device_handle);
	ON_ERR_GOTO(res, out_destroy_tok, "closing accelerator");

out_destroy_tok:
	res = fpgaDestroyToken(&fpga_device_token);
	ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
	return res;
}
