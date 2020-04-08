// Copyright(c) 2017-2020, Intel Corporation
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
 * @file hello_events.c
 * @brief A code sample of using OPAE event API.
 *
 * This sample starts two processes. One process injects an artificial fatal
 * error to sysfs; while the other tries to asynchronously capture and handle
 * the event. This
 * sample code exercises all major functions of the event API, including
 * creating and destroying event handles, register and unregister events,
 * polling on event file descriptor, and getting the OS object associated with
 * an event. For a full discussion of OPAE event API, refer to event.h.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <poll.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#include <opae/fpga.h>

int usleep(unsigned);

#define FME_SYSFS_INJECT_ERROR "errors/inject_error"

#define ON_ERR_GOTO(res, label, desc)              \
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

// Global configuration of bus, set during parse_args()

struct events_config {
	struct target {
		int bus;
	} target;
}

events_config = {
	.target = {
		.bus = -1
	}
};

fpga_result inject_ras_fatal_error(fpga_token fme_token, uint8_t err)
{
	fpga_result             res1       = FPGA_OK;
	fpga_result             res2       = FPGA_OK;
	fpga_handle             fme_handle = NULL;
	struct ras_inject_error inj_error  = { {0} };
	fpga_object             inj_err_object;

	res1 = fpgaOpen(fme_token, &fme_handle, FPGA_OPEN_SHARED);
	if (res1 != FPGA_OK) {
		OPAE_ERR("Failed to open FPGA");
		return res1;
	}

	res1 = fpgaHandleGetObject(fme_handle, FME_SYSFS_INJECT_ERROR, &inj_err_object, 0);
	ON_ERR_GOTO(res1, out_close, "Failed to get Handle Object");

	// Inject fatal error
	inj_error.fatal_error = err;

	res1 = fpgaObjectWrite64(inj_err_object, inj_error.csr, 0);
	ON_ERR_GOTO(res1, out_destroy_obj, "Failed to Read Object");

out_destroy_obj:
	res2 = fpgaDestroyObject(&inj_err_object);
	ON_ERR_GOTO(res2, out_close, "Failed to Destroy Object");
out_close:
	res2 = fpgaClose(fme_handle);
	if (res2 != FPGA_OK) {
		OPAE_ERR("Failed to close FPGA");
	}

	return res1 != FPGA_OK ? res1 : res2;
}

void *error_thread(void *arg)
{
	fpga_token token = (fpga_token) arg;
	fpga_result res;

	usleep(5000000);
	printf("injecting error\n");
	res = inject_ras_fatal_error(token, 1);
	if (res != FPGA_OK)
		print_err("setting inject error register", res);

	usleep(5000000);
	printf("clearing error\n");
	res = inject_ras_fatal_error(token, 0);
	if (res != FPGA_OK)
		print_err("clearing inject error register", res);

	return NULL;
}

/*
 * Parse command line arguments
 */
#define GETOPT_STRING "B:v"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{ "bus",     required_argument, NULL, 'B' },
		{ "version", no_argument,       NULL, 'v' },
		{ NULL,      0,                 NULL, 0   }
	};

	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING, longopts, &option_index))) {
		const char *tmp_optarg = optarg;
		/* checks to see if optarg is null and if not goes to value of optarg */
		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

	switch (getopt_ret) {
	case 'B': /* bus */
		if (NULL == tmp_optarg)
			return FPGA_EXCEPTION;
		endptr = NULL;
		events_config.target.bus = (int) strtoul(tmp_optarg, &endptr, 0);
		if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
			fprintf(stderr, "invalid bus: %s\n", tmp_optarg);
			return FPGA_EXCEPTION;
		}
		break;

	case 'v': /* version */
		printf("hello_events %s %s%s\n",
		       OPAE_VERSION,
		       OPAE_GIT_COMMIT_HASH,
		       OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
		return -1;

	default: /* invalid option */
		fprintf(stderr, "Invalid cmdline options\n");
		return FPGA_EXCEPTION;
	}
	}

	return FPGA_OK;
}

fpga_result find_fpga(fpga_token *fpga, uint32_t *num_matches)
{
	fpga_properties filter = NULL;
	fpga_result res        = FPGA_OK;
	fpga_result dres       = FPGA_OK;

	/* Get number of FPGAs in system */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting interface ID");

	if (-1 != events_config.target.bus) {
		res = fpgaPropertiesSetBus(filter, events_config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}

	res = fpgaEnumerate(&filter, 1, fpga, 1, num_matches);
	ON_ERR_GOTO(res, out_destroy, "enumerating FPGAs");

out_destroy:
	dres = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(dres, out, "destroying properties object");
out:
	return (res == FPGA_OK) ? dres : res;
}


/* function to get the bus number when there are multiple buses */
fpga_result get_bus(fpga_token tok, uint8_t *bus)
{
	fpga_result res1 = FPGA_OK;
	fpga_result res2 = FPGA_OK;
	fpga_properties props = NULL;

	res1 = fpgaGetProperties(tok, &props);
	ON_ERR_GOTO(res1, out, "reading properties from Token");

	res1 = fpgaPropertiesGetBus(props, bus);
	ON_ERR_GOTO(res1, out_destroy, "Reading bus from properties");

out_destroy:
	res2 = fpgaDestroyProperties(&props);
	ON_ERR_GOTO(res2, out, "fpgaDestroyProps");
out:
	return res1 != FPGA_OK ? res1 : res2;
}

int main(int argc, char *argv[])
{
	fpga_token         fpga_device_token = NULL;
	fpga_handle        fpga_device_handle = NULL;
	uint32_t           num_matches = 1;
	fpga_result        res1 = FPGA_OK;
	fpga_result        res2 = FPGA_OK;
	fpga_event_handle  eh;
	uint64_t           count = 0;
	int                res;
	struct pollfd      pfd;
	int                timeout = 10000;
	int                poll_ret = 0;
	ssize_t            bytes_read = 0;
	uint8_t            bus = 0xff;
	pthread_t          errthr;

	res1 = parse_args(argc, argv);
	if ((int)res1 < 0)
		goto out_exit;
	ON_ERR_GOTO(res1, out_exit, "parsing arguments");

	res1 = find_fpga(&fpga_device_token, &num_matches);
	ON_ERR_GOTO(res1, out_exit, "finding FPGA accelerator");

	if (num_matches < 1) {
		fprintf(stderr, "No matches for bus number provided.\n");
		res1 = FPGA_NOT_FOUND;
		goto out_exit;
	}

	res1 = get_bus(fpga_device_token, &bus);
	ON_ERR_GOTO(res1, out_destroy_tok, "getting bus num");

	if (num_matches > 1) {
		fprintf(stderr, "Found more than one suitable slot. ");
	}

	printf("Running on bus 0x%02x.\n", bus);

	res1 = fpgaOpen(fpga_device_token, &fpga_device_handle, FPGA_OPEN_SHARED);
	ON_ERR_GOTO(res1, out_destroy_tok, "opening accelerator");

	res1 = fpgaCreateEventHandle(&eh);
	ON_ERR_GOTO(res1, out_close, "creating event handle");

	res1 = fpgaRegisterEvent(fpga_device_handle, FPGA_EVENT_ERROR, eh, 0);
	ON_ERR_GOTO(res1, out_destroy_eh, "registering an FME event");

	printf("Waiting for interrupts now...\n");

	res = pthread_create(&errthr, NULL, error_thread, fpga_device_token);
	if (res) {
		printf("Failed to create error_thread.\n");
		res1 = FPGA_EXCEPTION;
		goto out_destroy_eh;
	}


	res1 = fpgaGetOSObjectFromEventHandle(eh, &pfd.fd);
	ON_ERR_GOTO(res1, out_join, "getting file descriptor");

	pfd.events = POLLIN;
	poll_ret = poll(&pfd, 1, timeout);

	if (poll_ret < 0) {
		printf("Poll error errno = %s\n", strerror(errno));
		res1 = FPGA_EXCEPTION;
		goto out_join;
	} else if (poll_ret == 0) {
		 printf("Poll timeout occurred\n");
		 res1 = FPGA_EXCEPTION;
		 goto out_join;
	} else {
		 printf("FME Interrupt occurred\n");
		 bytes_read = read(pfd.fd, &count, sizeof(count));
		 if (bytes_read <= 0)
			 printf("WARNING: error reading from poll fd: %s\n",
					 bytes_read < 0 ? strerror(errno) : "zero bytes read");
	}

	res1 = fpgaUnregisterEvent(fpga_device_handle, FPGA_EVENT_ERROR, eh);
	ON_ERR_GOTO(res1, out_join, "unregistering an FME event");

	printf("Successfully tested Register/Unregister for FME events!\n");

out_join:
	pthread_join(errthr, NULL);

out_destroy_eh:
	res2 = fpgaDestroyEventHandle(&eh);
	ON_ERR_GOTO(res2, out_close, "deleting event handle");

out_close:
	res2 = fpgaClose(fpga_device_handle);
	ON_ERR_GOTO(res2, out_destroy_tok, "closing accelerator");

out_destroy_tok:
	res2 = fpgaDestroyToken(&fpga_device_token);
	ON_ERR_GOTO(res2, out_exit, "destroying token");

out_exit:
	return res1 != FPGA_OK ? res1 : res2;
}
