// Copyright(c) 2021, Intel Corporation
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
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include <netinet/ether.h>
#include <opae/uio.h>
#include "../board_common/board_common.h"
#include "board_n6010.h"

#define FPGA_VAR_BUF_LEN       256
#define MACADDR_LEN            19
#define UNUSED_PARAM(x) ((void)x)
#define FEATURE_DEV "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region"\
					"/region*/dfl-fme*/dfl_dev*"
// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                     "dfl*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                     "dfl*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH                  "dfl*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                   "dfl*/mac_count"

#define HSSI_FEATURE_ID                  0x15
#define HSSI_100G_PROFILE                       27
#define HSSI_25G_PROFILE                        21
#define HSSI_10_PROFILE                         20

#define HSSI_FEATURE_LIST                       0xC
#define HSSI_PORT_ATTRIBUTE                     0x10

// hssi version
struct hssi_version {
	union {
		uint32_t csr;
		struct {
			uint32_t rsvd : 8;
			uint32_t minor : 8;
			uint32_t major : 16;
		};
	};
};

// hssi feature list CSR
struct hssi_feature_list {
	union {
		uint32_t csr;
		struct {
			uint32_t axi4_support : 1;
			uint32_t hssi_num : 4;
			uint32_t reserved : 27;
		};
	};
};

// hssi port attribute CSR
//Interface Attribute Port X Parameters, X =0-15
//Byte Offset: 0x10 + X * 4
struct hssi_port_attribute {
	union {
		uint32_t csr;
		struct {
			uint32_t profile : 6;
			uint32_t ready_latency : 4;
			uint32_t data_bus_width : 3;
			uint32_t low_speed_mac : 1;
			uint32_t dynamic_pr : 1;
			uint32_t reserved : 16;
		};
	};
};


// Parse firmware version
fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len)
{
	uint32_t  var = 0;
	fpga_result res = FPGA_OK;
	int retval = 0;
	char *endptr = NULL;

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

// Read BMC firmware version
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_ver, size_t len)
{
	fpga_result res = FPGA_OK;
	char buf[FPGA_VAR_BUF_LEN] = { 0 };

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
	fpga_result res = FPGA_OK;
	char mac_buf[MACADDR_LEN] = { 0 };
	char mac_count[MACADDR_LEN] = { 0 };
	uint64_t count = 0;
	char *endptr = NULL;

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

	printf("FPGA SmartNIC N6010 Card \n");
	printf("Board Management Controller, MAX10 NIOS FW version: %s \n", bmc_ver);
	printf("Board Management Controller, MAX10 Build version: %s \n", max10_ver);

	return resval;
}

// enumerate hssi feature 0x15
fpga_result enum_hssi_feature(fpga_token token,
		char *feature_dev)
{
	fpga_result res                 = FPGA_OK;
	char sysfs_path[SYSFS_MAX_SIZE] = { 0 };
	uint8_t bus                     = (uint8_t)-1;
	uint16_t segment                = (uint16_t)-1;
	uint8_t device                  = (uint8_t)-1;
	uint8_t function                = (uint8_t)-1;
	size_t i                        = 0;
	uint64_t value                  = 0;
	int gres                        = 0;
	glob_t pglob;

	res = get_fpga_sbdf(token, &segment, &bus, &device, &function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get sbdf ");
		return res;
	}

	if (snprintf(sysfs_path, sizeof(sysfs_path),
		FEATURE_DEV,
		segment, bus, device, function) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	gres = glob(sysfs_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfs_path, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	if (pglob.gl_pathc >= 1) {
		OPAE_ERR("gl_pathc = 1");
		memset(sysfs_path, 0, sizeof(sysfs_path));
		if (snprintf(sysfs_path, sizeof(sysfs_path),
			"%s/feature_id", pglob.gl_pathv[0]) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			res = FPGA_EXCEPTION;
			goto out;
		}

		res = sysfs_read_u64(sysfs_path, &value);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to read sysfs value");
			goto out;
		}

		if (value == HSSI_FEATURE_ID) {
			char *p = strstr(pglob.gl_pathv[i], "dfl_dev");
			if (p == NULL) {
				res = FPGA_NOT_FOUND;
				goto out;
			}

			if (snprintf(feature_dev, SYSFS_MAX_SIZE,
				"%s", p) < 0) {
				OPAE_ERR("snprintf buffer overflow");
				res = FPGA_EXCEPTION;
				goto out;
			}
		}
	} else {
		res = FPGA_NOT_FOUND;
		goto out;
	}


out:
	globfree(&pglob);
	return res;
}

// print phy group information
fpga_result print_phy_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	struct opae_uio uio;
	char feature_dev[SYSFS_MAX_SIZE] = { 0 };
	struct hssi_port_attribute port_profile;
	struct hssi_feature_list  feature_list;
	uint8_t *mmap_ptr = NULL;
	uint32_t *_ptr = NULL;
	uint32_t i = 0;

	res = enum_hssi_feature(token, feature_dev);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get hssi feature");
		return res;
	}
	res = opae_uio_open(&uio, feature_dev);
	if (res) {
		OPAE_ERR("Failed to open uio");
		return res;
	}

	res = opae_uio_region_get(&uio, 0, (uint8_t **)&mmap_ptr, NULL);
	if (res) {
		OPAE_ERR("Failed to get uio region");
		opae_uio_close(&uio);
		return res;
	}
	_ptr = (uint32_t *)mmap_ptr;
	feature_list.csr = *(_ptr + HSSI_FEATURE_LIST);
	for (i = 0; i < feature_list.hssi_num; i++) {
		port_profile.csr = *(_ptr + HSSI_PORT_ATTRIBUTE + i);

		if (port_profile.profile == HSSI_100G_PROFILE) {
			printf("Port%d : %s \n", i, "100GCAUI-4");
		} else if (port_profile.profile == HSSI_25G_PROFILE) {
			printf("Port%d : %s \n", i, "25GbE");
		} else if (port_profile.profile == HSSI_10_PROFILE) {
			printf("Port%d : %s \n", i, "10GbE");
		} else {
			printf("Invalid port ");
		}
	}

	opae_uio_close(&uio);
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
	UNUSED_PARAM(token);
	return FPGA_OK;;
}