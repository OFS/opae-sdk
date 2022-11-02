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

#include <sys/types.h>
#include <sys/socket.h>

#include "mock/opae_std.h"
#include "action.h"
#include "handle_client.h"
#include "udsserv.h"

typedef bool (*srv_handler)(opae_remote_context *c,
			    const char *req_json,
			    char **resp_json);

STATIC srv_handler handlers[] = {
	opae_handle_fpgaEnumerate_request_0,
	opae_handle_fpgaDestroyToken_request_1,
	opae_handle_fpgaCloneToken_request_2,
	opae_handle_fpgaGetProperties_request_3,
	opae_handle_fpgaUpdateProperties_request_4,
	opae_handle_fpgaOpen_request_5,
	opae_handle_fpgaClose_request_6,
	opae_handle_fpgaReset_request_7,
	opae_handle_fpgaGetPropertiesFromHandle_request_8,
	opae_handle_fpgaMapMMIO_request_9,
	opae_handle_fpgaUnmapMMIO_request_10,
	opae_handle_fpgaReadMMIO32_request_11,
	opae_handle_fpgaWriteMMIO32_request_12,
	opae_handle_fpgaReadMMIO64_request_13,
	opae_handle_fpgaWriteMMIO64_request_14,
	opae_handle_fpgaWriteMMIO512_request_15,

};

int handle_client(uds_server_context *c, void *remote_ctx, int sock)
{
	char buf[OPAE_RECEIVE_BUF_MAX];
	ssize_t n;
	opae_remote_context *remc = (opae_remote_context *)remote_ctx;

	struct json_object *root = NULL;
        enum json_tokener_error j_err = json_tokener_success;
	opae_request_header header;
	char *response_json = NULL;
	//bool res;

	n = recv(sock, buf, sizeof(buf), 0);
	if (n < 0) {
		OPAE_ERR("recv() failed");
		return (int)n;
	}

	if (!n) { // socket closed by peer
		uds_server_close_client(c, sock);
		return (int)n;
	}

	// deal with buf

	printf("%s\n", buf);

	root = json_tokener_parse_verbose(buf, &j_err);
        if (!root) {
                OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return 1;
        }

	if (!opae_decode_request_header_obj(root, &header)) {
                OPAE_ERR("request header decode failed");
		json_object_put(root);
                return 2;
        }

	json_object_put(root);

	if (header.request_id >= (sizeof(handlers) / sizeof(handlers[0]))) {
		// Deal with out-of-bounds request_id.

		return 3;
	}

	handlers[header.request_id](remc, buf, &response_json);
	if (response_json) {

		send(sock, response_json, strlen(response_json) + 1, 0);
		opae_free(response_json);

		return 0;
	}

	return 4;
}
