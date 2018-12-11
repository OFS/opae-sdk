// Copyright(c) 2017-2018, Intel Corporation
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
#include "intel-fpga.h"
#include "fpga-dfl.h"

#define FPGA_MAX_INTERFACE_NUM      1

//Assign Port to PF from Interface
#define ASSIGN_PORT_TO_PF           0

//Release Port from PF and Assign to Interface
#define ASSIGN_PORT_TO_HOST         1

//Assign and Release  port to a host interface
fpga_result __FPGA_API__ xfpga_fpgaAssignPortToInterface(fpga_handle fpga,
						uint32_t interface_num,
						uint32_t slot_num,
						int flags)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)fpga;
	struct _fpga_token *_token   = (struct _fpga_token *)_handle->token;
	fpga_result result           = FPGA_OK;
	struct fpga_fme_port_assign config                = {0};
	struct dfl_fpga_fme_port_assign dfl_port_assign   = {0};
	struct dfl_fpga_fme_port_release dfl_port_release = {0};
	int err;

	UNUSED_PARAM(flags);

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (interface_num > FPGA_MAX_INTERFACE_NUM) {
		FPGA_ERR("Invalid input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (_token->drv_devl_ver == FPGA_LATEST_DRV_VER) {

		config.argsz = sizeof(config);
		config.flags = 0;
		config.port_id = slot_num;

		// Assign Port to PF from Interface
		if (interface_num == ASSIGN_PORT_TO_PF) {

			result = ioctl(_handle->fddev, FPGA_FME_PORT_ASSIGN, &config);
			if (result != 0) {
				FPGA_ERR("Failed to assign port");
				result = FPGA_NOT_SUPPORTED;
				goto out_unlock;
			}
		}

		// Release Port from PF and assign to Interface
		if (interface_num == ASSIGN_PORT_TO_HOST) {

			result = ioctl(_handle->fddev, FPGA_FME_PORT_RELEASE, &config);
			if (result != 0) {
				FPGA_ERR("Failed to release port");
				result = FPGA_NOT_SUPPORTED;
				goto out_unlock;
			}
		}
	} else {

		// Assign Port to PF from Interface
		if (interface_num == ASSIGN_PORT_TO_PF) {

			dfl_port_assign.argsz = sizeof(dfl_port_assign);
			dfl_port_assign.flags = 0;
			dfl_port_assign.port_id = slot_num;


			result = ioctl(_handle->fddev, DFL_FPGA_FME_PORT_ASSIGN, &dfl_port_assign);
			if (result != 0) {
				FPGA_ERR("Failed to assign port");
				result = FPGA_NOT_SUPPORTED;
				goto out_unlock;
			}
		}

		// Release Port from PF and assign to Interface
		if (interface_num == ASSIGN_PORT_TO_HOST) {

			dfl_port_release.argsz = sizeof(dfl_port_release);
			dfl_port_release.flags = 0;
			dfl_port_release.port_id = slot_num;

			result = ioctl(_handle->fddev, DFL_FPGA_FME_PORT_RELEASE, &dfl_port_release);
			if (result != 0) {
				FPGA_ERR("Failed to release port");
				result = FPGA_NOT_SUPPORTED;
				goto out_unlock;
			}
		}
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;

}
