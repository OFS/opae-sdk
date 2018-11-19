// Copyright(c) 2017, Intel Corporation
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

#include <opae/access.h>
#include <opae/properties.h>
#include "common_int.h"
#include "ase_common.h"

fpga_result __FPGA_API__ fpgaCreateEventHandle(fpga_event_handle *handle)
{
	struct _fpga_event_handle *_eh;

	if (!handle)
		return FPGA_INVALID_PARAM;

	_eh = ase_malloc(sizeof(struct _fpga_event_handle));
	if (NULL == _eh) {
		FPGA_ERR("Could not allocate memory for event handle");
		return FPGA_NO_MEMORY;
	}

	/* create eventfd */
	_eh->fd = eventfd(0, 0);
	if (_eh->fd < 0) {
		FPGA_ERR("eventfd : %s", strerror(errno));
		free(_eh);
		_eh = NULL;
		return FPGA_NOT_FOUND;
	}

	*handle = (fpga_event_handle)_eh;
	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaDestroyEventHandle(fpga_event_handle *handle)
{
	struct _fpga_event_handle *_eh;
	if (!handle)
		return FPGA_INVALID_PARAM;

	_eh = (struct _fpga_event_handle *) *handle;

	if (NULL == _eh) {
		FPGA_ERR("Received NULL event handle");
		return FPGA_INVALID_PARAM;
	}

	if (close(_eh->fd) < 0) {
		FPGA_ERR("eventfd : %s", strerror(errno));
		if (errno == EBADF)
			return FPGA_INVALID_PARAM;
		else
			return FPGA_EXCEPTION;
	}

	free(*handle);
	*handle = NULL;
	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						int *fd)
{
	struct _fpga_event_handle *_eh = (struct _fpga_event_handle *) eh;
	if (NULL == _eh) {
		FPGA_ERR("Event handle is null");
		return FPGA_INVALID_PARAM;
	}

	*fd = _eh->fd;

	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaRegisterEvent(fpga_handle handle,
					   fpga_event_type type,
					   fpga_event_handle event_handle,
					   uint32_t flags)
{
	UNUSED_PARAM(handle);
	if (type != FPGA_EVENT_INTERRUPT)
		return FPGA_NOT_SUPPORTED;

	if (flags >= MAX_USR_INTRS)
		return FPGA_INVALID_PARAM;

	if (register_event(FILE_DESCRIPTOR(event_handle), flags) == 0)
		return FPGA_OK;
	else
		return FPGA_EXCEPTION;
}

fpga_result __FPGA_API__ fpgaUnregisterEvent(fpga_handle handle,
					     fpga_event_type event_type,
					     fpga_event_handle event_handle)
{
	UNUSED_PARAM(handle);
	if (event_type != FPGA_EVENT_INTERRUPT)
		return FPGA_NOT_SUPPORTED;

	if (unregister_event(FILE_DESCRIPTOR(event_handle)) == 0)
		return FPGA_OK;
	else
		return FPGA_EXCEPTION;
}
