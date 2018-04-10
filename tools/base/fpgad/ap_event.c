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

/*
 * ap_event.c : To monitor FPGA AP power states.
 */

#include "ap_event.h"
#include "evt.h"
#include "log.h"
#include "config_int.h"

#include "safe_string/safe_string.h"

enum fpga_power_state {
	NORAML_PWR = 0,
	AP1_STATE,
	AP2_STATE,
	AP6_STATE
};


static int read_event(char *sysfspath, uint64_t *value)
{
	struct stat st;
	int i;
	fpga_result res;

	if (sysfspath == NULL || value == NULL) {
		return -1;
	}

	i = stat(sysfspath, &st);
	if (i != 0) {// file may not exist on single-socket.
		return 0;
	}

	res = sysfs_read_u64(sysfspath, value);
	if (res != FPGA_OK) {
		return -1;
	}

	return 0;
}

static int poll_ap_event(struct fpga_ap_event *event)
{
	char sysfspath[SYSFS_PATH_MAX];
	uint64_t ap1_event;
	uint64_t ap2_event;
	uint64_t pwr_state;

	if (event == NULL) {
		return -1;
	}

	// Read AP1 Event
	snprintf_s_s(sysfspath, sizeof(sysfspath),
		"%s/ap1_event", event->sysfsfile);

	if (read_event(sysfspath, &ap1_event) != 0) {
		return  -1;
	}

	if (event->ap1_last_event != AP1_STATE && ap1_event == 1) {
		dlog("AP1 Triggerd for socket %d\n", event->socket);
	}

	// Read AP2 Event
	snprintf_s_s(sysfspath, sizeof(sysfspath),
		"%s/ap2_event", event->sysfsfile);

	if (read_event(sysfspath, &ap2_event) != 0) {
		return -1;
	}

	if (event->ap2_last_event != 1 && ap2_event == 1) {
		dlog("AP2 Triggerd for socket %d\n", event->socket);
	}

	// Read FPGA power state
	snprintf_s_s(sysfspath, sizeof(sysfspath),
		"%s/power_state", event->sysfsfile);

	if (read_event(sysfspath, &pwr_state) != 0) {
		return -1;
	}

	if (event->pwr_last_state != 1 && pwr_state == AP1_STATE) {
		dlog(" FPGA Power State changed to AP1 for socket %d \n",
				event->socket);
	} else if (event->pwr_last_state != 2 && pwr_state == AP2_STATE) {
		dlog(" FPGA Power State changed to AP2 for socket %d \n",
				event->socket);
	} else if (event->pwr_last_state != 3 && pwr_state == AP6_STATE) {
		dlog(" FPGA Power State changed to AP6 for socket %d \n",
				event->socket);
	} else if (event->pwr_last_state != 0 && pwr_state == 0) {
		dlog(" FPGA Power State changed to Normal for socket %d \n",
				event->socket);
	}

	event->ap1_last_event = ap1_event;
	event->ap2_last_event = ap2_event;
	event->pwr_last_state = pwr_state;

	return 0;
}

void *apevent_thread(void *thread_context)
{
	struct config *c = (struct config *)thread_context;

	struct fpga_ap_event apevt_socket1;
	struct fpga_ap_event apevt_socket2;

	memset(&apevt_socket1, 0, sizeof(apevt_socket1));
	memset(&apevt_socket2, 0, sizeof(apevt_socket2));

	// Max sockets count  2
	apevt_socket1.socket = 0;
	apevt_socket2.socket = 1;
	apevt_socket1.sysfsfile = SYSFS_PORT0;
	apevt_socket2.sysfsfile = SYSFS_PORT1;

	while (c->running) {
		/* read AP event and power state */

		// AP envent polling
		if ((poll_ap_event(&apevt_socket1) < 0) ||
			(poll_ap_event(&apevt_socket2) < 0))
				return NULL;

		usleep(c->poll_interval_usec);
	}

	return NULL;
}
