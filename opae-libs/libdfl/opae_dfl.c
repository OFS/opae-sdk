// Copyright(c) 2021, Intel Corporation
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
#define _GNU_SOURCE
#include <dirent.h>
#include <glob.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#undef _GNU_SOURCE

#include <opae/fpga.h>
#include "opae_dfl.h"

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])"
#define PCIE_PATH_PATTERN_GROUPS 5
#define SYSFS_CLASS_FPGA_REGION "/sys/class/fpga_region"
#define SYSFS_CLASS_FPGA_REGION_GLOB SYSFS_CLASS_FPGA_REGION "/region*"
#define SYSFS_CLASS_FPGA_DEVICES_GLOB SYSFS_CLASS_FPGA_REGION_GLOB "/dfl-*"
#define DFL_REGION_PATTERN "region*"
#define DFL_FME_PATTERN "dfl-fme.*"
#define DFL_PORT_PATTERN "dfl-port.*"
#define DFL_FME_PORT_PATTERN "dfl-@(fme|port).*"
#define DFL_ERRORS_PATTERN "errors/!(clear|revision|uevent|power)"
#define MAX_SYSFS_CLASS 8
#define MAX_DFL_RES 16
#define DFL_PCI "dfl-pci"
#define DFL_PCI_SZ 7
#define DFL_REGION "fpga_region"
#define DFL_REGION_SZ 11
#define DFL_COMPAT_ID "compat_id"
#define DFL_AFU_ID "afu_id"
#define DFL_BITSTREAM_ID "bitstream_id"
#define DFL_PORTS_NUM "ports_num"
#define DFL_NUMA_NODE "numa_node"

struct udev *_udev_ctx;

#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			OPAE_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)

static int parse_pcie_addr(dfl_device *device)
{
	char err[128] = {0};
	regex_t re;
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };
	int res = FPGA_EXCEPTION;
	const char *pciaddr = udev_device_get_sysname(device->pci);
	int reg_res = regcomp(&re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);
	if (reg_res) {
		OPAE_ERR("Error compling regex");
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, pciaddr, PCIE_PATH_PATTERN_GROUPS, matches, 0);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		OPAE_ERR("Error executing regex: %s", err);
		res = FPGA_EXCEPTION;
		goto out;
	} else {
		PARSE_MATCH_INT(pciaddr, matches[1], device->segment, 16, out);
		PARSE_MATCH_INT(pciaddr, matches[2], device->bus, 16, out);
		PARSE_MATCH_INT(pciaddr, matches[3], device->device, 16, out);
		PARSE_MATCH_INT(pciaddr, matches[4], device->function, 10, out);
	}
	res = FPGA_OK;

out:
	regfree(&re);
	return res;
}


void udev_print_list(struct udev_list_entry *le)
{
	while (le) {
		printf("%s: %s\n",
		       udev_list_entry_get_name(le),
		       udev_list_entry_get_value(le));
		le = udev_list_entry_get_next(le);
	}
}

static size_t udev_direct_read_attr(struct udev_device *dev, const char *attr, char **buffer)
{
	char attr_path[PATH_MAX];
	size_t pg_size = sysconf(_SC_PAGE_SIZE);
	snprintf(attr_path, sizeof(attr_path), "%s/%s", udev_device_get_syspath(dev), attr);
	FILE *fp = fopen(attr_path, "r");
	if (fp) {
		*buffer = calloc(pg_size, sizeof(uint8_t));
		size_t n = fread(*buffer, 1, pg_size, fp);
		fclose(fp);
		if (n)
			return n;
		free(*buffer);
	}
	return 0;
}

static int udev_direct_read_attr64(struct udev_device *dev,
				   const char *attr, uint64_t *value)
{
	int res = FPGA_EXCEPTION;
	char *buffer = NULL;
	if (udev_direct_read_attr(dev, attr, &buffer)) {
		char *endptr = NULL;
		*value = strtoull(buffer, &endptr, 0);
		if (endptr > buffer)
			res = FPGA_OK;
		free(buffer);
	}
	return res;
}

//static int udev_direct_read_attr32(struct udev_device *dev,
//				   const char *attr, uint32_t *value)
//{
//	int res = FPGA_EXCEPTION;
//	char *buffer = NULL;
//	if (udev_direct_read_attr(dev, attr, &buffer)) {
//		char *endptr = NULL;
//		*value = strtoul(buffer, &endptr, 0);
//		if (endptr > buffer)
//			res = FPGA_OK;
//		free(buffer);
//	}
//	return res;
//}

static int udev_parse_attr64(struct udev_device *dev, const char *attr, uint64_t *value)
{
	const char *attr_str = udev_device_get_sysattr_value(dev, attr);
	if (attr_str) {
		if (strlen(attr_str)) {
			char *endptr = NULL;
			*value = strtoull(attr_str, &endptr, 0);
			return endptr == attr_str ? FPGA_EXCEPTION : FPGA_OK;
		}
		return udev_direct_read_attr64(dev, attr, value);
	}
	return FPGA_NOT_FOUND;
}

static int udev_parse_attr32(struct udev_device *dev, const char *attr, uint32_t *value)
{
	const char *attr_str = udev_device_get_sysattr_value(dev, attr);
	if (attr_str) {
		char *endptr = NULL;
		*value = strtoul(attr_str, &endptr, 0);
		return endptr == attr_str ? FPGA_EXCEPTION : FPGA_OK;
	}
	return FPGA_NOT_FOUND;
}


static int dfl_device_parse_id(dfl_device *device)
{
	uint64_t value = 0;
	int res = udev_parse_attr64(device->pci, "device", &value);
	if (res) {
		OPAE_MSG("Error parsing device_id for device: %s",
			 udev_device_get_syspath(device->dev));
		return FPGA_EXCEPTION;
	}
	device->device_id = value;

	res = udev_parse_attr64(device->pci, "vendor", &value);

	if (res) {
		OPAE_ERR("Error parsing vendor_id for device: %s",
			 udev_device_get_syspath(device->dev));
		return FPGA_EXCEPTION;
	}
	device->vendor_id = value;

	return FPGA_OK;
}

static int dfl_get_errors(dfl_device *dev)
{
	struct udev_list_entry *attrs =
		udev_device_get_sysattr_list_entry(dev->dev);
	if (!attrs) {
		return FPGA_EXCEPTION;
	}
	struct udev_list_entry *le = NULL;
	udev_list_entry_foreach(le, attrs) {
		const char *attr = udev_list_entry_get_name(le);
		if (attr && !fnmatch(DFL_ERRORS_PATTERN, attr, FNM_EXTMATCH)) {
			dfl_error *err = (dfl_error*)malloc(sizeof(dfl_error));
			if (err) {
				++dev->num_errors;
				err->attr = attr;
				err->next = NULL;
				if (dev->errors) {
					dfl_error *tmp = dev->errors;
					while(tmp->next) tmp = tmp->next;
					tmp->next = err;
				} else {
					dev->errors = err;
				}
			} else {
				printf("error allocating memory for error struct\n");
			}
		}
	}
	return 0;
}


dfl_device *dfl_device_new(const char *path)
{
	dfl_device *dfl = (dfl_device*)malloc(sizeof(dfl_device));
	if (!dfl) {
		return NULL;
	}
	dfl->dev = udev_device_new_from_syspath(_udev_ctx, path);
	if (!dfl->dev) {
		OPAE_ERR("error new udev: %s", path);
		goto out_err;
	}
	dfl->num_errors = 0;
	dfl->errors = NULL;
	const char *sysname = udev_device_get_sysname(dfl->dev);
	if (!fnmatch(DFL_FME_PATTERN, sysname, 0))
		dfl->type = FPGA_DEVICE;
	else if (!fnmatch(DFL_PORT_PATTERN, sysname, 0))
		dfl->type = FPGA_ACCELERATOR;
	else {
		OPAE_ERR("unrecognized object type: %s", sysname);
		goto out_err;
	}

	struct udev_device *parent = udev_device_get_parent(dfl->dev);
	const char *driver = udev_device_get_driver(parent);
	const char *subsystem = udev_device_get_subsystem(parent);
	while(parent) {
		if (!strncmp(subsystem, DFL_REGION, DFL_REGION_SZ)) {
			dfl->region = parent;
		} else if (driver && !strncmp(driver, DFL_PCI, DFL_PCI_SZ)) {
			dfl->pci = parent;
			break;
		}
		parent = udev_device_get_parent(parent);
		driver = udev_device_get_driver(parent);
		subsystem = udev_device_get_subsystem(parent);
	}

	if (!dfl->pci || !dfl->region) {
		goto out_err;
	}

	if (parse_pcie_addr(dfl)){
		goto out_err;
	}

	if (dfl_device_parse_id(dfl)){
		goto out_err;
	}

	if (udev_parse_attr32(dfl->dev, DFL_NUMA_NODE, &dfl->numa_node)) {
		goto out_err;
	}

	if (dfl_get_errors(dfl)) {
		goto out_err;
	}

	dev_t devnum = dfl_device_get_devnum(dfl);
	dfl->object_id = ((major(devnum) & 0xFFF) << 20) | (minor(devnum) & 0xFFFFF);
	dfl->next = NULL;
	return dfl;
out_err:
	if (dfl->dev)
		udev_device_unref(dfl->dev);
	free(dfl);
	return NULL;
}

dfl_device *dfl_device_add(dfl_device *e, const char *path)
{
	dfl_device *dfl = dfl_device_new(path);
	if (!dfl) {
		return NULL;
	}
	if (e) {
		while(e->next) {
			e = e->next;
		}
		e->next = dfl;
	}
	return dfl;
}


dfl_device *dfl_device_enum()
{
	dfl_device *d = NULL;
	glob_t pglob;
	pglob.gl_pathc = 0;
	pglob.gl_pathv = NULL;
	if (glob(SYSFS_CLASS_FPGA_DEVICES_GLOB, 0, NULL, &pglob)) {
		OPAE_ERR("Could not find dfl regions in '%s'",
			 SYSFS_CLASS_FPGA_DEVICES_GLOB);
		return NULL;
	}

	for (size_t i = 0; i < pglob.gl_pathc; ++i) {
		if (d) {
			dfl_device_add(d, pglob.gl_pathv[i]);
		} else {
			d = dfl_device_new(pglob.gl_pathv[i]);
		}
	}
	globfree(&pglob);
	return d;
}


dfl_device *dfl_device_clone(dfl_device *e)
{
	const char *path = udev_device_get_syspath(e->dev);
	if (!path) {
		OPAE_ERR("Error getting path to udev_device");
		return NULL;

	}
	return dfl_device_new(path);
}


void dfl_device_destroy(dfl_device *e)
{
	while(e) {
		dfl_device *tmp = e;
		e = e->next;
		udev_device_unref(tmp->dev);
		dfl_error *err = tmp->errors;
		while(err){
			dfl_error *tmp_err = err;
			err = err->next;
			free(tmp_err);
		}
		free(tmp);
	}
}

const char *dfl_device_get_pci_addr(dfl_device *e)
{
	return udev_device_get_sysname(e->pci);
}

const char *dfl_device_get_attr(dfl_device *e, const char *attr)
{
	return udev_device_get_sysattr_value(e->dev, attr);
}

int dfl_device_set_attr(dfl_device *e, const char *attr, const char *value)
{
	return udev_device_set_sysattr_value(e->dev, attr, value);
}

dfl_device *dfl_device_get_parent(dfl_device *e)
{
	dfl_device *d = NULL;
	if (e->type == FPGA_DEVICE) return NULL;
	const char *region = udev_device_get_syspath(e->region);
	DIR *region_dir = opendir(region);
	struct dirent *de = NULL;
	if (!region_dir) {
		OPAE_ERR("Could not open '%s'\n", region);
		return NULL;
	}
	char fme_path[PATH_MAX] = {0};
	while ((de = readdir(region_dir))) {
		if (!fnmatch(DFL_FME_PATTERN, de->d_name, 0)) {
			snprintf(fme_path, sizeof(fme_path), "%s/%s", region, de->d_name);
			d = dfl_device_new(fme_path);
			break;
		}
	}

	
	closedir(region_dir);
	return d;
}

int dfl_parse_guid(const char *guid_str, fpga_guid guid)
{
	int i = 0;
	for (const char *ptr = guid_str; i < 16; ++i, ptr += 2)
		if (!sscanf(ptr, "%02x", (unsigned*)&guid[i])) {
			return 0;
		}
	return i;
}

int dfl_device_get_compat_id(dfl_device *d, fpga_guid guid)
{
	if (d->type != FPGA_DEVICE) {
		return FPGA_INVALID_PARAM;
	}

	int res = FPGA_OK;
	struct udev_list_entry *devices, *le = NULL;
	struct udev_device *dev = NULL;
	struct udev_enumerate *ue = udev_enumerate_new(_udev_ctx);
	if (!ue) {
		return FPGA_EXCEPTION;
	}
	udev_enumerate_add_match_subsystem(ue, DFL_REGION);
	udev_enumerate_scan_devices(ue);
	devices = udev_enumerate_get_list_entry(ue);
	if (!devices) {
		res = FPGA_NOT_FOUND;
		goto out_free;
	}
	const char *pci_path = udev_device_get_syspath(d->pci);
	const char *guid_str = NULL, *le_name = NULL;
	const char *this_region = udev_device_get_syspath(d->region);

	udev_list_entry_foreach(le, devices) {
		le_name = udev_list_entry_get_name(le);
		if (strcmp(le_name, this_region) &&
		    strstr(le_name, pci_path) == le_name) {
			dev = udev_device_new_from_syspath(_udev_ctx, le_name);
			guid_str = udev_device_get_sysattr_value(dev, DFL_COMPAT_ID);
			if (!guid_str) {
				res = FPGA_NOT_FOUND;
				goto out_free;
			}
			if (!dfl_parse_guid(guid_str, guid)){
				res = FPGA_EXCEPTION;
				goto out_free;
			}
			break;
		}

	}
out_free:
	if (dev) udev_device_unref(dev);
	udev_enumerate_unref(ue);
	return res;
}

int dfl_device_afu_id(dfl_device *d, fpga_guid guid)
{
	if (d->type != FPGA_ACCELERATOR) {
		return FPGA_INVALID_PARAM;
	}
	const char *afu_id = dfl_device_get_attr(d, DFL_AFU_ID);
	if (afu_id && dfl_parse_guid(afu_id, guid)) {
		return FPGA_OK;
	}
	return FPGA_EXCEPTION;
}

dev_t dfl_device_get_devnum(dfl_device *dev)
{
	return udev_device_get_devnum(dev->dev);
}

int dfl_device_get_ports_num(dfl_device *dev, uint32_t *value)
{
	if (dev->type == FPGA_DEVICE)
		return udev_parse_attr32(dev->dev, DFL_PORTS_NUM, value);
	return FPGA_INVALID_PARAM;
}

int dfl_device_get_bbs_id(dfl_device *dev, uint64_t *bbs_id)
{
	if (dev->type == FPGA_DEVICE)
		return udev_parse_attr64(dev->dev, DFL_BITSTREAM_ID, bbs_id);
	return FPGA_INVALID_PARAM;
}

int dfl_device_guid_match(dfl_device *dev, fpga_guid guid)
{
	fpga_guid this_guid;
	if (dev->type == FPGA_DEVICE)
		if (dfl_device_get_compat_id(dev, this_guid))
			return FPGA_INVALID_PARAM;
	if (dev->type == FPGA_ACCELERATOR)
		if (dfl_device_get_afu_id(dev, this_guid))
			return FPGA_INVALID_PARAM;
	return memcmp(this_guid, guid, sizeof(fpga_guid));
}

int dfl_device_read_attr64(dfl_device *dev, const char *attr, uint64_t *value)
{
	return udev_parse_attr64(dev->dev, attr, value);
}

int dfl_device_write_attr64(dfl_device *dev, const char *attr, uint64_t value)
{
	char value_str[4192] = {0};
	snprintf(value_str, sizeof(value_str), "0x%lx\n", value);
	if (udev_device_set_sysattr_value(dev->dev, attr, value_str))
		return FPGA_EXCEPTION;
	return FPGA_OK;
}
