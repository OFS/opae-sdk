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

#include <semaphore.h>
#include <time.h>
#include <inttypes.h>
//#include "event_dispatcher_thread.h"
#include "fpgad/event_dispatcher_thread.h"
#include "fpgad/api/logging.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("event_dispatcher_thread: " format, ##__VA_ARGS__)

struct fpgad_config global_config = {
    .poll_interval_usec = 100 * 1000,
    .daemon = 0,
    .running = 1,
    .api_socket = "/tmp/fpga_event_socket",
};

event_dispatcher_thread_config event_dispatcher_config = {
	.global = &global_config,
	.sched_policy = SCHED_RR,
	.sched_priority = 30,
};

#define EVENT_DISPATCH_QUEUE_DEPTH 512

typedef struct _evt_dispatch_queue {
	event_dispatch_queue_item q[EVENT_DISPATCH_QUEUE_DEPTH];
	unsigned head;
	unsigned tail;
	pthread_mutex_t lock;
} evt_dispatch_queue;

STATIC sem_t evt_dispatch_sem;

STATIC evt_dispatch_queue normal_queue = {
	{ { NULL, NULL, NULL }, },
	0,
	0,
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
};

STATIC evt_dispatch_queue high_priority_queue = {
	{ { NULL, NULL, NULL }, },
	0,
	0,
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
};

STATIC void evt_queue_init(evt_dispatch_queue *q)
{
//	memset_s(q->q, sizeof(q->q), 0);
	memset(q->q, 0, sizeof(q->q));
	q->head = q->tail = 0;
}

STATIC void evt_queue_destroy(evt_dispatch_queue *q)
{
	q->head = q->tail = 0;
}

STATIC volatile bool dispatcher_is_ready = false;

bool evt_dispatcher_is_ready(void)
{
	return dispatcher_is_ready;
}

STATIC bool evt_queue_is_full(evt_dispatch_queue *q)
{
	const size_t num = sizeof(q->q) / sizeof(q->q[0]);

	if (q->tail > q->head) {
		if ((q->head == 0) && (q->tail == (num - 1)))
			return true;
	} else if (q->tail < q->head) {
		if (q->tail == (q->head - 1))
			return true;
	}
	return false;
}

STATIC bool evt_queue_is_empty(evt_dispatch_queue *q)
{
	return q->head == q->tail;
}

STATIC bool _evt_queue_response(evt_dispatch_queue *q,
				fpgad_respond_event_t callback,
				fpgad_monitored_device *device,
				void *context)
{
	//errno_t res;
	int res;

	//opae_mutex_lock(res, &q->lock);
	fpgad_mutex_lock(res, &q->lock);

	if (evt_queue_is_full(q)) {
		//opae_mutex_unlock(res, &q->lock);
		fpgad_mutex_unlock(res, &q->lock);
		return false;
	}

	q->q[q->tail].callback = callback;
	q->q[q->tail].device = device;
	q->q[q->tail].context = context;

	q->tail = (q->tail + 1) % EVENT_DISPATCH_QUEUE_DEPTH;

	//opae_mutex_unlock(res, &q->lock);
	fpgad_mutex_unlock(res, &q->lock);

	sem_post(&evt_dispatch_sem);

	return true;
}

STATIC bool _evt_queue_get(evt_dispatch_queue *q,
			   event_dispatch_queue_item *item)
{
	//errno_t res;
	int res;

	//opae_mutex_lock(res, &q->lock);
	fpgad_mutex_lock(res, &q->lock);

	if (evt_queue_is_empty(q)) {
		//opae_mutex_unlock(res, &q->lock);
		fpgad_mutex_unlock(res, &q->lock);
		return false;
	}

	*item = q->q[q->head];
//	memset_s(&q->q[q->head],
//		 sizeof(q->q[0]),
//		 0);
    memset(&q->q[q->head], 0, sizeof(q->q[0]));
	q->head = (q->head + 1) % EVENT_DISPATCH_QUEUE_DEPTH;

	//opae_mutex_unlock(res, &q->lock);
	fpgad_mutex_unlock(res, &q->lock);

	return true;
}

bool evt_queue_response(fpgad_respond_event_t callback,
			fpgad_monitored_device *device,
			void *context)
{
	return _evt_queue_response(&normal_queue,
				   callback,
				   device,
				   context);
}

bool evt_queue_get(event_dispatch_queue_item *item)
{
	return _evt_queue_get(&normal_queue, item);
}

bool evt_queue_response_high(fpgad_respond_event_t callback,
			     fpgad_monitored_device *device,
			     void *context)
{
	return _evt_queue_response(&high_priority_queue,
				   callback,
				   device,
				   context);
}

bool evt_queue_get_high(event_dispatch_queue_item *item)
{
	return _evt_queue_get(&high_priority_queue, item);
}

void *event_dispatcher_thread(void *thread_context)
{
	event_dispatcher_thread_config *c =
		(event_dispatcher_thread_config *)thread_context;
	struct sched_param sched_param;
	int policy = 0;
	int res;
	struct timespec ts;

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

	evt_queue_init(&normal_queue);
	evt_queue_init(&high_priority_queue);

	if (sem_init(&evt_dispatch_sem, 0, 0)) {
		LOG("failed to init queue sem.\n");
		goto out_exit;
	}

	dispatcher_is_ready = true;

	while (c->global->running) {

		clock_gettime(CLOCK_REALTIME, &ts);

		ts.tv_nsec += c->global->poll_interval_usec * 1000;
		if (ts.tv_nsec > 1000000000) {
			++ts.tv_sec;
			ts.tv_nsec -= 1000000000;
		}

		res = sem_timedwait(&evt_dispatch_sem, &ts);

		if (!res) {
			event_dispatch_queue_item item;

			// Process all high-priority items first
			while (evt_queue_get_high(&item)) {
				LOG("dispatching (high) for object_id: 0x%" PRIx64 ".\n",
					item.device->object_id);
				item.callback(item.device, item.context);
			}

			if (evt_queue_get(&item)) {
				LOG("dispatching for object_id: 0x%" PRIx64 ".\n",
					item.device->object_id);
				item.callback(item.device, item.context);
			}
		}

	}

	dispatcher_is_ready = false;

	evt_queue_destroy(&normal_queue);
	evt_queue_destroy(&high_priority_queue);

	sem_destroy(&evt_dispatch_sem);

out_exit:
	LOG("exiting\n");
	return NULL;
}
