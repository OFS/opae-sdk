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

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "ase_common.h"
#include "ase_host_memory.h"

// Pin a page at specified virtual address.  Returns the corresponding
// I/O address (simulated host physical address).
int ase_host_memory_pin(void *va, uint64_t *iova, uint64_t length)
{
    *iova = ase_host_memory_va_to_pa(va);
    note_pinned_page(va, *iova, length);

    return 0;
}


// Unpin the page at iova.
int ase_host_memory_unpin(uint64_t iova, uint64_t length)
{
    note_unpinned_page(iova, length);

    return 0;
}


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
