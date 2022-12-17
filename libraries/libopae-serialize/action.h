// Copyright(c) 2022, Intel Corporation
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
#ifndef __OPAE_SERIALIZE_ACTION_H__
#define __OPAE_SERIALIZE_ACTION_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif // _GNU_SOURCE
#include <pthread.h>

#include "uds-ifc.h"
#include "pollsrv.h"

#include "request.h"
#include "response.h"
#include "hash_map.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define OPAE_MAX_TOKEN_HASH (HOST_NAME_MAX + 1 + 32)

typedef struct _opae_remote_context {
	int json_to_string_flags;

	in_addr_t client_address;
	in_port_t client_port;

	opae_hash_map remote_id_to_token_map;
	opae_hash_map remote_id_to_handle_map;
	opae_hash_map remote_id_to_mmio_map;
	opae_hash_map remote_id_to_buf_info_map;
	opae_hash_map remote_id_to_sysobject_map;
	opae_hash_map remote_id_to_event_handle_map;

	opae_poll_server *psrv;

	bool client_initialized;
	opae_remote_client_ifc events_client_ifc;
	volatile uint64_t events_client_registrations;
	pthread_mutex_t events_client_lock;
} opae_remote_context;

typedef struct _opae_server_event_context {
	opae_remote_context *remote_context;
	fpga_remote_id event_id;
} opae_server_event_context;

fpga_result opae_init_remote_context(opae_remote_context *c,
				     opae_poll_server *psrv);
fpga_result opae_release_remote_context(opae_remote_context *c);

bool opae_remote_handle_client_request(opae_remote_context *c,
				       const char *req_json,
				       char **resp_json);

int opae_poll_server_handle_client(opae_poll_server *psrv,
				   void *client_ctx,
				   int client_socket);

int opae_poll_server_init_client(opae_poll_server *psrv,
				 opae_poll_server_handler *handler,
				 nfds_t i,
				 void *extra,
				 socklen_t len);
int opae_poll_server_release_client(opae_poll_server *psrv,
				    opae_poll_server_handler *handler,
				    nfds_t i);

int opae_poll_server_handle_event(opae_poll_server *psrv,
				  void *event_ctx,
				  int event_socket);

int opae_poll_server_init_event(opae_poll_server *psrv,
				opae_poll_server_handler *handler,
				nfds_t i,
				void *extra,
				socklen_t len);
int opae_poll_server_release_event(opae_poll_server *psrv,
				   opae_poll_server_handler *handler,
				   nfds_t i);

/******************************************************************************/

typedef struct _opae_remote_events_context {
	int json_to_string_flags;
	opae_hash_map remote_id_to_eventreg_map;

	opae_server_type type;

} opae_remote_events_context;

fpga_result opae_init_remote_events_context(
	opae_remote_events_context *ec,
	opae_server_type type);

fpga_result opae_release_remote_events_context(
	opae_remote_events_context *c);

bool opae_remote_handle_events_client_request(opae_remote_events_context *c,
					      const char *req_json,
					      char **resp_json);

int opae_poll_server_handle_events_client(opae_poll_server *psrv,
					  void *client_ctx,
					  int client_socket);

int opae_poll_server_init_remote_events_client(
	struct _opae_poll_server *,
	opae_poll_server_handler *,
	nfds_t,
	void *,
	socklen_t);

int opae_poll_server_release_remote_events_client(
	struct _opae_poll_server *,
	opae_poll_server_handler *,
	nfds_t);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_SERIALIZE_ACTION_H__
