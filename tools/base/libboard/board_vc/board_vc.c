// Copyright(c) 2019, Intel Corporation
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

#include "safe_string/safe_string.h"
#include "board_vc.h"

// sysfs paths
#define SYSFS_BMCFW_VER                     "spi-altera.*.auto/spi_master/spi*/spi*.*/bmcfw_flash_ctrl/bmcfw_version"
#define SYSFS_MAX10_VER                     "spi-altera.*.auto/spi_master/spi*/spi*.*/max10_version"
#define SYSFS_PCB_INFO                      "spi-altera.*.auto/spi_master/spi*/spi*.*/pcb_info"
#define SYSFS_PKVL_POLL_MODE                "spi-altera.*.auto/spi_master/spi*/spi*.*/pkvl/polling_mode"
#define SYSFS_PKVL_STATUS                   "spi-altera.*.auto/spi_master/spi*/spi*.*/pkvl/status"
#define SYSFS_BS_ID                         "bitstream_id"
#define SYSFS_PHY_GROUP_INFO                "misc/eth_group*.*"
#define SYSFS_PHY_GROUP_INFO_DEV            "misc/eth_group*/dev"
#define SYSFS_EEPROM                        "*i2c*/i2c*/*/eeprom"
#define SYSFS_NVMEM                         "*i2c*/i2c*/*/nvmem"
#define SYSFS_PKVL_A_VER                    "spi-altera.*.auto/spi_master/spi*/spi*.*/pkvl/pkvl_a_version"
#define SYSFS_PKVL_B_VER                    "spi-altera.*.auto/spi_master/spi*/spi*.*/pkvl/pkvl_b_version"

// driver ioctl id
#define FPGA_PHY_GROUP_GET_INFO               0xB702

#define FPGA_BSID_SIZE                        32

// fpga phy group mode
#define FPGA_PHYGROUP_MODE_4_25G              1
#define FPGA_PHYGROUP_MODE_6_25G              3
#define FPGA_PHYGROUP_MODE_2_2_25G            4

// Read BMC firmware version
fpga_result read_bmcfw_version(fpga_token token, char *bmcfw_ver, size_t len)
{
	fpga_result res                = FPGA_OK;
	fpga_result resval             = FPGA_OK;
	uint32_t size                  = 0;
	char buf[FPGA_VAR_BUF_LEN]     = { 0 };
	fpga_object bmcfw_object;

	if (bmcfw_ver == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_BMCFW_VER, &bmcfw_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token object");
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
		FPGA_ERR("object size bigger then buffer size");
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
		goto out_destroy;
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
	uint8_t rev                = 0;
	uint32_t var               = 0;
	fpga_result res            = FPGA_OK;
	int retval                 = 0;

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
	var = strtoul(buf, NULL, 16);
	if (var == 0  &&
		errno != 0) {
		OPAE_ERR("Failed to covert buffer to integer: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	rev = (var >> 24) & 0xff;
	if ((rev >= 'A') && (rev <= 'Z')) {// range from 'A' to 'Z'
		retval = snprintf_s_ciii(fw_ver, len, "%c.%u.%u.%u", (char)rev, (var >> 16) & 0xff, (var >> 8) & 0xff, var & 0xff);
		if (retval < 0) {
			FPGA_ERR("error in formatting version" );
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
	fpga_result resval                   = FPGA_OK;
	uint32_t size                        = 0;
	char buf[FPGA_VAR_BUF_LEN]           = { 0 };
	fpga_object max10fw_object;

	if (max10fw_ver == NULL) {
		FPGA_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_MAX10_VER, &max10fw_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token object");
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
		FPGA_ERR("object size bigger then buffer size");
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
		goto out_destroy;
	}

out_destroy:
	res = fpgaDestroyObject(&max10fw_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}

// Read PCB information
fpga_result read_pcb_info(fpga_token token, char *pcb_info, size_t len)
{
	fpga_result res                = FPGA_OK;
	fpga_result resval             = FPGA_OK;
	uint32_t size                  = 0;
	fpga_object pcb_object;

	if (pcb_info == NULL ) {
		FPGA_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_PCB_INFO, &pcb_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token Object");
		return res;
	}

	res = fpgaObjectGetSize(pcb_object, &size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object size");
		resval = res;
		goto out_destroy;
	}

	// Return error if object size bigger then pcb info length
	if (size > len) {
		FPGA_ERR("object size bigger then pcb info size");
		resval = FPGA_EXCEPTION;
		goto out_destroy;
	}

	res = fpgaObjectRead(pcb_object, (uint8_t *)pcb_info, 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
	}

out_destroy:
	res = fpgaDestroyObject(&pcb_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}


// Read PKVL information
fpga_result read_pkvl_info(fpga_token token,
			   fpga_pkvl_info *pkvl_info,
			   int *fpga_mode)
{
	fpga_result res                    = FPGA_OK;
	fpga_result resval                 = FPGA_OK;
	uint64_t bs_id                     = 0;
	uint64_t poll_mode                 = 0;
	uint64_t status                    = 0;
	fpga_object poll_mode_object;
	fpga_object status_object;
	fpga_object bsid_object;

	if (pkvl_info == NULL ||
		fpga_mode == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_BS_ID, &bsid_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return res;
	}

	res = fpgaTokenGetObject(token, SYSFS_PKVL_POLL_MODE, &poll_mode_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		resval = res;
		goto out_destroy_bsid;
	}

	res = fpgaTokenGetObject(token, SYSFS_PKVL_STATUS, &status_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		resval = res;
		goto out_destroy_poll;
	}

	res = fpgaObjectRead64(bsid_object, &bs_id, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_status;
	}

	*fpga_mode = (bs_id >> FPGA_BSID_SIZE) & 0xf;

	res = fpgaObjectRead64(poll_mode_object, &poll_mode, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_status;
	}

	res = fpgaObjectRead64(status_object, &status, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_status;
	}

	pkvl_info->polling_mode = (uint32_t)poll_mode;
	pkvl_info->status = (uint32_t)status;

out_destroy_status:
	res = fpgaDestroyObject(&status_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

out_destroy_poll:
	res = fpgaDestroyObject(&poll_mode_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

out_destroy_bsid:
	res = fpgaDestroyObject(&bsid_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}

// Read PHY group information
fpga_result read_phy_group_info(fpga_token token,
				fpga_phy_group_info *group_info,
				uint32_t *group_num)
{
	fpga_result res = FPGA_OK;
	fpga_result resval = FPGA_OK;
	char path[SYSFS_MAX_SIZE] = { 0 };
	char cdevid[CDEV_ID_SIZE] = { 0 };
	size_t i = 0;
	uint32_t group_dev_count = 0;
	uint32_t obj_size = 0;
	fpga_object dev_obj;
	fpga_object group_object;
	fpga_object group_dev_object;


	if (group_num == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_PHY_GROUP_INFO,
		&group_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return FPGA_NOT_FOUND;
	}

	res = fpgaTokenGetObject(token, SYSFS_PHY_GROUP_INFO_DEV,
		&group_dev_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		resval = res;
		goto out_destroy_group;
	}

	res = fpgaObjectGetSize(group_dev_object, &group_dev_count, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size");
		resval = res;
		goto out_destroy_group_dev;
	}

	// Return number of group.
	if (group_info == NULL) {
		*group_num = group_dev_count;
		resval = FPGA_OK;
		goto out_destroy_group_dev;
	}

	// Return error if group device count bigger then group info array size
	if (group_dev_count > *group_num) {
		FPGA_ERR("group device count bigger then group info array size");
		resval = FPGA_EXCEPTION;
		goto out_destroy_group_dev;
	}

	for (i = 0; i < group_dev_count; i++) {

		res = fpgaObjectGetObjectAt(group_dev_object, i, &dev_obj);
		if (res != FPGA_OK) {
			OPAE_ERR("Failed to get device node object from group device object");
			resval = res;
			continue;
		}

		res = fpgaObjectGetSize(dev_obj, &obj_size, 0);
		if (res != FPGA_OK) {
			OPAE_ERR("Failed to get object size");
			resval = res;
			res = fpgaDestroyObject(&dev_obj);
			if (res != FPGA_OK) {
				OPAE_ERR("Failed to destroy object");
			}
			continue;
		}


		if (obj_size > CDEV_ID_SIZE) {
			OPAE_ERR("Device node obj size size bigger then buffer ");
			resval = FPGA_EXCEPTION;
			res = fpgaDestroyObject(&dev_obj);
			if (res != FPGA_OK) {
				OPAE_ERR("Failed to destroy object");
			}
			continue;
		}

		res = fpgaObjectRead(dev_obj, (uint8_t *)cdevid, 0, obj_size, 0);
		if (res != FPGA_OK) {
			OPAE_ERR("Failed to read device node");
			resval = res;
			res = fpgaDestroyObject(&dev_obj);
			if (res != FPGA_OK) {
				OPAE_ERR("Failed to destroy object");
			}
			continue;
		}


		res = fpgaDestroyObject(&dev_obj);
		if (res != FPGA_OK) {
			resval = res;
			OPAE_ERR("Failed to destroy object");
		}

		// append null char
		cdevid[obj_size - 1] = '\0';
		snprintf_s_s(path, sizeof(path), "/dev/char/%s", cdevid);

		res = get_phy_info(path, &group_info[i]);
		if (res != FPGA_OK) {
			OPAE_ERR("Failed to get phy group info");
			resval = res;
		}

	} // end for loop

out_destroy_group_dev:
	res = fpgaDestroyObject(&group_dev_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

out_destroy_group:
	res = fpgaDestroyObject(&group_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;

}

// get pyh group information
fpga_result get_phy_info(char *dev_path, fpga_phy_group_info *info)
{
	fpga_result res = FPGA_OK;
	int fd          = 0;

	if (dev_path == NULL ||
		info == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		OPAE_ERR("Open %s failed\n", dev_path);
		return FPGA_INVALID_PARAM;
	}

	memset_s(info, sizeof(fpga_phy_group_info), 0);
	info->argsz = sizeof(fpga_phy_group_info);

	if (0 != ioctl(fd, FPGA_PHY_GROUP_GET_INFO, info)) {
		OPAE_ERR("ioctl  FPGA_PHY_GROUP_GET_INFO error\n");
	}

	close(fd);

	return res;
}

// Read mac information
fpga_result read_mac_info(fpga_token token, unsigned char *mac_info, size_t len)
{
	fpga_result res                 = FPGA_OK;
	fpga_result resval              = FPGA_OK;
	unsigned char buf[8]            = {0};
	fpga_object mac_object;

	if (mac_info == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = fpgaTokenGetObject(token, SYSFS_EEPROM, &mac_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get token object");
		return res;
	}

	res = fpgaObjectRead(mac_object, (uint8_t *)buf, 0, sizeof(buf), 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_mac;
	}

	memcpy_s(mac_info, len, buf, sizeof(buf));

out_destroy_mac:
	res = fpgaDestroyObject(&mac_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}

// Read pkvl versoin
fpga_result print_pkvl_version(fpga_token token)
{
	fpga_result res                     = FPGA_OK;
	fpga_result resval                  = FPGA_OK;
	char ver_a_buf[FPGA_VAR_BUF_LEN]    = { 0 };
	char ver_b_buf[FPGA_VAR_BUF_LEN]    = { 0 };
	uint32_t size                       = 0;
	fpga_object pkvl_a_object;
	fpga_object pkvl_b_object;


	res = fpgaTokenGetObject(token, SYSFS_PKVL_A_VER, &pkvl_a_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token object");
		return res;
	}

	res = fpgaTokenGetObject(token, SYSFS_PKVL_B_VER, &pkvl_b_object, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to get token object");
		resval = res;
		goto out_destroy_obj_a;
	}

	res = fpgaObjectGetSize(pkvl_a_object, &size, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size");
		resval = res;
		goto out_destroy_obj_b;
	}

	if (size > FPGA_VAR_BUF_LEN) {
		OPAE_ERR("pkvl A version buffer bigger then version buffer");
		resval = FPGA_EXCEPTION;
		goto out_destroy_obj_b;
	}

	res = fpgaObjectRead(pkvl_a_object, (uint8_t *)ver_a_buf, 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_obj_b;
	}

	res = fpgaObjectGetSize(pkvl_b_object, &size, FPGA_OBJECT_GLOB);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get object size");
		resval = res;
		goto out_destroy_obj_b;
	}

	if (size > FPGA_VAR_BUF_LEN) {
		OPAE_ERR("pkvl B version buffer bigger then version buffer");
		resval = FPGA_EXCEPTION;
		goto out_destroy_obj_b;
	}


	res = fpgaObjectRead(pkvl_b_object, (uint8_t *)ver_b_buf, 0, size, 0);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read object ");
		resval = res;
		goto out_destroy_obj_b;
	}

	printf("%-32s : %s", "Retimer A Version", ver_a_buf);
	printf("%-32s : %s", "Retimer B Version", ver_b_buf);

out_destroy_obj_b:
	res = fpgaDestroyObject(&pkvl_b_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

out_destroy_obj_a:
	res = fpgaDestroyObject(&pkvl_a_object);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy object");
	}

	return resval;
}

// print mac information
fpga_result print_mac_info(fpga_token token)
{
	fpga_result res                  = FPGA_OK;
	unsigned char buf[MAC_BUF_SIZE]  = { 0 };
	int i                            = 0;
	int n                            = 0;
	pkvl_mac mac;

	res = read_mac_info(token, buf, MAC_BUF_SIZE);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read mac information");
		return res;
	}

	n = (int)buf[6];
	printf("%-32s : %d\n", "Number of MACs", n);
	mac.byte[0] = buf[5];
	mac.byte[1] = buf[4];
	mac.byte[2] = buf[3];
	mac.byte[3] = 0;
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
	char pcb_ver[FPGA_VAR_BUF_LEN]       = { 0 };

	res = read_bmcfw_version(token, bmc_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read bmc version");
	}

	res = read_max10fw_version(token, max10_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read max10 version");
	}

	res = read_pcb_info(token, pcb_ver, FPGA_VAR_BUF_LEN);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read pcb version");
	}

	printf("Board Management Controller, MAX10 NIOS FW version: %s \n", bmc_ver );
	printf("Board Management Controller, MAX10 Build version: %s \n", max10_ver);
	printf("PCB version: %s \n", pcb_ver);;

	return res;
}

// print phy group information
fpga_result print_phy_info(fpga_token token)
{
	fpga_result res                            = FPGA_OK;
	fpga_phy_group_info* phy_info_array        = NULL;
	uint32_t group_num                         = 0;
	int fpga_mode                              = 0;
	uint32_t i                                 = 0;
	int j                                      = 0;
	char mode[VER_BUF_SIZE]                    = { 0 };
	fpga_pkvl_info pkvl_info;


	res = read_phy_group_info(token, NULL, &group_num);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy group count");
		return res;
	}

	phy_info_array = calloc(sizeof(fpga_phy_group_info), group_num);
	if (phy_info_array == NULL) {
		OPAE_ERR(" Failed to allocate memory");
		return FPGA_NO_MEMORY;
	}


	res = read_phy_group_info(token, phy_info_array, &group_num);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read phy group array");
		goto out_free;
	}

	res = read_pkvl_info(token, &pkvl_info, &fpga_mode);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to read pkvl info");
		goto out_free;
	}


	for (i = 0; i < group_num; i++) {

		printf("//****** PHY GROUP %d ******//\n", i);
		printf("%-32s : %s\n", "Direction",
			phy_info_array[i].group_id == 0 ? "Line side" : "Fortville side");
		printf("%-32s : %d Gbps\n", "Speed", phy_info_array[i].speed);
		printf("%-32s : %d\n", "Number of PHYs", phy_info_array[i].phy_num);
	}


	int mask = 0;
	if (phy_info_array[0].speed == 10) {
		mask = 0xff;

	} else if (phy_info_array[0].speed == 25) {


		if (phy_info_array[0].phy_num == 4) {
			switch (fpga_mode) {
			case FPGA_PHYGROUP_MODE_4_25G: /* 4x25g */
				/* FALLTHROUGH */
			case FPGA_PHYGROUP_MODE_6_25G: /* 6x25g */
				mask = 0xf;
				break;

			case FPGA_PHYGROUP_MODE_2_2_25G: /* 2x2x25g */
				mask = 0x33;
				break;

			default:
				mask = 0xff;
				break;
			}
		}
		else {
			/* 2*1*25g */
			mask = 0x11;
		}

	}

	printf("//****** Intel C827 Retimer ******//\n");

	strncpy_s(mode, sizeof(mode), phy_info_array[0].speed == 25 ? "25G" : "10G", 3);
	for (i = 0, j = 0; i < MAX_PORTS; i++) {
		if (mask&(1 << i)) {
			printf("Port%-2d%-26s : %s\n", j, mode, pkvl_info.status&(1 << i) ? "Up" : "Down");
			j++;
		}
	}

	res = print_pkvl_version(token);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to read pkvl version");
		goto out_free;
	}

out_free:
	if (phy_info_array)
		free(phy_info_array);

	return res;

}