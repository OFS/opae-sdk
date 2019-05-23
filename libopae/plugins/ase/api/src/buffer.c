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
#endif

#include "opae/access.h"
#include "opae/utils.h"
#include "common_int.h"

#include "ase_common.h"
#include "ase_host_memory.h"

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
 * In simulation the physical page assignment is synthetic. We can afford to
 * be more lenient about the availability of huge pages on the host that
 * is running a simulation. This function first tries to use real huge pages.
 * If unavailable, it instead allocates a virtual region that is properly
 * aligned. The virtual region may actually be composed of smaller physical
 * pages but those page mappings are never exposed to the simulator. The
 * simulation will behave as though the underlying physical pages are huge.
 */
static void *sim_huge_mmap(void *addr, size_t length, int prot, int flags)
{
	void *addr_local;
	addr_local = mmap(addr, length, prot, flags, 0, 0);

	if (addr_local != MAP_FAILED) {
		// The huge mapping worked
		return addr_local;
	}

	// Try again without huge pages but with extra space for alignment.
	// Start by forcing the size to be a multiple of the desired pages.
	size_t length_extra = (flags & MAP_1G_HUGEPAGE) ? GB : 2 * MB;
	length = (length + length_extra - 1) & ~(length_extra - 1);

	addr_local = mmap(addr, length + length_extra, prot,
			  flags & ~(MAP_HUGETLB | MAP_1G_HUGEPAGE), 0, 0);
	if (addr_local == MAP_FAILED) {
		// Give up
		return MAP_FAILED;
	}

	// Extra alignment space not needed at the end
	size_t unaligned_extra_end = (size_t) addr_local & (length_extra - 1);
	// Extra alignment space to skip at the beginning
	size_t unaligned_extra_begin = length_extra - unaligned_extra_end;

	// Drop the unaligned extra parts of the buffer
	munmap(addr_local, unaligned_extra_begin);

	// Aligned start of the buffer
	addr_local = addr_local + unaligned_extra_begin;

	if (unaligned_extra_end) {
		munmap(addr_local + length, unaligned_extra_end);
	}

	return addr_local;
}

/*
 * Allocate (mmap) new buffer
 */
static fpga_result buffer_allocate(void **addr, uint64_t len, int flags)
{
	void *addr_local = NULL;

	UNUSED_PARAM(flags);

	ASSERT_NOT_NULL(addr);

	/* ! FPGA_BUF_PREALLOCATED, allocate memory using huge pages
	   For buffer > 2M, use 1G-hugepage to ensure pages are
	   contiguous */
	if (len > 2 * MB)
		addr_local = sim_huge_mmap(ADDR, len, PROTECTION, FLAGS_1G);
	else if (len > 4 * KB)
		addr_local = sim_huge_mmap(ADDR, len, PROTECTION, FLAGS_2M);
	else
		addr_local = mmap(ADDR, len, PROTECTION, FLAGS_4K, 0, 0);
	if (addr_local == MAP_FAILED) {
		if (errno == ENOMEM) {
			if (len > 2 * MB)
				FPGA_MSG("Could not allocate buffer (no free 1 "
					 "GiB huge pages)");
			if (len > 4 * KB)
				FPGA_MSG("Could not allocate buffer (no free 2 "
					 "MiB huge pages)");
			else
				FPGA_MSG("Could not allocate buffer (out of "
					 "memory)");
			return FPGA_NO_MEMORY;
		}
		FPGA_MSG("FPGA buffer mmap failed: %s", strerror(errno));
		return FPGA_INVALID_PARAM;
	}

	*addr = addr_local;
	return FPGA_OK;
}

/*
 * Release (unmap) allocated buffer
 */
static fpga_result buffer_release(void *addr, uint64_t len)
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
		FPGA_MSG("FPGA buffer munmap failed: %s",
			 strerror(errno));
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}


#define MAPS_BUF_SZ 4096
/*
 * Confirm that a page is mapped at vaddr and that it is at least
 * req_page_bytes.
 */
static fpga_result check_mapped_page(void *vaddr, size_t req_page_bytes)
{
	char line[MAPS_BUF_SZ];
	uint64_t addr = (uint64_t)vaddr;

	FILE *f = fopen("/proc/self/smaps", "r");
	// Deallocate the file buffer. This hurts performance, but is important for
	// accuracy since the buffer will be allocated within the address space
	// being checked.
	setvbuf(f, (char *)NULL, _IONBF, 0);
	if (f == NULL) {
		return FPGA_EXCEPTION;
	}

	while (fgets(line, MAPS_BUF_SZ, f)) {
		unsigned long long start, end;
		char *tmp0;
		char *tmp1;

		// Range entries begin with <start addr>-<end addr>
		start = strtoll(line, &tmp0, 16);
		// Was a number found and is the next character a dash?
		if ((tmp0 == line) || (*tmp0 != '-')) {
			// No -- not a range
			continue;
		}

		end = strtoll(++tmp0, &tmp1, 16);
		// Keep search if not a number or the address isn't in range.
		if ((tmp0 == tmp1) || (start > addr) || (end <= addr)) {
			continue;
		}

		while (fgets(line, MAPS_BUF_SZ, f)) {
			// Look for KernelPageSize
			unsigned page_kb;
			int ret = sscanf_s_u(line, "KernelPageSize: %d kB", &page_kb);
			if (ret == 0)
				continue;

			fclose(f);

			if (ret < 1 || page_kb == 0) {
				return FPGA_EXCEPTION;
			}

			// Is the page large enough?
			if (1024 * page_kb < req_page_bytes) {
				return FPGA_EXCEPTION;
			}

			return FPGA_OK;
		}
	}

	// We couldn't find an entry for this addr in smaps.
	fclose(f);
	return FPGA_NOT_FOUND;
}


fpga_result __FPGA_API__ ase_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
					   void **buf_addr, uint64_t *wsid,
					   int flags)
{
	void *addr = NULL;
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *) handle;
	int err;

	bool preallocated = (flags & FPGA_BUF_PREALLOCATED);
	bool quiet = (flags & FPGA_BUF_QUIET);

	uint64_t pg_size;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	/* Assure wsid is a valid pointer */
	if (!wsid) {
		FPGA_MSG("WSID is NULL");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (flags & (~(FPGA_BUF_PREALLOCATED | FPGA_BUF_QUIET))) {
		FPGA_MSG("Unrecognized flags");
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
			FPGA_MSG("No preallocated buffer address given");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		if (!(*buf_addr)) {
			FPGA_MSG("Preallocated buffer address is NULL");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		/* check length */
		if (!len || (len & (pg_size - 1))) {
			FPGA_MSG("Preallocated buffer size is not a non-zero multiple of page size");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		/* Does the page exist? */
		if (FPGA_OK != check_mapped_page(*buf_addr, len)) {
			FPGA_MSG("Preallocated buffer does not exist or the page is too small");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		addr = *buf_addr;
	} else {

		if (!buf_addr) {
			FPGA_MSG("buffer address is NULL");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}

		if (!len) {
			FPGA_MSG("buffer length is zero");
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

	/* Simulated equivalent of pinning the page */
	uint64_t dma_map_iova;

	if (ase_host_memory_pin(addr, &dma_map_iova, len) != 0) {
		if (!preallocated) {
			buffer_release(addr, len);
		}

		if (!quiet) {
			FPGA_MSG("FPGA_PORT_DMA_MAP ioctl failed: %s",
				 strerror(errno));
		}

		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	/* Generate unique workspace ID */
	*wsid = wsid_gen();

	/* Add to workspace id in order to store buffer length */
	if (!wsid_add(_handle->wsid_root,
		      *wsid,
		      (uint64_t) addr,
		      dma_map_iova,
		      len,
		      0,
		      0,
		      flags)) {
		if (!preallocated) {
			buffer_release(addr, len);
		}

		FPGA_MSG("Failed to add workspace id %lu", *wsid);
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
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ ase_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid)
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
		FPGA_MSG("WSID not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	buf_addr = (void *) wm->addr;
	iova = wm->phys;
	len = wm->len;

	bool preallocated = (wm->flags & FPGA_BUF_PREALLOCATED);

	/* Simulated equivalent of unpinning the page */
	if (ase_host_memory_unpin(iova, len) != 0) {
		if (!preallocated) {
			buffer_release(buf_addr, len);
		}

		FPGA_MSG("FPGA_PORT_DMA_UNMAP ioctl failed: %s",
			 strerror(errno));
		result = FPGA_INVALID_PARAM;
		goto ws_free;
	}

	/* If the buffer was allocated in fpgaPrepareBuffer() (i.e. it was not
	 * preallocated), we need to unmap it here. Otherwise (if it was
	 * preallocated) the mapping needs to stay intact. */
	if (!preallocated) {
		result = buffer_release(buf_addr, len);
		if (result != FPGA_OK) {
			FPGA_MSG("Buffer release failed");
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
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

fpga_result __FPGA_API__ ase_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
					  uint64_t *ioaddr)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct wsid_map *wm;
	fpga_result result = FPGA_OK;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;
	if (!ioaddr)
		return FPGA_INVALID_PARAM;

	wm = wsid_find(_handle->wsid_root, wsid);
	if (!wm) {
		FPGA_MSG("WSID not found");
		result = FPGA_NOT_FOUND;
	} else {
		*ioaddr = wm->phys;
	}

	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}
