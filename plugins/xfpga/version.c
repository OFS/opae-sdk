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

#include "safe_string/safe_string.h"
#include "common_int.h"
#include "types_int.h"

fpga_result __FPGA_API__ xfpga_fpgaGetOPAECVersion(fpga_version *version)
{
	if (!version) {
		FPGA_MSG("version is NULL");
		return FPGA_INVALID_PARAM;
	}

	version->major = OPAE_VERSION_MAJOR;
	version->minor = OPAE_VERSION_MINOR;
	version->patch = OPAE_VERSION_REVISION;

	return FPGA_OK;
}

fpga_result __FPGA_API__ xfpga_fpgaGetOPAECVersionString(char *version_str, size_t len)
{
	errno_t err = 0;

	if (!version_str) {
		FPGA_MSG("version_str is NULL");
		return FPGA_INVALID_PARAM;
	}

	err = strncpy_s(version_str, len, OPAE_VERSION,
		  sizeof(OPAE_VERSION));

	if (err) {
		FPGA_ERR("strncpy_s failed with error %i", err);
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result __FPGA_API__ xfpga_fpgaGetOPAECBuildString(char *build_str, size_t len)
{
	errno_t err = 0;

	if (!build_str) {
		FPGA_MSG("build_str is NULL");
		return FPGA_INVALID_PARAM;
	}

	err = strncpy_s(build_str, len, INTEL_FPGA_API_HASH,
		  sizeof(INTEL_FPGA_API_HASH));

	if (err) {
		FPGA_ERR("strncpy_s failed with error %i", err);
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}
