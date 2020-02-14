// Copyright(c) 2017-2020, Intel Corporation
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

#include "opae/access.h"
#include "opae/utils.h"
#include "opae/manage.h"
#include "common_int.h"
#include "opae_drv.h"

//Assign Port to PF from Interface
#define ASSIGN_PORT_TO_PF           0

//Release Port from PF and Assign to Interface
#define ASSIGN_PORT_TO_HOST         1

//Assign and Release  port to a host interface
fpga_result __XFPGA_API__ xfpga_fpgaAssignPortToInterface(fpga_handle fpga,
						uint32_t interface_num,
						uint32_t slot_num,
						int flags)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)fpga;
	fpga_result result = FPGA_OK;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	switch (interface_num) {
	case ASSIGN_PORT_TO_PF:
		result = opae_fme_port_assign(_handle->fddev, flags, slot_num);
		if (result) {
			FPGA_ERR("Failed to assign port");
		}
		break;
	case ASSIGN_PORT_TO_HOST:
		result = opae_fme_port_release(_handle->fddev, flags, slot_num);
		if (result) {
			FPGA_ERR("Failed to releae port");
		}
		break;
	default:
		FPGA_MSG("Unknown port assignment operation: %d",
			 interface_num);
		result = FPGA_INVALID_PARAM;
	}


out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;

}
