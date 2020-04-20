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

#include "board_dc.h"

#define MACADDR_LEN 17
#define FPGA_STR_SIZE     256
#define SDR_HEADER_LEN    3
#define SDR_MSG_LEN       40

// sysfs paths
#define SYSFS_MACADDR_PATH                  "spi-*/spi_master/spi*/spi*.*/mac_address"
#define SYSFS_MACCNT_PATH                   "spi-*/spi_master/spi*/spi*.*/mac_count"

// DFL SYSFS
#define DFL_SYSFS_BMCFW_VER                 "dfl-fme*/spi-altera*/spi_master/spi*/spi*/bmcfw_version"
#define DFL_SYSFS_MAX10_VER                 "dfl-fme*/spi-altera*/spi_master/spi*/spi*/bmc_version"

#define DFL_SYSFS_SEC_GLOB "dfl-fme*/spi-altera*/spi_master/spi*/spi*/ifpga_sec_mgr/ifpga_sec*/security/"
#define DFL_SYSFS_SEC_USER_FLASH_COUNT         DFL_SYSFS_SEC_GLOB "user_flash_count"
#define DFL_SYSFS_SEC_BMC_CANCEL               DFL_SYSFS_SEC_GLOB "bmc_canceled_csks"
#define DFL_SYSFS_SEC_BMC_ROOT                 DFL_SYSFS_SEC_GLOB "bmc_root_hash"
#define DFL_SYSFS_SEC_PR_CANCEL                DFL_SYSFS_SEC_GLOB "pr_canceled_csks"
#define DFL_SYSFS_SEC_PR_ROOT                  DFL_SYSFS_SEC_GLOB"pr_root_hash"
#define DFL_SYSFS_SEC_SR_CANCEL                DFL_SYSFS_SEC_GLOB "sr_canceled_csks"
#define DFL_SYSFS_SEC_SR_ROOT                  DFL_SYSFS_SEC_GLOB "sr_root_hash"


// Read BMC firmware version
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_ver, size_t len)
{
	fpga_result res                = FPGA_OK;
	fpga_result resval             = FPGA_OK;
	uint32_t size                  = 0;
	char buf[FPGA_VAR_BUF_LEN]     = { 0 };
	fpga_object bmcfw_object;

	if (bmcfw_ver == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, DFL_SYSFS_BMCFW_VER, &bmcfw_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return res;
	}

	res = fpgaObjectGetSize(bmcfw_object, &size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object size ");
		resval = res;
		goto out_destroy;
	}

	// Return error if object size bigger then buffer size
	if (size > FPGA_VAR_BUF_LEN) {
		OPAE_ERR("object size bigger then buffer size");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	res = fpgaObjectRead(bmcfw_object, (uint8_t *)buf, 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy;
	}

	res = parse_fw_ver(buf, bmcfw_ver, len);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to parse version ");
		resval = res;
	}


out_destroy:
	res = fpgaDestroyObject(&bmcfw_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}

fpga_result parse_fw_ver(char *buf, char *fw_ver, size_t len)
{
	uint32_t var               = 0;
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
	if (var == 0 &&
		errno != 0) {
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
	fpga_result res                      = FPGA_OK;
	fpga_result resval                   = FPGA_OK;
	uint32_t size                        = 0;
	char buf[FPGA_VAR_BUF_LEN]           = { 0 };
	fpga_object max10fw_object;

	if (max10fw_ver == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, DFL_SYSFS_MAX10_VER, &max10fw_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return res;
	}

	res = fpgaObjectGetSize(max10fw_object, &size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size ");
		resval = res;
		goto out_destroy;
	}

	// Return error if object size bigger then buffer size
	if (size > FPGA_VAR_BUF_LEN) {
		OPAE_ERR("object size bigger then buffer size");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	res = fpgaObjectRead(max10fw_object, (uint8_t *)buf, 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy;
	}

	res = parse_fw_ver(buf, max10fw_ver, len);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to parse version ");
		resval = res;
	}

out_destroy:
	res = fpgaDestroyObject(&max10fw_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}


// Read mac information
fpga_result read_mac_info(fpga_token token, uint32_t afu_channel_num,
							struct ether_addr *mac_addr)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	char macaddr_buf[MACADDR_LEN];
	fpga_object mac_addr_obj;
	fpga_object mac_channel_obj;
	uint64_t count;

	if (mac_addr == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_MACADDR_PATH, &mac_addr_obj, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return res;
	}

	res = fpgaObjectRead(mac_addr_obj, (uint8_t *)macaddr_buf, 0, sizeof(macaddr_buf), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_mac;
	}


	ether_aton_r(macaddr_buf, mac_addr);

	res = fpgaTokenGetObject(token, SYSFS_MACCNT_PATH, &mac_channel_obj, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		resval = res;
		goto out_destroy_mac;
	}

	res = fpgaObjectRead64(mac_channel_obj, &count, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_channel;
	}

	if (afu_channel_num >= count) {
		resval = FPGA_INVALID_PARAM;
		OPAE_ERR("Invalid Input parameters");
		goto out_destroy_channel;
	}



	if ((mac_addr->ether_addr_octet[0] == 0xff) &&
		(mac_addr->ether_addr_octet[1] == 0xff) &&
		(mac_addr->ether_addr_octet[2] == 0xff) &&
		(mac_addr->ether_addr_octet[3] == 0xff) &&
		(mac_addr->ether_addr_octet[4] == 0xff) &&
		(mac_addr->ether_addr_octet[5] == 0xff)) {

		resval = FPGA_NOT_FOUND;
		OPAE_ERR("Invalid MAC address");
		goto out_destroy_channel;
	}

	mac_addr->ether_addr_octet[5] += afu_channel_num;

out_destroy_channel:
	res = fpgaDestroyObject(&mac_channel_obj);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
		resval = res;
	}

out_destroy_mac:
	res = fpgaDestroyObject(&mac_addr_obj);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
		resval = res;
	}

	return resval;
}


// print board information
fpga_result print_board_info(fpga_token token)
{
	fpga_result res                      = FPGA_OK;
	fpga_result resval                   = FPGA_OK;
	char bmc_ver[FPGA_VAR_BUF_LEN]       = { 0 };
	char max10_ver[FPGA_VAR_BUF_LEN]     = { 0 };
	char mac_str[18] = { 0 };
	struct ether_addr MAC ;


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

	memset((void *)&MAC, 0, sizeof(MAC));

	res = read_mac_info(token, 0, &MAC);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac address");
		resval = res;
	} else {
		printf("%-1s : %s\n", "MAC address",
			ether_ntoa_r(&MAC, mac_str));

	}

	return resval;
}

// Read sysfs
fpga_result read_sysfs(fpga_token token, char *sysfs_path, char *sysfs_name)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	uint32_t size = 0;
	char name[FPGA_STR_SIZE] = { 0 };
	fpga_object sec_object;

	if (sysfs_path == NULL ||
		sysfs_name == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, sysfs_path, &sec_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token Object");
		return res;
	}

	res = fpgaObjectGetSize(sec_object, &size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size ");
		resval = res;
		goto out_destroy;
	}

	if (size > FPGA_STR_SIZE) {
		OPAE_ERR("object size bigger then buffer size");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	res = fpgaObjectRead(sec_object, (uint8_t *)(&name), 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	snprintf(sysfs_name, FPGA_STR_SIZE, "%s", (char *)name);

out_destroy:
	res = fpgaDestroyObject(&sec_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}

	return resval;
}


fpga_result print_sec_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	fpga_object tcm_object;
	char name[FPGA_STR_SIZE] = { 0 };

	res = fpgaTokenGetObject(token, DFL_SYSFS_SEC_GLOB, &tcm_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}
	printf("********** SEC Info START ************ \n");

	// BMC Keys
	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_BMC_ROOT, name) == FPGA_OK)
		printf("BMC root entry hash: %s", name);
	else
		OPAE_MSG("Failed to Read TCM BMC root entry hash");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_BMC_CANCEL, name) == FPGA_OK)
		printf("BMC CSK IDs canceled: %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read BMC CSK IDs canceled");

	// PR Keys
	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_PR_ROOT, name) == FPGA_OK)
		printf("PR root entry hash: %s", name);
	else
		OPAE_MSG("Failed to Read PR root entry hash");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_PR_CANCEL, name) == FPGA_OK)
		printf("AFU/PR CSK IDs canceled: %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read AFU CSK/PR IDs canceled");

	// SR Keys
	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_SR_ROOT, name) == FPGA_OK)
		printf("FIM root entry hash: %s", name);
	else
		OPAE_MSG("Failed to Read FIM root entry hash");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_SR_CANCEL, name) == FPGA_OK)
		printf("FIM CSK IDs canceled : %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read FIM CSK IDs canceled");

	// User flash count
	memset(name, 0, sizeof(name));
	if (read_sysfs(token, DFL_SYSFS_SEC_USER_FLASH_COUNT, name) == FPGA_OK)
		printf("User flash update counter: %s", name);
	else
		OPAE_MSG("Failed to Read User flash update counter");

	res = fpgaDestroyObject(&tcm_object);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Object");
	}

	printf("********** SEC Info END ************ \n");

	return res;
}
