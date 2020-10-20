// Copyright(c) 2020, Intel Corporation
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
#include "legacy_dbg.h"

#include <arpa/inet.h>
#include <new>

#include "mmlink_server.h"
#include "mm_debug_link_interface.h"

int legacy_dbg::run(volatile uint64_t *mmio_ptr, const char *address, int port)
{
	mmlink_server *server = NULL;
	struct sockaddr_in sock;
  int res = -1;

	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);
	if (1 != inet_pton(AF_INET, address, &sock.sin_addr)) {
		PRINT_ERR("Failed to convert IP address: %s\n", address);
		return -1;
	}


	mm_debug_link_interface *driver = get_mm_debug_link();
	server = new (std::nothrow) mmlink_server(&sock, driver);
	if (!server) {
		PRINT_ERR("Failed to allocate memory \n");
		return -1;
	}

	// Run MMLink server
	res = server->run((unsigned char*)mmio_ptr);
  delete server;
  return res;
}
