// Copyright(c) 2023, Intel Corporation
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

#include <json-c/json.h>
#include <opae/types.h>
#include <opae/log.h>

#include "opae_int.h"
#include "mock/opae_std.h"
#include "remote.h"
//#include "request.h"
//#include "response.h"
//#include "action.h"

//#include "hash_map.h"
//#include "pollsrv.h"

enum request_type { REGISTER_EVENT = 0, UNREGISTER_EVENT = 1 };

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};

#if 0
struct _remote_event_handle *
opae_create_remote_event_handle(void)
{
	struct _remote_event_handle *p =
		(struct _remote_event_handle *)
			opae_calloc(1, sizeof(*p));
	if (p) {
		p->client_event_fd = -1;
	}
	return p;
}

void opae_destroy_remote_event_handle(struct _remote_event_handle *eh)
{
	if (eh->client_event_fd >= 0)
		close(eh->client_event_fd);
	opae_free(eh);
}

#endif

fpga_result __REMOTE_API__
remote_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
#if 1
(void) event_handle;

return FPGA_OK;

#else

	if (!event_handle) {
		OPAE_ERR("NULL event_handle pointer");
		return FPGA_INVALID_PARAM;
	}

	// We don't have a token, nor a remoting interface
	// at this point. Just create and return an empty
	// _remote_event_handle struct.

	*event_handle = opae_create_remote_event_handle();
	if (!*event_handle) {
		OPAE_ERR("calloc() failed");
		return FPGA_NO_MEMORY;
	}

	return FPGA_OK;
#endif
}

fpga_result __REMOTE_API__
remote_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
#if 1
(void) event_handle;

return FPGA_OK;

#else
	struct _remote_event_handle *eh;
	fpga_result res = FPGA_OK;

	if (!event_handle || !*event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	eh = *event_handle;

	if (eh->handle) {
		opae_fpgaDestroyEventHandle_request req;
		opae_fpgaDestroyEventHandle_response resp;
		struct _remote_token *tok;
		struct _remote_handle *h;
		char *req_json;
		char *resp_json = NULL;

		h = eh->handle;
		tok = h->token;

		req.eh_id = eh->eh_id;

		req_json = opae_encode_fpgaDestroyEventHandle_request_51(
			&req, tok->json_to_string_flags);

		res = opae_client_send_and_receive(tok, req_json, &resp_json);
		if (res)
			goto out_destroy;

		if (!opae_decode_fpgaDestroyEventHandle_response_51(
			resp_json, &resp)) {
			res = FPGA_EXCEPTION;
			goto out_destroy;
		}

		res = resp.result;
	}

out_destroy:
	opae_destroy_remote_event_handle(eh);
	*event_handle = NULL;

	return res;
#endif
}

fpga_result __REMOTE_API__
remote_fpgaGetOSObjectFromEventHandle(
	const fpga_event_handle event_handle,
	int *fd)
{
#if 1
(void) event_handle;
(void) fd;
return FPGA_OK;
#else
	
//	opae_fpgaGetOSObjectFromEventHandle_request req;
//	opae_fpgaGetOSObjectFromEventHandle_response resp;
//	struct _remote_token *tok;
//	struct _remote_handle *h;
	struct _remote_event_handle *eh;
//	char *req_json;
//	char *resp_json = NULL;
//	fpga_result res;

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	if (!fd) {
		OPAE_ERR("NULL fd pointer");
		return FPGA_INVALID_PARAM;
	}

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle || (eh->client_event_fd < 0)) {
		OPAE_ERR("You must call fpgaRegisterEvent() prior "
			 "to requesting the OS Object.");
		return FPGA_INVALID_PARAM;
	}

	*fd = eh->client_event_fd;

	return FPGA_OK;
#endif

#if 0
	h = eh->handle;
	tok = h->token;

	req.eh_id = eh->eh_id;

	req_json = opae_encode_fpgaGetOSObjectFromEventHandle_request_50(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaGetOSObjectFromEventHandle_response_50(
		resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*fd = resp.fd;

	return resp.result;
#endif
}

/******************************************************************************/

#if 0
STATIC void *events_srv_thr_fn(void *arg)
{
	opae_poll_server *psrv =
		(opae_poll_server *)arg;

	psrv->handle_client_message = opae_poll_server_handle_events_client;
	psrv->init_client =
		opae_poll_server_init_remote_events_client;
	psrv->release_client =
		opae_poll_server_release_remote_events_client;

	psrv->handle_event = NULL;
	psrv->init_event = NULL;
	psrv->release_event = NULL;

	psrv->running = true;
	opae_poll_server_loop(psrv);

	psrv->release_server(psrv);

	return NULL;
}
#endif

#if 0
STATIC fpga_result
opae_add_client_event_registration(opae_comms_channel *comms)
{
	opae_poll_server *psrv = NULL;
	fpga_result res = FPGA_OK;
	int ires;
	uint32_t timeout = 10000;

	opae_mutex_lock(ires, &comms->events_srv_lock);

	++comms->events_srv_registrations;

	if (!comms->events_srv) {

		if (comms->client.type == OPAE_CLIENT_UDS) {
			opae_uds_server *events_srv;
			opae_events_uds_data *uds_data =
				(opae_events_uds_data *)comms->events_data;

			events_srv = opae_calloc(1, sizeof(opae_uds_server));
			if (!events_srv) {
				OPAE_ERR("calloc() failed");
				res = FPGA_NO_MEMORY;
				goto out_err;
			}

			ires = opae_uds_server_init(events_srv,
						    uds_data->events_socket);
			if (ires) {
				OPAE_ERR("uds server init failed");
				res = FPGA_EXCEPTION;
				goto out_err;
			}

			events_srv->psrv.running = false;
			events_srv->psrv.release_server =
				opae_uds_server_release_free;

			comms->events_srv = events_srv;
			psrv = &events_srv->psrv;

		} else if (comms->client.type == OPAE_CLIENT_INET) {
			opae_inet_server *events_srv;
			opae_events_inet_data *inet_data =
				(opae_events_inet_data *)comms->events_data;

			events_srv = opae_calloc(1, sizeof(opae_inet_server));
			if (!events_srv) {
				OPAE_ERR("calloc() failed");
				res = FPGA_NO_MEMORY;
				goto out_err;
			}

			ires = opae_inet_server_init(events_srv,
						     inet_data->events_ip,
						     (in_port_t)inet_data->events_port);
			if (ires) {
				OPAE_ERR("inet server init failed");
				res = FPGA_EXCEPTION;
				goto out_err;

			}

			events_srv->psrv.running = false;
			events_srv->psrv.release_server =
				opae_inet_server_release_free;

			comms->events_srv = events_srv;
			psrv = &events_srv->psrv;
		}

		ires = pthread_create(&comms->events_srv_thr,
				      NULL,
				      events_srv_thr_fn,
				      comms->events_srv);
		if (ires) {
			OPAE_ERR("failed to create events server thread");
			res = FPGA_EXCEPTION;
			goto out_err;
		}

		while (!psrv->running) {
			usleep(100);
			if (--timeout == 0) {
				OPAE_ERR("events server thread "
					 "failed to start");
				res = FPGA_EXCEPTION;
				goto out_err;
			}
		}

	}

	opae_mutex_unlock(ires, &comms->events_srv_lock);
	return res;

out_err:
	--comms->events_srv_registrations;
	opae_mutex_unlock(ires, &comms->events_srv_lock);
	return res;
}
#endif

#if 0
STATIC fpga_result
opae_remove_client_event_registration(opae_comms_channel *comms)
{
	opae_poll_server *psrv = NULL;
	int ires;

	opae_mutex_lock(ires, &comms->events_srv_lock);

	if (comms->events_srv_registrations > 0) {
		--comms->events_srv_registrations;

		if (comms->events_srv_registrations == 0) {
			psrv = (opae_poll_server *)comms->events_srv;

			psrv->running = false;
			pthread_join(comms->events_srv_thr, NULL);

			comms->events_srv = NULL;
		}

	}

	opae_mutex_unlock(ires, &comms->events_srv_lock);
	return FPGA_OK;
}
#endif

/******************************************************************************/

fpga_result __REMOTE_API__ remote_fpgaRegisterEvent(fpga_handle handle,
						 fpga_event_type event_type,
						 fpga_event_handle event_handle,
						 uint32_t flags)
{
#if 1
(void) handle;
(void) event_type;
(void) event_handle;
(void) flags;

return FPGA_OK;
#else
	opae_fpgaRegisterEvent_request req;
	opae_fpgaRegisterEvent_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	struct _remote_event_handle *eh;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle) {
		// We were created by fpgaCreateEventHandle(), but
		// that function has no handle/token nor a remoting
		// interface. Do what fpgaCreateEventHandle() would
		// have done, if it had a remoting interface.
		opae_fpgaCreateEventHandle_request create_req;
		opae_fpgaCreateEventHandle_response create_resp;

		eh->handle = h;

		req_json = opae_encode_fpgaCreateEventHandle_request_47(
				&create_req, tok->json_to_string_flags);

		res = opae_client_send_and_receive(tok, req_json, &resp_json);
		if (res)
			return res;

		if (!opae_decode_fpgaCreateEventHandle_response_47(
			resp_json, &create_resp))
			return FPGA_EXCEPTION;

		if (create_resp.result != FPGA_OK)
			return create_resp.result;

		eh->eh_id = create_resp.eh_id;
	}

	// Check to see whether our server thread is running,
	// and start it if it isn't.
	res = opae_add_client_event_registration(tok->comms);
	if (res)
		return res;

	req.handle_id = h->hdr.handle_id;
	req.event_type = event_type;
	req.eh_id = eh->eh_id;
	req.flags = flags;
	req.client_type = tok->comms->client.type;
	req.events_data = tok->comms->events_data;

	req_json = opae_encode_fpgaRegisterEvent_request_48(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaRegisterEvent_response_48(resp_json, &resp))
		return FPGA_EXCEPTION;

	eh->client_event_fd = resp.client_event_fd;

	return resp.result;
#endif
}

fpga_result __REMOTE_API__
remote_fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
			   fpga_event_handle event_handle)
{
#if 1
(void) handle;
(void) event_type;	
(void) event_handle;

return FPGA_OK;
#else
	opae_fpgaUnregisterEvent_request req;
	opae_fpgaUnregisterEvent_response resp;
	struct _remote_token *tok;
	struct _remote_handle *h;
	struct _remote_event_handle *eh;
	char *req_json;
	char *resp_json = NULL;
	fpga_result res;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (!event_handle) {
		OPAE_ERR("NULL event_handle");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	eh = (struct _remote_event_handle *)event_handle;

	if (!eh->handle) {
		OPAE_ERR("no event registered");
		return FPGA_INVALID_PARAM;
	}

	if (eh->handle != h) {
		OPAE_ERR("handle / event_handle mismatch");
		return FPGA_INVALID_PARAM;
	}

	req.handle_id = h->hdr.handle_id;
	req.event_type = event_type;
	req.eh_id = eh->eh_id;

	req_json = opae_encode_fpgaUnregisterEvent_request_49(
		&req, tok->json_to_string_flags);

	res = opae_client_send_and_receive(tok, req_json, &resp_json);
	if (res)
		return res;

	if (!opae_decode_fpgaUnregisterEvent_response_49(resp_json, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		// Decrement our event registration count, and see if we need
		// to stop the server thread, releasing the server.
		opae_remove_client_event_registration(tok->comms);
	}

	return resp.result;
#endif
}
