// Copyright(c) 2019-2021, Intel Corporation
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
#include <net/if.h>
#include <net/ethernet.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include <sys/ioctl.h>

#include "board_common.h"

#define DFL_SYSFS_SEC_GLOB "*dfl*/*spi*/*spi*/*spi*/**/security/"
#define DFL_SYSFS_SEC_USER_FLASH_COUNT         DFL_SYSFS_SEC_GLOB "*flash_count"
#define DFL_SYSFS_SEC_BMC_CANCEL               DFL_SYSFS_SEC_GLOB "bmc_canceled_csks"
#define DFL_SYSFS_SEC_BMC_ROOT                 DFL_SYSFS_SEC_GLOB "bmc_root_entry_hash"
#define DFL_SYSFS_SEC_PR_CANCEL                DFL_SYSFS_SEC_GLOB "pr_canceled_csks"
#define DFL_SYSFS_SEC_PR_ROOT                  DFL_SYSFS_SEC_GLOB "pr_root_entry_hash"
#define DFL_SYSFS_SEC_SR_CANCEL                DFL_SYSFS_SEC_GLOB "sr_canceled_csks"
#define DFL_SYSFS_SEC_SR_ROOT                  DFL_SYSFS_SEC_GLOB "sr_root_entry_hash"

#define DFL_SYSFS_ETHINTERFACE   "dfl*.*/net/%s*"
#define ETHTOOL_STR              "ethtool"
#define IFCONFIG_STR             "ifconfig"
#define IFCONFIG_UP_STR          "up"

#define SYSFS_FEATURE_ID "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/"\
			"fpga_region/region*/dfl-fme*/dfl_dev*/feature_id"

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
		printf("BMC CSK IDs canceled: %s\n", strlen(name) > 0 ? name : "None");
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
		printf("AFU/PR CSK IDs canceled: %s\n", strlen(name) > 0 ? name : "None");
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
		printf("FIM CSK IDs canceled: %s\n", strlen(name) > 0 ? name : "None");
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

// prints FPGA ethernet interface info
fpga_result print_eth_interface_info(fpga_token token, const char *interface_name)
{
	fpga_result res                = FPGA_NOT_FOUND;
	struct if_nameindex *if_nidxs  = NULL;
	struct if_nameindex *intf      = NULL;
	char cmd[SYSFS_PATH_MAX]       = { 0 };
	char glob[SYSFS_PATH_MAX]      = { 0 };
	int result                     = 0;
	fpga_object fpga_object;

	if_nidxs = if_nameindex();
	if (if_nidxs != NULL) {
		for (intf = if_nidxs; intf->if_index != 0
			|| intf->if_name != NULL; intf++) {

			char *p = strstr(intf->if_name, interface_name);
			if (p) {
				memset(glob, 0, sizeof(glob));
				if (snprintf(glob, sizeof(glob),
					DFL_SYSFS_ETHINTERFACE, p) < 0) {
					OPAE_ERR("snprintf failed");
					res = FPGA_EXCEPTION;
					goto out_free;
				}

				// Check interface associated to bdf
				res = fpgaTokenGetObject(token, glob,
					&fpga_object, FPGA_OBJECT_GLOB);
				if (res != FPGA_OK) {
					OPAE_DBG("Failed to get token Object");
					res = FPGA_OK;
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



fpga_result sysfs_read_u64(const char *path, uint64_t *u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = { 0 };
	int b = 0;

	if (!path || !u) {
		OPAE_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		OPAE_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		OPAE_MSG("seek failed");
		goto out_close;
	}

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			OPAE_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			OPAE_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0'
		&& (unsigned)b < sizeof(buf));

	// erase \n
	buf[b - 1] = 0;

	*u = strtoull(buf, NULL, 0);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}


fpga_result get_fpga_sbdf(fpga_token token,
			uint16_t *segment,
			uint8_t *bus,
			uint8_t *device,
			uint8_t *function)

{
	fpga_result res = FPGA_OK;
	fpga_properties props = NULL;

	if (!segment || !bus ||
		!device || !function) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get properties ");
		return res;
	}

	res = fpgaPropertiesGetBus(props, bus);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get bus ");
		return res;
	}

	res = fpgaPropertiesGetSegment(props, segment);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Segment ");
		return res;
	}
	res = fpgaPropertiesGetDevice(props, device);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Device ");
		return res;
	}

	res = fpgaPropertiesGetFunction(props, function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Function ");
		return res;
	}

	return res;
}


fpga_result find_dev_feature(fpga_token token,
	 uint32_t feature_id,
	 char *dfl_dev_str)
{
	fpga_result res          = FPGA_NOT_FOUND;
	fpga_result retval       = FPGA_OK;
	int gres                 = 0;
	size_t i                 = 0;
	uint64_t value           = 0;
	uint16_t segment         = 0;
	uint8_t bus              = 0;
	uint8_t device           = 0;
	uint8_t function         = 0;
	glob_t pglob;
	char feature_path[SYSFS_PATH_MAX] = { 0 };

	retval = get_fpga_sbdf(token, &segment, &bus, &device, &function);
	if (retval != FPGA_OK) {
		OPAE_ERR("Failed to get sbdf ");
		return retval;
	}

	if (snprintf(feature_path, sizeof(feature_path),
		SYSFS_FEATURE_ID,
		segment, bus, device, function) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	gres = glob(feature_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s",
			feature_path, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	for (i = 0; i < pglob.gl_pathc; i++) {
		retval = sysfs_read_u64(pglob.gl_pathv[i], &value);
		if (retval != FPGA_OK) {
			OPAE_MSG("Failed to read sysfs value");
			continue;
		}

		if (value == feature_id) {
			res = FPGA_OK;
			if (dfl_dev_str) {
				char *p = strstr(pglob.gl_pathv[i], "dfl_dev");
				if (p == NULL) {
					res = FPGA_NOT_FOUND;
				}

				if (snprintf(dfl_dev_str, SYSFS_PATH_MAX,
					"%s", p) < 0) {
					OPAE_ERR("snprintf buffer overflow");
					res = FPGA_EXCEPTION;
				}
			}
			break;
		}

	}
	if (gres)
		globfree(&pglob);
	return res;
}