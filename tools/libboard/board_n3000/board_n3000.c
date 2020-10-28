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
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdlib.h>

#include "../board_common/board_common.h"
#include "board_n3000.h"

// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                  "dfl*/*spi*/spi_master/spi*/spi*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                  "dfl*/*spi*/spi_master/spi*/spi*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH               "dfl*/*spi*/spi_master/spi*/spi*.*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                "dfl*/*spi*/spi_master/spi*/spi*.*/mac_count"

#define DFL_SYSFS_PKVL_A_SBUS_VER            "dfl*/*spi*/spi_master/spi*/spi*.*/*pkvl*/A_sbus_version"
#define DFL_SYSFS_PKVL_A_SERDES_VER          "dfl*/*spi*/spi_master/spi*/spi*.*/*pkvl*/A_serdes_version"

#define DFL_SYSFS_PKVL_B_SBUS_VER            "dfl*/*spi*/spi_master/spi*/spi*.*/*pkvl*/B_sbus_version"
#define DFL_SYSFS_PKVL_B_SERDES_VER          "dfl*/*spi*/spi_master/spi*/spi*.*/*pkvl*/B_serdes_version"


// driver ioctl id
#define FPGA_PHY_GROUP_GET_INFO               0xB702

#define FPGA_BSID_SIZE                        32

// fpga phy group mode
#define FPGA_PHYGROUP_MODE_4_25G              1
#define FPGA_PHYGROUP_MODE_6_25G              3
#define FPGA_PHYGROUP_MODE_2_2_25G            4

#define ETHTOOL_STR              "ethtool"
#define IFCONFIG_STR             "ifconfig"
#define IFCONFIG_UP_STR          "up"
#define FPGA_ETHINTERFACE_NAME   "npac"
#define DFL_ETHINTERFACE         "dfl-fme*/net/npac*"


#define FPGA_BSID_REVISION(id)	(((id) >> 36) & 0xfff)
#define FPGA_BSID_INTERFACE(id)	(((id) >> 32) & 0xf)
#define FPGA_BSID_FLAGS(id)		(((id) >> 24) & 0xff)
#define FPGA_BSID_BUILD_VER(id)	(((id) >> 0) & 0xffffff)
#define FPGA_BSID_FLAG_FVL_BYPASS		0x01
#define FPGA_BSID_FLAG_MAC_LIGHTWEIGHT	0x02
#define FPGA_BSID_FLAG_DISAGGREGATE		0x04
#define FPGA_BSID_FLAG_LIGHTWEIGHT		0x08
#define FPGA_BSID_FLAG_SEU				0x10
#define FPGA_BSID_FLAG_PTP1588			0x20
#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)
#define DFL_BITSTREAM_ID    "bitstream_id"


// Read BMC firmware version
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_ver, size_t len)
{
	fpga_result res                = FPGA_OK;
	char buf[FPGA_VAR_BUF_LEN]     = { 0 };

	if (bmcfw_ver == NULL) {
		FPGA_ERR("Invalid Input parameters");
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
	uint8_t rev                = 0;
	uint32_t var               = 0;
	fpga_result res            = FPGA_OK;
	int retval                 = 0;
	char *endptr               = NULL;
	if (buf == NULL ||
		fw_ver == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}


	/* BMC FW version format reading
	NIOS II Firmware Build 0x0 32 RW[23:0] 24 hFFFFFF Build version of NIOS II Firmware
	NIOS FW is up e.g. 1.0.1 for first release
	[31:24] 8hFF Firmware Support Revision - ASCII code
	0xFF is the default value without NIOS FW, will be changed after NIOS FW is up
	0x41(A)-For RevA
	0x42(B)-For RevB
	0x43(C)-For RevC
	0x44(D)-For RevD
	*/

	errno = 0;
	var = strtoul(buf, &endptr, 16);
	if (endptr != buf + strlen(buf)) {
		OPAE_ERR("Failed to convert buffer to integer: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	rev = (var >> 24) & 0xff;
	if ((rev >= 'A') && (rev <= 'Z')) {// range from 'A' to 'Z'
		retval = snprintf(fw_ver, len, "%c.%u.%u.%u", (char)rev, (var >> 16) & 0xff, (var >> 8) & 0xff, var & 0xff);
		if (retval < 0) {
			FPGA_ERR("error in formatting version");
			return FPGA_EXCEPTION;
		}
	} else {
		OPAE_ERR("Invalid firmware version");
		res = FPGA_EXCEPTION;
	}

	return res;
}

// Read MAX10 firmware version
fpga_result read_max10fw_version(fpga_token token, char *max10fw_ver, size_t len)
{
	fpga_result res                      = FPGA_OK;
	char buf[FPGA_VAR_BUF_LEN]           = { 0 };

	if (max10fw_ver == NULL) {
		FPGA_ERR("Invalid input parameters");
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



// Read pkvl versoin
fpga_result print_pkvl_version(fpga_token token)
{
	fpga_result res                     = FPGA_OK;
	char ver_a_buf[FPGA_VAR_BUF_LEN]    = { 0 };
	char ver_b_buf[FPGA_VAR_BUF_LEN]    = { 0 };
	int retval = 0;
	uint64_t  sub_ver;
	uint64_t serdes_ver;

	res = read_sysfs_int64(token, DFL_SYSFS_PKVL_A_SBUS_VER, &sub_ver);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}
	res = read_sysfs_int64(token, DFL_SYSFS_PKVL_A_SERDES_VER, &serdes_ver);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	retval = snprintf(ver_a_buf, FPGA_VAR_BUF_LEN, "%lx.%lx", sub_ver, serdes_ver);
	if (retval < 0) {
		FPGA_ERR("error in formatting version");
		return FPGA_EXCEPTION;
	}

	res = read_sysfs_int64(token, DFL_SYSFS_PKVL_B_SBUS_VER, &sub_ver);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}
	res = read_sysfs_int64(token, DFL_SYSFS_PKVL_B_SERDES_VER, &serdes_ver);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get read object");
		return res;
	}

	retval = snprintf(ver_b_buf, FPGA_VAR_BUF_LEN, "%lx.%lx", sub_ver, serdes_ver);
	if (retval < 0) {
		FPGA_ERR("error in formatting version");
		return FPGA_EXCEPTION;
	}

	printf("%-32s : %s \n", "Retimer A Version ", ver_a_buf);
	printf("%-32s : %s \n", "Retimer B Version ", ver_b_buf);

	return res;
}

// print mac information
fpga_result print_mac_info(fpga_token token)
{
	fpga_result res                  = FPGA_OK;
	char buf[MAC_BUF_LEN]            = { 0 };
	char count[MAC_BUF_LEN]          = { 0 };
	int i                            = 0;
	int n                            = 0;
	char *endptr                     = NULL;
	pkvl_mac mac;
	unsigned int mac_byte[6] = { 0 };

	res = read_sysfs(token, DFL_SYSFS_MACADDR_PATH, (char *)buf, MAC_BUF_LEN - 1);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac information");
		return res;
	}

	sscanf(buf, "%x:%x:%x:%x:%x:%x", &mac_byte[0], &mac_byte[1],
		&mac_byte[2], &mac_byte[3], &mac_byte[4], &mac_byte[5]);
	for (i = 0; i < 6; i++)
		buf[i] = (unsigned char)mac_byte[i];

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
	mac.byte[0] = buf[5];
	mac.byte[1] = buf[4];
	mac.byte[2] = buf[3];
	mac.byte[3] = 0;

	if (n < 0 || n > 0xFFFF) {
		OPAE_ERR("Invalid mac count");
		return FPGA_EXCEPTION;
	}

	for (i = 0; i < n; ++i) {
		printf("%s %-20d : %02X:%02X:%02X:%02X:%02X:%02X\n",
			"MAC address", i, buf[0], buf[1], buf[2],
			mac.byte[2], mac.byte[1], mac.byte[0]);
		mac.dword += 1;
	}

	return res;
}

// print board information
fpga_result print_board_info(fpga_token token)
{
	fpga_result res                      = FPGA_OK;
	char bmc_ver[FPGA_VAR_BUF_LEN]       = { 0 };
	char max10_ver[FPGA_VAR_BUF_LEN]     = { 0 };

	res = read_bmcfw_version(token, bmc_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read bmc version");
	}

	res = read_max10fw_version(token, max10_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read max10 version");
	}

	printf("Board Management Controller, MAX10 NIOS FW version: %s \n", bmc_ver);
	printf("Board Management Controller, MAX10 Build version: %s \n", max10_ver);

	return res;
}

// prints FPGA ethernet interface info
fpga_result print_eth_interface_info(fpga_token token)
{
	fpga_result res                = FPGA_NOT_FOUND;
	struct if_nameindex *if_nidxs  = NULL;
	struct if_nameindex *intf      = NULL;
	char cmd[SYSFS_PATH_MAX]       = { 0 };
	int result                     = 0;
	fpga_object fpga_object;

	if_nidxs = if_nameindex();
	if (if_nidxs != NULL) {
		for (intf = if_nidxs; intf->if_index != 0
			|| intf->if_name != NULL; intf++) {

			char *p = strstr(intf->if_name, FPGA_ETHINTERFACE_NAME);
			if (p) {
				// Check interface associated to bdf
				res = fpgaTokenGetObject(token, DFL_ETHINTERFACE,
					&fpga_object, FPGA_OBJECT_GLOB);
				if (res != FPGA_OK) {
					OPAE_ERR("Failed to get token Object");
					continue;
				}
				res = fpgaDestroyObject(&fpga_object);
				if (res != FPGA_OK) {
					OPAE_ERR("Failed to Destroy Object");
				}

				// Interface up
				memset(cmd, 0, sizeof(cmd));
				if (snprintf(cmd, sizeof(cmd),
					"%s %s %s", IFCONFIG_STR, intf->if_name,
					IFCONFIG_UP_STR) < 0) {
					OPAE_ERR("snprintf failed");
					res = FPGA_EXCEPTION;
					goto out_free;
				}
				result = system(cmd);
				if (result < 0) {
					res = FPGA_EXCEPTION;
					OPAE_ERR("Failed to run cmd: %s  %s",
						cmd, strerror(errno));
				}
				// eth tool command
				memset(cmd, 0, sizeof(cmd));
				if (snprintf(cmd, sizeof(cmd),
					"%s %s", ETHTOOL_STR, intf->if_name) < 0) {
					OPAE_ERR("snprintf failed");
					res = FPGA_EXCEPTION;
					goto out_free;
				}
				result = system(cmd);
				if (result < 0) {
					res = FPGA_EXCEPTION;
					OPAE_ERR("Failed to run cmd: %s  %s", cmd,
						strerror(errno));
				}

			}
		}

out_free:
		if_freenameindex(if_nidxs);
	}

	return res;
}

// print phy group information
fpga_result print_phy_info(fpga_token token)
{
	fpga_result res           = FPGA_OK;

	res = print_pkvl_version(token);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy group count");
		return res;
	}

	res = print_eth_interface_info(token);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy group count");
		return res;
	}

	return res;

}

// Sec info
fpga_result print_sec_info(fpga_token token)
{
	return print_sec_common_info(token);
}


// print fme verbose info
fpga_result print_fme_verbose_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	fpga_object fpga_object;
	uint64_t bitstream_id;
	uint32_t major = 0;
	uint32_t val = 0;

	res = fpgaTokenGetObject(token, DFL_BITSTREAM_ID,
		&fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	res = fpgaObjectRead64(fpga_object, &bitstream_id, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		goto out_destroy;
	}

	char *platform[] = { "PAC N3000", "PAC GX FPGA", "PAC D5005",
					"PAC S10" };
	major = FPGA_BBS_VER_MAJOR(bitstream_id);
	printf("%-32s : ", "Platform");
	if (major < sizeof(platform) / sizeof(char *))
		printf("%s\n", platform[major]);
	else
		printf("unknown\n");

	val = FPGA_BBS_VER_MINOR(bitstream_id);
	printf("%-32s : 1.%u\n", "DCP Version", val);

	char *phase[] = { "Pre-Alpha", "Alpha", "Beta", "PV" };
	val = FPGA_BBS_VER_PATCH(bitstream_id);
	printf("%-32s : ", "Phase");
	if (val < sizeof(phase) / sizeof(char *))
		printf("%s\n", phase[val]);
	else
		printf("unknown\n");

	val = FPGA_BSID_REVISION(bitstream_id);
	printf("%-32s : %03x\n", "Revision", val);

	val = FPGA_BSID_INTERFACE(bitstream_id);
	if (major == 0) {	// PAC N3000
		char *intf[] = { "8x10G", "4x25G", "2x1x25G", "4x25G+2x25G", "2x2x25G",
						"2x1x25Gx2FVL", "1x2x25G" };
		printf("%-32s : ", "Interface");
		if (val < sizeof(intf) / sizeof(char *))
			printf("%s\n", intf[val]);
		else
			printf("unknown\n");
	} else {
		printf("%-32s : %x", "HSSI Id", val);
	}

	val = FPGA_BSID_FLAGS(bitstream_id);
	printf("%-32s : %s\n", "Bypass Mode",
		val & FPGA_BSID_FLAG_FVL_BYPASS ? "enabled" : "disabled");
	printf("%-32s : %s\n", "MAC Lightweight Mode",
		val & FPGA_BSID_FLAG_MAC_LIGHTWEIGHT ? "enabled" : "disabled");
	printf("%-32s : %s\n", "Disaggregate Mode",
		val & FPGA_BSID_FLAG_DISAGGREGATE ? "enabled" : "disabled");
	printf("%-32s : %s\n", "Lightweight Mode",
		val & FPGA_BSID_FLAG_LIGHTWEIGHT ? "enabled" : "disabled");
	printf("%-32s : %s\n", "SEU detection",
		val & FPGA_BSID_FLAG_SEU ? "enabled" : "disabled");
	printf("%-32s : %s\n", "PTP functionality",
		val & FPGA_BSID_FLAG_PTP1588 ? "enabled" : "disabled");

	val = FPGA_BSID_BUILD_VER(bitstream_id);
	printf("%-32s : %d.%d.%d\n", "Build Version",
		(val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);

out_destroy:
	res = fpgaDestroyObject(&fpga_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}

	return res;
}