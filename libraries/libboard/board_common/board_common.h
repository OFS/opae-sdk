// Copyright(c) 2019-2023, Intel Corporation
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

#include <netinet/ether.h>
#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SYSFS_PATH_MAX             256

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

/**
* Get bdf of fpga device.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[out] segment        returns segment of fpga
* @param[out] bus            returns bus of fpga
* @param[out] device         returns device of fpga
* @param[out] function       returns function of fpga
* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid sbdf.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result get_fpga_sbdf(fpga_token token,
			uint16_t *segment,
			uint8_t *bus,
			uint8_t *device,
			uint8_t *function);

/**
* Get sysfs 64 bit value.
*
* @param[in] path             pointer to sysfs path
* @param[out] u               returns sysfs 64 bit value
* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid sysfs path.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result sysfs_read_u64(const char *path, uint64_t *u);

/**
* find fpga feature id.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @param[in] feature_id      fpga dev feature id
* @param[inout] dev_str      returns fpga dev str
* FPGA_INVALID_PARAM if invalid parameters were provided
* FPGA_NOT_FOUND if feature_id not found
*
*/
fpga_result find_dev_feature(fpga_token token,
	uint32_t feature_id,
	char *dfl_dev_str);

/**
* Prints fpga boot page info.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if invalid boot info.
* FPGA_INVALID_PARAM if invalid parameters were provided
*
*/
fpga_result print_common_boot_info(fpga_token token);

/**
 * Prints a number of incremented MAC addresses.
 *
 * @param[in] eth_addr pointer to MAC address in network byte order
 * @param[in] count    number of MAC address to print
 *
 */
void print_mac_address(struct ether_addr *eth_addr, int count);


/**
* Replace all occurrences of needle in haystack with substitute.
*
* @param[inout] haystack         Input event log str
* @param[in] needle              needle in a haystack
* @param[in] substitute          substitute string
* @param[in] max_haystack_len    max length of heystack
* @param[in] res                 FPGA_OK on success
*
* @returns FPGA_OK on success. FPGA_NO_MEMORY if fails to allocte memory.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result replace_str_in_str(
	char *const haystack,
	const char *const needle,
	const char *const substitute,
	const size_t max_haystack_len,
	fpga_result res);

/**
* Replace all occurrences of needle in haystack with substitute.
*
* Reformat BOM Critical Components info to be directly printable.
* Keys and values may not include commas.
* Spaces and tabs are removed around commas.
* Line endings are converted to LF (linefeed).
* Empty lines are removed.
* All Key,Value pairs are converted to Key: Value
* @param[inout] bom_info            bmc info str
* @param[in] len                 length of bmc info
* @param[in] max_result_len      max result len
*
* @returns FPGA_OK on success.
* FPGA_INVALID_PARAM if invalid parameters were provided
*/
fpga_result reformat_bom_info(
	char *const bom_info,
	const size_t len,
	const size_t max_result_len);

/**
* prints QSFP cable status.
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success.
* FPGA_EXCEPTION if invalid sysfs path
*/
fpga_result qsfp_cable_status(const fpga_token token);

/**
* prints common phy information, including qsfp_cable_status
*
* @param[in] token           fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success.
* FPGA_EXCEPTION if invalid sysfs path
*/
fpga_result print_common_phy_info(fpga_token token);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_DC_H__ */

