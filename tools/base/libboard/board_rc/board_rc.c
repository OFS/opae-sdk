// Copyright(c) 2018-2019, Intel Corporation
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

#include "safe_string/safe_string.h"
#include "board_rc.h"

// BMC sysfs path
#define SYSFS_DEVID_FILE "avmmi-bmc.*.auto/bmc_info/device_id"
#define SYSFS_RESET_FILE "avmmi-bmc.*.auto/bmc_info/reset_cause"
#define SYSFS_PWRDN_FILE "avmmi-bmc.*.auto/bmc_info/power_down_cause"

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

	memset_s(&bmc_dev, sizeof(bmc_dev), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&bmc_dev), 0, sizeof(bmc_dev), 0);
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

	memset_s(&bmc_pd, sizeof(bmc_pd), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&bmc_pd), 0, sizeof(bmc_pd), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	if (bmc_pd.completion_code == 0) {
		snprintf_s_s(pwr_down_cause, bmc_pd.count, "%s", (char*)bmc_pd.message);
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

	memset_s(&bmc_rc, sizeof(bmc_rc), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&bmc_rc), 0, sizeof(bmc_rc), 0);
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
		snprintf_s_s(reset_cause_str, 256, "%s", "None");
		goto out_destroy;
	}


	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_EXTRST) {
		snprintf_s_s(reset_cause_str, 256, "%s", "External reset");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_BOD_IO) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Brown-out detected");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_OCD) {
		snprintf_s_s(reset_cause_str, 256, "%s", "On-chip debug system");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_POR) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Power-on-reset");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_SOFT) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Software reset");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_SPIKE) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Spike detected");
	}

	if (bmc_rc.reset_cause & CHIP_RESET_CAUSE_WDT) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Watchdog timeout");
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

	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}
	return res;
}