// Copyright(c) 2018, Intel Corporation
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

/**
 * \local_memcpy.c
 * \brief FPGA DMA User-mode driver
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "fpga_dma_internal.h"

/**
* local_memcpy
*
* @brief                memcpy using SSE2 or REP MOVSB
* @param[in] dst        Pointer to the destination memory
* @param[in] src        Pointer to the source memory
* @param[in] n          Size in bytes
* @return dst
*
*/
void *local_memcpy(void *dst, void *src, size_t n)
{
#ifdef USE_MEMCPY
	return memcpy(dst, src, n);
#else
	void *ldst = dst;
	void *lsrc = (void *)src;
	if (IS_CL_ALIGNED(src) && IS_CL_ALIGNED(dst))	// 64-byte aligned
	{
		if (n >= MIN_SSE2_SIZE)	// Arbitrary crossover performance point
		{
			debug_print("copying 0x%lx bytes with SSE2\n",
				    (uint64_t) ALIGN_TO_CL(n));
			aligned_block_copy_sse2((int64_t * __restrict) dst,
						(int64_t * __restrict) src,
						ALIGN_TO_CL(n));
			ldst = (void *)((uint64_t) dst + ALIGN_TO_CL(n));
			lsrc = (void *)((uint64_t) src + ALIGN_TO_CL(n));
			n -= ALIGN_TO_CL(n);
		}
	} else {
		if (n >= MIN_SSE2_SIZE)	// Arbitrary crossover performance point
		{
			debug_print
			    ("copying 0x%lx bytes (unaligned) with SSE2\n",
			     (uint64_t) ALIGN_TO_CL(n));
			unaligned_block_copy_sse2((int64_t * __restrict) dst,
						  (int64_t * __restrict) src,
						  ALIGN_TO_CL(n));
			ldst = (void *)((uint64_t) dst + ALIGN_TO_CL(n));
			lsrc = (void *)((uint64_t) src + ALIGN_TO_CL(n));
			n -= ALIGN_TO_CL(n);
		}
	}

	if (n) {
		register unsigned long int dummy;
		debug_print("copying 0x%lx bytes with REP MOVSB\n", n);
		__asm__ __volatile__("rep movsb\n":"=&D"(ldst), "=&S"(lsrc),
				     "=&c"(dummy)
				     :"0"(ldst), "1"(lsrc), "2"(n)
				     :"memory");
	}

	return dst;
#endif
}
