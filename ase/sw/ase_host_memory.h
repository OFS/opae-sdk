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
//
typedef enum {
    HOST_MEM_REQ_READ_LINE,
    HOST_MEM_REQ_WRITE_LINE
} ase_host_memory_req;

//
// Read request, simulator to application.
//
typedef struct {
    ase_host_memory_req req;
    uint64_t addr;
} ase_host_memory_read_req;

//
// Read response, application to simulator.
//
typedef struct {
    uint8_t data[CL_BYTE_WIDTH];

    // Simulated host physical address
    uint64_t pa;
    // Virtual address in application space
    void *va;

    // Does the response hold valid data? Was the request to a valid address?
    bool valid;
} ase_host_memory_read_rsp;

//
// Write request, simulator to application.
//
typedef struct {
    ase_host_memory_req req;
    uint64_t addr;
    uint8_t data[CL_BYTE_WIDTH];
} ase_host_memory_write_req;

//
// Write response, application to simulator.
//
typedef struct {
    // Simulated host physical address
    uint64_t pa;
    // Virtual address in application space
    void *va;

    // Was the request to a valid address?
    bool valid;
} ase_host_memory_write_rsp;


#ifndef SIM_SIDE

//
// Translate to/from simulated physical address space.
//
uint64_t ase_host_memory_va_to_pa(void* va);
void* ase_host_memory_pa_to_va(uint64_t pa);

#endif // not SIM_SIDE

#endif // _ASE_HOST_MEMORY_H_
