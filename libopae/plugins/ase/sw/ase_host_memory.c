// Copyright(c) 2018, Intel Corporation
//
// Redistribution  and	use	 in source	and	 binary	 forms,	 with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of	 source code  must retain the  above copyright notice,
//	 this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//	 this list of conditions and the following disclaimer in the documentation
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
// **************************************************************************

//
// Manage simulated host memory.  This code runs on the application side.
// The simulator makes requests that are serviced inside the application,
// thus allowing the application to update share any page at any point
// in a run.
//
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>

#include "ase_common.h"
#include "ase_host_memory.h"

#define KB 1024
#define MB (1024 * KB)
#define GB (1024UL * MB)

static pthread_mutex_t ase_pt_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t **ase_pt_root;
static bool ase_pt_enable_debug;

STATIC int ase_pt_length_to_level(uint64_t length);
static uint64_t ase_pt_level_to_bit_idx(int pt_level);
static void ase_pt_delete_tree(uint64_t **pt, int pt_level);
static bool ase_pt_check_addr(uint64_t iova, int *pt_level);
static int ase_pt_pin_page(uint64_t va, uint64_t *iova, int pt_level);
static int ase_pt_unpin_page(uint64_t iova, int pt_level);


/*
 * Pin a page at specified virtual address. Returns the corresponding
 * I/O address (simulated host physical address).
 */
int ase_host_memory_pin(void *va, uint64_t *iova, uint64_t length)
{
	if (pthread_mutex_lock(&ase_pt_lock)) {
		ASE_ERR("pthread_mutex_lock could not attain the lock !\n");
		return -1;
	}

	// Map buffer length to a level in the page table.
	int pt_level = ase_pt_length_to_level(length);
	if (pt_level == -1)
		return -1;

	int status = ase_pt_pin_page((uint64_t)va, iova, pt_level);
	if (status == 0) {
		note_pinned_page((uint64_t)va, *iova, length);
	}

	if (pthread_mutex_unlock(&ase_pt_lock)) {
		ASE_ERR("pthread_mutex_lock could not unlock !\n");
		status = -1;
	}

	return status;
}


/*
 * Unpin the page at iova.
 */
int ase_host_memory_unpin(uint64_t iova, uint64_t length)
{
	int status = 0;

	if (pthread_mutex_lock(&ase_pt_lock)) {
		ASE_ERR("pthread_mutex_lock could not attain lock !\n");
		return -1;
	}

	if (ase_pt_root != NULL) {
		int pt_level = ase_pt_length_to_level(length);
		if (pt_level == -1)
			return -1;

		status = ase_pt_unpin_page(iova, pt_level);
	}

	if (pthread_mutex_unlock(&ase_pt_lock)) {
		ASE_ERR("pthread_mutex_lock could not attain lock !\n");
		status = -1;
	}

	note_unpinned_page(iova, length);
	return status;
}


/*
 * Generate an XOR mask that will be used to map between virtual and physical
 * addresses.
 */
static uint64_t ase_host_memory_gen_xor_mask(int pt_level)
{
	// CCI-P (and our processors) have 48 bit byte-level addresses.
	// The mask here inverts all but the high 48th bit. We could legally
	// invert that bit too, except that it causes problems when simulating
	// old architectures such as the Broadwell integrated Xeon+FPGA, which
	// use smaller physical address ranges.
	return 0x7fffffffffffUL & (~0UL << ase_pt_level_to_bit_idx(pt_level));
}

/*
 * Translate to simulated physical address space.
 */
uint64_t ase_host_memory_va_to_pa(uint64_t va, uint64_t length)
{
	// We use a simple transformation to generate a fake physical space, merely
	// inverting bits of the page address. Inverting these bits forces AFUs to
	// translate addresses but avoids wasting simulator memory on complex page
	// mappings.

	// Confirm that the top virtual bits are 0. They should be non-zero only
	// for kernel addresses.  This simulation is user space.
	if (va >> 48) {
		ASE_ERR("Unexpected virtual address (0x" PRIx64 "), high bits set!", va);
		raise(SIGABRT);
	}

	int pt_level = ase_pt_length_to_level(length);
	return va ^ ase_host_memory_gen_xor_mask(pt_level);
}

/*
 * Translate from simulated physical address space. Optionally hold the lock
 * after translation so that the buffer remains pinned. Callers that set
 * "lock" must call ase_host_memory_unlock() or subsequent calls to
 * ase_host_memory functions will deadlock.
 */
uint64_t ase_host_memory_pa_to_va(uint64_t pa, bool lock)
{
	// The inverse of ase_host_memory_va_to_pa.
	if (pa >> 48) {
		ASE_ERR("Unexpected physical address (0x" PRIx64 "), high bits set!", pa);
		raise(SIGABRT);
	}

	if (pthread_mutex_lock(&ase_pt_lock)) {
		ASE_ERR("pthread_mutex_lock could not attain lock !\n");
		return 0;
	}

	// Is the page pinned?
	int pt_level;
	bool found_addr;
	found_addr = ase_pt_check_addr(pa, &pt_level);

	if (!lock) {
		if (pthread_mutex_unlock(&ase_pt_lock)) {
			ASE_ERR("pthread_mutex_lock could not unlock !\n");
			return 0;
		}
	}

	if (!found_addr) {
		return 0;
	}

	return pa ^ ase_host_memory_gen_xor_mask(pt_level);
}


void ase_host_memory_unlock(void)
{
	if (pthread_mutex_unlock(&ase_pt_lock))
		ASE_ERR("pthread_mutex_lock could not unlock !\n");
}


int ase_host_memory_initialize(void)
{
	// Turn on debugging messages when the environment variable ASE_PT_DBG
	// is defined.
	ase_pt_enable_debug = ase_checkenv("ASE_PT_DBG");

	return 0;
}


void ase_host_memory_terminate(void)
{
	if (pthread_mutex_lock(&ase_pt_lock))
		ASE_ERR("pthread_mutex_lock could not attain the lock !\n");

	ase_pt_delete_tree(ase_pt_root, 3);
	ase_pt_root = NULL;

	if (pthread_mutex_unlock(&ase_pt_lock))
		ASE_ERR("pthread_mutex_lock could not unlock !\n");
}


// ========================================================================
//
//	Maintain a simulated page table in order to track pinned pages.
//
// ========================================================================

/*
 * The page table here has a single job: indicate whether a given physical
 * address (iova) is pinned. Since the simulator uses a simple XOR function
 * for mapping virtual to physical addresses, the table is not needed for
 * actual translation.
 *
 * The table is physically indexed but the internal pointers are simple
 * virtual addresses. At the lowest level (the 4KB pages), the table uses
 * a 512 entry bit vector to indicate whether a page is pinned. This saves
 * space and is, once again, possible since the table isn't used for
 * actual translation.
 *
 * A pointer value of -1 at higher levels in the table indicates the
 * presence of a pinned huge page.
 */

/*
 * Page size to level in the table. Level 3 is the root, though we never
 * return 3 since the hardware won't allocated 512GB huge pages.
 */
STATIC int ase_pt_length_to_level(uint64_t length)
{
	int pt_level;

	if (length > 2 * MB)
		pt_level = (length == GB) ? 2 : -1;
	else if (length > 4 * KB)
		pt_level = (length <= 2 * MB) ? 1 : -1;
	else
		pt_level = 0;

	return pt_level;
}

/*
 * Return the bit index of the low bit of an address corresponding to
 * pt_level. All address bits lower than the returned index will be
 * offsets into the page.
 */
static inline uint64_t ase_pt_level_to_bit_idx(int pt_level)
{
	// Level 0 is 4KB pages, so 12 bits.
	uint64_t idx = 12;

	// Each level up adds 9 bits, corresponding to 512 entries in each
	// page table level in the tree.
	idx += pt_level * 9;

	return idx;
}

/*
 * Index of a 512 entry set of page pointers in the table, given a level.
 */
static inline int ase_pt_idx(uint64_t iova, int pt_level)
{
	// The low 12 bits are the 4KB page offset. Groups of 9 bits above that
	// correspond to increasing levels in the page table hierarchy.
	assert(pt_level <= 3);
	return 0x1ff & (iova >> ase_pt_level_to_bit_idx(pt_level));
}

/*
 * Dump the page table for debugging.
 */
static void ase_pt_dump(uint64_t **pt, uint64_t iova, int pt_level)
{
	if (pt == NULL)
		return;

	if (pt == (uint64_t **) -1) {
		printf("  0x%016lx	  %ld\n", iova, 4096 * (1UL << (9 * (pt_level + 1))));
		return;
	}

	int idx;
	for (idx = 0; idx < 512; idx++) {
		if (pt_level > 0) {
			ase_pt_dump((uint64_t **)pt[idx],
				    iova | ((uint64_t)idx << ase_pt_level_to_bit_idx(pt_level)),
				    pt_level - 1);
		} else {
			if ((uint64_t)pt[idx / 64] & (1UL << (idx & 63))) {
				printf("  0x%016lx	  4096\n", iova | (idx << 12));
			}
		}
	}
}


/*
 * Delete a sub-tree in the table.
 */
static void ase_pt_delete_tree(uint64_t **pt, int pt_level)
{
	if ((pt == NULL) || (pt == (uint64_t **) -1))
		return;

	if (pt_level) {
		// Drop sub-trees and then release this node.
		int idx;
		for (idx = 0; idx < 512; idx++) {
			ase_pt_delete_tree((uint64_t **)pt[idx], pt_level - 1);
		}
		munmap(pt, 4096);
	} else {
		// Terminal node of flags for 4KB pages.
		free(pt);
	}
}


/*
 * Is iova in the table? The level at which it is found is stored
 * in *pt_level.
 */
static bool ase_pt_check_addr(uint64_t iova, int *pt_level)
{
	*pt_level = -1;

	int level = 3;
	uint64_t **pt = ase_pt_root;

	while (level > 0) {
		if (pt == NULL) {
			// Not found
			return false;
		}

		// Walk down to the next level. Unlike a normal page table, addresses
		// here are simple virtual pointers. We can do this since the table
		// isn't actually translating -- it is simply indicating whether a
		// physical address is pinned.
		pt = (uint64_t **) pt[ase_pt_idx(iova, level)];
		if (pt == (uint64_t **) -1) {
			// -1 indicates a valid huge page mapping at this level.
			*pt_level = level;
			return true;
		}

		level -= 1;
	}

	// The last level mapping is a simple 512 entry bit vector -- not a set
	// of pointers. We do this to save space since the table only has to
	// indicate whether a page is valid.
	int idx = ase_pt_idx(iova, 0);
	if (pt && ((uint64_t)pt[idx / 64] & (1UL << (idx & 63)))) {
		*pt_level = 0;
		return true;
	}

	// Not found
	if (ase_pt_enable_debug) {
		printf("\nASE simulated page table (IOVA 0x%" PRIx64 " not found):\n", iova);
		ase_pt_dump(ase_pt_root, 0, 3);
		printf("\n");
	}

	return false;
}

static int ase_pt_pin_page(uint64_t va, uint64_t *iova, int pt_level)
{
	assert((pt_level >= 0) && (pt_level < 3));

	// Virtual to physical mapping is just an XOR
	uint64_t length = 4096 * (1UL << (9 * pt_level));
	*iova = ase_host_memory_va_to_pa(va, length);

	ASE_MSG("Add pinned page VA 0x%" PRIx64 ", IOVA 0x%" PRIx64 ", level %d\n", va, *iova, pt_level);

	int idx;
	int level = 3;
	uint64_t **pt = ase_pt_root;

	// Does the translation table need a page of pointers for this portion of
	// the tree?
	if (pt == NULL) {
		ase_pt_root = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
						   MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		pt = ase_pt_root;
		if (ase_pt_root == MAP_FAILED) {
			ASE_ERR("Simulated page table out of memory!\n");
			return -1;
		}
		ase_memset(pt, 0, 4096);
	}

	while (level != pt_level) {
		idx = ase_pt_idx(*iova, level);
		if (pt[idx] == NULL) {
			if (level > 1) {
				pt[idx] = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
							   MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			} else {
				pt[idx] = ase_malloc(64);
			}
			if ((pt[idx] == NULL) || (pt[idx] == MAP_FAILED)) {
				ASE_ERR("Simulated page table out of memory!\n");
				pt[idx] = NULL;
				return -1;
			}

			ase_memset(pt[idx], 0, (level > 1) ? 4096 : 64);
		}

		if (pt == (uint64_t **) -1) {
			ASE_ERR("Attempt to map smaller page inside existing huge page\n");
			return -1;
		}

		pt = (uint64_t **) pt[idx];
		level -= 1;
	}

	idx = ase_pt_idx(*iova, level);
	if (level) {
		if (pt[idx] != NULL) {
			// The page is already pinned. What should we do? mmap() allows overwriting
			// existing mappings, so we behave like it for now.
			ase_pt_delete_tree((uint64_t **)pt[idx], level - 1);
		}

		pt[idx] = (uint64_t *) -1;
	} else {
		pt[idx / 64] = (uint64_t *)((uint64_t)pt[idx / 64] | (1UL << (idx & 63)));
	}

	// Lock the "pinned" page so it more closely resembles the behavior of
	// pages pinned by the FPGA driver. This is valuable for code that is
	// tracking address map changes, such as the MPF building block.
	//
	// The result can be ignored since ASE will continue to work whether
	// or not the lock succeeds.
	mlock((void *)va, length);

	if (ase_pt_enable_debug) {
		printf("\nASE simulated page table (pinned VA 0x%" PRIx64 ", IOVA 0x%" PRIx64 "):\n",
		       va, *iova);
		ase_pt_dump(ase_pt_root, 0, 3);
		printf("\n");
	}

	return 0;
}

static int ase_pt_unpin_page(uint64_t iova, int pt_level)
{
	assert((pt_level >= 0) && (pt_level < 3));

	ASE_MSG("Remove pinned page IOVA 0x%" PRIx64 ", level %d\n", iova, pt_level);

	int idx;
	int level = 3;
	uint64_t **pt = ase_pt_root;
	uint64_t length = 4096 * (1UL << (9 * pt_level));

	uint64_t va = iova ^ ase_host_memory_gen_xor_mask(pt_level);
	if (va) {
		munlock((void *)va, length);
	}

	while (level != pt_level) {
		idx = ase_pt_idx(iova, level);
		if (pt[idx] == NULL) {
			ASE_ERR("Attempt to unpin non-existent page.\n");
			return -1;
		}

		if (pt == (uint64_t **) -1) {
			ASE_ERR("Attempt to unpin smaller page inside existing huge page\n");
			return -1;
		}

		pt = (uint64_t **) pt[idx];
		level -= 1;
	}

	idx = ase_pt_idx(iova, level);
	if (level) {
		// Drop a huge page
		if (pt[idx] != (uint64_t *) -1) {
			ASE_ERR("Attempt to unpin non-existent page.\n");
			return -1;
		}
		pt[idx] = NULL;
	} else if (pt) {
		// Drop a 4KB page
		pt[idx / 64] = (uint64_t *)((uint64_t)pt[idx / 64] & ~(1UL << (idx & 63)));
	}

	if (ase_pt_enable_debug) {
		printf("\nASE simulated page table (unpinned IOVA 0x%" PRIx64 "):\n", iova);
		ase_pt_dump(ase_pt_root, 0, 3);
		printf("\n");
	}

	return 0;
}
