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
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>

#include "safe_string/safe_string.h"
#include "board_rc.h"

// BMC sysfs path
#define SYSFS_DEVID_FILE "avmmi-bmc.*.auto/bmc_info/device_id"
#define SYSFS_RESET_FILE "avmmi-bmc.*.auto/bmc_info/reset_cause"
#define SYSFS_PWRDN_FILE "avmmi-bmc.*.auto/bmc_info/power_down_cause"

#define FPGA_STR_SIZE   256

// Read bmc version
fpga_result read_bmc_version(fpga_token token, int *version)
{
	fpga_result res               = FPGA_OK;
	fpga_result resval            = FPGA_OK;
	device_id dev;
	fpga_object bmc_object;
	fpga_handle handle;

	if (version == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaOpen(token, &handle, FPGA_OPEN_SHARED);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to open fpga device");
		resval = res;
		goto out;
	}

	res = fpgaHandleGetObject(handle, SYSFS_DEVID_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get handle Object");
		resval = res;
		goto out_close;
	}

	memset_s(&dev, sizeof(dev), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&dev), 0, sizeof(dev), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	*version = dev.aux_fw_rev_0_7
		| (dev.aux_fw_rev_8_15 << 8)
		| (dev.aux_fw_rev_16_23 << 16)
		| (dev.aux_fw_rev_24_31 << 24);


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}

out_close:
	res = fpgaClose(handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to close handle");
		resval = res;
	}

out:
	return resval;
}

// Read power down cause
fpga_result read_bmc_pwr_down_cause(fpga_token token, char *pwr_down_cause)
{
	fpga_result res               = FPGA_OK;
	fpga_result resval            = FPGA_OK;
	fpga_object bmc_object;
	fpga_handle  handle;
	powerdown_cause pd;

	if (pwr_down_cause == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaOpen(token, &handle, FPGA_OPEN_SHARED);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to open fpga device");
		resval = res;
		goto out;
	}

	res = fpgaHandleGetObject(handle, SYSFS_PWRDN_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get handle Object");
		resval = res;
		goto out_close;
	}

	memset_s(&pd, sizeof(pd), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&pd), 0, sizeof(pd), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	if (pd.completion_code == 0) {
		snprintf_s_s(pwr_down_cause, pd.count, "%s", (char*)pd.message);
	} else {
		OPAE_ERR("unavailable read power down cause: %d ", pd.completion_code);
		resval = FPGA_EXCEPTION;
	}


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}

out_close:
	res = fpgaClose(handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to close handle");
		resval = res;
	}

out:
	return resval;
}


// Read reset cause
fpga_result read_bmc_reset_cause(fpga_token token, char *reset_cause_str)
{
	fpga_result res              = FPGA_OK;
	fpga_result resval           = FPGA_OK;
	fpga_object bmc_object;
	fpga_handle  handle;
	reset_cause rc;

	if (reset_cause_str == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaOpen(token, &handle, FPGA_OPEN_SHARED);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to open fpga device");
		resval = res;
		goto out;
	}

	res = fpgaHandleGetObject(handle, SYSFS_RESET_FILE, &bmc_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get handle Object");
		resval = res;
		goto out_close;
	}

	memset_s(&rc, sizeof(rc), 0);

	res = fpgaObjectRead(bmc_object, (uint8_t*)(&rc), 0, sizeof(rc), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read Object ");
		resval = res;
		goto out_destroy;
	}

	if (rc.completion_code != 0) {
		OPAE_ERR("Failed to Read Reset cause \n");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	if (0 == rc.reset_cause) {
		snprintf_s_s(reset_cause_str, 256, "%s", "None");
		goto out_destroy;
	}


	if (rc.reset_cause & CHIP_RESET_CAUSE_EXTRST) {
		snprintf_s_s(reset_cause_str, 256, "%s", "External reset");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_BOD_IO) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Brown-out detected");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_OCD) {
		snprintf_s_s(reset_cause_str, 256, "%s", "On-chip debug system");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_POR) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Power-on-reset");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_SOFT) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Software reset");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_SPIKE) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Spike detected");
	}

	if (rc.reset_cause & CHIP_RESET_CAUSE_WDT) {
		snprintf_s_s(reset_cause_str, 256, "%s", "Watchdog timeout");
	}


out_destroy:
	res = fpgaDestroyObject(&bmc_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}

out_close:
	res = fpgaClose(handle);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to close handle");
		resval = res;
	}

out:
	return resval;
}

// Print BMC version, Power down cause and Reset cause
fpga_result print_borad_info(fpga_token token)
{
	fpga_result res                         = FPGA_OK;
	int version                             = 0;
	char pwr_down_cause[FPGA_STR_SIZE]      = { 0 };
	char reset_cause[FPGA_STR_SIZE]         = { 0 };

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