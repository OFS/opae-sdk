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

#ifndef __FPGA_BOARD_N3000_H__
#define __FPGA_BOARD_N3000_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_PORTS                  8
#define SYSFS_MAX_SIZE             256
#define MAC_BYTE_SIZE              4
#define VER_BUF_SIZE               16
#define FPGA_VAR_BUF_LEN           256
#define MAC_BUF_LEN                19
#define ETH_GROUP_COUNT            2
#define MAX_FPGA_FEATURE_COUNT     24

typedef union _pkvl_mac {
	unsigned int dword;
	unsigned char byte[MAC_BYTE_SIZE];
} pkvl_mac;

/**
* Prints BMC, MAX10 and NIOS version.
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if MAX10 or NIOS sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
*
*/
fpga_result print_board_info(fpga_token token);

/**
* Prints phy group informantion.
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if phy group sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
*
*/
fpga_result print_phy_info(fpga_token token);

/**
* Prints mac informantion.
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if mac sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
*
*/
fpga_result print_mac_info(fpga_token token);

/**
* Get Max10 firmware version.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[inout] max10fw_var  pointer to char pcb_info string
*                            user allocates memory and free input string
* @param[in] len             length of char max10fw_var string
* @returns FPGA_OK on success. FPGA_NOT_FOUND if MAX10  sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
*
*/
fpga_result read_max10fw_version(fpga_token token, char *max10fw_var, size_t len);

/**
* Get BMC/NIOS firmware version.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[inout] bmcfw_var    pointer to char bmcfw_var string
*                            user allocates memory and free input string
* @param[in] len             length of char bmcfw_var string
* @returns FPGA_OK on success. FPGA_NOT_FOUND if NIOS sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
*
*/
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_var, size_t len);

/**
* Parse bmc/max10 version.
*
* @param[in] buf             pointer to firmware version
* @param[inout] fw_ver       pointer to char firmware string
* @param[in] len             length of char fw var string
* @returns FPGA_OK on success. FPGA_EXCEPTION if FW version is invlaid.
*
*/
fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len);

/**
* Prints pkvl information
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_EXCEPTION if token is invalid.
*
*/
fpga_result print_pkvl_version(fpga_token token);

/**
* Prints Security information.
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if MAX10 or NIOS sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result print_sec_info(fpga_token token);

/**
* Prints fme verbose info
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if eth feature not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result print_fme_verbose_info(fpga_token token);

/**
* enumerate ethernet group feature
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @param[inout] eth_feature   pointer to eth group feature array of strings
* @param[inout] size          size of eth group feature array
* @returns FPGA_OK on success. FPGA_NOT_FOUND if eth feature not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result enum_eth_group_feature(fpga_token token,
			char eth_feature[ETH_GROUP_COUNT][SYSFS_MAX_SIZE],
			uint32_t size);


/**
* enumerate pkvl regmap sysfs path
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @param[inout] pkvl_path     pointer to pkvl sysfs path
* @returns FPGA_OK on success. FPGA_NOT_FOUND if eth feature not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result enum_pkvl_sysfs_path(fpga_token token, char *pkvl_path);


/**
* reads regmap reg registers
*
* @param[in] sysfs_path      pointer to regmap sysfs path
* @param[in] index           regmap registers index
* @param[out] value          pointer to regmap offset value
* @returns FPGA_OK on success. FPGA_NOT_FOUND if regmap offset not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result read_regmap(char *sysfs_path,
			uint64_t index,
			uint32_t *value);


/**
* prints retimer info
*
* @param[in] token            fpga_token object for device (FPGA_DEVICE type)
* @param[in] speed            ethernet line side speed
* @returns FPGA_OK on success. FPGA_NOT_FOUND if retimer version offset not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result print_retimer_info(fpga_token token, uint32_t speed);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_N3000_H__ */
