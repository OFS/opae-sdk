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

#ifndef __FPGAD_API_OPAE_EVENTS_API_H__
#define __FPGAD_API_OPAE_EVENTS_API_H__

#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <opae/types.h>

#include "fpgad/fpgad.h"
#include "fpgad/monitored_device.h"

enum request_type {
	REGISTER_EVENT = 0,
	UNREGISTER_EVENT
};

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};

typedef struct _api_client_event_registry {
	int conn_socket;
	int fd;
	uint64_t data;
	fpga_event_type event;
	uint64_t object_id;
	struct _api_client_event_registry *next;
} api_client_event_registry;

// 0 on success
int opae_api_register_event(int conn_socket,
			    int fd,
			    fpga_event_type e,
			    uint64_t object_id);

// 0 on success
int opae_api_unregister_event(int conn_socket,
			      fpga_event_type e,
			      uint64_t object_id);

void opae_api_unregister_all_events_for(int conn_socket);

void opae_api_unregister_all_events(void);

void opae_api_for_each_registered_event(void (*cb)(api_client_event_registry *r,
						   void *context),
					void *context);

void opae_api_send_EVENT_ERROR(fpgad_monitored_device *d);

void opae_api_send_EVENT_POWER_THERMAL(fpgad_monitored_device *d);

#endif /* __FPGAD_API_OPAE_EVENTS_API_H__ */
