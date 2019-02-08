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

#include <dlfcn.h>
#include "monitored_device.h"
#include "monitor_thread.h"
#include "event_dispatcher_thread.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("monitor_thread: " format, ##__VA_ARGS__)

monitor_thread_config monitor_config = {
	.global = &global_config,
	.sched_policy = SCHED_RR,
	.sched_priority = 20,
};

STATIC pthread_mutex_t mon_list_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
STATIC fpgad_monitored_device *monitored_device_list;

STATIC void mon_queue_response(fpgad_detection_status status,
			       fpgad_respond_event_t response,
			       fpgad_monitored_device *d,
			       void *response_context)
{
	if (status == FPGAD_STATUS_DETECTED_HIGH) {

		if (evt_queue_response_high(response,
					    d,
					    response_context)) {
			pthread_yield();
		} else {
			LOG("high priority event queue is full. Dropping!\n");
		}

	} else if (status == FPGAD_STATUS_DETECTED) {

		if (evt_queue_response(response,
				       d,
				       response_context)) {
			pthread_yield();
		} else {
			LOG("event queue is full. Dropping!\n");
		}

	}
}

STATIC void mon_monitor(fpgad_monitored_device *d)
{
	unsigned i;

	if (!d->detections)
		return;

	for (i = 0 ; d->detections[i] ; ++i) {
		fpgad_detection_status result;
		fpgad_detect_event_t detect =
			d->detections[i];
		void *detect_context =
			d->detection_contexts ?
			d->detection_contexts[i] : NULL;

		result = detect(d, detect_context);

		if (result != FPGAD_STATUS_NOT_DETECTED && d->responses) {
			fpgad_respond_event_t response =
				d->responses[i];
			void *response_context =
				d->response_contexts ?
				d->response_contexts[i] : NULL;

			if (response) {
				mon_queue_response(result,
						   response,
						   d,
						   response_context);
			}
		}
	}
}

STATIC volatile bool mon_is_ready = false;

bool monitor_is_ready(void)
{
	return mon_is_ready;
}

void *monitor_thread(void *thread_context)
{
	monitor_thread_config *c = (monitor_thread_config *)thread_context;
	struct sched_param sched_param;
	int policy = 0;
	int res;
	errno_t err;
	fpgad_monitored_device *d;

	LOG("starting\n");

	res = pthread_getschedparam(pthread_self(), &policy, &sched_param);
	if (res) {
		LOG("error getting scheduler params: %s\n", strerror(res));
	} else {
		policy = c->sched_policy;
		sched_param.sched_priority = c->sched_priority;

		res = pthread_setschedparam(pthread_self(),
					    policy,
					    &sched_param);
		if (res) {
			LOG("error setting scheduler params"
			    " (got root?): %s\n", strerror(res));
		}
	}

	mon_is_ready = true;

	while (c->global->running) {
		fpgad_mutex_lock(err, &mon_list_lock);

		for (d = monitored_device_list ; d ; d = d->next) {
			mon_monitor(d);
		}

		fpgad_mutex_unlock(err, &mon_list_lock);

		usleep(c->global->poll_interval_usec);
	}

	while (evt_dispatcher_is_ready()) {
		// Wait for the event dispatcher to complete
		// before we destroy the monitored devices.
		usleep(c->global->poll_interval_usec);
	}

	mon_destroy();
	mon_is_ready = false;

	LOG("exiting\n");
	return NULL;
}

void mon_monitor_device(fpgad_monitored_device *d)
{
	errno_t err;
	fpgad_monitored_device *trav;

	fpgad_mutex_lock(err, &mon_list_lock);

	d->next = NULL;

	if (!monitored_device_list) {
		monitored_device_list = d;
		goto out_unlock;
	}

	for (trav = monitored_device_list ;
		trav->next ;
			trav = trav->next)
		/* find the end of the list */ ;

	trav->next = d;

out_unlock:
	fpgad_mutex_unlock(err, &mon_list_lock);
}

extern fpgad_supported_device supported_devices_table[];
void mon_destroy(void)
{
	unsigned i;
	errno_t err;
	fpgad_monitored_device *d;

	fpgad_mutex_lock(err, &mon_list_lock);

	for (d = monitored_device_list ; d ; ) {
		fpgad_monitored_device *trash = d;
		fpgad_plugin_destroy_t destroy;

		d = d->next;

		if (trash->type == FPGAD_PLUGIN_TYPE_THREAD) {

			if (trash->thread_stop_fn) {
				trash->thread_stop_fn();
			} else {
				LOG("Thread plugin \"%s\" has"
				    " no thread_stop_fn\n",
				    trash->supported->library_path);
				pthread_cancel(trash->thread);
			}

			pthread_join(trash->thread, NULL);
		}

		destroy = (fpgad_plugin_destroy_t)
			dlsym(trash->supported->dl_handle,
				FPGAD_PLUGIN_DESTROY);

		if (destroy) {
			destroy(trash);
		} else {
			LOG("warning - no destructor for \"%s\"\n",
				trash->supported->library_path);
		}

		if (trash->token)
			fpgaDestroyToken(&trash->token);

		free(trash);
	}
	monitored_device_list = NULL;

	for (i = 0 ; supported_devices_table[i].library_path ; ++i) {
		fpgad_supported_device *d = &supported_devices_table[i];

		if (d->flags & FPGAD_DEV_LOADED) {
			dlclose(d->dl_handle);
		}

		d->flags = 0;
		d->dl_handle = NULL;
	}

	fpgad_mutex_unlock(err, &mon_list_lock);
}
