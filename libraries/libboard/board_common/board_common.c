// Copyright(c) 2019-2023, Intel Corporation
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

#include <limits.h>
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
#include <opae/uio.h>
#include <sys/ioctl.h>

#include "board_common.h"
#include "mock/opae_std.h"

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

#define FACTORY_BIT (1ULL << 36)

// QSFP cable status
#define DFL_SYSFS_QSFP "*dfl*dev.%ld/qsfp_connected"
#define MAX_DEV_FEATURE_COUNT 256

#define DFH_CSR_ADDR                  0x18
#define DFH_CSR_SIZE                  0x20
#define FPGA_VAR_BUF_LEN       256

#define HSSI_FEATURE_ID                  0x15
#define HSSI_100G_PROFILE                       27
#define HSSI_25G_PROFILE                        21
#define HSSI_10_PROFILE                         20

#define HSSI_FEATURE_LIST                       0xC
#define HSSI_PORT_ATTRIBUTE                     0x10
#define HSSI_VERSION                            0x8
#define HSSI_PORT_STATUS                        0x818
#define GET_BIT(var, pos) ((var >> pos) & (1))

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

//Physical Port Enable
/*
[6] - Port 0 Enable
[7] - Port 1 Enable
:
[21] - Port 15 Enable
*/
#define PORT_ENABLE_COUNT 20

// hssi feature list CSR
struct hssi_feature_list {
	union {
		uint32_t csr;
		struct {
			uint32_t axi4_support : 1;
			uint32_t hssi_num : 5;
			uint32_t port_enable : 20;
			uint32_t reserved : 6;
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
			uint32_t low_speed_mac : 2;
			uint32_t dynamic_pr : 1;
			uint32_t sub_profile : 5;
			uint32_t reserved : 11;
		};
	};
};

//HSSI Ethernet Port Status
//Byte Offset: 0x818
struct hssi_port_status {
	union {
		uint64_t csr;
		struct {
			uint64_t txplllocked : 16;
			uint64_t txlanestable : 16;
			uint64_t rxpcsready : 16;
			uint64_t reserved : 16;
		};
	};
};


struct dfh {
	union {
		uint64_t csr;
		struct {
			uint64_t id : 12;
			uint64_t feature_rev : 4;
			uint64_t next : 24;
			uint64_t eol : 1;
			uint64_t reserved41 : 7;
			uint64_t feature_minor_rev : 4;
			uint64_t dfh_version : 8;
			uint64_t type : 4;
		};
	};
};

struct dfh_csr_addr {
	union {
		uint32_t csr;
		struct {
			uint64_t rel : 1;
			uint64_t addr : 63;
		};
	};
};

struct dfh_csr_group {
	union {
		uint32_t csr;
		struct {
			uint64_t instance_id : 16;
			uint64_t grouping_id : 15;
			uint64_t has_params : 1;
			uint64_t csr_size : 32;
		};
	};
};



typedef struct hssi_port_profile {

	uint32_t port_index;
	char profile[FPGA_VAR_BUF_LEN];

} hssi_port_profile;

#define HSS_PORT_PROFILE_SIZE 34

static hssi_port_profile hssi_port_profiles[] = {

	{.port_index = 0, .profile = "LL100G"},
	{.port_index = 1, .profile = "Ultra100G"},
	{.port_index = 2, .profile = "LL50G"},
	{.port_index = 3, .profile = "LL40G"},
	{.port_index = 4, .profile = "Ultra40G"},
	{.port_index = 5, .profile = "25_50G"},
	{.port_index = 6, .profile = "10_25G"},
	{.port_index = 7, .profile = "MRPHY"},
	{.port_index = 8, .profile = "LL10G"},
	{.port_index = 9, .profile = "TSE PCS"},
	{.port_index = 10, .profile = "TSE MAC"},
	{.port_index = 11, .profile = "Flex-E"},
	{.port_index = 12, .profile = "OTN"},
	{.port_index = 13, .profile = "General PCS-Direct"},
	{.port_index = 14, .profile = "General FEC-Direct"},
	{.port_index = 15, .profile = "General PMA-Direct"},
	{.port_index = 16, .profile = "MII"},
	{.port_index = 17, .profile = "Ethernet PCS-Direct"},
	{.port_index = 18, .profile = "Ethernet FEC-Direct"},
	{.port_index = 19, .profile = "Ethernet PMA-Direct"},
	{.port_index = 20, .profile = "10GbE"},
	{.port_index = 21, .profile = "25GbE"},
	{.port_index = 22, .profile = "40GCAUI-4"},
	{.port_index = 23, .profile = "50GAUI-2"},
	{.port_index = 24, .profile = "50GAUI-1"},
	{.port_index = 25, .profile = "100GAUI-1"},
	{.port_index = 26, .profile = "100GAUI-2"},
	{.port_index = 27, .profile = "100GCAUI-4"},
	{.port_index = 28, .profile = "200GAUI-2"},
	{.port_index = 29, .profile = "200GAUI-4"},
	{.port_index = 30, .profile = "200GAUI-8"},
	{.port_index = 31, .profile = "400GAUI-4"},
	{.port_index = 32, .profile = "400GAUI-8"},
	{.port_index = 33, .profile = "CPRI"}
 };

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

	fd = opae_open(path, O_RDONLY);
	if (fd < 0) {
		OPAE_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		OPAE_MSG("seek failed");
		goto out_close;
	}

	do {
		res = opae_read(fd, buf + b, sizeof(buf) - b);
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

	opae_close(fd);
	return FPGA_OK;

out_close:
	opae_close(fd);
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
	fpga_result resval = FPGA_OK;

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
		resval = res;
		goto out_destroy;
	}

	res = fpgaPropertiesGetSegment(props, segment);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Segment ");
		resval = res;
		goto out_destroy;
	}
	res = fpgaPropertiesGetDevice(props, device);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Device ");
		resval = res;
		goto out_destroy;
	}

	res = fpgaPropertiesGetFunction(props, function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Function ");
		resval = res;
		goto out_destroy;
	}

out_destroy:
	res = fpgaDestroyProperties(&props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy properties");
	}

	return resval;
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

	gres = opae_glob(feature_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s",
			feature_path, strerror(errno));
		opae_globfree(&pglob);
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
					goto free;
				}

				char *end = strchr(p, '/');
				if (end == NULL) {
					res = FPGA_NOT_FOUND;
					goto free;
				}
				strncpy(dfl_dev_str, p, end - p);
				*(dfl_dev_str + (end - p)) = '\0';

			}
			break;
		}

	}

free:
	opae_globfree(&pglob);
	return res;
}

//Prints fpga boot page info
fpga_result print_common_boot_info(fpga_token token)
{
	fpga_properties props       = NULL;
	fpga_result res             = FPGA_OK;
	fpga_result retval          = FPGA_OK;
	uint64_t bbs_id             = (uint64_t)-1;
	fpga_objtype objtype;

	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Properties ");
		return res;
	}

	retval = fpgaPropertiesGetBBSID(props, &bbs_id);
	if (retval != FPGA_OK) {
		OPAE_ERR("Failed to get bbsid ");
		res = retval;
		goto pro_destroy;
	}

	retval = fpgaPropertiesGetObjectType(props, &objtype);
	if (retval != FPGA_OK) {
		OPAE_ERR("Failed to get object type ");
		res = retval;
		goto pro_destroy;
	}

	if (objtype == FPGA_DEVICE) {
		printf("%-32s : %s\n", "Boot Page",
			bbs_id & FACTORY_BIT ? "factory" : "user");
	}

pro_destroy:
	retval = fpgaDestroyProperties(&props);
	if (retval != FPGA_OK) {
		OPAE_ERR("Failed to destroy Properties ");
		res = retval;
	}

	return res;
}

static uint64_t macaddress_uint64(const struct ether_addr *eth_addr)
{
	uint64_t value   = 0;
	const uint8_t *ptr = (uint8_t *) eth_addr;

	for (int i = 5; i >= 0; i--) {
		value |= (uint64_t)*ptr++ << (CHAR_BIT * i);
	}
	return value;
}

static void uint64_macaddress(const uint64_t value, struct ether_addr *eth_addr)
{
	uint8_t *ptr = (uint8_t *)eth_addr;
	for (int i = 5; i >= 0; i--) {
		*ptr++ = value >> (CHAR_BIT * i);
	}
}

// print mac address
void print_mac_address(struct ether_addr *eth_addr, int count)
{
	uint64_t value = 0;
	if (eth_addr == NULL || count <= 0)
		return;

	printf("%s %-20d : %02X:%02X:%02X:%02X:%02X:%02X\n",
		"MAC address", 0, eth_addr->ether_addr_octet[0],
		eth_addr->ether_addr_octet[1], eth_addr->ether_addr_octet[2],
		eth_addr->ether_addr_octet[3], eth_addr->ether_addr_octet[4],
		eth_addr->ether_addr_octet[5]);

	for (int i = 1; i < count; ++i) {

		// convert MAC to uint64
		value = macaddress_uint64(eth_addr);
		++value;
		// convert uint64 to MAC
		uint64_macaddress(value, eth_addr);

		printf("%s %-20d : %02X:%02X:%02X:%02X:%02X:%02X\n",
			"MAC address", i, eth_addr->ether_addr_octet[0],
			eth_addr->ether_addr_octet[1], eth_addr->ether_addr_octet[2],
			eth_addr->ether_addr_octet[3], eth_addr->ether_addr_octet[4],
			eth_addr->ether_addr_octet[5]);

	}
}


// Replace all occurrences of needle in haystack with substitute
fpga_result replace_str_in_str(
	char *const haystack,
	const char *const needle,
	const char *const substitute,
	const size_t max_haystack_len,
	fpga_result res)
{
	if (res != FPGA_OK)
		return res;

	if (haystack == NULL || needle == NULL || substitute == NULL)
		return FPGA_INVALID_PARAM;

	if (strcmp(needle, substitute) == 0) // Corner case that causes infinite loop!
		return FPGA_OK;

	const size_t needle_len = strlen(needle);
	if (needle_len == 0) // Corner case that causes infinite loop!
		return FPGA_INVALID_PARAM;
	const size_t substitute_len = strlen(substitute);

	while (true) {
		const char *haystack_ptr;
		const char *needle_loc;

		const size_t haystack_len = strlen(haystack);

		// Find how many times the needle occurs in the haystack
		size_t needle_count = 0;
		for (haystack_ptr = haystack; (needle_loc = strstr(haystack_ptr, needle));
			haystack_ptr = needle_loc + needle_len)
			++needle_count;

		if (needle_count == 0)
			break;    // Nothing to replace

		// Reserve memory for the new string
		const size_t result_len = haystack_len + needle_count * (substitute_len - needle_len);
		if (result_len >= max_haystack_len) {
			OPAE_ERR("Not enough buffer space: %llu >= %llu", result_len, max_haystack_len);
			res = FPGA_INVALID_PARAM;
			break;
		}
		char *const result = (char *)opae_malloc(result_len + 1);
		if (result == NULL)
			return FPGA_NO_MEMORY;

		// Copy the haystack, replacing all the instances of the needle
		char *result_ptr = result;
		for (haystack_ptr = haystack; (needle_loc = strstr(haystack_ptr, needle));
			haystack_ptr = needle_loc + needle_len) {

			size_t const skip_len = needle_loc - haystack_ptr;
			// Copy the section until the occurence of the needle
			strncpy(result_ptr, haystack_ptr, skip_len);
			result_ptr += skip_len;
			// Copy the substitute
			memcpy(result_ptr, substitute, substitute_len);
			result_ptr += substitute_len;
		}

		const size_t remaining_result_size = result_len + 1 - (result_ptr - result);
		strncpy(result_ptr, haystack_ptr, remaining_result_size);  // Copy the rest of haystack to result
		strncpy(haystack, result, max_haystack_len);               // Update haystack with the result
		haystack[result_len] = '\0';

		opae_free(result);
	}

	return res;
}

// Reformat BOM Critical Components info to be directly printable.
// - Keys and values may not include commas.
// - Spaces and tabs are removed around commas.
// - Line endings are converted to LF (linefeed).
// - Empty lines are removed.
// - All Key,Value pairs are converted to Key: Value
fpga_result reformat_bom_info(
	char *const bom_info,
	const size_t len,
	const size_t max_result_len)
{
	if (bom_info == NULL)
		return FPGA_INVALID_PARAM;

	fpga_result res = FPGA_OK;

	// Remove trailing 0xFF:
	char *bom_info_ptr = bom_info + len - 1;
	while (bom_info_ptr > bom_info) {
		if (*bom_info_ptr == '\xFF')
			--bom_info_ptr;
		else
			break;
	}

	// Append LF if it is missing at the end:
	if (bom_info_ptr > bom_info) {
		if (*bom_info_ptr == '\n')
			++bom_info_ptr;
		else {
			++bom_info_ptr;
			*bom_info_ptr++ = '\n';  // Append missing linefeed
		}
	}
	*bom_info_ptr = '\x00';    // NUL terminate the bom_info string

	// Manipulate the bom_info string so it becomes directly printable.
	// Starting with line endings:
	res = replace_str_in_str(bom_info, "\r\n", "\n", max_result_len, res);
	res = replace_str_in_str(bom_info, "\n\r", "\n", max_result_len, res);
	res = replace_str_in_str(bom_info, "\r", "\n", max_result_len, res);
	res = replace_str_in_str(bom_info, "\n\n", "\n", max_result_len, res);  // Remove empty lines!

	// Remove all spaces and tabs before and after commas:
	while (strstr(bom_info, " ,") || strstr(bom_info, "\t,") ||
		strstr(bom_info, ", ") || strstr(bom_info, ",\t")) {

		res = replace_str_in_str(bom_info, " ,", ",", max_result_len, res);
		res = replace_str_in_str(bom_info, "\t,", ",", max_result_len, res);
		res = replace_str_in_str(bom_info, ", ", ",", max_result_len, res);
		res = replace_str_in_str(bom_info, ",\t", ",", max_result_len, res);
	}

	// Finally replace commas with ': ':
	res = replace_str_in_str(bom_info, ",", ": ", max_result_len, res);

	return res;
}

// QSFP cable status
fpga_result qsfp_cable_status(const fpga_token token)
{
	fpga_object fpga_object;
	fpga_result res              = FPGA_OK;
	uint64_t value               = 0;
	size_t i                     = 0;
	char qsfp_path[PATH_MAX]     = { 0 };
	int retval                   = 0;
	size_t qsfp_count            = 0;

	for (i = 0; i < MAX_DEV_FEATURE_COUNT; i++) {

		memset(qsfp_path, 0, sizeof(qsfp_path));

		retval = snprintf(qsfp_path, sizeof(qsfp_path),
			DFL_SYSFS_QSFP, i);
		if (retval < 0) {
			OPAE_MSG("error in formatting qsfp cable status");
			return FPGA_EXCEPTION;
		}

		res = fpgaTokenGetObject(token, qsfp_path,
			&fpga_object, FPGA_OBJECT_GLOB);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to get token Object");
			continue;
		}

		res = fpgaObjectRead64(fpga_object, &value, 0);
		if (res == FPGA_OK) {
			OPAE_MSG("Failed to Read object ");

			switch (value) {
			case 0:
				printf("QSFP%-28ld : %s \n", qsfp_count, "Not Connected");
				break;
			case 1:
				printf("QSFP%-28ld : %s \n", qsfp_count, "Connected");
				break;
			default:
				printf("QSFP%-28ld : %s \n", qsfp_count, "N/A");
			}

			qsfp_count++;

		} else {
			OPAE_MSG("Failed to Read object ");
		}

		res = fpgaDestroyObject(&fpga_object);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to Destroy Object");
		}

	}

	return res;
}

static fpga_result print_hssi_port_status(uint8_t *uio_ptr)
{
	uint32_t i                     = 0;
	uint32_t k                     = 0;
	uint32_t ver_offset            = 0;
	uint32_t feature_list_offset   = 0;
	uint32_t port_sts_offset       = 0;
	uint32_t port_attr_offset      = 0;
	struct dfh dfh_csr;
	struct dfh_csr_addr csr_addr;
	struct hssi_port_attribute port_profile;
	struct hssi_feature_list  feature_list;
	struct hssi_version  hssi_ver;
	struct hssi_port_status port_status;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	dfh_csr.csr = *((uint64_t *)(uio_ptr + 0x0));
	// dfhv0
	if ((dfh_csr.feature_rev == 0) ||
		(dfh_csr.feature_rev == 0x1)) {
		ver_offset = HSSI_VERSION;
		feature_list_offset = HSSI_FEATURE_LIST;
		port_sts_offset = HSSI_PORT_STATUS;
		port_attr_offset = HSSI_PORT_ATTRIBUTE;
	} else if ((dfh_csr.feature_rev >= 0x2) && (dfh_csr.feature_rev < 0xf)) { // dfhv0.5
		csr_addr.csr = *((uint64_t *)(uio_ptr + DFH_CSR_ADDR));
		ver_offset = csr_addr.addr;
		feature_list_offset = csr_addr.addr + 0x4;
		port_sts_offset = HSSI_PORT_STATUS;
		port_attr_offset = csr_addr.addr + 0x8;

	} else {
		printf("DFH feature revision not supported:%x \n", dfh_csr.feature_rev);
		return FPGA_NOT_SUPPORTED;
	}

	feature_list.csr = *((uint32_t *)(uio_ptr + feature_list_offset));
	hssi_ver.csr = *((uint32_t *)(uio_ptr + ver_offset));
	port_status.csr = *((volatile uint64_t *)(uio_ptr
		+ port_sts_offset));

	printf("//****** HSSI information ******//\n");
	printf("%-32s : %d.%d  \n", "HSSI version", hssi_ver.major, hssi_ver.minor);
	printf("%-32s : %d  \n", "Number of ports", feature_list.hssi_num);

	for (i = 0; i < PORT_ENABLE_COUNT; i++) {

		// prints only active/enabled ports
		if ((GET_BIT(feature_list.port_enable, i) == 0)) {
			continue;
		}

		port_profile.csr = *((volatile uint32_t *)(uio_ptr +
			port_attr_offset + i * 4));

		if (port_profile.profile > HSS_PORT_PROFILE_SIZE) {
			printf("Port%-28d :%s\n", i, "N/A");
			continue;
		}

		for (int j = 0; j < HSS_PORT_PROFILE_SIZE; j++) {
			if (hssi_port_profiles[j].port_index == port_profile.profile) {
				// lock, tx, rx bits set - link status UP
				// lock, tx, rx bits not set - link status DOWN
				if ((GET_BIT(port_status.txplllocked, k) == 1) &&
					(GET_BIT(port_status.txlanestable, k) == 1) &&
					(GET_BIT(port_status.rxpcsready, k) == 1)) {
					printf("Port%-28d :%-12s %s\n", i,
						hssi_port_profiles[j].profile, "UP");
				} else {
					printf("Port%-28d :%-12s %s\n", i,
						hssi_port_profiles[j].profile, "DOWN");
				}
				k++;
				break;
			}
		}
	}

	return FPGA_OK;
}

// print phy group information
fpga_result print_common_phy_info(fpga_token token)
{
	fpga_result res = FPGA_OK;
	struct opae_uio uio;
	char feature_dev[SYSFS_PATH_MAX] = { 0 };
	uint8_t *mmap_ptr = NULL;

	res = qsfp_cable_status(token);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to find QSFP cable info");
	}

	res = find_dev_feature(token, HSSI_FEATURE_ID, feature_dev);
	if (res != FPGA_OK) {
		OPAE_MSG("Failed to find feature HSSI");
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

	res = print_hssi_port_status(mmap_ptr);
	if (res) {
		OPAE_ERR("Failed to read hssi port status");
	}

	opae_uio_close(&uio);
	return res;
}
