// Copyright(c) 2019-2020, Intel Corporation
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

#ifndef __FPGA_BOARD_COMMON_H__
#define __FPGA_BOARD_COMMON_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
* Get sysfs value.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[in] sysfs_path      pointer to sysfs path
* @param[inout] sysfs_name   returns sysfs value as string
* @param[in] len             size of sysfs path

* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid MAC address.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result read_sysfs(fpga_token token, char *sysfs_path,
	char *sysfs_name, size_t len);

/**
* Prints sec info.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid Security.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result print_sec_common_info(fpga_token token);

/**
* Prints ethernet interface info
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @param[in] interface_name  substring to match interface names against
* @returns FPGA_OK on success. FPGA_EXCEPTION if eth interface not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result print_eth_interface_info(fpga_token token, const char *interface_name);

/**
* Get sysfs value.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[in] sysfs_path      pointer to sysfs path
* @param[inout] value        returns sysfs value
* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid MAC address.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result read_sysfs_int64(fpga_token token, char *sysfs_path,
	uint64_t *value);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_DC_H__ */
