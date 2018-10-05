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
// **************************************************************************

//
// Manage simulated host memory.  This code runs on the application side.
// The simulator makes requests that are serviced inside the application,
// thus allowing the application to update share any page at any point
// in a run.
//

#include "ase_common.h"
#include "ase_host_memory.h"

//
// Translate to simulated physical address space.
//
uint64_t ase_host_memory_va_to_pa(void* va)
{
    // We use a simple transformation to generate a fake physical space, merely
    // inverting bits [47:30], which correspond to the first and second indices
    // in a usual x86_64 page table. The second index is the 1GB pages, so the
    // inversion guarantees we will have no physical address clashes with
    // page sizes up to 1GB. Inverting these bits forces AFUs to translate
    // addresses but avoids wasting simulator memory on complex page
    // mappings.

    // Confirm that the top virtual bits are 0.  They should be non-zero only
    // for kernel addresses.  This simulation is user space.
    if ((uint64_t) va >> 48) {
        ASE_ERR("Unexpected virtual address (0x" PRIx64 "), high bits set!", va);
		raise(SIGABRT);
    }

    uint64_t mask = (uint64_t) 0x3ffff << 30;
    return (uint64_t) va ^ mask;
}

//
// Translate from simulated physical address space.
//
void* ase_host_memory_pa_to_va(uint64_t pa)
{
    // The inverse of ase_host_memory_va_to_pa.
    if (pa >> 48) {
        ASE_ERR("Unexpected physical address (0x" PRIx64 "), high bits set!", pa);
		raise(SIGABRT);
    }

    uint64_t mask = (uint64_t) 0x3ffff << 30;
    return (void*) (pa ^ mask);
}



/*
 * ASE Physical address to virtual address converter
 * Takes in a simulated physical address from AFU, converts it
 *   to virtual address
 */
#ifdef FOOBAR
uint64_t *ase_fakeaddr_to_vaddr(uint64_t req_paddr)
{
	FUNC_CALL_ENTRY;

	// Traversal ptr
	struct buffer_t *trav_ptr = (struct buffer_t *) NULL;

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
				calc_pbase = trav_ptr->vbase;
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
	}


	// If accesses are correct, ASE should not reach this point
	ASE_ERR
	    ("@ERROR: ASE has detected a memory operation to an unallocated memory region.\n"
		 "          Simulation cannot continue, please check the code. \n"
	     "        Failure @ phys_addr = 0x%" PRIx64 ". See ERROR log file => ase_memory_error.log\n"
	     "@ERROR: Check that previously requested memories have not been deallocated before an AFU transaction could access them\n"
	     "        NOTE: If your application polls for an AFU completion message, and you deallocate after that, consider using \n"
		 "        a WriteFence before AFU status message\n"
	     "        The simulator may be committing AFU transactions out of order\n", req_paddr);

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

	return (uint64_t *) NOT_OK;

	FUNC_CALL_EXIT;
}
#endif
