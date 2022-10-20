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

//#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <inttypes.h>

//#include "common_int.h"
//#include "sysfs_int.h"
//#include "types_int.h"
//#include <opae/types_enum.h>
//#include <opae/sysobject.h>
#include <opae/types.h>
#include <opae/log.h>

#include "mock/opae_std.h"

fpga_result __REMOTE_API__
remote_fpgaTokenGetObject(fpga_token token,
			  const char *name,
			  fpga_object *object,
			  int flags)
{
	fpga_result result = FPGA_OK;
(void) token;
(void) name;
(void) object;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaHandleGetObject(fpga_token handle,
			   const char *name,
			   fpga_object *object,
			   int flags)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) name;
(void) object;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetObject(fpga_object parent,
			   const char *name,
			   fpga_object *object,
			   int flags)
{
	fpga_result result = FPGA_OK;
(void) parent;
(void) name;
(void) object;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaCloneObject(fpga_object src, fpga_object *dst)
{
	fpga_result result = FPGA_OK;
(void) src;
(void) dst;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetObjectAt(fpga_object parent,
			     size_t idx,
			     fpga_object *object)
{
	fpga_result result = FPGA_OK;
(void) parent;
(void) idx;
(void) object;





	return result;
}

fpga_result __REMOTE_API__ remote_fpgaDestroyObject(fpga_object *obj)
{
	fpga_result result = FPGA_OK;
(void) obj;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetSize(fpga_object obj,
			 uint32_t *size,
			 int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) size;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectRead64(fpga_object obj,
			uint64_t *value,
			int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) value;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectRead(fpga_object obj,
		      uint8_t *buffer,
		      size_t offset,
		      size_t len,
		      int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) buffer;
(void) offset;
(void) len;
(void) flags;




	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectWrite64(fpga_object obj,
			 uint64_t value,
			 int flags)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) value;
(void) flags;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetType(fpga_object obj, enum fpga_sysobject_type *type)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) type;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaObjectGetName(fpga_object obj, char *name, size_t max_len)
{
	fpga_result result = FPGA_OK;
(void) obj;
(void) name;
(void) max_len;




	return result;
}
