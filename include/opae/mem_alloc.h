// Copyright(c) 2020, Intel Corporation
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
#ifndef __OPAE_MEM_ALLOC_H__
#define __OPAE_MEM_ALLOC_H__

/**
* Provides an API for allocating/freeing a logical address space. There
* is no interaction with any OS memory allocation infrastructure, whether
* malloc, mmap, etc. The "address ranges" tracked by this allocator are
* arbitrary 64-bit integers. The allocator simply provides the bookeeping
* logic that ensures that a unique address with the appropriate size is
* returned for each allocation request, and that an allocation can be freed,
* ie released back to the available pool of logical address space for future
* allocations. The memory backing the allocator's internal data structures
* is managed by malloc()/free().
*/

#include <stdint.h>

struct mem_link {
	uint64_t address;
	uint64_t size;
	struct mem_link *prev;
	struct mem_link *next;
};

struct mem_alloc {
	struct mem_link free;
	struct mem_link allocated;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Initialize a memory allocator object
 *
 * After the call, the allocator is initialized but "empty". To add
 * allocatable memory regions, further initialize the allocator with
 * mem_alloc_add_free().
 *
 * @param[out] m The address of the memory allocator to initialize.
 */
void mem_alloc_init(struct mem_alloc *m);

/**
 * Destroy a memory allocator object
 *
 * Frees all of the allocator's internal resources.
 *
 * @param[in] m The address of the memory allocator to destroy.
 */
void mem_alloc_destroy(struct mem_alloc *m);

/**
 * Add a memory region to an allocator.
 *
 * The memory region is added to the allocatable space and is
 * immediately ready for allocation.
 *
 * @param[in, out] m       The memory allocator object.
 * @param[in]      address The beginning address of the memory region.
 * @param[in]      size    The size of the memory region.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.c}
 * struct mem_alloc m;
 *
 * mem_alloc_init(&m);
 *
 * if (mem_alloc_add_free(&m, 0x4000, 4096)) {
 *   // handle error
 * }
 * @endcode
 */
int mem_alloc_add_free(struct mem_alloc *m,
		       uint64_t address,
		       uint64_t size);

/** Allocate memory
 *
 * Retrieve an available memory address for a free block
 * that is at least size bytes.
 *
 * @param[in, out] m       The memory allocator object.
 * @param[out]     address The retrieved address for the allocation.
 * @param[in]      size    The request size in bytes.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.c}
 * struct mem_alloc m;
 * uint64_t addr = 0;
 *
 * mem_alloc_init(&m);
 *
 * if (mem_alloc_add_free(&m, 0x4000, 4096)) {
 *   // handle error
 * }
 *
 * ...
 *
 * if (mem_alloc_get(&m, &addr, 4096)) {
 *   // handle allocation error
 * }
 * @endcode
 */
int mem_alloc_get(struct mem_alloc *m,
		  uint64_t *address,
		  uint64_t size);

/** Free memory
 *
 * Release a previously-allocated memory block.
 *
 * @param[in, out] m       The memory allocator object.
 * @param[in]      address The address to free.
 * @returns Non-zero on error. Zero on success.
 *
 * Example
 * @code{.c}
 * struct mem_alloc m;
 * uint64_t addr = 0;
 *
 * mem_alloc_init(&m);
 *
 * if (mem_alloc_add_free(&m, 0x4000, 4096)) {
 *   // handle error
 * }
 *
 * ...
 *
 * if (mem_alloc_get(&m, &addr, 4096)) {
 *   // handle allocation error
 * }
 *
 * ...
 *
 * if (mem_alloc_put(&m, addr)) {
 *   // handle free error
 * }
 * @endcode
 */
int mem_alloc_put(struct mem_alloc *m,
		  uint64_t address);

/** Apply free list constraints from a second allocator.
 *
 * Apply the memory region constraints from the free
 * list of m_constr to the m allocator object. After the
 * call, all of allocator m's free address ranges are
 * guaranteed to be within free ranges also found in m_constr.
 *
 * @param[in, out] m        The memory allocator object.
 * @param[in]      m_constr A second allocator with new constraints.
 * @returns Non-zero on error. Zero on success.
 */
int mem_alloc_apply_constraint(struct mem_alloc *m,
			       struct mem_alloc *m_constr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_MEM_ALLOC_H__
