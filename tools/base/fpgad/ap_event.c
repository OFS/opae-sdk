// Copyright(c) 2017-2018, Intel Corporation
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


#define SYSFS_PORT0 "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0"
#define SYSFS_PORT1 "/sys/class/fpga/intel-fpga-dev.1/intel-fpga-port.1"

enum fpga_power_state {
	NORMAL_PWR = 0,
	AP1_STATE,
	AP2_STATE,
	AP6_STATE
};

#define PORT_AP(fil, field, lo, hi) { fil, field, lo, hi, NULL }

struct fpga_err mcp_ap_event_table_rev_0[] = {
	PORT_AP("ap1_event",   "AP1 Triggered!",      0, 0),
	PORT_AP("ap2_event",   "AP2 Triggered!",      0, 0),
	PORT_AP("power_state", "Power state changed", 0, 1),
	TABLE_TERMINATOR
};

supported_devices ap_supported_port_devices[] = {
	{ 0x8086, 0xbcc0, 0, mcp_ap_event_table_rev_0 },
	{ 0x8086, 0xbcc1, 0, mcp_ap_event_table_rev_0 },
};

monitored_device *ap_monitored_device_list;




monitored_device * ap_add_monitored_device(fpga_token token,
					   uint8_t socket_id,
					   supported_device *device)
{
	monitored_device *md = malloc(sizeof(monitored_device));

	if (md) {
		md->token = token;
		md->socket_id = socket_id;
		md->device = device;
		md->num_error_occurrences = 0;

		/* Add it to the list */
		md->next = ap_monitored_device_list;
		ap_monitored_device_list = md;
	}

	return md;
}



int log_ap_event(monitored_device *d, struct fpga_err *e)
{
	if (error_already_occurred(d, e))
		return 0;

	error_just_occurred(d, e);

	dlog("socket %i: %s\n", d->socket_id, e->reg_field);

	if (e->callback)
		e->callback(d->socket_id, e);

	return 1;
}

int ap_poll_error(monitored_device *d, struct fpga_err *e)
{
	int i;
	int count = 0;

	uint64_t err = 0;
	uint64_t mask;

	fpga_result res;
	fpga_object err_obj = NULL;

	res = fpgaTokenGetObject(d->token, e->sysfsfile, &err_obj, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to get error object\n");
		return -1;
	}

	res = fpgaObjectRead64(err_obj, &err, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to read error object\n");
		fpgaDestroyObject(&err_obj);
		return -1;
	}

	fpgaDestroyObject(&err_obj);

	mask = 0;
	for (i = e->lowbit ; i <= e->highbit ; ++i)
		mask |= 1ULL << i;

	if (err & mask) {
		count += log_ap_event(err & mask, d, e);
	} else {
		clear_occurrences_of(d, e);
	}

	return count;
}

/*
 * Poll AP Event registers
 *
 * @returns number of (new) events that were found
 */
int ap_poll_errors(monitored_device *d)
{
	unsigned i;
	int errors = 0;
	int res;

	for (i = 0 ; d->device->error_table[i].sysfsfile ; ++i) {
		res = ap_poll_error(d, &d->device->error_table[i]);
		if (-1 == res)
			return res;
		errors += res;
	}

	return errors;
}

void *apevent_thread(void *thread_context)
{
	struct config *c = (struct config *)thread_context;

	fpga_result res;
	uint32_t i;
	uint32_t num_matches = 0;
	fpga_token *tokens = NULL;
	monitored_device *d;

	ap_monitored_device_list = NULL;

	dlog("apevent: starting\n");

	/* Enumerate all devices */
	res = fpgaEnumerate(NULL, 0, NULL, 0, &num_matches);
	if (res != FPGA_OK) {
		dlog("apevent: enumeration failed\n");
		goto out_exit;
	}

	if (!num_matches) {
		dlog("apevent: no devices present. Nothing to do.\n");
		goto out_exit;
	}

	tokens = calloc(num_matches, sizeof(fpga_token));
	if (!tokens) {
		dlog("apevent: calloc failed.\n");
		goto out_exit;
	}

	res = fpgaEnumerate(NULL, 0, tokens, num_matches, &num_matches);
	if (res != FPGA_OK) {
		dlog("apevent: enumeration failed\n");
		free(tokens);
		goto out_exit;
	}

	/*
	** Determine if we support this device. If so,
	** then add it to monitored_device_list.
	*/
	for (i = 0 ; i < num_matches ; ++i) {
		ap_consider_device(tokens[i]);
	}

	if (!ap_monitored_device_list) {
		dlog("apevent: no matching devices\n");
		goto out_destroy_tokens;
	}

	while (c->running) {

		for (d = ap_monitored_device_list ; d ; d = d->next) {
			if (ap_poll_errors(d) < 0) {
				dlog("apevent: error polling errors. Aborting!\n");
				goto out_destroy_list;
			}
		}

		usleep(c->poll_interval_usec);
	}

out_destroy_list:
	for (d = ap_monitored_device_list ; d ; ) {
		monitored_device *trash = d;
		d = d->next;
		free(trash);
	}
	ap_monitored_device_list = NULL;

out_destroy_tokens:
	for (i = 0 ; i < num_matches ; ++i) {
		fpgaDestroyToken(&tokens[i]);
	}
	free(tokens);

out_exit:
	dlog("apevent: thread exiting\n");
	return NULL;
}

#if 0
static int poll_ap_event(struct fpga_ap_event *event)
{
	char sysfspath[SYSFS_PATH_MAX];
	uint64_t ap1_event = 0;
	uint64_t ap2_event = 0;
	uint64_t pwr_state = 0;

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
		dlog("AP1 Triggered for socket %d\n", event->socket);
	}

	// Read AP2 Event
	snprintf_s_s(sysfspath, sizeof(sysfspath),
		"%s/ap2_event", event->sysfsfile);

	if (read_event(sysfspath, &ap2_event) != 0) {
		return -1;
	}

	if (event->ap2_last_event != 1 && ap2_event == 1) {
		dlog("AP2 Triggered for socket %d\n", event->socket);
	}

	// Read FPGA power state
	snprintf_s_s(sysfspath, sizeof(sysfspath),
		"%s/power_state", event->sysfsfile);

	if (read_event(sysfspath, &pwr_state) != 0) {
		return -1;
	}

	if (event->pwr_last_state != 1 && pwr_state == AP1_STATE) {
		dlog(" FPGA Power State changed to AP1 for socket %d\n",
				event->socket);
	} else if (event->pwr_last_state != 2 && pwr_state == AP2_STATE) {
		dlog(" FPGA Power State changed to AP2 for socket %d\n",
				event->socket);
	} else if (event->pwr_last_state != 3 && pwr_state == AP6_STATE) {
		dlog(" FPGA Power State changed to AP6 for socket %d\n",
				event->socket);
	} else if (event->pwr_last_state != 0 && pwr_state == NORMAL_PWR) {
		dlog(" FPGA Power State changed to Normal for socket %d\n",
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

	memset_s(&apevt_socket1, sizeof(apevt_socket1), 0);
	memset_s(&apevt_socket2, sizeof(apevt_socket2), 0);

	// Max sockets count  2
	apevt_socket1.socket = 0;
	apevt_socket2.socket = 1;
	apevt_socket1.sysfsfile = SYSFS_PORT0;
	apevt_socket2.sysfsfile = SYSFS_PORT1;

	while (c->running) {
		/* read AP event and power state */

		// AP event polling
		if ((poll_ap_event(&apevt_socket1) < 0) ||
			(poll_ap_event(&apevt_socket2) < 0))
				return NULL;

		usleep(c->poll_interval_usec);
	}

	return NULL;
}
#endif
