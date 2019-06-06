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
#include "common_int.h"
#include "opae_drv.h"
#include "intel-fpga.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>

/* Port UAFU */
#define AFU_PERMISSION (FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP)
#define AFU_SIZE	0x40000
#define AFU_OFFSET	0

STATIC fpga_result port_mmap_region(fpga_handle handle,
			     void **vaddr,
			     uint64_t size,
			     uint32_t flags,
			     uint64_t offset,
			     uint32_t mmio_num)
{
	void *addr;
	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	fpga_result result = FPGA_OK;

	UNUSED_PARAM(mmio_num);

	/* Assure returning pointer contains allocated memory */
	ASSERT_NOT_NULL(vaddr);

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	/* Map MMIO memory */
	addr = (void *) mmap(NULL, size, flags, MAP_SHARED, _handle->fddev, offset);
	if (addr == MAP_FAILED) {
		FPGA_MSG("Unable to map MMIO region. Error value is : %s",
			 strerror(errno));
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	/* Save return address */
	*vaddr = addr;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

STATIC fpga_result map_mmio_region(fpga_handle handle, uint32_t mmio_num)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	void     *addr  = NULL;
	uint64_t wsid   = 0;
	fpga_result result = FPGA_OK;
	opae_port_region_info rinfo = { 0 };

	result = opae_get_port_region_info(_handle->fddev, mmio_num, &rinfo);
	// TODO: process result seperately of rinfo.flags
	if (result || (rinfo.flags != AFU_PERMISSION)) {
		FPGA_MSG("Invalid MMIO permission flags");
		result = FPGA_NO_ACCESS;
		return result;
	}

	/* Map UAFU MMIO */
	result = port_mmap_region(handle,
				    (void **) &addr,
				    rinfo.size,
				    PROT_READ | PROT_WRITE,
				    rinfo.offset,
				    mmio_num);
	if (result != FPGA_OK)
		return result;

	/* Add to MMIO list */
	wsid = wsid_gen();
	if (!wsid_add(_handle->mmio_root,
		      wsid,
		      (uint64_t) addr,
		      (uint64_t) NULL,
		      rinfo.size,
		      (uint64_t) addr,
		      mmio_num,
		      0)) {
		if (munmap(addr, rinfo.size)) {
			FPGA_MSG("munmap failed. Error value is : %s",
				 strerror(errno));
			return FPGA_INVALID_PARAM;
		} else {
			FPGA_MSG("Failed to add MMIO id: %d", mmio_num);
			return FPGA_NO_MEMORY;
		}
	}

	return FPGA_OK;
}

/* Lazy mapping of MMIO region (only map if not already mapped) */
STATIC fpga_result find_or_map_wm(fpga_handle handle, uint32_t mmio_num,
				struct wsid_map **wm_out)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	wm = wsid_find_by_index(_handle->mmio_root, mmio_num);
	if (!wm) {
		result = map_mmio_region(handle, mmio_num);
		if (result != FPGA_OK) {
			FPGA_ERR("failed to map mmio region %d", mmio_num);
			return result;
		}
		wm = wsid_find_by_index(_handle->mmio_root, mmio_num);
		if (!wm) {
			FPGA_ERR("unable to map wsid for mmio region %d", mmio_num);
			return FPGA_NO_MEMORY;
		}
	}

	*wm_out = wm;
	return FPGA_OK;
}

fpga_result __FPGA_API__ xfpga_fpgaWriteMMIO32(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset,
					 uint32_t value)
{

	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	if (offset % sizeof(uint32_t) != 0) {
		FPGA_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	if (offset > wm->len) {
		FPGA_MSG("offset out of bounds");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint32_t *) ((uint8_t *)wm->offset + offset)) = value;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaReadMMIO32(fpga_handle handle,
					uint32_t mmio_num,
					uint64_t offset,
					uint32_t *value)
{
	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	if (offset % sizeof(uint32_t) != 0) {
		FPGA_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	if (offset > wm->len) {
		FPGA_MSG("offset out of bounds");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*value = *((volatile uint32_t *) ((uint8_t *)wm->offset + offset));

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaWriteMMIO64(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset,
					 uint64_t value)
{
	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	if (offset % sizeof(uint64_t) != 0) {
		FPGA_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	if (offset > wm->len) {
		FPGA_MSG("offset out of bounds");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint64_t *) ((uint8_t *)wm->offset + offset)) = value;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaReadMMIO64(fpga_handle handle,
					uint32_t mmio_num,
					uint64_t offset,
					uint64_t *value)
{
	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	if (offset % sizeof(uint64_t) != 0) {
		FPGA_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	if (offset > wm->len) {
		FPGA_MSG("offset out of bounds");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*value = *((volatile uint64_t *) ((uint8_t *)wm->offset + offset));

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

static inline void copy512(const void *src, void *dst)
{
    asm volatile("vmovdqu64 (%0), %%zmm0;"
		 "vmovdqu64 %%zmm0, (%1);"
		 :
		 : "r"(src), "r"(dst));
}

fpga_result __FPGA_API__ xfpga_fpgaWriteMMIO512(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset,
					 const void *value)
{
	int err;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;

	if (offset % 64 != 0) {
		FPGA_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	if (offset > wm->len) {
		FPGA_MSG("offset out of bounds");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	copy512(value, (uint8_t *)wm->offset + offset);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaMapMMIO(fpga_handle handle,
				     uint32_t mmio_num,
				     uint64_t **mmio_ptr)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct wsid_map *wm = NULL;
	fpga_result result = FPGA_OK;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = find_or_map_wm(handle, mmio_num, &wm);
	if (result)
		goto out_unlock;

	/* Store return value only if return pointer has allocated memory */
	if (mmio_ptr)
		*mmio_ptr = (uint64_t *)wm->addr;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaUnmapMMIO(fpga_handle handle,
				       uint32_t mmio_num)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	void *mmio_ptr;
	fpga_result result = FPGA_OK;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	/* Fetch the MMIO physical address and length */
	struct wsid_map *wm = wsid_find_by_index(_handle->mmio_root, mmio_num);
	if (!wm) {
		FPGA_MSG("MMIO region %d not found", mmio_num);
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	/* Unmap UAFU MMIO */
	mmio_ptr = (void *) wm->offset;
	if (munmap((void *) mmio_ptr, wm->len)) {
		FPGA_MSG("munmap failed: %s",
			 strerror(errno));
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	/* Remove MMIO */
	wsid_del(_handle->mmio_root, wm->wsid);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}
