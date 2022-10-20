// Copyright(c) 2022, Intel Corporation
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

#include <opae/types.h>

//#include "opae/access.h"
//#include "opae/utils.h"
//#include "common_int.h"
//#include "opae_drv.h"
//#include "intel-fpga.h"

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <stdbool.h>
//#include <stdint.h>


fpga_result __REMOTE_API__
remote_fpgaWriteMMIO32(fpga_handle handle,
		       uint32_t mmio_num,
		       uint64_t offset,
		       uint32_t value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaReadMMIO32(fpga_handle handle,
		      uint32_t mmio_num,
		      uint64_t offset,
		      uint32_t *value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;


	return result;
}

fpga_result __REMOTE_API__
remote_fpgaWriteMMIO64(fpga_handle handle,
		       uint32_t mmio_num,
		       uint64_t offset,
		       uint64_t value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;


	return result;
}

fpga_result __REMOTE_API__
remote_fpgaReadMMIO64(fpga_handle handle,
		      uint32_t mmio_num,
		      uint64_t offset,
		      uint64_t *value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaWriteMMIO512(fpga_handle handle,
			uint32_t mmio_num,
			uint64_t offset,
			const void *value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaMapMMIO(fpga_handle handle,
		   uint32_t mmio_num,
		   uint64_t **mmio_ptr)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) mmio_ptr;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;


	return result;
}
