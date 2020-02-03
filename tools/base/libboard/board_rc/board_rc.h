// Copyright(c) 2019, Intel Corporation
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

#ifndef __FPGA_BOARD_RC_H__
#define __FPGA_BOARD_RC_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


 /**
 * Get Baseboard Management Controller version.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[inout] version      pointer to BMC version
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
 * FPGA_INVALID_PARAM if invalid parameters were provide
 *
 */
fpga_result read_bmc_version(fpga_token token, int *version);

/**
 * Get BMC power down root cause
 *
 * @param[in] token                    fpga_token object for device (FPGA_DEVICE type)
 * @param[inout] pwr_down_cause        pointer to power down root cause string.
 *                                     user allocates memory and free input string
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
 * FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result read_bmc_pwr_down_cause(fpga_token token, char *pwr_down_cause);

/**
 * Get BMC last reset root cause
 *
 * @param[in] token                    fpga_token object for device (FPGA_DEVICE type)
 * @param[inout] reset_cause           pointer to reset root cause string.
 *                                     user allocates memory and free input string
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
 * FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result read_bmc_reset_cause(fpga_token token, char *reset_causee);

/**
 * Prints BMC version, Power down cause and Reset cause
 *
 * @param[in] token              fpga_token object for device (FPGA_DEVICE type)
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
 * FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result print_board_info(fpga_token token);


fpga_result read_sysfs(fpga_token token,
			char *sysfs_path,
			char *sysfs_name);

fpga_result print_sec_info(fpga_token token);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_RC_H__ */
