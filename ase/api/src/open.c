// Copyright(c) 2014-2018, Intel Corporation
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

#include <opae/access.h>
#include <opae/utils.h>
#include "common_int.h"
#include "types_int.h"
#include <ase_common.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

fpga_result __FPGA_API__ fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result result = FPGA_NOT_FOUND;
	struct _fpga_handle *_handle;
	struct _fpga_token *_token;
	if (NULL == token) {
		FPGA_MSG("token is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (flags & ~FPGA_OPEN_SHARED) {
		FPGA_MSG("Unrecognized flags");
		return FPGA_INVALID_PARAM;
	}

	/* if (flags & FPGA_OPEN_SHARED) { */
	/* 	FPGA_MSG("Flag \"FPGA_OPEN_SHARED\" is not supported by ASE\n"); */
	/* 	BEGIN_RED_FONTCOLOR; */
	/* 	printf("  [APP]  Flag \"FPGA_OPEN_SHARED\" is not supported by ASE\n"); */
	/* 	END_RED_FONTCOLOR; */
	/* 	return FPGA_NOT_SUPPORTED; */
	/* } */

	_token = (struct _fpga_token *)token;

	if (_token->magic != ASE_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	// ASE Session Initialization
	session_init();

	_handle = ase_malloc(sizeof(struct _fpga_handle));
	if (NULL == _handle) {
		FPGA_MSG("Failed to allocate memory for handle");
		return FPGA_NO_MEMORY;
	}

	ase_memset(_handle, 0, sizeof(*_handle));

	_handle->token = token;
	_handle->magic = FPGA_HANDLE_MAGIC;

	// Init MMIO table
	_handle->mmio_root = NULL;

	// Init workspace table
	_handle->wsid_root = wsid_tracker_init(16384);

	// set handle return value
	*handle = (void *)_handle;

	result = FPGA_OK;
	return result;
}
