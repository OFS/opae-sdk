// Copyright(c) 2018-2019, Intel Corporation
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

#include <inttypes.h>

#include "opae_events_api.h"

#include "logging.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("opae_events_api: " format, ##__VA_ARGS__)

STATIC pthread_mutex_t list_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
STATIC api_client_event_registry *event_registry_list;

int opae_api_register_event(int conn_socket,
			    int fd,
			    fpga_event_type e,
			    uint64_t object_id)
{
	api_client_event_registry *r =
		(api_client_event_registry *) malloc(sizeof(*r));
	//errno_t err;
	int err;

	if (!r)
		return ENOMEM;

	r->conn_socket = conn_socket;
	r->fd = fd;
	r->data = 1;
	r->event = e;
	r->object_id = object_id;

	fpgad_mutex_lock(err, &list_lock);

	r->next = event_registry_list;
	event_registry_list = r;

	fpgad_mutex_unlock(err, &list_lock);

	return 0;
}

STATIC void release_event_registry(api_client_event_registry *r)
{
	close(r->fd);
	free(r);
}

int opae_api_unregister_event(int conn_socket,
			      fpga_event_type e,
			      uint64_t object_id)
{
	api_client_event_registry *trash;
	api_client_event_registry *save;
	//errno_t err;
	int err;
	int res = 0;

	fpgad_mutex_lock(err, &list_lock);

	trash = event_registry_list;

	if (!trash) { // empty list
		res = 1;
		goto out_unlock;
	}

	if ((conn_socket == trash->conn_socket) &&
		(e == trash->event) &&
		(object_id == trash->object_id)) {

		// found at head of list

		event_registry_list = event_registry_list->next;
		release_event_registry(trash);
		goto out_unlock;
	}

	save = trash;
	trash = trash->next;
	while (trash) {

		if ((conn_socket == trash->conn_socket) &&
			(e == trash->event) &&
			(object_id == trash->object_id))
			break;

		save = trash;
		trash = trash->next;
	}

	if (!trash) { // not found
		res = 1;
		goto out_unlock;
	}

	// found at trash
	save->next = trash->next;
	release_event_registry(trash);

out_unlock:
	fpgad_mutex_unlock(err, &list_lock);
	return res;
}

STATIC api_client_event_registry *
find_event_for(int conn_socket)
{
	api_client_event_registry *r;

	for (r = event_registry_list ; r ; r = r->next)
		if (conn_socket == r->conn_socket)
			break;

	return r;
}

void opae_api_unregister_all_events_for(int conn_socket)
{
	api_client_event_registry *r;
	//errno_t err;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	r = find_event_for(conn_socket);
	while (r) {
		opae_api_unregister_event(conn_socket, r->event, r->object_id);
		r = find_event_for(conn_socket);
	}

	fpgad_mutex_unlock(err, &list_lock);
}

void opae_api_unregister_all_events(void)
{
	api_client_event_registry *r;
	//errno_t err;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	for (r = event_registry_list ; r != NULL ; ) {
		api_client_event_registry *trash;
		trash = r;
		r = r->next;
		release_event_registry(trash);
	}

	event_registry_list = NULL;

	fpgad_mutex_unlock(err, &list_lock);
}

void opae_api_for_each_registered_event
(void (*cb)(api_client_event_registry *r, void *context),
void *context)
{
	api_client_event_registry *r;
	//errno_t err;
	int err;

	fpgad_mutex_lock(err, &list_lock);

	for (r = event_registry_list; r != NULL; r = r->next) {
		cb(r, context);
	}

	fpgad_mutex_unlock(err, &list_lock);
}

STATIC void check_and_send_EVENT_ERROR(api_client_event_registry *r,
				       void *context)
{
	fpgad_monitored_device *d =
		(fpgad_monitored_device *)context;

	if ((r->event == FPGA_EVENT_ERROR) &&
	    (r->object_id == d->object_id)) {
		LOG("object_id: 0x%" PRIx64 " event: FPGA_EVENT_ERROR\n",
			d->object_id);
		if (write(r->fd, &r->data, sizeof(r->data)) < 0)
			LOG("write failed: %s\n", strerror(errno));
		r->data++;
	}
}

void opae_api_send_EVENT_ERROR(fpgad_monitored_device *d)
{
	opae_api_for_each_registered_event(check_and_send_EVENT_ERROR,
					   d);
}

STATIC void check_and_send_EVENT_POWER_THERMAL(api_client_event_registry *r,
					       void *context)
{
	fpgad_monitored_device *d =
		(fpgad_monitored_device *)context;

	if ((r->event == FPGA_EVENT_POWER_THERMAL) &&
	    (r->object_id == d->object_id)) {
		LOG("object_id: 0x%" PRIx64 " event: FPGA_EVENT_POWER_THERMAL\n",
			d->object_id);
		if (write(r->fd, &r->data, sizeof(r->data)) < 0)
			LOG("write failed: %s\n", strerror(errno));
		r->data++;
	}
}

void opae_api_send_EVENT_POWER_THERMAL(fpgad_monitored_device *d)
{
	opae_api_for_each_registered_event(check_and_send_EVENT_POWER_THERMAL,
					   d);
}
