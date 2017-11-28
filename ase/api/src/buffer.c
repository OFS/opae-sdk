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
#include <ase_common.h>

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


fpga_result __FPGA_API__ fpgaPrepareBuffer(fpga_handle handle,
					   uint64_t len, void **buf_addr,
					   uint64_t *wsid, int flags)
{
	struct buffer_t *buf;
	uint64_t pg_size;
	fpga_result result = FPGA_NOT_FOUND;

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (flags & FPGA_BUF_PREALLOCATED) {
		return FPGA_INVALID_PARAM;
	}
	pg_size = (uint64_t) sysconf(_SC_PAGE_SIZE);
	/* round up to nearest page boundary */
	if (!len || (len & (pg_size - 1))) {
		len = pg_size + (len & ~(pg_size - 1));
	}
	uint64_t *inp_buf_addr;

	buf = (struct buffer_t *) ase_malloc(sizeof(struct buffer_t));
	buf->memsize = (uint32_t) len;

	inp_buf_addr = (uint64_t *) (*buf_addr);

	// Allocate buffer (ASE call)
	allocate_buffer(buf, inp_buf_addr);

	if ((ASE_BUFFER_VALID != buf->valid) ||
	    (MAP_FAILED == (void *) buf->vbase) ||
	    (0 == buf->fake_paddr)) {
		printf("Error Allocating ASE buffer ... EXITING\n");
		result = FPGA_NO_MEMORY;
	} else {
		result = FPGA_OK;
	}

	*wsid = buf->index;

	*buf_addr = (void **) buf->vbase;

	return result;
}


fpga_result __FPGA_API__ fpgaReleaseBuffer(fpga_handle handle,
					   uint64_t wsid)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	fpga_result result = FPGA_NOT_FOUND;

	if (!_handle) {
		FPGA_MSG("Handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (!wsid)
		result = FPGA_NOT_FOUND;
	else {
		if (wsid < 2) {
			result = FPGA_INVALID_PARAM;
		} else {
			// Call deallocate_buffer_by_WSID
			if (!deallocate_buffer_by_index(wsid))
				result = FPGA_INVALID_PARAM;
			else
				result = FPGA_OK;
		}
	}
	return result;
}


fpga_result __FPGA_API__ fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
					  uint64_t *iova)
{

	struct buffer_t *iova_match_buf;

	fpga_result result;
	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	iova_match_buf = find_buffer_by_index(wsid);

	if (iova_match_buf != NULL) {
		result = FPGA_OK;
		*iova = iova_match_buf->fake_paddr;
	} else {
		result = FPGA_NOT_FOUND;
		*iova = (uint64_t) NULL;
	}

	return result;
}
