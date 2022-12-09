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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <opae/log.h>
#include "mock/opae_std.h"
#include "cfg-file.h"

#include "comms.h"

STATIC int parse_remote_unix_domain_socket(json_object *j_remote_i,
					   opae_comms_channel *comms)
{
	opae_uds_server *server;
	char *socket = NULL;
	char *events_socket = NULL;
	int res;

	if (!parse_json_string(j_remote_i, "socket", &socket)) {
		OPAE_ERR("UNIX_DOMAIN_SOCKET remote missing \"socket\" key");
		return 1;
	}

	if (!parse_json_string(j_remote_i, "events_socket", &events_socket)) {
		OPAE_ERR("UNIX_DOMAIN_SOCKET remote missing "
			 "\"events_socket\" key");
		return 2;
	}

	res = opae_uds_ifc_init(&comms->client, socket, 0, 0);
	if (res)
		return res;

	server = opae_malloc(sizeof(opae_uds_server));
	if (!server) {
		OPAE_ERR("malloc() failed");
		return 3;
	}

	if (opae_uds_server_init(server, events_socket))
		return 4;

	server->psrv.release_server = opae_uds_server_release_free;

	comms->server = (opae_poll_server *)server;
	comms->valid = true;

	return 0;
}

STATIC int parse_remote_inet_socket(json_object *j_remote_i,
				    opae_comms_channel *comms)
{
	opae_inet_server *server;
	char *ip_or_host = NULL;
	int port = 0;
	int res;

	if (!parse_json_string(j_remote_i, "server", &ip_or_host)) {
		OPAE_ERR("INET_SOCK_STREAM remote missing \"server\" key");
		return 1;
	}

	if (!parse_json_int(j_remote_i, "port", &port)) {
		OPAE_ERR("INET_SOCK_STREAM remote missing \"port\" key");
		return 2;
	}

	res = opae_inet_ifc_init(&comms->client, ip_or_host, port, 0, 0);
	if (res)
		return res;

	server = opae_malloc(sizeof(opae_inet_server));
	if (!server) {
		OPAE_ERR("malloc() failed");
		return 3;
	}


//TODO	if (opae_inet_server_init(server,


	server->psrv.release_server = opae_inet_server_release_free;

	comms->server = (opae_poll_server *)server;
	comms->valid = true;

	return 0;
}

opae_comms_channel *
opae_parse_remote_json(const char *json_input)
{
	opae_comms_channel *comms = NULL;

	enum json_tokener_error j_err = json_tokener_success;
	json_object *root = NULL;
	json_object *j_remotes = NULL;

	int i;
	int j;
	int num_remotes = 0;

	root = json_tokener_parse_verbose(json_input, &j_err);
	if (!root) {
		OPAE_ERR("Config JSON parse failed: %s",
			 json_tokener_error_desc(j_err));
		goto out_free;
	}

	j_remotes = parse_json_array(root, "remotes", &num_remotes);
	if (!j_remotes)
		goto out_free;

	comms = opae_calloc(num_remotes + 1, sizeof(*comms));
	if (!comms) {
		OPAE_ERR("calloc() failed");
		goto out_free;
	}

	for (i = j = 0 ; i < num_remotes ; ++i) {
		json_object *j_remote_i =
			json_object_array_get_idx(j_remotes, i);
		char *interface = NULL;
		json_object *j_enabled = NULL;
		bool enabled = false;

		j_enabled = parse_json_boolean(j_remote_i, "enabled", &enabled);
		if (!j_enabled || !enabled) {
			// "enabled" key not found or is false.
			OPAE_DBG("remote %d is disabled", i);
			continue; // not fatal
		}

		if (!parse_json_string(j_remote_i, "interface", &interface)) {
			OPAE_ERR("remote %d missing \"interface\" key", i);
			goto out_parse_failed;
		}

		if (!strcmp(interface, "UNIX_DOMAIN_SOCKET")) {
			if (parse_remote_unix_domain_socket(j_remote_i, &comms[j]))
				goto out_parse_failed;
			++j;
		} else if (!strcmp(interface, "INET_SOCK_STREAM")) {
			if (parse_remote_inet_socket(j_remote_i, &comms[j]))
				goto out_parse_failed;
			++j;
		} else {
			OPAE_ERR("Unrecognized remote interface: %s", interface);
			goto out_parse_failed;
		}

	}

	goto out_free;

out_parse_failed:
	opae_free(comms);
	comms = NULL;
out_free:
	if (root)
		json_object_put(root);
	return comms;
}
