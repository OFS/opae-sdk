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

//#include <sys/stat.h>
//#include <unistd.h>
//#include <dirent.h>
//#include <stdio.h>

#include <opae/types.h>

//#include "common_int.h"
//#include "opae/error.h"

//#include "error_int.h"

#include "mock/opae_std.h"

//#define INJECT_ERROR "inject_error"

fpga_result __REMOTE_API__
remote_fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value)
{
	fpga_result res = FPGA_OK;
(void) token;
(void) error_num;
(void) value;



	return res;
}

fpga_result __REMOTE_API__
remote_fpgaClearError(fpga_token token, uint32_t error_num)
{
	fpga_result res = FPGA_OK;
(void) token;
(void) error_num;




	return res;
}

fpga_result __REMOTE_API__ remote_fpgaClearAllErrors(fpga_token token)
{
	fpga_result res = FPGA_OK;
(void) token;



	return res;
}

fpga_result __REMOTE_API__ remote_fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info)
{
(void) token;
(void) error_num;
(void) error_info;



	return FPGA_OK;
}
