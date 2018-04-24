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

fpga_result save_and_bind(struct _fpga_handle *fpga_h, bool bind)
{
	fpga_result res = FPGA_OK;

	if (!fpga_h)
		return FPGA_INVALID_PARAM;

	if (!bind)
		return res;
#ifdef ENABLE_NUMA
	if (NULL == fpga_h->numa)
		return res;

	if ((NULL != fpga_h->numa->saved.membind_mask) || (NULL != fpga_h->numa->saved.runnode_mask)) {
		FPGA_MSG("Warning: attempting to save when context already exists");
		numa_free_nodemask(fpga_h->numa->saved.membind_mask);
		numa_free_cpumask(fpga_h->numa->saved.runnode_mask);
	}
	fpga_h->numa->saved.membind_mask = numa_get_membind();
	fpga_h->numa->saved.runnode_mask = numa_get_run_node_mask();

	numa_bind(fpga_h->numa->fpgaNodeMask);
#endif
	return res;
}

fpga_result restore_and_unbind(struct _fpga_handle *fpga_h, bool bind)
{
	fpga_result res = FPGA_OK;

	if (!fpga_h)
		return FPGA_INVALID_PARAM;

	if (!bind)
		return res;

#ifdef ENABLE_NUMA
	if (NULL == fpga_h->numa)
		return res;

	if ((NULL == fpga_h->numa->saved.membind_mask) || (NULL == fpga_h->numa->saved.runnode_mask)) {
		FPGA_MSG("Error: attempting to restore to a NULL context");
		return FPGA_EXCEPTION;
	}
	numa_set_membind(fpga_h->numa->saved.membind_mask);
	numa_run_on_node_mask(fpga_h->numa->saved.runnode_mask);

	numa_free_nodemask(fpga_h->numa->saved.membind_mask);
	numa_free_cpumask(fpga_h->numa->saved.runnode_mask);
	fpga_h->numa->saved.membind_mask = NULL;
	fpga_h->numa->saved.runnode_mask = NULL;
#endif
	return res;
}

fpga_result move_memory_to_node(struct _fpga_handle *fpga_h, void *ptr, size_t size)
{
	fpga_result res = FPGA_OK;

	if ((!ptr) || (!fpga_h))
		return FPGA_INVALID_PARAM;

#ifdef ENABLE_NUMA
	numa_tonodemask_memory(ptr, size, fpga_h->numa->fpgaNodeMask);
#endif

	return res;
}
