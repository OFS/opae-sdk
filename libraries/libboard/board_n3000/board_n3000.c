// Copyright(c) 2019-2022, Intel Corporation
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
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <opae/uio.h>
#include "../board_common/board_common.h"
#include "board_n3000.h"
#include "mock/opae_std.h"

// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                  "dfl*/*spi*/spi_master/spi*/spi*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                  "dfl*/*spi*/spi_master/spi*/spi*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH               "dfl*/*spi*/spi_master/spi*/spi*.*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                "dfl*/*spi*/spi_master/spi*/spi*.*/mac_count"

#define DFL_BITSTREAM_ID                      "bitstream_id"

#define FPGA_BSID_SIZE                        32
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

#define ETH_GROUP_FEATURE_ID                  0x10
#define MAX_LINE_LENGTH                       80
#define MAX_NUM_LINES                         0x500
#define MAX10_REG_BASE                        0x300800
#define MAX10_PKVL_LINK_STATUS                0x164
#define MAX10_PKVL1_VAR                       0x254
#define MAX10_PKVL2_VAR                       0x258

#define FEATURE_DEV "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region"\
					"/region*/dfl-fme*/dfl_dev*"
#define FEATURE_DEV_SPI "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region"\
					"/region*/dfl-fme*/dfl_dev*/*spi*/spi_master/spi*/spi*"

// Eth group CSR
struct eth_group_info {
	union {
		uint64_t csr;
		struct {
			uint64_t  group_num : 8;
			uint64_t phy_num : 8;
			uint64_t speed : 8;
			uint64_t direction : 1;
			uint64_t light_wight_mac : 1;
			uint64_t reserved : 38;
		};
	};
};

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
	uint8_t rev                = 0;
	uint32_t var               = 0;
	fpga_result res            = FPGA_OK;
	int retval                 = 0;
	char *endptr               = NULL;
	if (buf == NULL ||
		fw_ver == NULL) {
		OPAE_ERR("Invalid Input parameters");
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
			OPAE_ERR("error in formatting version");
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
		OPAE_ERR("Invalid input parameters");
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

fpga_result enum_eth_group_feature(fpga_token token,
			char eth_feature[ETH_GROUP_COUNT][SYSFS_MAX_SIZE],
			uint32_t size)
{
	fpga_result res                    = FPGA_OK;
	char sysfs_path[SYSFS_MAX_SIZE]    = { 0 };
	uint32_t index                     = 0;
	uint8_t bus                        = (uint8_t)-1;
	uint16_t segment                   = (uint16_t)-1;
	uint8_t device                     = (uint8_t)-1;
	uint8_t function                   = (uint8_t)-1;
	size_t i                           = 0;
	uint64_t value                     = 0;
	int gres                           = 0;
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

	gres = opae_glob(sysfs_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfs_path, strerror(errno));
		opae_globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	// for loop
	for (i = 0; i < pglob.gl_pathc; i++) {
		memset(sysfs_path, 0, sizeof(sysfs_path));
		if (snprintf(sysfs_path, sizeof(sysfs_path),
			"%s/feature_id", pglob.gl_pathv[i]) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			res = FPGA_EXCEPTION;
			goto out;
		}

		res = sysfs_read_u64(sysfs_path, &value);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to read sysfs value");
			goto out;
		}

		if (value == ETH_GROUP_FEATURE_ID) {
			char *p = strstr(pglob.gl_pathv[i], "dfl_dev");
			if (p == NULL) {
				res = FPGA_NOT_FOUND;
				goto out;
			}

			if (snprintf(eth_feature[index], SYSFS_MAX_SIZE,
				"%s", p) < 0) {
				OPAE_ERR("snprintf buffer overflow");
				res = FPGA_EXCEPTION;
				goto out;
			}
			index = index + 1;
			if (index >= size) {
				goto out;
			}
		}
	}

out:
	opae_globfree(&pglob);
	return res;
}


//enum pkvl regmap path
// /sys/kernel/debug/regmap/spi%d.0/registers
fpga_result enum_pkvl_sysfs_path(fpga_token token,
			char *pkvl_path)
{

	char sysfs_path[SYSFS_MAX_SIZE]    = { 0 };
	fpga_result res                    = FPGA_OK;
	uint8_t bus                        = (uint8_t)-1;
	uint16_t segment                   = (uint16_t)-1;
	uint8_t device                     = (uint8_t)-1;
	uint8_t function                   = (uint8_t)-1;
	int gres                           = 0;
	glob_t pglob;

	if (!pkvl_path) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = get_fpga_sbdf(token, &segment, &bus, &device, &function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get sbdf ");
		return res;
	}

	if (snprintf(sysfs_path, sizeof(sysfs_path),
			FEATURE_DEV_SPI,
			segment, bus, device, function) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	gres = opae_glob(sysfs_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfs_path, strerror(errno));
		opae_globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	if (pglob.gl_pathc == 1) {

		char *p = strrchr(pglob.gl_pathv[0], '/');
		if (p == NULL) {
			res = FPGA_INVALID_PARAM;
			goto out;
		}

		if (snprintf(pkvl_path, SYSFS_MAX_SIZE,
			"/sys/kernel/debug/regmap/%s/registers", p+1) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			res = FPGA_EXCEPTION;
			goto out;
		}
	} else {
		res = FPGA_NOT_FOUND;
		goto out;
	}

out:
	opae_globfree(&pglob);
	return res;
}

//enum pkvl regmap path
fpga_result read_regmap(char *sysfs_path,
			uint64_t index,
			uint32_t *value)
{
	FILE *fp                          = NULL;
	char *endptr                      = NULL;
	uint64_t line_count               = 0;
	char search_str[SYSFS_MAX_SIZE]   = { 0 };
	char line[MAX_LINE_LENGTH]        = { 0 };

	if (!value || !sysfs_path) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	if (snprintf(search_str, SYSFS_MAX_SIZE,
				"%lx", index) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	fp = opae_fopen(sysfs_path, "r");
	if (!fp) {
		OPAE_ERR("Error opening:%s  %s", sysfs_path, strerror(errno));
		return FPGA_EXCEPTION;
	}

	while (fgets(line, MAX_LINE_LENGTH, fp)) {

		if (strstr(line, search_str)) {
			char *p = strstr(line, ":");
			if (p == NULL) {
				opae_fclose(fp);
				return FPGA_NOT_FOUND;
			}
			*value = strtoul(p + 1, &endptr, 16);
			opae_fclose(fp);
			return FPGA_OK;

		}
		if (line_count > MAX_NUM_LINES) {
			OPAE_ERR("Not found in regmap");
			return FPGA_NOT_FOUND;
		}

	}

	opae_fclose(fp);

	return FPGA_NOT_FOUND;
}

// print retimer info
fpga_result print_retimer_info(fpga_token token,
			uint32_t speed)
{
	fpga_result res                   = FPGA_OK;
	uint32_t link_status              = 0;
	uint32_t fpga_mode                = 0;
	char sysfs_path[SYSFS_MAX_SIZE]   = { 0 };
	char mode[VER_BUF_SIZE]           = { 0 };
	uint32_t mask                     = 0;
	uint64_t bs_id                    = 0;
	uint32_t i                        = 0;
	uint32_t j                        = 0;

	res = enum_pkvl_sysfs_path(token, sysfs_path);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to enum sysfs path");
		return res;
	}

	// read regmap
	res = read_regmap(sysfs_path,
					MAX10_REG_BASE + MAX10_PKVL_LINK_STATUS,
					&link_status);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read regmap");
		return res;
	}

	// read bistream Id
	res = read_sysfs_int64(token, DFL_BITSTREAM_ID, &bs_id);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read feature id");
	}

	fpga_mode = (bs_id >> FPGA_BSID_SIZE) & 0xf;

	if (speed == 10) {
		/* 8x10g */
		mask = 0xff;
	} else if (speed == 25) {
		switch (fpga_mode) {
		case 1: /* 4x25g */
		case 3: /* 6x25g */
			mask = 0xf;
			break;
		case 2: /* 2x1x25g */
		case 5: /* 2x1x25gx2FVL */
			mask = 0x11;
			break;
		case 4: /* 2x2x25g */
			mask = 0x33;
			break;
		case 6: /* 1x2x25g */
			mask = 0x03;
			break;
		default:
			mask = 0x0;
			break;
		}
	}

	printf("//****** Intel C827 Retimer ******//\n");
	strncpy(mode, speed == 25 ? "25G" : "10G", 4);

	for (i = 0, j = 0; i < MAX_PORTS; i++) {
		if (mask&(1 << i)) {
			printf("Port%-2d%-26s : %s\n", j, mode,
				link_status&(1 << i) ? "Up" : "Down");
			j++;
		}
	}

	return res;
}

// Read pkvl versoin
fpga_result print_pkvl_version(fpga_token token)
{
	fpga_result res                    = FPGA_OK;
	uint32_t value                     = 0;
	char sysfs_path[SYSFS_MAX_SIZE]    = { 0 };
	char ver_buf[FPGA_VAR_BUF_LEN]     = { 0 };

	res = enum_pkvl_sysfs_path(token, sysfs_path);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to enum sysfs path");
		return res;
	}

	res = read_regmap(sysfs_path,
				MAX10_REG_BASE + MAX10_PKVL1_VAR,
				&value);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read regmap");
		return res;
	}

	if (snprintf(ver_buf, FPGA_VAR_BUF_LEN, "%x.%x",
				value >> 16, value & 0x0000ffff) < 0) {
		OPAE_ERR("error in formatting version");
		return FPGA_EXCEPTION;
	}

	printf("%-32s : %s \n", "Retimer A Version", ver_buf);

	res = read_regmap(sysfs_path,
				MAX10_REG_BASE + MAX10_PKVL2_VAR,
				&value);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy group count");
		return res;
	}

	if (snprintf(ver_buf, FPGA_VAR_BUF_LEN, "%x.%x",
				value >> 16, value & 0x0000ffff) < 0) {
		OPAE_ERR("error in formatting version");
		return FPGA_EXCEPTION;
	}

	printf("%-32s : %s \n", "Retimer B Version", ver_buf);

	return res;
}

// print phy group information
fpga_result print_phy_info(fpga_token token)
{
	fpga_result res            = FPGA_OK;
	uint8_t *mmap_ptr          = NULL;
	uint32_t i                 = 0;
	uint64_t *_ptr             = NULL;
	uint32_t speed             = 10;
	struct opae_uio uio;
	struct eth_group_info eth_info;
	char eth_feature_dev[ETH_GROUP_COUNT][SYSFS_MAX_SIZE];

	res = enum_eth_group_feature(token,
						eth_feature_dev,
						ETH_GROUP_COUNT);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read eth group feature");
		return res;
	}

	for (i = 0; i < ETH_GROUP_COUNT; i++) {
		res = opae_uio_open(&uio, eth_feature_dev[i]);
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
		_ptr = (uint64_t *)mmap_ptr;

		eth_info.csr = *(_ptr + 1);

		printf("//****** PHY GROUP %d ******//\n", eth_info.group_num);
		printf("%-32s : %s\n", "Direction",
			eth_info.direction == 0 ? "Line side" : "Host side");

		printf("%-32s : %d Gbps\n", "Speed", eth_info.speed);
		printf("%-32s : %d\n", "Number of PHYs", eth_info.phy_num);

		if (eth_info.group_num == 0) {
			speed = eth_info.speed;
		}

		opae_uio_close(&uio);
	}

	res = print_retimer_info(token, speed);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to print retimer info");
		return res;
	}

	res = print_pkvl_version(token);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to print pkvl version");
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

// prints fpga boot page info
fpga_result fpga_boot_info(fpga_token token)
{
	return print_common_boot_info(token);
}
