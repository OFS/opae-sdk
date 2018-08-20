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

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif // HAVE_CONFIG_H

#include "opae/access.h"
#include "opae/utils.h"
#include "common_int.h"
#include "intel-fpga.h"
#include "numa_int.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

fpga_result save_and_bind(struct _fpga_handle *_handle, bool bind, bool save_state)
{
#ifndef ENABLE_NUMA
	UNUSED_PARAM(save_state);
#endif

	fpga_result res = FPGA_OK;

	if (!_handle) {
		return FPGA_INVALID_PARAM;
	}

	if (!bind) {
		return res;
	}

#ifdef ENABLE_NUMA

	if (NULL == _handle->numa) {
		return res;
	}

	if (save_state) {
		if ((NULL != _handle->numa->saved.membind_mask) || (NULL != _handle->numa->saved.runnode_mask)) {
			FPGA_MSG("Warning: attempting to save when context already exists");
			numa_free_nodemask(_handle->numa->saved.membind_mask);
			numa_free_cpumask(_handle->numa->saved.runnode_mask);
		}

		_handle->numa->saved.membind_mask = numa_get_membind();
		_handle->numa->saved.runnode_mask = numa_get_run_node_mask();
	}

	numa_bind(_handle->numa->fpgaNodeMask);
#endif
	return res;
}

fpga_result restore_and_unbind(struct _fpga_handle *_handle, bool bind, bool restore_state)
{
#ifndef ENABLE_NUMA
	UNUSED_PARAM(restore_state);
#endif

	fpga_result res = FPGA_OK;

	if (!_handle) {
		return FPGA_INVALID_PARAM;
	}

	if (!bind) {
		return res;
	}

#ifdef ENABLE_NUMA

	if (NULL == _handle->numa) {
		return res;
	}

	if (restore_state) {
		if ((NULL == _handle->numa->saved.membind_mask) || (NULL == _handle->numa->saved.runnode_mask)) {
			FPGA_MSG("Error: attempting to restore to a NULL context");
			return FPGA_EXCEPTION;
		}

		numa_set_membind(_handle->numa->saved.membind_mask);
		numa_run_on_node_mask(_handle->numa->saved.runnode_mask);

		numa_free_nodemask(_handle->numa->saved.membind_mask);
		numa_free_cpumask(_handle->numa->saved.runnode_mask);
		_handle->numa->saved.membind_mask = NULL;
		_handle->numa->saved.runnode_mask = NULL;
	} else {
		numa_set_membind(_handle->numa->at_open.membind_mask);
		numa_run_on_node_mask(_handle->numa->at_open.runnode_mask);
	}

#endif
	return res;
}

fpga_result move_memory_to_node(struct _fpga_handle *_handle, void *ptr, size_t size)
{
	fpga_result res = FPGA_OK;

	if ((!ptr) || (!_handle) || (0 == size)) {
		return FPGA_INVALID_PARAM;
	}

#ifdef ENABLE_NUMA
	numa_tonodemask_memory(ptr, size, _handle->numa->fpgaNodeMask);
#endif

	return res;
}

fpga_result __FPGA_API__ fpgaAdviseDmaBuffer(fpga_handle fpga_h, void *buf, uint64_t len,
					     bool bind_thread)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *) fpga_h;
	int err;

	result = handle_check_and_lock(_handle);

	if (result) {
		return result;
	}

	/* Assure buf is a valid pointer */
	if (!buf) {
		FPGA_MSG("Buffer is NULL");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (0 == len) {
		FPGA_MSG("Cannot handle zero-length buffer");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (NULL == _handle->numa) {
		goto out_unlock;
	}

	// Write to the page in order to bring it into memory
	volatile char *ptr = (volatile char *)buf;
	char tmp = *ptr;
	*ptr = tmp;

	// Migrate buffer to NUMA node
	result = move_memory_to_node(_handle, buf, len);

	if (FPGA_OK != result) {
		FPGA_MSG("move_memory_to_node failure");
		result = FPGA_EXCEPTION;
		goto out_unlock;
	}

	// Bind to NUMA node if requested
	if (bind_thread) {
		result = save_and_bind(_handle, true, false);

		if (FPGA_OK != result) {
			FPGA_MSG("save_and_bind failure");
			result = FPGA_EXCEPTION;
			goto out_unlock;
		}
	}

	// Tell the kernel we'll need this buffer and it is sequential
	uint64_t addr = (uint64_t) buf;
	uint64_t pg_size = (uint64_t)getpagesize();
	size_t remainder = (pg_size - (addr & (pg_size - 1))) & ~(pg_size - 1);
	addr = addr & ~(pg_size - 1);   // Align down to page boundary
	madvise((void *)addr, len + remainder, MADV_SEQUENTIAL);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);

	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;
}
