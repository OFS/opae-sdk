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

#include "board_common.h"


#define DFL_SYSFS_SEC_GLOB "dfl-fme*/*spi*/spi_master/spi*/spi*/m10bmc-*/ifpga_sec_mgr/ifpga_sec*/security/"
#define DFL_SYSFS_SEC_USER_FLASH_COUNT         DFL_SYSFS_SEC_GLOB "user_flash_count"
#define DFL_SYSFS_SEC_BMC_CANCEL               DFL_SYSFS_SEC_GLOB "bmc_canceled_csks"
#define DFL_SYSFS_SEC_BMC_ROOT                 DFL_SYSFS_SEC_GLOB "bmc_root_entry_hash"
#define DFL_SYSFS_SEC_PR_CANCEL                DFL_SYSFS_SEC_GLOB "pr_canceled_csks"
#define DFL_SYSFS_SEC_PR_ROOT                  DFL_SYSFS_SEC_GLOB "pr_root_entry_hash"
#define DFL_SYSFS_SEC_SR_CANCEL                DFL_SYSFS_SEC_GLOB "sr_canceled_csks"
#define DFL_SYSFS_SEC_SR_ROOT                  DFL_SYSFS_SEC_GLOB "sr_root_entry_hash"

// Read sysfs
fpga_result read_sysfs(fpga_token token, char *sysfs_path,
		char *sysfs_name, size_t len)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	uint32_t size = 0;
	char name[SYSFS_PATH_MAX] = { 0 };
	fpga_object fpga_object;

	if (sysfs_path == NULL ||
		sysfs_name == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, sysfs_path, &fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	res = fpgaObjectGetSize(fpga_object, &size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size ");
		resval = res;
		goto out_destroy;
	}

	if (size > len) {
		OPAE_ERR("object size bigger then buffer size");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	res = fpgaObjectRead(fpga_object, (uint8_t *)(&name), 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
		resval = res;
		goto out_destroy;
	}

	len = strnlen(name, len - 1);
	memcpy(sysfs_name, name, len);
	sysfs_name[len] = '\0';
	if (sysfs_name[len-1] == '\n')
		sysfs_name[len-1] = '\0';

out_destroy:
	res = fpgaDestroyObject(&fpga_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
		resval = res;
	}
	return resval;
}

// read sysfs value
fpga_result read_sysfs_int64(fpga_token token, char *sysfs_path,
	uint64_t *value)
{
	fpga_result res = FPGA_OK;
	fpga_object fpga_object;

	if (sysfs_path == NULL) {
		OPAE_ERR("Invalid input parameter");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, sysfs_path, &fpga_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	res = fpgaObjectRead64(fpga_object, value, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Read object ");
	}

	res = fpgaDestroyObject(&fpga_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object");
	}
	return res;
}

// Sec info
fpga_result print_sec_common_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	fpga_object tcm_object;
	char name[SYSFS_PATH_MAX] = { 0 };

	res = fpgaTokenGetObject(token, DFL_SYSFS_SEC_GLOB, &tcm_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token Object");
		return res;
	}
	printf("********** SEC Info START ************ \n");

	// BMC Keys
	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_BMC_ROOT, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("BMC root entry hash: %s\n", name);
	} else {
		OPAE_MSG("Failed to Read TCM BMC root entry hash");
		printf("BMC root entry hash: %s\n", "None");
		resval = res;
	}

	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_BMC_CANCEL, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("BMC CSK IDs canceled: %s\n", strlen(name) > 1 ? name : "None");
	} else {
		OPAE_MSG("Failed to Read BMC CSK IDs canceled");
		printf("BBMC CSK IDs canceled: %s\n", "None");
		resval = res;
	}

	// PR Keys
	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_PR_ROOT, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("PR root entry hash: %s\n", name);
	} else {
		OPAE_MSG("Failed to Read PR root entry hash");
		printf("PR root entry hash: %s\n", "None");
		resval = res;
	}

	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_PR_CANCEL, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("AFU/PR CSK IDs canceled: %s\n", strlen(name) > 1 ? name : "None");
	} else {
		OPAE_MSG("Failed to Read AFU CSK/PR IDs canceled");
		printf("AFU/PR CSK IDs canceled: %s\n", "None");
		resval = res;
	}

	// SR Keys
	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_SR_ROOT, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("FIM root entry hash: %s\n", name);
	} else {
		OPAE_MSG("Failed to Read FIM root entry hash");
		printf("FIM root entry hash: %s\n", "None");
		resval = res;
	}

	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_SR_CANCEL, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("FIM CSK IDs canceled: %s\n", strlen(name) > 1 ? name : "None");
	} else {
		OPAE_MSG("Failed to Read FIM CSK IDs canceled");
		printf("FIM CSK IDs canceled: %s\n", "None");
		resval = res;
	}

	// User flash count
	memset(name, 0, sizeof(name));
	res = read_sysfs(token, DFL_SYSFS_SEC_USER_FLASH_COUNT, name, SYSFS_PATH_MAX - 1);
	if (res == FPGA_OK) {
		printf("User flash update counter: %s\n", name);
	} else {
		OPAE_MSG("Failed to Read User flash update counter");
		printf("User flash update counter: %s\n", "None");
		resval = res;
	}

	res = fpgaDestroyObject(&tcm_object);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Object");
		resval = res;
	}

	printf("********** SEC Info END ************ \n");

	return resval;
}
