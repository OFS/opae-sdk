// Copyright(c) 2014-2017, Intel Corporation
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
// **************************************************************************
/*
 * Module Info: Memory Model operations (C module)
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * Purpose: Keeping cci_to_mem_translator.c clutter free and modular
 * test and debug. Includes message queue management by DPI.
 * NOTE: These functions must be called by DPI side ONLY.
 */

#include "ase_common.h"

// Base addresses of required regions
uint64_t *mmio_afu_vbase;
uint64_t *umsg_umas_vbase;

// ASE error file
static FILE *error_fp;

// System Memory
uint64_t sysmem_size;
uint64_t sysmem_phys_lo;
uint64_t sysmem_phys_hi;

/*
* Calculate Sysmem & CAPCM ranges to be used by ASE
*/
void calc_phys_memory_ranges(void)
{
	sysmem_size = cfg->phys_memory_available_gb * pow(1024, 3);
	sysmem_phys_lo = 0;
	sysmem_phys_hi = sysmem_size - 1;

	// Calculate address mask
	PHYS_ADDR_PREFIX_MASK =
	    ((sysmem_phys_hi >> MEMBUF_2MB_ALIGN) << MEMBUF_2MB_ALIGN);
#ifdef ASE_DEBUG
	ASE_DBG("PHYS_ADDR_PREFIX_MASK = 0x%" PRIx64 "\n",
		(uint64_t) PHYS_ADDR_PREFIX_MASK);
#endif

	ASE_MSG("        System memory range  => 0x%016" PRIx64 "-0x%016"
		PRIx64 " | %" PRId64 "~%" PRId64 " GB \n", sysmem_phys_lo,
		sysmem_phys_hi, sysmem_phys_lo / (uint64_t) pow(1024, 3),
		(uint64_t) (sysmem_phys_hi + 1) / (uint64_t) pow(1024, 3));
}

// ---------------------------------------------------------------
// ASE graceful shutdown - Called if: error() occurs
// Deallocate & Unlink all shared memories and message queues
// ---------------------------------------------------------------
void ase_perror_teardown(void)
{
	FUNC_CALL_ENTRY;

	self_destruct_in_progress = 1;

	ase_destroy();

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// DPI ALLOC buffer action - Allocate buffer action inside DPI
// Receive buffer_t pointer with memsize, memname and index populated
// Calculate fd, pbase and fake_paddr
// --------------------------------------------------------------------
void ase_alloc_action(struct buffer_t *mem)
{
	FUNC_CALL_ENTRY;

	struct buffer_t *new_buf;
	int fd_alloc;

	ASE_DBG("SIM-C : Adding a new buffer \"%s\"...\n", mem->memname);

	// Obtain a file descriptor
	fd_alloc = shm_open(mem->memname, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd_alloc < 0) {
		ase_error_report("shm_open", errno, ASE_OS_SHM_ERR);
		ase_perror_teardown();
		start_simkill_countdown();
	} else {
		// Add to IPC list
		add_to_ipc_list("SHM", mem->memname);

		// Mmap to pbase, find one with unique low 38 bit
		mem->pbase =
		    (uintptr_t) mmap(NULL, mem->memsize,
				     PROT_READ | PROT_WRITE, MAP_SHARED,
				     fd_alloc, 0);
		if (mem->pbase == 0) {
			ase_error_report("mmap", errno, ASE_OS_MEMMAP_ERR);
			ase_perror_teardown();
			start_simkill_countdown();
		}
		if (ftruncate(fd_alloc, (off_t) mem->memsize) != 0) {
			ase_error_report("ftruncate", errno,
					 ASE_OS_SHM_ERR);
			ASE_MSG("Running ftruncate to %d bytes\n",
				(off_t) mem->memsize);
		}
		close(fd_alloc);

		// Record fake address
		mem->fake_paddr = get_range_checked_physaddr(mem->memsize);
		mem->fake_paddr_hi =
		    mem->fake_paddr + (uint64_t) mem->memsize;

		// Received buffer is valid
		mem->valid = ASE_BUFFER_VALID;

		// Create a buffer and store the information
		new_buf = (struct buffer_t *) ase_malloc(BUFSIZE);
		ase_memcpy(new_buf, mem, BUFSIZE);

		// Append to linked list
		ll_append_buffer(new_buf);
#ifdef ASE_LL_VIEW
		ll_traverse_print();
#endif

		// Convert buffer_t to string
		mqueue_send(sim2app_alloc_tx, (char *) mem,
			    sizeof(struct buffer_t));

		// If memtest is enabled
#ifdef ASE_MEMTEST_ENABLE
		ase_dbg_memtest(mem);
#endif

		if (mem->is_mmiomap == 1) {
			// Pin CSR address
			mmio_afu_vbase =
			    (uint64_t *) (uintptr_t) (mem->pbase +
						      MMIO_AFU_OFFSET);
			ASE_DBG("Global CSR Base address = %p\n",
				(void *) mmio_afu_vbase);
		}
#ifdef ASE_DEBUG
		if (fp_pagetable_log != NULL) {
			if (mem->index % 20 == 0) {
				fprintf(fp_pagetable_log,
					"Index\tAppVBase\tASEVBase\tBufsize\tBufname\t\tPhysBase\n");
			}

			fprintf(fp_pagetable_log,
				"%d\t0x%" PRIx64 "\t0x%" PRIx64
				"\t%x\t%s\t\t0x%" PRIx64 "\n", mem->index,
				mem->vbase, mem->pbase, mem->memsize,
				mem->memname, mem->fake_paddr);
		}
#endif
	}

	FUNC_CALL_EXIT;
}


// --------------------------------------------------------------------
// DPI dealloc buffer action - Deallocate buffer action inside DPI
// Receive index and invalidates buffer
// --------------------------------------------------------------------
void ase_dealloc_action(struct buffer_t *buf, int mq_enable)
{
	FUNC_CALL_ENTRY;

	char buf_str[ASE_MQ_MSGSIZE];
	memset(buf_str, 0, ASE_MQ_MSGSIZE);

	// Traversal pointer
	struct buffer_t *dealloc_ptr;
	// dealloc_ptr = (struct buffer_t *) ase_malloc(sizeof(struct buffer_t));

	// Search buffer and Invalidate
	dealloc_ptr = ll_search_buffer(buf->index);

	//  If deallocate returns a NULL, dont get hosed
	if (dealloc_ptr == NULL) {
		ASE_INFO_2
		    ("NULL deallocation request received ... ignoring.\n");
	} else {
		ASE_INFO_2("Request to deallocate \"%s\" ...\n",
			   dealloc_ptr->memname);
		// Mark buffer as invalid & deallocate
		dealloc_ptr->valid = ASE_BUFFER_INVALID;
		munmap((void *) (uintptr_t) dealloc_ptr->pbase,
		       (size_t) dealloc_ptr->memsize);
		shm_unlink(dealloc_ptr->memname);
		// Respond back
		ll_remove_buffer(dealloc_ptr);
		ase_memcpy(buf_str, dealloc_ptr, sizeof(struct buffer_t));
		// If Buffer removal is requested by APP, send back notice, else no response
		if (mq_enable == 1) {
			mqueue_send(sim2app_dealloc_tx, buf_str,
				    ASE_MQ_MSGSIZE);
		}
#ifdef ASE_LL_VIEW
		ll_traverse_print();
#endif
	}

	// Free dealloc_ptr
	// free(dealloc_ptr);

	FUNC_CALL_EXIT;
}

// --------------------------------------------------------------------
// ase_empty_buffer: create an empty buffer_t object
// Create a buffer with all parameters set to 0
// --------------------------------------------------------------------
void ase_empty_buffer(struct buffer_t *buf)
{
	buf->index = 0;
	buf->valid = ASE_BUFFER_INVALID;
	memset(buf->memname, 0, ASE_FILENAME_LEN);
	buf->memsize = 0;
	buf->vbase = 0;
	buf->pbase = 0;
	buf->fake_paddr = 0;
	buf->next = NULL;
}


// --------------------------------------------------------------------
// ase_destroy : Destroy everything, called before exiting OR to
// reset simulation environment
//
// OPERATION:
// Traverse trough linked list
// - Remove each shared memory region
// - Remove each buffer_t
// --------------------------------------------------------------------
void ase_destroy(void)
{
	FUNC_CALL_ENTRY;

#ifdef ASE_DEBUG
	char str[256];
	snprintf(str, 256, "ASE destroy called");
	buffer_msg_inject(1, str);
#endif

	struct buffer_t *ptr;

	ptr = head;
	if (head != NULL) {
		while (ptr != (struct buffer_t *) NULL) {
			ase_dealloc_action(ptr, 0);
			ptr = ptr->next;
		}
	}

	FUNC_CALL_EXIT;
}


/*
 * Range check a Physical address to check if used
 * Created to integrate Sysmem & CAPCM and prevent corner case overwrite
 *   issues (Mon Oct 13 13:33:59 PDT 2014)
 * Operation: When allocating a fake physical address, this function
 * will return an unused physical address range
 * This will be used by SW allocate buffer funtion ONLY
 */
uint64_t get_range_checked_physaddr(uint32_t size)
{
	int unique_physaddr_needed = 1;
	uint64_t ret_fake_paddr;
	uint32_t search_flag;
	uint32_t opposite_flag;
	uint32_t zero_pbase_flag;
#ifdef ASE_DEBUG
	int tries = 0;
#endif

	// Generate a new address
	while (unique_physaddr_needed) {
		// Generate a random physical address for system memory
		ret_fake_paddr =
		    sysmem_phys_lo + ase_rand64() % sysmem_size;
		// 2MB align and sanitize
		// ret_fake_paddr = ret_fake_paddr & 0x00003FFFE00000 ;
		ret_fake_paddr = ret_fake_paddr & PHYS_ADDR_PREFIX_MASK;

		// Check for conditions
		// Is address in sysmem range, go back
		search_flag = check_if_physaddr_used(ret_fake_paddr);

		// Is HI smaller than LO, go back
		opposite_flag = 0;
		if ((ret_fake_paddr + (uint64_t) size) < ret_fake_paddr)
			opposite_flag = 1;

		// Zero base flag
		zero_pbase_flag = 0;
		if (ret_fake_paddr == 0)
			zero_pbase_flag = 1;

		// If all OK
		unique_physaddr_needed =
		    search_flag | opposite_flag | zero_pbase_flag;
#ifdef ASE_DEBUG
		tries++;
#endif
	}

#ifdef ASE_DEBUG
	if (fp_memaccess_log != NULL) {
		fprintf(fp_memaccess_log,
			"  [DEBUG]  ASE took %d tries to generate phyaddr\n",
			tries);
	}
#endif

	return ret_fake_paddr;
}


/*
 * ASE Physical address to virtual address converter
 * Takes in a simulated physical address from AFU, converts it
 *   to virtual address
 */
uint64_t *ase_fakeaddr_to_vaddr(uint64_t req_paddr)
{
	FUNC_CALL_ENTRY;

	// Traversal ptr
	struct buffer_t *trav_ptr = (struct buffer_t *) NULL;
	// int buffer_found = 0;

	if (req_paddr != 0) {
		// Clean up address of signed-ness (limit to CCI-P 42 bits)
		req_paddr = req_paddr & (((uint64_t) 1 << 42) - 1);

		// DPI pbase address
		uint64_t *ase_pbase;

		// This is the real offset to perform read/write
		uint64_t real_offset, calc_pbase;

		// For debug only
#ifdef ASE_DEBUG
		if (fp_memaccess_log != NULL) {
			fprintf(fp_memaccess_log,
				"req_paddr = 0x%" PRIx64 " | ", req_paddr);
		}
#endif

		// Search which buffer offset_from_pin lies in
		trav_ptr = head;
		while (trav_ptr != NULL) {
			if ((req_paddr >= trav_ptr->fake_paddr)
			    && (req_paddr < trav_ptr->fake_paddr_hi)) {
				real_offset =
				    (uint64_t) req_paddr -
				    (uint64_t) trav_ptr->fake_paddr;
				calc_pbase = trav_ptr->pbase;
				ase_pbase =
				    (uint64_t *) (uintptr_t) (calc_pbase +
							      real_offset);

				// Debug only
#ifdef ASE_DEBUG
				if (fp_memaccess_log != NULL) {
					fprintf(fp_memaccess_log,
						"offset=0x%016" PRIx64
						" | pbase=%p\n",
						real_offset, ase_pbase);
				}
#endif
				return ase_pbase;
			} else {
				trav_ptr = trav_ptr->next;
			}
		}
	} else {
		// buffer_found = 0;
		trav_ptr = NULL;
	}

	// If accesses are correct, ASE should not reach this point
	if (trav_ptr == NULL) {
		ASE_ERR
		    ("@ERROR: ASE has detected a memory operation to an unallocated memory region.\n");
		ASE_ERR
		    ("        Simulation cannot continue, please check the code.\n");
		ASE_ERR("        Failure @ phys_addr = 0x%" PRIx64 "\n",
			req_paddr);
		ASE_ERR
		    ("        See ERROR log file => ase_memory_error.log\n");
		ASE_ERR
		    ("@ERROR: Check that previously requested memories have not been deallocated before an AFU transaction could access them\n");
		ASE_ERR
		    ("        NOTE: If your application polls for an AFU completion message, and you deallocate after that, consider using a WriteFence before AFU status message\n");
		ASE_ERR
		    ("              The simulator may be committing AFU transactions out of order\n");

		// Write error to file
		error_fp = (FILE *) NULL;
		error_fp = fopen("ase_memory_error.log", "w");
		if (error_fp != NULL) {
			fprintf(error_fp,
				"*** ASE stopped on an illegal memory access ERROR ***\n"
				"        AFU requested access @ physical memory 0x%"
				PRIx64 "\n"
				"        Address not found in requested workspaces\n"
				"        Timestamped transaction to this address is listed in ccip_transactions.tsv\n"
				"        Check that previously requested memories have not been deallocated before an AFU transaction could access them"
				"        NOTE: If your application polls for an AFU completion message, and you deallocate after that, consider using a WriteFence before AFU status message\n"
				"              The simulator may be committing AFU transactions out of order\n",
				req_paddr);

			fclose(error_fp);
		}
		// Request SIMKILL
		start_simkill_countdown();
	}

	return (uint64_t *) NOT_OK;

	FUNC_CALL_EXIT;
}
