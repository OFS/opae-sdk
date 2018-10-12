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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <safe_string/safe_string.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <opae/fpga.h>

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

// Global configuration of bus, set during parse_args()

struct config{
	struct target {
		int bus;
	} target;
}

config = {
	.target = {
		.bus = -1
	}
};

static fpga_result inject_ras_fatal_error(fpga_token fme_token, uint8_t err)
{
	fpga_result result                    = FPGA_OK;
	fpga_handle fme_handle                = NULL;
	struct ras_inject_error  inj_error    = { {0} };
	fpga_object inj_err_object;

	result = fpgaOpen(fme_token, &fme_handle, FPGA_OPEN_SHARED);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to open FPGA");
		return result;
	}

	result = fpgaHandleGetObject(fme_handle, FME_SYSFS_INJECT_ERROR, &inj_err_object, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Handle Object");
		goto out_close;
	}

	// Inject fatal error
	inj_error.fatal_error = err;

	result = fpgaObjectWrite64(inj_err_object, inj_error.csr, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Read Object ");
		goto out_destroy_obj;
	}

out_destroy_obj:
	result = fpgaDestroyObject(&inj_err_object);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		goto out_close;
	}

out_close:
	result = fpgaClose(fme_handle);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to close FPGA");
	}
	return result;
}

/*
 * Parse command line arguments
 */
#define GETOPT_STRING "B:"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"bus", required_argument, NULL, 'B'},
		{NULL, 0, NULL, 0}
	};
	
	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING, longopts, &option_index))){
		const char *tmp_optarg = optarg;
		/* checks to see if optarg is null and if not goes to value of optarg */
		if ((optarg) && ('=' == *tmp_optarg)){
			++tmp_optarg;
		}
		
	switch (getopt_ret) {
	case 'B': /* bus */
		if (NULL == tmp_optarg)
			return FPGA_EXCEPTION;
		endptr = NULL;
		config.target.bus = (int) strtoul(tmp_optarg, &endptr, 0);
		if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
			fprintf(stderr, "invalid bus: %s\n", tmp_optarg);
			return FPGA_EXCEPTION;
		}
		break;
	
	default: /* invalid option */
		fprintf(stderr, "Invalid cmdline options\n");
		return FPGA_EXCEPTION;
	}
	}
	
	return FPGA_OK;
}


int find_fpga(fpga_token *fpga, uint32_t *num_matches)
{
	fpga_properties filter = NULL;
	fpga_result res        = FPGA_OK;
	fpga_result dres       = FPGA_OK;

	/* Get number of FPGAs in system*/
	res = fpgaInitialize(NULL);
	ON_ERR_GOTO(res, out, "Failed to initilize ");

	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy, "setting interface ID");

	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}
		
	res= fpgaEnumerate(&filter, 1, fpga, 1, num_matches);
	ON_ERR_GOTO(res, out_destroy, "enumerating FPGAs");

out_destroy:
	dres = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(dres, out, "destroying properties object");
	
out:
	return (res == FPGA_OK) ? dres: res;
}


/* functions to get the bus number when there are multiple buses */
struct bdf_info {
	uint8_t bus;
};

fpga_result get_bus_info(fpga_token tok, struct bdf_info *finfo){
	fpga_result res = FPGA_OK;
	fpga_properties props;
	res = fpgaGetProperties(tok, &props);
	ON_ERR_GOTO(res, out, "reading properties from Token");

	res = fpgaPropertiesGetBus(props, &finfo->bus);
	ON_ERR_GOTO(res, out_destroy, "Reading bus from properties");
	
out_destroy:
	res = fpgaDestroyProperties(&props);
	ON_ERR_GOTO(res, out, "fpgaDestroyProps");

out:
	return res;
}

void print_bus_info(struct bdf_info *info){
	printf("Running on bus 0x%02X. \n", info->bus);
}






int main(int argc, char *argv[])
{
	fpga_token         fpga_device_token;
	fpga_handle        fpga_device_handle;
	uint32_t           num_matches = 1;
	fpga_result res;
	fpga_event_handle eh;
	uint64_t count = 0;
	pid_t pid;
	struct pollfd pfd;
	int timeout = 10000;
	int poll_ret = 0;
	ssize_t bytes_read = 0;
	struct bdf_info info;

	res = parse_args(argc, argv);
	ON_ERR_GOTO(res, out_exit, "parsing arguments");


	res = find_fpga(&fpga_device_token, &num_matches);
	ON_ERR_GOTO(res, out_exit, "finding FPGA accelerator");

	if (num_matches < 1) {
		fprintf(stderr, "No matches for bus number provided \n");
		res = FPGA_NOT_FOUND;
		goto out_exit;
	}

	if (num_matches > 1) {
		fprintf(stderr, "Found more than one suitable slot. ");
		res = get_bus_info(fpga_device_token, &info);
		ON_ERR_GOTO(res, out_exit, "getting bus num");	
	}
       
	res = get_bus_info(fpga_device_token, &info); 
        print_bus_info(&info);

	pid = fork();
	if (pid == -1) {
		printf("Could not create a thread to inject error");
		goto out_exit;
	}
	if (pid == 0) {
		usleep(5000000);
		res = inject_ras_fatal_error(fpga_device_token, 1);
		ON_ERR_GOTO(res, out_destroy_tok, "setting inject error register");

		res = inject_ras_fatal_error(fpga_device_token, 0);
		ON_ERR_GOTO(res, out_destroy_tok, "unsetting inject error register");

		goto out_destroy_tok;
	} else {
		res = fpgaOpen(fpga_device_token, &fpga_device_handle, FPGA_OPEN_SHARED);
		ON_ERR_GOTO(res, out_destroy_tok, "opening accelerator");

		res = fpgaCreateEventHandle(&eh);
		ON_ERR_GOTO(res, out_close, "creating event handle");

		res = fpgaRegisterEvent(fpga_device_handle, FPGA_EVENT_ERROR, eh, 0);
		ON_ERR_GOTO(res, out_destroy_eh, "registering an FME event");

		printf("Waiting for interrupts now...\n");
	

		res = fpgaGetOSObjectFromEventHandle(eh, &pfd.fd);
		ON_ERR_GOTO(res, out_destroy_eh, "getting file descriptor");

		pfd.events = POLLIN;
		poll_ret = poll(&pfd, 1, timeout);

		if (poll_ret < 0) {
			printf("Poll error errno = %s\n", strerror(errno));
			res = FPGA_EXCEPTION;
			goto out_destroy_eh;
		} else if (poll_ret == 0) {
			 printf("Poll timeout occured\n");
			 res = FPGA_EXCEPTION;
			 goto out_destroy_eh;
		} else {
			 printf("FME Interrupt occured\n");
			 bytes_read = read(pfd.fd, &count, sizeof(count));
			 if (bytes_read <= 0)
				 printf("WARNING: error reading from poll fd: %s\n",
						 bytes_read < 0 ? strerror(errno) : "zero bytes read");
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
	ON_ERR_GOTO(res, out_exit, "destroying token");

out_exit:
	return (res == FPGA_OK) ? 0: 1;
}
