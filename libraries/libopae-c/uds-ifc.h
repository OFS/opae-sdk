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
#ifndef __OPAE_UDS_IFC_H__
#define __OPAE_UDS_IFC_H__
#include "rmt-ifc.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct _opae_uds_client_connection {
        char socket_name[OPAE_SOCKET_NAME_MAX];
        int client_socket;
	int send_flags;
	int receive_flags;
} opae_uds_client_connection;

int opae_uds_client_open(void *con);

int opae_uds_client_close(void *con);

ssize_t opae_uds_client_send(void *con, const void *buf, size_t len);

ssize_t opae_uds_client_receive(void *con, void *buf, size_t len);

int opae_uds_client_release(void *con);

int opae_uds_ifc_init(opae_remote_client_ifc *i,
		      const char *socket_name,
		      int send_flags,
		      int receive_flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_UDS_IFC_H__
