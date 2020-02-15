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
#include "common_int.h"
#include "intel-fpga.h"

#include "opae_drv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/* Others */
#define KB 1024
#define MB (1024 * KB)
#define GB (1024UL * MB)

#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_1G_HUGEPAGE	(0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#endif


/*
 * Allocate (mmap) new buffer
 */
STATIC fpga_result buffer_allocate(void **addr, uint64_t len, int flags)
{
	void *addr_local = NULL;

	UNUSED_PARAM(flags);

	ASSERT_NOT_NULL(addr);

	/* ! FPGA_BUF_PREALLOCATED, allocate memory using huge pages
	   For buffer > 2M, use 1G-hugepage to ensure pages are
	   contiguous */
	if (len > 2 * MB)
		addr_local = mmap(ADDR, len, PROTECTION, FLAGS_1G, 0, 0);
	else if (len > 4 * KB)
		addr_local = mmap(ADDR, len, PROTECTION, FLAGS_2M, 0, 0);
	else
		addr_local = mmap(ADDR, len, PROTECTION, FLAGS_4K, 0, 0);
	if (addr_local == MAP_FAILED) {
		if (errno == ENOMEM) {
			if (len > 2 * MB)
				OPAE_MSG("Could not allocate buffer (no free 1 "
					 "GiB huge pages)");
			if (len > 4 * KB)
				OPAE_MSG("Could not allocate buffer (no free 2 "
					 "MiB huge pages)");
			else
				OPAE_MSG("Could not allocate buffer (out of "
					 "memory)");
			return FPGA_NO_MEMORY;
		}
		OPAE_MSG("FPGA buffer mmap failed: %s", strerror(errno));
		return FPGA_INVALID_PARAM;
	}

	*addr = addr_local;
	return FPGA_OK;
}

/*
 * Release (unmap) allocated buffer
 */
STATIC fpga_result buffer_release(void *addr, uint64_t len)
{
	/* If the buffer allocation was backed by hugepages, then
	 * len must be rounded up to the nearest hugepage size,
	 * otherwise munmap will fail.
	 *
	 * Buffer with size larger than 2MB is backed by 1GB page(s),
	 * round up the size to the nearest GB boundary.
	 *
	 * Buffer with size smaller than 2MB but larger than 4KB is
	 * backed by a 2MB pages, round up the size to 2MB.
	 *
	 * Buffer with size smaller than 4KB is backed by a 4KB page,
	 * and its size is already 4KB aligned.
	 */

	if (len > 2 * MB)
		len = (len + (1 * GB - 1)) & (~(1 * GB - 1));
	else if (len > 4 * KB)
		len = 2 * MB;

	if (munmap(addr, len)) {
		OPAE_MSG("FPGA buffer munmap failed: %s",
			 strerror(errno));
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
					   void **buf_addr, uint64_t *wsid,
					   int flags)
{
	void *addr = NULL;
	fpga_result result = FPGA_OK;
	uint64_t io_addr = 0;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	int err;

	bool preallocated = (flags & FPGA_BUF_PREALLOCATED);
	bool quiet = (flags & FPGA_BUF_QUIET);

	bool read_only = (flags & FPGA_BUF_READ_ONLY);
	uint32_t map_flags = (read_only ? FPGA_DMA_TO_DEV : 0);

	uint64_t pg_size;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	/* Assure wsid is a valid pointer */
	if (!wsid) {
		OPAE_MSG("WSID is NULL");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (flags & (~(FPGA_BUF_PREALLOCATED | FPGA_BUF_QUIET |
		       FPGA_BUF_READ_ONLY))) {
		OPAE_MSG("Unrecognized flags");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	pg_size = (uint64_t) sysconf(_SC_PAGE_SIZE);

	if (preallocated) {
		/* A special case: respond FPGA_OK when !buf_addr and !len
		 * as an indication that FPGA_BUF_PREALLOCATED is supported
		 * by the library. */
		if (!buf_addr && !len) {
			result = FPGA_OK;
			goto out_unlock;
		}

		/* buffer is already allocated, check addresses */
		if (!buf_addr) {
			OPAE_MSG("No preallocated buffer address given");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		if (!(*buf_addr)) {
			OPAE_MSG("Preallocated buffer address is NULL");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		/* check length */
		if (!len || (len & (pg_size - 1))) {
			OPAE_MSG("Preallocated buffer size is not a non-zero multiple of page size");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		addr = *buf_addr;
	} else {

		if (!buf_addr) {
			OPAE_MSG("buffer address is NULL");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}

		if (!len) {
			OPAE_MSG("buffer length is zero");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}

		/* round up to nearest page boundary */
		if (len & (pg_size - 1)) {
			len = pg_size + (len & ~(pg_size - 1));
		}

		result = buffer_allocate(&addr, len, flags);
		if (result != FPGA_OK) {
			goto out_unlock;
		}
	}

	if (opae_port_map(_handle->fddev, addr, len, map_flags, &io_addr)) {
		if (!preallocated) {
			buffer_release(addr, len);
		}

		if (!quiet) {
			OPAE_MSG("FPGA_PORT_DMA_MAP ioctl failed: %s",
				 strerror(errno));
		}

		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}


	/* Generate unique workspace ID */
	*wsid = wsid_gen();

	/* Add to workspace id in order to store buffer length */
	if (!wsid_add(_handle->wsid_root, *wsid, (uint64_t)addr, io_addr, len,
		      0, 0, flags)) {
		if (!preallocated) {
			buffer_release(addr, len);
		}

		OPAE_MSG("Failed to add workspace id %lu", *wsid);
		result = FPGA_NO_MEMORY;
		goto out_unlock;
	}


	/* Update buf_addr */
	if (buf_addr)
		*buf_addr = addr;

	/* Return */
	result = FPGA_OK;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __XFPGA_API__
xfpga_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid)
{
	void *buf_addr;
	uint64_t iova;
	uint64_t len;
	int err;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result result = FPGA_NOT_FOUND;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	/* Fetch the buffer physical address and length */
	struct wsid_map *wm = wsid_find(_handle->wsid_root, wsid);
	if (!wm) {
		OPAE_MSG("WSID not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	buf_addr = (void *) wm->addr;
	iova = wm->phys;
	len = wm->len;

	bool preallocated = (wm->flags & FPGA_BUF_PREALLOCATED);

	if (opae_port_unmap(_handle->fddev, iova)) {
		OPAE_MSG("FPGA_PORT_DMA_UNMAP ioctl failed: %s",
			 strerror(errno));
		result = FPGA_INVALID_PARAM;
		goto ws_free;
	}

	/* If the buffer was allocated in xfpga_fpgaPrepareBuffer() (i.e. it was not
	 * preallocated), we need to unmap it here. Otherwise (if it was
	 * preallocated) the mapping needs to stay intact. */
	if (!preallocated) {
		result = buffer_release(buf_addr, len);
		if (result != FPGA_OK) {
			OPAE_MSG("Buffer release failed");
			goto ws_free;
		}
	}

	/* Return */
	result = FPGA_OK;

ws_free:
	/* Remove workspace */
	wsid_del(_handle->wsid_root, wsid);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __XFPGA_API__ xfpga_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
					  uint64_t *ioaddr)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct wsid_map *wm;
	fpga_result result = FPGA_OK;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	wm = wsid_find(_handle->wsid_root, wsid);
	if (!wm) {
		OPAE_MSG("WSID not found");
		result = FPGA_NOT_FOUND;
	} else {
		*ioaddr = wm->phys;
	}

	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}
