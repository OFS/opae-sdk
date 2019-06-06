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
#endif				// HAVE_CONFIG_H

#include <opae/access.h>
#include <opae/utils.h>
#include "common_int.h"
#include "ase_common.h"

#include <errno.h>
#include <malloc.h>		/* malloc */
#include <stdlib.h>		/* exit */
#include <stdio.h>		/* printf */
#include <string.h>		/* memcpy */
#include <unistd.h>		/* getpid */
#include <sys/types.h>		/* pid_t */
#include <sys/ioctl.h>		/* ioctl */
#include <sys/mman.h>		/* mmap & munmap */
#include <sys/time.h>		/* struct timeval */


fpga_result __FPGA_API__ ase_fpgaWriteMMIO32(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset, uint32_t value)
{
	UNUSED_PARAM(mmio_num);

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (NULL == mmio_afu_vbase) {
		return FPGA_NOT_FOUND;
	} else {
		if (offset % sizeof(uint32_t) != 0) {
			FPGA_MSG("Misaligned MMIO access");
			return FPGA_INVALID_PARAM;
		} else {
			if (offset > MMIO_AFU_OFFSET) {
				FPGA_MSG("Offset out of bounds");
				return FPGA_INVALID_PARAM;
			}

			mmio_write32(offset, value);
			return FPGA_OK;
		}
	}

	return FPGA_OK;
}

fpga_result __FPGA_API__ ase_fpgaReadMMIO32(fpga_handle handle,
					uint32_t mmio_num, uint64_t offset,
					uint32_t *value)
{
	UNUSED_PARAM(mmio_num);
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (NULL == mmio_afu_vbase) {
		return FPGA_NOT_FOUND;
	} else {
		if (offset % sizeof(uint32_t) != 0) {
			FPGA_MSG("Misaligned MMIO access");
			return FPGA_INVALID_PARAM;
		} else {
			if (offset > MMIO_AFU_OFFSET) {
				FPGA_MSG("offset out of bounds");
				return FPGA_INVALID_PARAM;
			}

			mmio_read32(offset, value);
			return FPGA_OK;
		}
	}

}


fpga_result __FPGA_API__ ase_fpgaWriteMMIO64(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset, uint64_t value)
{
	UNUSED_PARAM(mmio_num);
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (NULL == mmio_afu_vbase) {
		return FPGA_NOT_FOUND;
	} else {
		if (offset % sizeof(uint64_t) != 0) {
			FPGA_MSG("Misaligned MMIO access");
			return FPGA_INVALID_PARAM;
		} else {
			if (offset > MMIO_AFU_OFFSET) {
				FPGA_MSG("Offset out of bounds");
				return FPGA_INVALID_PARAM;
			}
			mmio_write64(offset, value);
			return FPGA_OK;
		}
	}

}

fpga_result __FPGA_API__ ase_fpgaReadMMIO64(fpga_handle handle,
					uint32_t mmio_num, uint64_t offset,
					uint64_t *value)
{
	UNUSED_PARAM(mmio_num);
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (NULL == mmio_afu_vbase) {
		return FPGA_NOT_FOUND;
	} else {
		if (offset % sizeof(uint64_t) != 0) {
			FPGA_MSG("Misaligned MMIO access");
			return FPGA_INVALID_PARAM;
		} else {
			if (offset > MMIO_AFU_OFFSET) {
				FPGA_MSG("Offset out of bounds");
				return FPGA_INVALID_PARAM;
			}
			mmio_read64(offset, (uint64_t *) value);
			return FPGA_OK;
		}
	}

}

fpga_result __FPGA_API__ ase_fpgaWriteMMIO512(fpga_handle handle,
					 uint32_t mmio_num,
					 uint64_t offset, const void *value)
{
	UNUSED_PARAM(mmio_num);

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (NULL == mmio_afu_vbase) {
		return FPGA_NOT_FOUND;
	} else {
		if (offset % 64 != 0) {
			FPGA_MSG("Misaligned MMIO access");
			return FPGA_INVALID_PARAM;
		} else {
			if (offset > MMIO_AFU_OFFSET) {
				FPGA_MSG("Offset out of bounds");
				return FPGA_INVALID_PARAM;
			}

			mmio_write512(offset, value);
			return FPGA_OK;
		}
	}

	return FPGA_OK;
}

fpga_result __FPGA_API__ ase_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
				     uint64_t **mmio_ptr)
{
	UNUSED_PARAM(mmio_num);
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	fpga_result result = FPGA_OK;

	//check handle
	if (!_handle) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}
	// TODO: check mmio_num?

	if (!_handle->fpgaMMIO_is_mapped)
		_handle->fpgaMMIO_is_mapped = true;

	if (mmio_ptr) {
		BEGIN_YELLOW_FONTCOLOR;
		printf
			("  [APP] ** WARNING ** => ASE does not support pointer access to MMIO, use mmio{Read,Write}{32,64} functions\n");
		END_YELLOW_FONTCOLOR;
		return FPGA_NOT_SUPPORTED;
	} else {
		BEGIN_YELLOW_FONTCOLOR;
		printf("  [APP] ** WARNING ** => Calling fpgaMapMMIO() without passing a pointer is deprecated\n");
		END_YELLOW_FONTCOLOR;
	}
	return result;
}

fpga_result __FPGA_API__ ase_fpgaUnmapMMIO(fpga_handle handle,
				       uint32_t mmio_num)
{
	UNUSED_PARAM(handle);
	UNUSED_PARAM(mmio_num);

	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;

	if (!_handle) {
		FPGA_ERR("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		FPGA_MSG("Invalid handle object");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->fpgaMMIO_is_mapped) {
		_handle->fpgaMMIO_is_mapped = false;
		return FPGA_OK;
	} else {
		return FPGA_INVALID_PARAM;
	}
}

fpga_result __FPGA_API__ ase_fpgaReset(fpga_handle handle)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	if (_handle == NULL) {
		FPGA_ERR("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		FPGA_MSG("Invalid handle object");
		return FPGA_INVALID_PARAM;
	}

	send_swreset();

	return FPGA_OK;
}
