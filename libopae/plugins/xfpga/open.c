// Copyright(c) 2017-2018, Intel Corporation
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

#include "common_int.h"
#include <opae/access.h>
#include <opae/utils.h>
#include "types_int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "safe_string/safe_string.h"

fpga_result __FPGA_API__
xfpga_fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result result = FPGA_NOT_FOUND;
	struct _fpga_handle *_handle;
	struct _fpga_token *_token;
	int fddev = -1;
	pthread_mutexattr_t mattr;
	int open_flags = 0;

	if (NULL == token) {
		FPGA_MSG("token is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == handle) {
		FPGA_MSG("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (flags & ~FPGA_OPEN_SHARED) {
		FPGA_MSG("unrecognized flags");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)token;

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	_handle = malloc(sizeof(struct _fpga_handle));
	if (NULL == _handle) {
		FPGA_MSG("Failed to allocate memory for handle");
		return FPGA_NO_MEMORY;
	}

	memset_s(_handle, sizeof(*_handle), 0);

	// mark data structure as valid
	_handle->magic = FPGA_HANDLE_MAGIC;

	_handle->token = token;

	_handle->fdfpgad = -1;

	// Init MMIO table
	_handle->mmio_root = wsid_tracker_init(4);

	// Init workspace table
	_handle->wsid_root = wsid_tracker_init(16384);

	// Init metric enum
	_handle->metric_enum_status = false;
	_handle->bmc_handle = NULL;
	_handle->_bmc_metric_cache_value = NULL;

	// Open resources in exclusive mode unless FPGA_OPEN_SHARED is given
	open_flags = O_RDWR | ((flags & FPGA_OPEN_SHARED) ? 0 : O_EXCL);
	fddev = open(_token->devpath, open_flags);
	if (-1 == fddev) {
		FPGA_MSG("open(%s) failed: %s", _token->devpath, strerror(errno));
		switch (errno) {
		case EACCES:
			result = FPGA_NO_ACCESS;
			break;
		case EBUSY:
			result = FPGA_BUSY;
			break;
		default:
			result = FPGA_NO_DRIVER;
			break;
		}
		goto out_free;
	}

	// Save the file descriptor for close.
	_handle->fddev = fddev;

	if (pthread_mutexattr_init(&mattr)) {
		FPGA_MSG("Failed to init handle mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_handle->lock, &mattr)) {
		FPGA_MSG("Failed to init handle mutex");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	pthread_mutexattr_destroy(&mattr);

	_handle->flags = 0;
#if GCC_VERSION >= 40900
	__builtin_cpu_init();
	if (__builtin_cpu_supports("avx512f")) {
		_handle->flags |= OPAE_FLAG_HAS_MMX512;
	}
#endif

	// set handle return value
	*handle = (void *)_handle;

	return FPGA_OK;

out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);

out_free:
	wsid_tracker_cleanup(_handle->wsid_root, NULL);
	wsid_tracker_cleanup(_handle->mmio_root, NULL);
	free(_handle);

	if (-1 != fddev) {
		close(fddev);
	}

	return result;
}
