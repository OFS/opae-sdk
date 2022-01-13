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

#ifndef __FPGA_VERSION_H__
#define __FPGA_VERSION_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get version information about the OPAE library
 *
 * Retrieve major version, minor version, and revision information about the
 * OPAE library.
 *
 * @param[out]  version  FPGA version
 * @returns FPGA_INVALID_PARAM if any of the output parameters is NULL, FPGA_OK
 * otherwise.
 */
fpga_result fpgaGetOPAECVersion(fpga_version *version);

/**
 * Get version information about the OPAE library as a string
 *
 * Retrieve major version, minor version, and revision information about the
 * OPAE library, encoded in a human-readable string (e.g. "1.0.0").
 *
 * @param[out]  version_str  String to copy version information into
 * @param[in]   len          Length of `version_str`
 * @returns FPGA_INVALID_PARAM if `version_str` is NULL, FPGA_EXCEPTION if the
 * version string cannot be copied into `version_str`, FPGA_OK otherwise.
 */
fpga_result fpgaGetOPAECVersionString(char *version_str, size_t len);
#define FPGA_VERSION_STR_MAX 10

/**
 * Get build information about the OPAE library as a string
 *
 * Retrieve the build identifier of the OPAE library.
 *
 * @param[out]  build_str  String to copy build information into
 * @param[in]   len        Length of `build_str`
 * @returns FPGA_INVALID_PARAM if `build_str` is NULL, FPGA_EXCEPTION if the
 * version string cannot be copied into `build_str`, FPGA_OK otherwise.
 */
fpga_result fpgaGetOPAECBuildString(char *build_str, size_t len);
#define FPGA_BUILD_STR_MAX 41

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_VERSION_H__
