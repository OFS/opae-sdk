// Copyright(c) 2023, Intel Corporation
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


#include <limits.h>
#include <glob.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include "../board_common/board_common.h"
#include "board_cmc.h"
#include "mock/opae_std.h"

#define FPGA_VAR_BUF_LEN       256
#define MAC_BUF_LEN            19
#define UNUSED_PARAM(x) ((void)x)
#define FEATURE_DEV "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region"\
					"/region*/dfl-fme*/dfl_dev*"
// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                     "dfl*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                     "dfl*/bmc_version"

#define DFL_SYSFS_MACADDR_PATH                  "dfl*/mac_address"
#define DFL_SYSFS_MACCNT_PATH                   "dfl*/mac_count"

#define DFL_SEC_PMCI_GLOB "*dfl*/**/security/"

typedef struct fpga_sec_key {
	const char *name;
	const char *sysfs;
} fpga_sec_key;

#define SEC_ARRAY_MAX_SIZE 7

fpga_sec_key sec_key_data[] = {
	{.name = "BMC root entry hash",
	 .sysfs = "*bmc_root_entry_hash"
	},
	{.name = "BMC CSK IDs canceled",
	 .sysfs = "*bmc_canceled_csks"
	},
	{.name = "PR root entry hash",
	 .sysfs = "*pr_root_entry_hash"
	},
	{.name = "AFU/PR CSK IDs canceled",
	 .sysfs = "*pr_canceled_csks"
	},
	{.name = "FIM root entry hash",
	 .sysfs = "*sr_root_entry_hash"
	},
	{.name = "FIM CSK IDs canceled",
	 .sysfs = "*sr_canceled_csks"
	},
	{.name = "User flash update counter",
	 .sysfs = "*flash_count"
	},
	{}
};

// boot page info sysfs
#define DFL_SYSFS_BOOT_GLOB "*dfl*/**/fpga_boot_image"
#define BOOTPAGE_PATTERN "_([0-9a-zA-Z]+)"

// image info sysfs
#define DFL_SYSFS_IMAGE_INFO_GLOB "*dfl*/**/fpga_image_directory*/nvmem"
#define IMAGE_INFO_STRIDE 0x10000
#define IMAGE_INFO_SIZE     32
#define IMAGE_INFO_COUNT     3
#define GET_BIT(var, pos) ((var >> pos) & (1))

// BOM info
#define DFL_SYSFS_BOM_INFO_GLOB "*dfl*/**/bom_info*/nvmem"
#define FPGA_BOM_INFO_BUF_LEN   0x2000


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



// Read BOM Critical Components info from the FPGA
static fpga_result read_bom_info(
	const fpga_token token,
	char * const bom_info,
	const size_t len)
{
	if (bom_info == NULL)
		return FPGA_INVALID_PARAM;

	fpga_result resval = FPGA_OK;
	fpga_object fpga_object;

	fpga_result res = fpgaTokenGetObject(token, DFL_SYSFS_BOM_INFO_GLOB,
					     &fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		// Simulate reading of empty BOM info filled with 0xFF
		// so that FPGA with no BOM info produces no output.
		// Return FPGA_OK!
		memset(bom_info, 0xFF, len);
		return FPGA_OK;
	}

	res = fpgaObjectRead(fpga_object, (uint8_t *)bom_info, 0, len, FPGA_OBJECT_RAW);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read BOM info");
		memset(bom_info, 0xFF, len); // Simulate reading of empty BOM info filled with 0xFF
		resval = res;
	}

	res = fpgaDestroyObject(&fpga_object);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Object");
		if (resval == FPGA_OK)
			resval = res;
	}

	return resval;
}


// print BOM info
fpga_result print_bom_info(const fpga_token token)
{
	fpga_result resval = FPGA_OK;
	const size_t max_result_len = 2 * FPGA_BOM_INFO_BUF_LEN;
	char * const bom_info = (char *)opae_malloc(max_result_len);

	if (bom_info == NULL)
		return FPGA_NO_MEMORY;

	fpga_result res = read_bom_info(token, bom_info, FPGA_BOM_INFO_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read BOM info");
		opae_free(bom_info);
		return res;
	}

	// Terminated by a null character '\0'
	bom_info[FPGA_BOM_INFO_BUF_LEN] = '\0';

	res = reformat_bom_info(bom_info, FPGA_BOM_INFO_BUF_LEN, max_result_len);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to reformat BOM info");
		if (resval == FPGA_OK)
			resval = res;
	}

	printf("%s", bom_info);

	opae_free(bom_info);

	return resval;
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

	printf("Board Management Controller NIOS FW version: %s \n", bmc_ver);
	printf("Board Management Controller Build version: %s \n", max10_ver);

	res = print_bom_info(token);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to print BOM info");
		if (resval == FPGA_OK)
			resval = res;
	}

	return resval;
}

// Sec info
fpga_result print_sec_info(fpga_token token)
{
	fpga_result res                   = FPGA_OK;
	fpga_result resval                = FPGA_OK;
	fpga_object tcm_object;
	char name[SYSFS_PATH_MAX]         = { 0 };
	char sysfs_path[SYSFS_PATH_MAX]   = { 0 };
	fpga_sec_key *p                   = NULL;

	res = fpgaTokenGetObject(token, DFL_SEC_PMCI_GLOB, &tcm_object,
		FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}
	printf("********** SEC Info START ************ \n");

	for (p = sec_key_data; p->name; p++) {
		memset(name, 0, sizeof(name));
		memset(sysfs_path, 0, sizeof(sysfs_path));

		if (snprintf(sysfs_path, sizeof(sysfs_path),
			"%s/%s", DFL_SEC_PMCI_GLOB, p->sysfs) < 0) {
			OPAE_ERR("snprintf failed");
			resval = FPGA_EXCEPTION;
			goto exit;
		}

		res = read_sysfs(token, sysfs_path, name, SYSFS_PATH_MAX - 1);
		if (res == FPGA_OK) {
			printf("%-32s : %s\n", p->name,
				strlen(name) > 0 ? name : "None");
		} else {
			OPAE_MSG("Failed to Read %s", p->name);
			printf("%-32s : %s\n", p->name, "None");
			resval = res;
		}
	}

exit:
	res = fpgaDestroyObject(&tcm_object);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Object");
		resval = res;
	}

	printf("********** SEC Info END ************ \n");

	return resval;
}


// prints fpga boot page info
fpga_result fpga_boot_info(fpga_token token)
{
	char boot[SYSFS_PATH_MAX] = { 0 };
	char page[SYSFS_PATH_MAX] = { 0 };
	fpga_result res           = FPGA_OK;
	int reg_res               = 0;
	char err[128]           = { 0 };
	regex_t re;
	regmatch_t matches[3];

	// boot page
	memset(boot, 0, sizeof(boot));
	res = read_sysfs(token, DFL_SYSFS_BOOT_GLOB, boot, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {

		reg_res = regcomp(&re, BOOTPAGE_PATTERN, REG_EXTENDED | REG_ICASE);
		if (reg_res) {
			OPAE_ERR("Error compiling regex");
			return FPGA_EXCEPTION;
		}

		reg_res = regexec(&re, boot, 3, matches, 0);
		if (reg_res) {
			regerror(reg_res, &re, err, sizeof(err));
			OPAE_MSG("Error executing regex: %s", err);
			regfree(&re);
			return FPGA_EXCEPTION;
		}
		memcpy(page, boot + matches[0].rm_so + 1,
			matches[0].rm_eo - (matches[0].rm_so + 1));
		page[matches[0].rm_eo - (matches[0].rm_so + 1)] = '\0';

		printf("%-32s : %s\n", "Boot Page", page);
		regfree(&re);
	} else {
		OPAE_MSG("Failed to Read Boot Page");
		printf("%-32s : %s\n", "Boot Page", "N/A");
	}

	return res;
}

// prints fpga image info
fpga_result fpga_image_info(fpga_token token)
{
	const char *image_info_label[IMAGE_INFO_COUNT] = {
	"User1 Image Info",
	"User2 Image Info",
	"Factory Image Info"
	};
	fpga_object fpga_object;
	fpga_result res;
	size_t i;

	res = fpgaTokenGetObject(token, DFL_SYSFS_IMAGE_INFO_GLOB,
			&fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	for (i = 0; i < IMAGE_INFO_COUNT; i++) {
		size_t offset = IMAGE_INFO_STRIDE * i;
		uint8_t data[IMAGE_INFO_SIZE + 1] = { 0 };
		char *image_info = (char *)data;
		size_t p;

		printf("%-32s : ", image_info_label[i]);

		res = fpgaObjectRead(fpga_object, data, offset,
				IMAGE_INFO_SIZE, FPGA_OBJECT_RAW);
		if (res != FPGA_OK) {
			printf("N/A\n");
			continue;
		}

		for (p = 0; p < IMAGE_INFO_SIZE; p++)
			if (data[p] != 0xff)
				break;

		if (p >= IMAGE_INFO_SIZE) {
			printf("None\n");
			continue;
		}

		if (strlen(image_info) == 0) {
			printf("Empty\n");
			continue;
		}

		printf("%s\n", image_info);
	}

	if (fpgaDestroyObject(&fpga_object) != FPGA_OK)
		OPAE_ERR("Failed to Destroy Object");

	return res;
}

