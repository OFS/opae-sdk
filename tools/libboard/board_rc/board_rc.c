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
#include <sys/stat.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>

#include "board_rc.h"

// BMC sysfs path
#define SYSFS_DEVID_FILE "avmmi-bmc.*.auto/bmc_info/device_id"
#define SYSFS_RESET_FILE "avmmi-bmc.*.auto/bmc_info/reset_cause"
#define SYSFS_PWRDN_FILE "avmmi-bmc.*.auto/bmc_info/power_down_cause"

#define SYSFS_TCM_GLOB "tcm/*"
#define SYSFS_TCM_BIP_VER "tcm/bip_version"
#define SYSFS_TCM_BMC_CANCEL "tcm/bmc_canceled_csks"
#define SYSFS_TCM_BMC_FLASH_COUNT "tcm/bmc_flash_count"
#define SYSFS_TCM_BMC_FWVERS "tcm/bmcfw_version"
#define SYSFS_TCM_BMC_ROOT "tcm/bmc_root_hash"
#define SYSFS_TCM_CRYPTO_VER "tcm/crypto_version"
#define SYSFS_TCM_PR_CANCEL "tcm/pr_canceled_csks"
#define SYSFS_TCM_PR_ROOT "tcm/pr_root_hash"
#define SYSFS_TCM_QSPI_COUNT "tcm/qspi_flash_count"
#define SYSFS_TCM_SR_CANCEL "tcm/sr_canceled_csks"
#define SYSFS_TCM_SR_ROOT "tcm/sr_root_hash"
#define SYSFS_TCM_FW_VER "tcm/tcmfw_version"
#define FPGA_STR_SIZE     256
#define SDR_HEADER_LEN    3
#define SDR_MSG_LEN       40

typedef struct _bmc_powerdown_cause {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t iana[SDR_HEADER_LEN];
	uint8_t count;
	uint8_t message[SDR_MSG_LEN];
} bmc_powerdown_cause;

typedef struct _bmc_reset_cause {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t iana[SDR_HEADER_LEN];
	uint8_t reset_cause;
} bmc_reset_cause;


typedef enum {
	CHIP_RESET_CAUSE_POR = 0x01,
	CHIP_RESET_CAUSE_EXTRST = 0x02,
	CHIP_RESET_CAUSE_BOD_IO = 0x04,
	CHIP_RESET_CAUSE_WDT = 0x08,
	CHIP_RESET_CAUSE_OCD = 0x10,
	CHIP_RESET_CAUSE_SOFT = 0x20,
	CHIP_RESET_CAUSE_SPIKE = 0x40,
} bmc_ResetCauses;

typedef struct _bmc_device_id {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t device_id;
	union {
		struct {
			uint8_t device_revision : 3;
			uint8_t _unused : 3;
			uint8_t provides_sdrs : 2;
		} bits;
		uint8_t _value;
	} device_revision;
	union {
		struct {
			uint8_t device_available : 7;
			uint8_t major_fw_revision : 1;
		} bits;
		uint8_t _value;
	} firmware_revision_1;
	uint8_t firmware_revision_2;
	uint8_t ipmi_version;
	union {
		struct {
			uint8_t sensor_device : 1;
			uint8_t sdr_repository_device : 1;
			uint8_t sel_device : 1;
			uint8_t fru_inventory_device : 1;
			uint8_t ipmb_event_receiver : 1;
			uint8_t ipmb_event_generator : 1;
			uint8_t bridge : 1;
			uint8_t chassis_device : 1;
		} bits;
		uint8_t _value;
	} additional_device_support;
	uint8_t manufacturer_id_0_7;
	uint8_t manufacturer_id_8_15;
	uint8_t manufacturer_id_16_23;
	uint8_t product_id_0_7;
	uint8_t product_id_8_15;
	uint8_t aux_fw_rev_0_7;
	uint8_t aux_fw_rev_8_15;
	uint8_t aux_fw_rev_16_23;
	uint8_t aux_fw_rev_24_31;
} bmc_device_id;


// Read bmc version
fpga_result read_bmc_version(fpga_token token, int *version)
{
	fpga_result res               = FPGA_OK;
	fpga_result resval            = FPGA_OK;
	bmc_device_id bmc_dev;
	fpga_object bmc_object;

	if (version == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_DEVID_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token Object");
		return res;
	}

	memset(&bmc_dev, 0, sizeof(bmc_dev));

	res = fpgaObjectRead(bmc_object, (uint8_t *)(&bmc_dev), 0, sizeof(bmc_dev), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	*version = bmc_dev.aux_fw_rev_0_7
		| (bmc_dev.aux_fw_rev_8_15 << 8)
		| (bmc_dev.aux_fw_rev_16_23 << 16)
		| (bmc_dev.aux_fw_rev_24_31 << 24);


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}


	return resval;
}

// Read power down cause
fpga_result read_bmc_pwr_down_cause(fpga_token token, char *pwr_down_cause)
{
	fpga_result res               = FPGA_OK;
	fpga_result resval            = FPGA_OK;
	fpga_object bmc_object;
	bmc_powerdown_cause bmc_pd;

	if (pwr_down_cause == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_PWRDN_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token Object");
		return res;
	}

	memset(&bmc_pd, 0, sizeof(bmc_pd));

	res = fpgaObjectRead(bmc_object, (uint8_t *)(&bmc_pd), 0, sizeof(bmc_pd), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	if (bmc_pd.completion_code == 0) {
		strncpy(pwr_down_cause, (char *)bmc_pd.message, bmc_pd.count);
	} else {
		OPAE_ERR("unavailable read power down cause: %d ", bmc_pd.completion_code);
		resval = FPGA_EXCEPTION;
	}


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}

	return resval;
}


// Read reset cause
fpga_result read_bmc_reset_cause(fpga_token token, char *reset_cause_str)
{
	fpga_result res              = FPGA_OK;
	fpga_result resval           = FPGA_OK;
	fpga_object bmc_object;
	bmc_reset_cause bmc_rc;

	if (reset_cause_str == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_RESET_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token Object");
		return res;
	}

	memset(&bmc_rc, 0, sizeof(bmc_rc));

	res = fpgaObjectRead(bmc_object, (uint8_t *)(&bmc_rc), 0, sizeof(bmc_rc), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read Object ");
		resval = res;
		goto out_destroy;
	}

	if (bmc_rc.completion_code != 0) {
		OPAE_ERR("Failed to Read Reset cause \n");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	if (0 == bmc_rc.reset_cause) {
		strncpy(reset_cause_str, "None", 5);
		goto out_destroy;
	}


	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_EXTRST) {
		strncpy(reset_cause_str, "External reset", 15);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_BOD_IO) {
		strncpy(reset_cause_str, "Brown-out detected", 19);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_OCD) {
		strncpy(reset_cause_str, "On-chip debug system", 21);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_POR) {
		strncpy(reset_cause_str, "Power-on-reset", 15);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_SOFT) {
		strncpy(reset_cause_str, "Software reset", 15);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_SPIKE) {
		strncpy(reset_cause_str, "Spike detected", 15);
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_WDT) {
		strncpy(reset_cause_str, "Watchdog timeout", 17);
	}


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}

	return resval;
}

// Print BMC version, Power down cause and Reset cause
fpga_result print_board_info(fpga_token token)
{
	fpga_result res                         = FPGA_OK;
	int version                             = 0;
	char pwr_down_cause[FPGA_STR_SIZE]      = { 0 };
	char reset_cause[FPGA_STR_SIZE]         = { 0 };
	struct stat st;
	fpga_object bmc_object;


	if (!stat("/sys/bus/pci/drivers/dfl-pci", &st)) {
		res = fpgaTokenGetObject(token, SYSFS_DEVID_FILE, &bmc_object, FPGA_OBJECT_GLOB);
		if (res != FPGA_OK) {
			printf("Board Management Controller, microcontroller FW version: %s\n", "Not Supported");
			printf("Last Power down cause: %s\n", "Not Supported");
			printf("Last Reset cause: %s\n", "Not Supported");
			return res;
		}
		res = fpgaDestroyObject(&bmc_object);
		if (res != FPGA_OK) {
			OPAE_ERR("Failed to Destroy Object");
		}

	}

	res = read_bmc_version(token, &version);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read BMC FW version");
	}

	res = read_bmc_pwr_down_cause(token, pwr_down_cause);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read power down cause");
	}

	res = read_bmc_reset_cause(token, reset_cause);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read reset cause");
	}

	// Print BMC info
	printf("Board Management Controller, microcontroller FW version: %d\n", version);
	printf("Last Power down cause:%s\n", pwr_down_cause);
	printf("Last Reset cause: %s\n", reset_cause);

	return res;
}

fpga_result read_sysfs(fpga_token token, char *sysfs_path, char *sysfs_name)
{
	fpga_result res                 = FPGA_OK;
	fpga_result resval              = FPGA_OK;
	uint32_t size                   = 0;
	char name[FPGA_STR_SIZE]        = { 0, };
	fpga_object sec_object;
	size_t len;

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

	len = strnlen(name, FPGA_STR_SIZE - 1);
	strncpy(sysfs_name, name, len + 1);

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

	res = fpgaTokenGetObject(token, SYSFS_TCM_GLOB, &tcm_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	if (read_sysfs(token, SYSFS_TCM_BMC_FWVERS, name) == FPGA_OK)
		printf("BMC FW Version: %s", name);
	else
		OPAE_MSG("Failed to Read BMC FW Version");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_BIP_VER, name) == FPGA_OK)
		printf("BIP Version: %s", name);
	else
		OPAE_MSG("Failed to Read BIP Version");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_FW_VER, name) == FPGA_OK)
		printf("TCM FW Version: %s", name);
	else
		OPAE_MSG("Failed to Read TCM FW Version");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_CRYPTO_VER, name) == FPGA_OK)
		printf("Crypto block Version: %s", name);
	else
		OPAE_MSG("Failed to Read Crypto block Version");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_SR_ROOT, name) == FPGA_OK)
		printf("FIM root entry hash: %s", name);
	else
		OPAE_MSG("Failed to Read FIM root entry hash");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_BMC_ROOT, name) == FPGA_OK)
		printf("BMC root entry hash: %s", name);
	else
		OPAE_MSG("Failed to Read TCM BMC root entry hash");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_PR_ROOT, name) == FPGA_OK)
		printf("PR root entry hash: %s", name);

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_BMC_FLASH_COUNT, name) == FPGA_OK)
		printf("BMC flash update counter: %s", name);
	else
		OPAE_MSG("Failed to Read BMC flash update counter");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_QSPI_COUNT, name) == FPGA_OK)
		printf("User flash update counter: %s", name);
	else
		OPAE_MSG("Failed to Read User flash update counter");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_SR_CANCEL, name) == FPGA_OK)
		printf("FIM CSK IDs canceled : %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read FIM CSK IDs canceled");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_BMC_CANCEL, name) == FPGA_OK)
		printf("BMC CSK IDs canceled: %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read BMC CSK IDs canceled");

	memset(name, 0, sizeof(name));
	if (read_sysfs(token, SYSFS_TCM_PR_CANCEL, name) == FPGA_OK)
		printf("AFU CSK IDs canceled: %s", strlen(name) > 1 ? name : "None\n");
	else
		OPAE_MSG("Failed to Read AFU CSK IDs canceled");

	res = fpgaDestroyObject(&tcm_object);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Object");
	}

	return res;
}
