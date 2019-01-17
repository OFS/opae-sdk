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

#include "errtable.h"
#include "srv.h"
#include "log.h"
#include "ap6.h"

#include "safe_string/safe_string.h"

static void evt_notify_error_callback(struct client_event_registry *r,
				      uint64_t object_id,
				      const struct fpga_err *e)
{
	if ((FPGA_EVENT_ERROR == r->event) &&
	    (object_id == r->object_id)) {
		dlog("object: 0x%" PRIx64 " event: FPGA_EVENT_ERROR (%s)\n",
			object_id,
			e->reg_field);
		if (write(r->fd, &r->data, sizeof(r->data)) < 0)
			dlog("write: %s\n", strerror(errno));
		r->data++;
	}
}

void evt_notify_error(uint8_t socket_id,
		      uint64_t object_id,
		      const struct fpga_err *e)
{
	UNUSED_PARAM(socket_id);
	for_each_registered_event(evt_notify_error_callback, object_id, e);
}

static void evt_notify_ap6_callback(struct client_event_registry *r,
				    uint64_t object_id,
				    const struct fpga_err *e)
{
	if ((FPGA_EVENT_POWER_THERMAL == r->event) &&
	    (object_id == r->object_id)) {
		dlog("object: 0x%" PRIx64
		     " event: FPGA_EVENT_POWER_THERMAL (%s)\n",
		     object_id,
		     e->reg_field);
		if (write(r->fd, &r->data, sizeof(r->data)) < 0)
			dlog("write: %s\n", strerror(errno));
		r->data++;
	}
}

void evt_notify_ap6(uint8_t socket_id,
		    uint64_t object_id,
		    const struct fpga_err *e)
{
	UNUSED_PARAM(socket_id);
	for_each_registered_event(evt_notify_ap6_callback, object_id, e);
}

/* trigger NULL bitstream programming and notify AP6 event clients */
void evt_notify_ap6_and_null(uint8_t socket_id,
			     uint64_t object_id,
			     const struct fpga_err *e)
{
	if (socket_id >= MAX_SOCKETS) {
		dlog("internal error: exceeded max supported sockets\n");
		return;
	}

	dlog("triggering NULL bitstream programming on socket %d\n",
		(int)socket_id);
	sem_post(&ap6_sem[socket_id]);
	evt_notify_ap6(socket_id, object_id, e);
}

