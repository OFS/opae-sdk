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

#include <opae/log.h>
#include "mmap_int.h"
#include "common_int.h"

void *alloc_buffer(uint64_t len)
{
	void *addr = NULL;

	/* For buffer > 2M, use 1G-hugepage to ensure pages are continuous */
	if (len > 2 * MB)
		addr = mmap(ADDR, len, PROTECTION, FLAGS_1G, 0, 0);
	else if (len > 4 * KB)
		addr = mmap(ADDR, len, PROTECTION, FLAGS_2M, 0, 0);
	else
		addr = mmap(ADDR, len, PROTECTION, FLAGS_4K, 0, 0);
	if (addr == MAP_FAILED) {
		OPAE_ERR("mmap failed");
		addr = NULL;
	}

	return addr;
}

int free_buffer(void *addr, uint64_t len)
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
		OPAE_ERR("munmap failed");
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

