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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "common_int.h"
#include "types_int.h"
#include "config_int.h"

#include <string.h>

fpga_result __FPGA_API__ fpgaGetVersion(uint8_t *major, uint8_t *minor, uint8_t *rev)
{
	if (!major || !minor || !rev) {
		FPGA_MSG("arguments are NULL");
		return FPGA_INVALID_PARAM;
	}

	*major = INTEL_FPGA_API_VER_MAJOR;
	*minor = INTEL_FPGA_API_VER_MINOR;
	*rev = INTEL_FPGA_API_VER_REV;

	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaGetVersionString(char *version_str, size_t len)
{
	if (!version_str) {
		FPGA_MSG("build_str is NULL");
		return FPGA_INVALID_PARAM;
	}

	strncpy(version_str, INTEL_FPGA_API_VERSION, len);

	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaGetBuildString(char *build_str, size_t len)
{
	if (!build_str) {
		FPGA_MSG("build_str is NULL");
		return FPGA_INVALID_PARAM;
	}

	strncpy(build_str, INTEL_FPGA_API_HASH, len);

	return FPGA_OK;
}
