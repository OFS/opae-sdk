// Original work Copyright(c) 2019-2020, Intel Corporation
// Modifications Copyright(c) 2021, Silicom Denmark A/S
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

#include "board_n5010.h"
#include "../board_common/board_common.h"

#define MAC_BUF_LEN            19

// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                     "dfl*/**/spi_master/spi*/spi*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                     "dfl*/**/spi_master/spi*/spi*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH                  "dfl*/**/spi_master/spi*/spi*.*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                   "dfl*/**/spi_master/spi*/spi*.*/mac_count"

#define DFL_SYSFS_N5014_BOARD_INFO		"dfl*/**/spi_master/spi*/spi*.*/n5010bmc-phy*/board_info"

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
fpga_result read_mac_addr(fpga_token token, struct ether_addr *mac_addr)
{
	fpga_result res = FPGA_OK;
	char buf[MAC_BUF_LEN] = { 0 };
	memset(mac_addr, 0, sizeof(struct ether_addr));

	res = read_sysfs(token, DFL_SYSFS_MACADDR_PATH, (char *)buf, MAC_BUF_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac information");
		return res;
	}

	ether_aton_r(buf, mac_addr);

	if ((mac_addr->ether_addr_octet[0] == 0xff) &&
		(mac_addr->ether_addr_octet[1] == 0xff) &&
		(mac_addr->ether_addr_octet[2] == 0xff) &&
		(mac_addr->ether_addr_octet[3] == 0xff) &&
		(mac_addr->ether_addr_octet[4] == 0xff) &&
		(mac_addr->ether_addr_octet[5] == 0xff)) {
		OPAE_ERR("Invalid MAC address");
		return FPGA_EXCEPTION;
	}

	return res;
}

// Read board info
fpga_result read_n5014_board_info(fpga_token token, char *board_info)
{
	fpga_result res                = FPGA_OK;

	if (board_info == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = read_sysfs(token, DFL_SYSFS_N5014_BOARD_INFO, board_info, FPGA_VAR_BUF_LEN - 1);

	return res;
}

// print BOM info
fpga_result print_bom_info(const fpga_token token)
{
	fpga_result res = FPGA_OK;
	struct ether_addr mac_addr;

	res = read_mac_addr(token, &mac_addr);
	if (res != FPGA_OK) {
		return res;
	}

	// print card serial and board info if Silicom Denmark MAC
	if ((mac_addr.ether_addr_octet[0] == 0x00) &&
		(mac_addr.ether_addr_octet[1] == 0x21) &&
		(mac_addr.ether_addr_octet[2] == 0xb2) &&
		((mac_addr.ether_addr_octet[3] == 0x2c) || (mac_addr.ether_addr_octet[3] == 0x2d))) {
		char serial_name[] = "FB4CGG2@S10D21-D20";  //N5014
		if (mac_addr.ether_addr_octet[3] == 0x2d)
			serial_name[16] = '0';  //N5013
		uint32_t number = (mac_addr.ether_addr_octet[4] << 4) + (mac_addr.ether_addr_octet[5] >> 4);
		char board_info[FPGA_VAR_BUF_LEN] = { 0 };
		res = read_n5014_board_info(token, board_info);
		printf("%-32s : %s.%04u", "Board Serial", serial_name, number);
		if (res == FPGA_OK)
			printf(" (%s)", board_info);
		printf("\n");
	}

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

	print_bom_info(token); // ignore errors

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
	fpga_result res = FPGA_OK;
	char count[MAC_BUF_LEN] = { 0 };
	int n = 0;
	char *endptr = NULL;
	struct ether_addr mac_addr ;

	res = read_mac_addr(token, &mac_addr);
	if (res != FPGA_OK) {
		return res;
	}

	res = read_sysfs(token, DFL_SYSFS_MACCNT_PATH, (char *)count, MAC_BUF_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac information");
		return res;
	}

	errno = 0;
	n = strtol(count, &endptr, 10);
	if (endptr != count + strlen(count)) {
		OPAE_ERR("Failed to convert buffer to integer: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}
	printf("%-32s : %d\n", "Number of MACs", n);

	if (n < 0 || n > 0xFFFF) {
		OPAE_ERR("Invalid mac count");
		return FPGA_EXCEPTION;
	}

	print_mac_address(&mac_addr, n);

	return res;
}

// print phy group information
fpga_result print_phy_info(fpga_token token)
{
	fpga_result res           = FPGA_OK;

	res = print_eth_interface_info(token, "n5010");
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy info");
		return res;
	}

	return res;
}

// prints fpga boot page info
fpga_result fpga_boot_info(fpga_token token)
{
	return print_common_boot_info(token);
}
