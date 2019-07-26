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

#ifndef _ASE_HOST_MEMORY_H_
#define _ASE_HOST_MEMORY_H_

//
// Definitions for accessing host memory from the FPGA simulator.  Host
// memory address mapping and contents are managed entirely in the application.
// The RTL simulator sends requests to the application for all reads, writes
// and address translation.
//
// The primary reason for handling all memory in the application instead of
// by allocating shared memory buffers with the simulator is to be able
// to map and access an arbitrary location from the FPGA, even if the page
// is already mapped and in use by the application.  It is nearly impossible
// in Linux to reclassify an existing memory page as shared.  Handling all
// accesses on the application side makes shared mapping unnecessary.
//

//
// Requests types that may be sent from simulator to application.
// We define these as const uint64_t instead of enum for alignment
// when passing between 32 bit simulators and 64 bit applications.
//
#define HOST_MEM_REQ_READ_LINE 0
#define HOST_MEM_REQ_WRITE_LINE 1
typedef uint64_t ase_host_memory_req;

#define HOST_MEM_STATUS_VALID 0
// Reference to illegal address
#define HOST_MEM_STATUS_ILLEGAL 1
// Reference to address that isn't pinned for I/O
#define HOST_MEM_STATUS_NOT_PINNED 2
// Address is pinned but the page is unmapped. This state most likely
// happens when the program unmaps a page that is still pinned.
#define HOST_MEM_STATUS_NOT_MAPPED 3
typedef uint64_t ase_host_memory_status;

//
// Read request, simulator to application.
//
typedef struct {
	uint64_t addr;
	ase_host_memory_req req;
} ase_host_memory_read_req;

//
// Read response, application to simulator.
//
typedef struct {
	uint8_t data[CL_BYTE_WIDTH];

	// Simulated host physical address
	uint64_t pa;
	// Virtual address in application space.  We store this as
	// a uint64_t so the size is consistent even in 32 bit simulators.
	uint64_t va;

	// Does the response hold valid data?
	ase_host_memory_status status;
} ase_host_memory_read_rsp;

//
// Write request, simulator to application.
//
typedef struct {
	uint64_t addr;
	uint8_t data[CL_BYTE_WIDTH];
	ase_host_memory_req req;

	// Byte range
	uint8_t byte_en;         // Access limited to byte range within line when non-zero
	uint8_t byte_start;      // Index of first byte update
	uint8_t byte_len;        // Number of bytes to update, starting with byte_start

	uint32_t dummy;          // Force alignment to 64 bits
} ase_host_memory_write_req;

//
// Write response, application to simulator.
//
typedef struct {
	// Simulated host physical address
	uint64_t pa;
	// Virtual address in application space.  We store this as
	// a uint64_t so the size is consistent even in 32 bit simulators.
	uint64_t va;

	// Was the request to a valid address?
	ase_host_memory_status status;
} ase_host_memory_write_rsp;


#ifndef SIM_SIDE

// Pin a page at specified virtual address. Returns the corresponding
// I/O address (simulated host physical address).
int ase_host_memory_pin(void *va, uint64_t *iova, uint64_t length);
// Unpin the page at iova.
int ase_host_memory_unpin(uint64_t iova, uint64_t length);

// Translate to simulated physical address space.
uint64_t ase_host_memory_va_to_pa(uint64_t va, uint64_t length);

// Translate from simulated physical address space.  By setting "lock"
// in the request, the internal page table lock is not released on
// return. This allows a caller to be sure that a page will remain
// in the table long enough to access the data to which pa points.
// Callers using "lock" must call ase_host_memory_unlock() to
// release the page table lock and avoid deadlocks.
uint64_t ase_host_memory_pa_to_va(uint64_t pa, bool lock);
void ase_host_memory_unlock(void);

// Initialize/terminate page address translation.
int ase_host_memory_initialize(void);
void ase_host_memory_terminate(void);

#endif // not SIM_SIDE

#endif // _ASE_HOST_MEMORY_H_
