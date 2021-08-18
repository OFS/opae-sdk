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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <glob.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/ethernet.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include "board_d5005.h"
#include "../board_common/board_common.h"

#define MACADDR_LEN 19

// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                     "dfl*/*spi*/spi_master/spi*/spi*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                     "dfl*/*spi*/spi_master/spi*/spi*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH                  "dfl*/*spi*/spi_master/spi*/spi*.*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                   "dfl*/*spi*/spi_master/spi*/spi*.*/mac_count"


// Read BMC firmware version
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_ver, size_t len)
{
	fpga_result res                = FPGA_OK;
	char buf[FPGA_VAR_BUF_LEN]     = { 0 };

	if (bmcfw_ver == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = read_sysfs(token, DFL_SYSFS_BMCFW_VER, buf, FPGA_VAR_BUF_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	res = parse_fw_ver(buf, bmcfw_ver, len);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to parse version ");
	}

	return res;
}

fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len)
{
	uint32_t  var              = 0;
	fpga_result res            = FPGA_OK;
	int retval                 = 0;
	char *endptr               = NULL;

	if (buf == NULL || fw_ver == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	/* BMC FW version format reading
	NIOS II Firmware Build 0x0 32 RW[23:0] 24 hFFFFFF Build version of NIOS II Firmware
	NIOS FW is up e.g. 1.0.1 for first release
	[31:24] 8hFF Firmware Support Revision - ASCII code
	0xFF is the default value without NIOS FW, will be changed after NIOS FW is up
	*/

	errno = 0;
	var = strtoul(buf, &endptr, 16);
	if (endptr != buf + strlen(buf)) {
		OPAE_ERR("Failed to convert buffer to integer: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	retval = snprintf(fw_ver, len, "%u.%u.%u", (var >> 16) & 0xff, (var >> 8) & 0xff, var & 0xff);
	if (retval < 0) {
			OPAE_ERR("error in formatting version");
			return FPGA_EXCEPTION;
	}

	return res;
}

// Read MAX10 firmware version
fpga_result read_max10fw_version(fpga_token token, char *max10fw_ver, size_t len)
{
	fpga_result res = FPGA_OK;
	char buf[FPGA_VAR_BUF_LEN] = { 0 };

	if (max10fw_ver == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = read_sysfs(token, DFL_SYSFS_MAX10_VER, buf, FPGA_VAR_BUF_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	res = parse_fw_ver(buf, max10fw_ver, len);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to parse version ");
	}

	return res;
}


// Read mac information
fpga_result read_mac_info(fpga_token token, uint32_t afu_channel_num,
						struct ether_addr *mac_addr)
{
	fpga_result res                = FPGA_OK;
	char mac_buf[MACADDR_LEN]      = { 0 };
	char mac_count[MACADDR_LEN]    = { 0 };
	uint64_t count                 = 0;
	char *endptr                   = NULL;

	if (mac_addr == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = read_sysfs(token, DFL_SYSFS_MACADDR_PATH, mac_buf, MACADDR_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	ether_aton_r(mac_buf, mac_addr);

	res = read_sysfs(token, DFL_SYSFS_MACCNT_PATH, mac_count, MACADDR_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	errno = 0;
	count = strtoul(mac_count, &endptr, 16);
	if (endptr != mac_count + strlen(mac_count)) {
		OPAE_ERR("Failed to convert buffer to integer: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	if (afu_channel_num >= count) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	if ((mac_addr->ether_addr_octet[0] == 0xff) &&
		(mac_addr->ether_addr_octet[1] == 0xff) &&
		(mac_addr->ether_addr_octet[2] == 0xff) &&
		(mac_addr->ether_addr_octet[3] == 0xff) &&
		(mac_addr->ether_addr_octet[4] == 0xff) &&
		(mac_addr->ether_addr_octet[5] == 0xff)) {
		OPAE_ERR("Invalid MAC address");
		return FPGA_INVALID_PARAM;
	}

	mac_addr->ether_addr_octet[5] += afu_channel_num;

	return res;
}

// print board information
fpga_result print_board_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	char bmc_ver[FPGA_VAR_BUF_LEN] = { 0 };
	char max10_ver[FPGA_VAR_BUF_LEN] = { 0 };

	res = read_bmcfw_version(token, bmc_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read bmc version");
		resval = res;
	}

	res = read_max10fw_version(token, max10_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read max10 version");
		resval = res;
	}

	printf("Board Management Controller, MAX10 NIOS FW version: %s \n", bmc_ver);
	printf("Board Management Controller, MAX10 Build version: %s \n", max10_ver);

	return resval;
}

// print board information
fpga_result print_sec_info(fpga_token token)
{
	return print_sec_common_info(token);
}

// print mac information
fpga_result print_mac_info(fpga_token token)
{
	char mac_str[18] = { 0 };
	struct ether_addr MAC;
	fpga_result res = FPGA_OK;

	memset((void *)&MAC, 0, sizeof(MAC));

	res = read_mac_info(token, 0, &MAC);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac address");
	} else {
		printf("%-1s : %s\n", "MAC address",
			ether_ntoa_r(&MAC, mac_str));
	}

	return res;
}
