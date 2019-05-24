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

#include <opae/access.h>
#include <opae/utils.h>
#include "common_int.h"
#include "ase_common.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>


// Get number of Umsgs
fpga_result __FPGA_API__ ase_fpgaGetNumUmsg(fpga_handle handle, uint64_t *value)
{
    fpga_result result = FPGA_OK;
	UNUSED_PARAM(handle);

    if (ase_capability.umsg_feature != 0) {
	// Get Umsg Number
	*value = NUM_UMSG_PER_AFU;
	result = FPGA_OK;
    } else {
	result = FPGA_NOT_SUPPORTED;
    }

    return result;
}

// Set Umsg Attributes
fpga_result __FPGA_API__ ase_fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value)
{
    fpga_result result;
	UNUSED_PARAM(handle);

    if (ase_capability.umsg_feature != 0) {
	// Send UMSG setup (call ASE)
	umsg_set_attribute(value);
	result = FPGA_OK;
    } else {
	result = FPGA_NOT_SUPPORTED;
    }

    return result;
}

// Gets Umsg address
fpga_result __FPGA_API__ ase_fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr)
{
    fpga_result result = FPGA_OK;
	UNUSED_PARAM(handle);

    if (ase_capability.umsg_feature != 0) {
	*umsg_ptr = umsg_umas_vbase;
	if (*umsg_ptr == NULL) {
	    result = FPGA_NO_MEMORY;
	} else {
	    result = FPGA_OK;
	}
    } else {
	result = FPGA_NOT_SUPPORTED;
    }

    return result;
}


// Trigger umsg
fpga_result ase_fpgaTriggerUmsg(fpga_handle handle, uint64_t value)
{
	fpga_result result            = FPGA_OK;
	uint64_t *umsg_ptr            = NULL;

	// TODO: make thread-safe and check for errors

	result = ase_fpgaGetUmsgPtr(handle, &umsg_ptr);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get UMsg buffer");
		return result;
	}

	// Assign Value to UMsg
	*((volatile uint64_t *) (umsg_ptr)) = value;

	return result;
}
