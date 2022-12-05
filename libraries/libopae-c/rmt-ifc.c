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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <opae/log.h>

#include "rmt-ifc.h"

ssize_t chunked_send(int sockfd, const void *buf, size_t len, int flags)
{
	ssize_t total_bytes = 0;
	ssize_t sent;
	const uint8_t *rdbuf = (const uint8_t *)buf;

	do {
		sent = send(sockfd,
			    rdbuf + total_bytes,
			    len - total_bytes,
			    flags);

		if (sent >= 0) {
			total_bytes += sent;
		} else {
			// sent < 0: Error
			if (errno != EINTR) {
				OPAE_ERR("send() failed: %s",
					 strerror(errno));
				return -1;
			}
		}

	} while ((size_t)total_bytes < len);

	return total_bytes;
}

ssize_t chunked_recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t total_bytes = 0;
	ssize_t received;
	uint8_t *rdbuf = (uint8_t *)buf;

	do {
		received = recv(sockfd,
				rdbuf + total_bytes,
				len - total_bytes,
				flags);

		if (!received) {
			// Orderly shutdown by peer.
			return -2;
		} else if (received > 0) {
			total_bytes += received;
		} else {
			// received < 0: Error
			if (errno != EINTR) {
				OPAE_ERR("recv() failed: %s",
					 strerror(errno));
				return -1;
			}
		}

	} while (!total_bytes || (rdbuf[total_bytes - 1] != 0));
#if 0
	printf("%s\n", (const char *)buf);
#endif
	return total_bytes;
}
