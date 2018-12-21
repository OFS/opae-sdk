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
#include "opae_ioctl.h"
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
		FPGA_ERR("Invalid handle file descriptor");
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
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// Set Umsg Attributes
fpga_result __FPGA_API__
xfpga_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value)
{
	struct _fpga_handle  *_handle         = (struct _fpga_handle *)handle;
	fpga_result result                    = FPGA_OK;
	struct fpga_port_umsg_cfg umsg_cfg    = {0};
	int err                               = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	// Set ioctl Umsg  config struct parameters
	umsg_cfg.argsz = sizeof(umsg_cfg);
	umsg_cfg.flags = 0;
	umsg_cfg.hint_bitmap = (__u32)value ;

	result = ioctl(_handle->fddev, FPGA_PORT_UMSG_SET_MODE, &umsg_cfg);
	if (result != 0) {
		FPGA_MSG("FPGA_PORT_UMSG_SET_MODE ioctl failed");
		if ((errno == EINVAL) ||
		(errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// Gets Umsg address
fpga_result __FPGA_API__
xfpga_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr)
{
	struct _fpga_handle  *_handle           = (struct _fpga_handle *)handle;
	struct fpga_port_dma_map dma_map         = {0};
	struct fpga_port_dma_unmap dma_unmap     = {0};
	struct fpga_port_umsg_base_addr baseaddr = {0};

	fpga_result result                        = FPGA_OK;
	uint64_t umsg_count                       = 0;
	uint64_t umsg_size                        = 0;
	int pagesize                              = 0;
	void *umsg_virt                           = NULL;
	int err                                   = 0;

	ASSERT_NOT_NULL(umsg_ptr);
	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
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
	result = xfpga_fpgaGetNumUmsg(handle, &umsg_count);
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to get UMSG count");
		result = FPGA_EXCEPTION;
		goto out_unlock;
	}

	umsg_size = (uint64_t)umsg_count  * pagesize;
	umsg_virt = alloc_buffer(umsg_size);
	if (umsg_virt == NULL) {
		FPGA_MSG("Failed to allocate memory");
		result = FPGA_NO_MEMORY;
		goto out_unlock;
	}

	// Map Umsg Buffer
	dma_map.argsz = sizeof(dma_map);
	dma_map.flags = 0;
	dma_map.user_addr = (__u64) umsg_virt;
	dma_map.length = umsg_size;
	dma_map.iova = 0 ;

	result = ioctl(_handle->fddev, FPGA_PORT_DMA_MAP, &dma_map);
	if (result != 0) {
		FPGA_MSG("Failed to map UMSG buffer");
		result = FPGA_INVALID_PARAM;
		goto umsg_exit;
	}

	// Set Umsg Address
	baseaddr.argsz = sizeof(baseaddr);
	baseaddr.flags = 0;
	baseaddr.iova = dma_map.iova ;

	result = ioctl(_handle->fddev, FPGA_PORT_UMSG_SET_BASE_ADDR, &baseaddr);
	if (result != 0) {
		FPGA_MSG("Failed to set UMSG base address");
		if ((errno == EINVAL) ||
		(errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
		goto umsg_map_exit;
	}

	result = ioctl(_handle->fddev, FPGA_PORT_UMSG_ENABLE, NULL);
	if (result != 0) {
		FPGA_MSG("Failed to enable UMSG");
		if ((errno == EINVAL) ||
		(errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
		goto umsg_map_exit;
	}

	*umsg_ptr = (uint64_t *) umsg_virt;
	_handle->umsg_iova = (uint64_t *) dma_map.iova;
	_handle->umsg_virt = umsg_virt;
	_handle->umsg_size = umsg_size;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;

umsg_map_exit:
	dma_unmap.argsz = sizeof(dma_unmap);
	dma_unmap.flags = 0;
	dma_unmap.iova = dma_map.iova;

	result = ioctl(_handle->fddev, FPGA_PORT_DMA_UNMAP, &dma_unmap);
	if (result != 0) {
		FPGA_MSG("Failed to unmap UMSG buffer");
		if ((errno == EINVAL) ||
		    (errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
	}

umsg_exit:
	if (umsg_virt != NULL)
		free_buffer(umsg_virt, umsg_size);

	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
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
		struct fpga_port_umsg_base_addr baseaddr;
		struct fpga_port_dma_unmap dma_unmap;

		if (ioctl(_handle->fddev, FPGA_PORT_UMSG_DISABLE, NULL) != 0) {
			FPGA_ERR("Failed to disable UMSG");
		}

		baseaddr.argsz = sizeof(baseaddr);
		baseaddr.flags = 0;
		baseaddr.iova = 0;

		if (ioctl(_handle->fddev, FPGA_PORT_UMSG_SET_BASE_ADDR, &baseaddr) != 0) {
			FPGA_ERR("Failed to zero UMSG address");
		}

		dma_unmap.argsz = sizeof(dma_unmap);
		dma_unmap.flags = 0;
		dma_unmap.iova = (__u64) _handle->umsg_iova;

		if (ioctl(_handle->fddev, FPGA_PORT_DMA_UNMAP, &dma_unmap) != 0) {
			FPGA_ERR("Failed to unmap UMSG Buffer");
		}

		free_buffer(_handle->umsg_virt, _handle->umsg_size);

		_handle->umsg_virt = NULL;
		_handle->umsg_size = 0;
		_handle->umsg_iova = NULL;
	}

	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
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
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = xfpga_fpgaGetUmsgPtr(handle, &umsg_ptr);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get UMsg buffer");
		goto out_unlock;
	}

	// Assign Value to UMsg
	*((volatile uint64_t *) (umsg_ptr)) = value;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}
