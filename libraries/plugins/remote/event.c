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

//#ifndef _GNU_SOURCE
//#define _GNU_SOURCE
//#endif // _GNU_SOURCE

//#include <string.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <sys/eventfd.h>
//#include <errno.h>

#include <opae/types.h>

//#include <opae/properties.h>
//#include "xfpga.h"
//#include "common_int.h"
//#include "opae_drv.h"
//#include "types_int.h"
//#include "intel-fpga.h"

#include "mock/opae_std.h"

#define EVENT_SOCKET_NAME "/tmp/fpga_event_socket"
#define EVENT_SOCKET_NAME_LEN 23

enum request_type { REGISTER_EVENT = 0, UNREGISTER_EVENT = 1 };

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};


fpga_result __REMOTE_API__
remote_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	fpga_result result = FPGA_OK;
(void) event_handle;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
(void) event_handle;

	return FPGA_OK;
}

fpga_result __REMOTE_API__
remote_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh, int *fd)
{
(void) eh;
(void) fd;


	return FPGA_OK;
}

fpga_result __REMOTE_API__ remote_fpgaRegisterEvent(fpga_handle handle,
						 fpga_event_type event_type,
						 fpga_event_handle event_handle,
						 uint32_t flags)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) event_type;
(void) event_handle;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
			   fpga_event_handle event_handle)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) event_type;
(void) event_handle;


	return result;
}
