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
#endif  // HAVE_CONFIG_H

#include <json-c/json.h>
#include <opae/log.h>
#include <opae/types.h>

#include "grpc_client.hpp"
#include "grpc_server_runner.hpp"
#include "mock/opae_std.h"
#include "opae_int.h"
#include "remote.h"

enum request_type { REGISTER_EVENT = 0, UNREGISTER_EVENT = 1 };

struct event_request {
  enum request_type type;
  fpga_event_type event;
  uint64_t object_id;
};

struct _remote_event_handle *opae_create_remote_event_handle(void) {
  struct _remote_event_handle *p =
      (struct _remote_event_handle *)opae_calloc(1, sizeof(*p));
  if (p) {
    p->client_event_fd = -1;
  }
  return p;
}

void opae_destroy_remote_event_handle(struct _remote_event_handle *eh) {
  if (eh->client_event_fd >= 0) close(eh->client_event_fd);
  opae_free(eh);
}

fpga_result __REMOTE_API__
remote_fpgaCreateEventHandle(fpga_event_handle *event_handle) {
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
}

fpga_result __REMOTE_API__
remote_fpgaDestroyEventHandle(fpga_event_handle *event_handle) {
  _remote_event_handle *eh;
  fpga_result res = FPGA_EXCEPTION;

  if (!event_handle || !*event_handle) {
    OPAE_ERR("NULL event_handle");
    return FPGA_INVALID_PARAM;
  }

  eh = reinterpret_cast<_remote_event_handle *>(*event_handle);

  if (eh->handle) {
    _remote_handle *h = eh->handle;
    _remote_token *tok = h->token;
    OPAEClient *client = reinterpret_cast<OPAEClient *>(tok->comms->client);

    res = client->fpgaDestroyEventHandle(eh->eh_id);
  }

  opae_destroy_remote_event_handle(eh);
  *event_handle = NULL;

  return res;
}

fpga_result __REMOTE_API__ remote_fpgaGetOSObjectFromEventHandle(
    const fpga_event_handle event_handle, int *fd) {
  struct _remote_event_handle *eh;

  if (!event_handle) {
    OPAE_ERR("NULL event_handle");
    return FPGA_INVALID_PARAM;
  }

  if (!fd) {
    OPAE_ERR("NULL fd pointer");
    return FPGA_INVALID_PARAM;
  }

  eh = reinterpret_cast<_remote_event_handle *>(event_handle);

  if (!eh->handle || (eh->client_event_fd < 0)) {
    OPAE_ERR(
        "You must call fpgaRegisterEvent() prior "
        "to requesting the OS Object.");
    return FPGA_INVALID_PARAM;
  }

  *fd = eh->client_event_fd;

  return FPGA_OK;
}

/******************************************************************************/

STATIC fpga_result
opae_add_client_event_registration(opae_comms_channel *comms) {
  fpga_result res = FPGA_OK;

  pthread_lock_guard g(&comms->events_srv_lock);

  ++comms->events_srv_registrations;
  if (comms->events_srv_registrations > 1) return res;

  gRPCServerRunner<OPAEEventsServiceImpl> *runner =
      new gRPCServerRunner<OPAEEventsServiceImpl>(
          comms->events_data.events_ip, comms->events_data.events_port,
          comms->debug);

  comms->events_srv_runner = runner;

  runner->start();

  uint32_t timeout = 10000;
  while (!runner->is_running()) {
    usleep(100);
    if (--timeout == 0) {
      OPAE_ERR("events server thread failed to start");
      comms->events_srv_registrations = 0;
      delete runner;
      comms->events_srv_runner = nullptr;
      res = FPGA_EXCEPTION;
      break;
    }
  }

  return res;
}

STATIC fpga_result
opae_remove_client_event_registration(opae_comms_channel *comms) {
  pthread_lock_guard g(&comms->events_srv_lock);

  if (comms->events_srv_registrations > 0) {
    --comms->events_srv_registrations;
    if (comms->events_srv_registrations == 0) {
      gRPCServerRunner<OPAEEventsServiceImpl> *runner =
          reinterpret_cast<gRPCServerRunner<OPAEEventsServiceImpl> *>(
              comms->events_srv_runner);

      runner->stop();
      runner->join();
      delete runner;
      comms->events_srv_runner = nullptr;
    }
  }

  return FPGA_OK;
}

/******************************************************************************/

fpga_result __REMOTE_API__
remote_fpgaRegisterEvent(fpga_handle handle, fpga_event_type event_type,
                         fpga_event_handle event_handle, uint32_t flags) {
  _remote_handle *h;
  _remote_token *tok;
  OPAEClient *client;
  _remote_event_handle *eh;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!event_handle) {
    OPAE_ERR("NULL event_handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);
  eh = reinterpret_cast<_remote_event_handle *>(event_handle);

  if (!eh->handle) {
    // We were created by fpgaCreateEventHandle(), but
    // that function has no handle/token nor a remoting
    // interface. Do what fpgaCreateEventHandle() would
    // have done, if it had a remoting interface.

    eh->handle = h;
    res = client->fpgaCreateEventHandle(eh->eh_id);
    if (res) return res;
  }

  // Check to see whether our Events Server is running,
  // starting it if it isn't.
  res = opae_add_client_event_registration(tok->comms);
  if (res) return res;

  return client->fpgaRegisterEvent(h->hdr.handle_id, event_type, eh->eh_id,
                                   flags, tok->comms->events_data.events_port,
                                   eh->client_event_fd);
}

fpga_result __REMOTE_API__
remote_fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
                           fpga_event_handle event_handle) {
  _remote_handle *h;
  _remote_token *tok;
  _remote_event_handle *eh;
  OPAEClient *client;
  fpga_result res;

  if (!handle) {
    OPAE_ERR("NULL handle");
    return FPGA_INVALID_PARAM;
  }

  if (!event_handle) {
    OPAE_ERR("NULL event_handle");
    return FPGA_INVALID_PARAM;
  }

  h = reinterpret_cast<_remote_handle *>(handle);
  tok = h->token;
  client = reinterpret_cast<OPAEClient *>(tok->comms->client);
  eh = reinterpret_cast<_remote_event_handle *>(event_handle);

  if (!eh->handle) {
    OPAE_ERR("no event registered");
    return FPGA_INVALID_PARAM;
  }

  if (eh->handle != h) {
    OPAE_ERR("handle / event_handle mismatch");
    return FPGA_INVALID_PARAM;
  }

  res = client->fpgaUnregisterEvent(h->hdr.handle_id, event_type, eh->eh_id);
  if (res == FPGA_OK) {
    // Decrement our event registration count, and see if we need
    // to stop the server thread, releasing the server.
    opae_remove_client_event_registration(tok->comms);
  }

  return res;
}
