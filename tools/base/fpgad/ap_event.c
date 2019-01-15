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

enum fpga_power_state {
	NORMAL_PWR = 0,
	AP1_STATE,
	AP2_STATE,
	AP6_STATE
};

#define PORT_AP(fil, field, lo, hi) { fil, field, lo, hi, NULL }

struct fpga_err mcp_ap_event_table_rev_0[] = {
	PORT_AP("ap1_event",   "AP1 Triggered!",          0, 0),
	PORT_AP("ap2_event",   "AP2 Triggered!",          0, 0),
	PORT_AP("power_state", "Power state changed to ", 0, 1),
	TABLE_TERMINATOR
};

supported_device ap_supported_port_devices[] = {
	{ 0x8086, 0xbcc0, 0, mcp_ap_event_table_rev_0 },
	{ 0x8086, 0xbcc1, 0, mcp_ap_event_table_rev_0 },
};

monitored_device *ap_monitored_device_list;

monitored_device *ap_add_monitored_device(fpga_token token,
					  uint8_t socket_id,
					  uint64_t object_id,
					  supported_device *device)
{
	monitored_device *md = malloc(sizeof(monitored_device));

	if (md) {
		md->token = token;
		md->socket_id = socket_id;
		md->object_id = object_id;
		md->device = device;
		md->num_error_occurrences = 0;

		/* Add it to the list */
		md->next = ap_monitored_device_list;
		ap_monitored_device_list = md;
	}

	return md;
}

/*
** Use the ap_supported_port_devices table
** to determine whether the Port represented by
** token should be monitored for events. If so, then add
** an entry to ap_monitored_device_list.
*/
void ap_consider_device(fpga_token token)
{
	uint32_t i;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t socket_id;
	uint64_t object_id;
	fpga_objtype objtype;
	fpga_properties props = NULL;
	fpga_result res;
	bool added;

	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get properties\n");
		return;
	}

	vendor_id = 0;
	res = fpgaPropertiesGetVendorID(props, &vendor_id);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get vendor ID\n");
		goto out_destroy_props;
	}

	device_id = 0;
	res = fpgaPropertiesGetDeviceID(props, &device_id);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get device ID\n");
		goto out_destroy_props;
	}

	objtype = FPGA_DEVICE;
	res = fpgaPropertiesGetObjectType(props, &objtype);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get object type\n");
		goto out_destroy_props;
	}

	socket_id = 0;
	res = fpgaPropertiesGetSocketID(props, &socket_id);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get socket ID\n");
		goto out_destroy_props;
	}

	object_id = 0;
	res = fpgaPropertiesGetObjectID(props, &object_id);
	if (res != FPGA_OK) {
		dlog("apevent: failed to get object ID\n");
		goto out_destroy_props;
	}

	added = false;

	if (objtype == FPGA_ACCELERATOR) {
		/* Determine if the Port matches */
		for (i = 0 ;
		      i < sizeof(ap_supported_port_devices) /
				sizeof(ap_supported_port_devices[0]) ;
		       ++i) {
			supported_device *d = &ap_supported_port_devices[i];

			if ((vendor_id == d->vendor_id) &&
			    (device_id == d->device_id)) {

				if (ap_add_monitored_device(token,
							    socket_id,
							    object_id,
							    d)) {
					added = true;
				}
			}

		}

		if (added) {
			dlog("apevent: monitoring Port device 0x%04x:0x%04x on socket %d\n",
				vendor_id, device_id, (int) socket_id);
		} else {
			dlog("apevent: skipping unsupported Port device 0x%04x:0x%04x\n",
				vendor_id, device_id);
		}
	}

out_destroy_props:
	fpgaDestroyProperties(&props);
}

int log_ap_event(uint64_t reg_val, monitored_device *d, struct fpga_err *e)
{
	const char *power_states[] = {
		"NORMAL",
		"AP1",
		"AP2",
		"AP6"
	};
	int indicator = -1;

	if (error_already_occurred(d, e))
		return 0;

	error_just_occurred(d, e);

	dlog("socket %d, object 0x%" PRIx64 ": %s",
		d->socket_id, d->object_id, e->reg_field);

	strcmp_s(e->sysfsfile, 32,
		 "power_state", &indicator);

	if (!indicator &&
	    (reg_val < sizeof(power_states) / sizeof(power_states[0])))
		dlog("%s\n", power_states[reg_val]);
	else
		dlog("\n");

	if (e->callback)
		e->callback(d->socket_id, d->object_id, e);

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
		dlog("apevent: failed to get error object\n");
		return -1;
	}

	res = fpgaObjectRead64(err_obj, &err, 0);
	if (res != FPGA_OK) {
		dlog("apevent: failed to read error object\n");
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
	** then add it to ap_monitored_device_list.
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
