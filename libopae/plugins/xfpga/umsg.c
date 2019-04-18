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

#include "opae/access.h"
#include "opae/utils.h"
#include "opae/umsg.h"
#include "common_int.h"
#include "opae_drv.h"
#include "intel-fpga.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>


// Get number of Umsgs
fpga_result __FPGA_API__
xfpga_fpgaGetNumUmsg(fpga_handle handle, uint64_t *value)
{
	struct _fpga_handle  *_handle = (struct _fpga_handle *)handle;
	fpga_result result            = FPGA_OK;
	int err                       = 0;
	opae_port_info port_info      = { 0 };

	ASSERT_NOT_NULL(value);
	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}


	result = opae_get_port_info(_handle->fddev, &port_info);
	if (!result) {
		*value = port_info.num_umsgs;
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// Set Umsg Attributes
fpga_result __FPGA_API__
xfpga_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value)
{
	struct _fpga_handle  *_handle         = (struct _fpga_handle *)handle;
	fpga_result result                    = FPGA_OK;
	int err                               = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}


	result = opae_port_umsg_cfg(_handle->fddev, 0, value);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// Gets Umsg address
fpga_result __FPGA_API__
xfpga_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr)
{
	struct _fpga_handle  *_handle            = (struct _fpga_handle *)handle;
	opae_port_info port_info                 = { 0 };

	fpga_result result                       = FPGA_OK;
	uint64_t umsg_count                      = 0;
	uint64_t umsg_size                       = 0;
	int pagesize                             = 0;
	void *umsg_virt                          = NULL;
	int err                                  = 0;
	uint64_t io_addr = 0;

	ASSERT_NOT_NULL(umsg_ptr);
	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (_handle->umsg_iova != NULL) {
		*umsg_ptr = _handle->umsg_virt;
		goto out_unlock;
	}

	// Page size
	pagesize = sysconf(_SC_PAGESIZE);

	// get umsg count
	result = opae_get_port_info(_handle->fddev, &port_info);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to get UMSG count");
		goto out_unlock;
	}
	umsg_count = port_info.num_umsgs;
	umsg_size = (uint64_t)umsg_count  * pagesize;
	umsg_virt = alloc_buffer(umsg_size);
	if (umsg_virt == NULL) {
		OPAE_ERR("Failed to allocate memory");
		result = FPGA_NO_MEMORY;
		goto out_unlock;
	}

	// Map Umsg Buffer
	result = opae_port_map(_handle->fddev, umsg_virt, umsg_size, 0, &io_addr);
	if (result != 0) {
		OPAE_ERR("Failed to map UMSG buffer");
		goto umsg_exit;
	}

	// Set Umsg Address
	result = opae_port_umsg_set_base_addr(_handle->fddev, 0, io_addr);
	if (result != 0) {
		OPAE_ERR("Failed to set UMSG base address");
		goto umsg_map_exit;
	}

	result = opae_port_umsg_enable(_handle->fddev);
	if (result != 0) {
		OPAE_ERR("Failed to enable UMSG");
		goto umsg_map_exit;
	}

	*umsg_ptr = (uint64_t *) umsg_virt;
	_handle->umsg_iova = (uint64_t *)io_addr;
	_handle->umsg_virt = umsg_virt;
	_handle->umsg_size = umsg_size;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;

umsg_map_exit:
	result = opae_port_unmap(_handle->fddev, io_addr);
	if (result) {
		OPAE_MSG("Failed to unmap UMSG buffer");
	}

umsg_exit:
	if (umsg_virt != NULL)
		free_buffer(umsg_virt, umsg_size);

	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result free_umsg_buffer(fpga_handle handle)
{
	fpga_result result                = FPGA_OK;
	struct _fpga_handle  *_handle     = (struct _fpga_handle *)handle;
	int err                           = 0;


	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->umsg_virt != NULL) {
		if (opae_port_umsg_disable(_handle->fddev)) {
			OPAE_ERR("Failed to disable UMSG");
		}

		if (opae_port_umsg_set_base_addr(_handle->fddev, 0, 0)) {
			OPAE_ERR("Failed to zero UMSG address");
		}

		if (opae_port_unmap(_handle->fddev, (uint64_t)_handle->umsg_iova)) {
			OPAE_ERR("Failed to unmap UMSG Buffer");
		}

		free_buffer(_handle->umsg_virt, _handle->umsg_size);

		_handle->umsg_virt = NULL;
		_handle->umsg_size = 0;
		_handle->umsg_iova = NULL;
	}

	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// Trigger umsg
fpga_result __FPGA_API__
xfpga_fpgaTriggerUmsg(fpga_handle handle, uint64_t value)
{
	struct _fpga_handle  *_handle = (struct _fpga_handle *)handle;
	fpga_result result            = FPGA_OK;
	uint64_t *umsg_ptr            = NULL;
	int err                       = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = xfpga_fpgaGetUmsgPtr(handle, &umsg_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get UMsg buffer");
		goto out_unlock;
	}

	// Assign Value to UMsg
	*((volatile uint64_t *) (umsg_ptr)) = value;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}
