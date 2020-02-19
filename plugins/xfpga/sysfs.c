// Copyright(c) 2017-2020, Intel Corporation
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

#define _GNU_SOURCE
#include <pthread.h>
#include <glob.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <regex.h>
#undef _GNU_SOURCE

#include <opae/types.h>
#include <opae/log.h>
#include <opae/types_enum.h>

#include "safe_string/safe_string.h"

#include "types_int.h"
#include "sysfs_int.h"
#include "common_int.h"

// substring that identifies a sysfs directory as the FME device.
#define FPGA_SYSFS_FME "fme"
#define FPGA_SYSFS_FME_LEN 3
// substring that identifies a sysfs directory as the AFU device.
#define FPGA_SYSFS_PORT "port"
#define FPGA_SYSFS_PORT_LEN 4
#define OPAE_KERNEL_DRIVERS 2


typedef struct _sysfs_formats {
	const char *sysfs_class_path;
	const char *sysfs_pcidrv_fpga;
	const char *sysfs_device_fmt;
	const char *sysfs_region_fmt;
	const char *sysfs_device_glob;
	const char *sysfs_fme_glob;
	const char *sysfs_port_glob;
	const char *sysfs_compat_id;
	const char *sysfs_fme_pwr_glob;
	const char *sysfs_fme_temp_glob;
	const char *sysfs_fme_perf_glob;
	const char *sysfs_port_err;
	const char *sysfs_port_err_clear;
	const char *sysfs_bmc_glob;
	const char *sysfs_max10_glob;
} sysfs_formats;

static sysfs_formats sysfs_path_table[OPAE_KERNEL_DRIVERS] = {
	// upstream driver sysfs formats
	{.sysfs_class_path = "/sys/class/fpga_region",
	 .sysfs_pcidrv_fpga = "fpga_region",
	 .sysfs_device_fmt = "(region)([0-9])+",
	 .sysfs_region_fmt = "dfl-(fme|port)\\.([0-9]+)",
	 .sysfs_device_glob = "region*",
	 .sysfs_fme_glob = "dfl-fme.*",
	 .sysfs_port_glob = "dfl-port.*",
	 .sysfs_compat_id = "/dfl-fme-region.*/fpga_region/region*/compat_id",
	 .sysfs_fme_temp_glob = "hwmon/hwmon*/temp*_*",
	 .sysfs_fme_pwr_glob = "hwmon/hwmon*/power*_*",
	 .sysfs_fme_perf_glob = "*perf",
	 .sysfs_port_err = "errors/errors",
	 .sysfs_port_err_clear = "errors/errors",
	 .sysfs_bmc_glob = "avmmi-bmc.*/bmc_info",
	 .sysfs_max10_glob = "spi-*/spi_master/spi*/spi*.*"
	},
	// intel driver sysfs formats
	{.sysfs_class_path = "/sys/class/fpga",
	 .sysfs_pcidrv_fpga = "fpga",
	 .sysfs_device_fmt = "(intel-fpga-dev\\.)([0-9]+)",
	 .sysfs_region_fmt = "intel-fpga-(fme|port)\\.([0-9]+)",
	 .sysfs_device_glob = "intel-fpga-dev.*",
	 .sysfs_fme_glob = "intel-fpga-fme.*",
	 .sysfs_port_glob = "intel-fpga-port.*",
	 .sysfs_compat_id = "pr/interface_id",
	 .sysfs_fme_temp_glob = "thermal_mgmt/*",
	 .sysfs_fme_pwr_glob = "power_mgmt/*",
	 .sysfs_fme_perf_glob = "*perf",
	 .sysfs_port_err = "errors/errors",
	 .sysfs_port_err_clear = "errors/clear",
	 .sysfs_bmc_glob = "avmmi-bmc.*/bmc_info",
	 .sysfs_max10_glob = "spi-*/spi_master/spi*/spi*.*"
	} };

// RE_MATCH_STRING is index 0 in a regex match array
#define RE_MATCH_STRING 0
// RE_DEVICE_GROUPS is the matching groups for the device regex in the
// sysfs_path_table above.
// Currently this only has three groups:
// * The matching string itself - group 0
// * The prefix (either 'region' or 'intel-fpga-dev.') - group 1
// * The number - group 2
// These indices are used when indexing a regex match object
#define RE_DEVICE_GROUPS 3
#define RE_DEVICE_GROUP_PREFIX 1
#define RE_DEVICE_GROUP_NUM 2

// RE_REGION_GROUPS is the matching groups for the region regex in the
// sysfs_path_table above.
// Currently this only has three groups:
// * The matching string itself - group 0
// * The type ('fme' or 'port') - group 1
// * The number - group 2
// These indices are used when indexing a regex match object
#define RE_REGION_GROUPS 3
#define RE_REGION_GROUP_TYPE 1
#define RE_REGION_GROUP_NUM 2

static sysfs_formats *_sysfs_format_ptr;
static uint32_t _sysfs_device_count;
/* mutex to protect sysfs device data structures */
pthread_mutex_t _sysfs_device_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define SYSFS_FORMAT(s) (_sysfs_format_ptr ? _sysfs_format_ptr->s : NULL)


#define SYSFS_MAX_DEVICES 128
static sysfs_fpga_device _devices[SYSFS_MAX_DEVICES];

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])/fpga"
#define PCIE_PATH_PATTERN_GROUPS 5

#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			OPAE_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)

#define FREE_IF(var)                                                           \
	do {                                                                   \
		if (var) {                                                     \
			free(var);                                             \
			var = NULL;                                            \
		}                                                              \
	} while (0)

STATIC int parse_pcie_info(sysfs_fpga_device *device, char *buffer)
{
	char err[128] = {0};
	regex_t re;
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };
	int res = FPGA_EXCEPTION;

	int reg_res = regcomp(&re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);
	if (reg_res) {
		OPAE_ERR("Error compling regex");
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, buffer, PCIE_PATH_PATTERN_GROUPS, matches, 0);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		OPAE_ERR("Error executing regex: %s", err);
		res = FPGA_EXCEPTION;
		goto out;
	} else {
		PARSE_MATCH_INT(buffer, matches[1], device->segment, 16, out);
		PARSE_MATCH_INT(buffer, matches[2], device->bus, 16, out);
		PARSE_MATCH_INT(buffer, matches[3], device->device, 16, out);
		PARSE_MATCH_INT(buffer, matches[4], device->function, 10, out);
	}
	res = FPGA_OK;

out:
	regfree(&re);
	return res;
}

int sysfs_parse_attribute64(const char *root, const char *attr_path, uint64_t *value)
{
	uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
	char path[SYSFS_PATH_MAX];
	char buffer[pg_size];
	int fd = -1;
	ssize_t bytes_read = 0;
	int len = snprintf_s_ss(path, SYSFS_PATH_MAX, "%s/%s",
				root, attr_path);
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			 root, attr_path);
		return FPGA_EXCEPTION;
	}
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		OPAE_MSG("Error opening %s: %s", path, strerror(errno));
		return FPGA_EXCEPTION;
	}
	bytes_read = eintr_read(fd, buffer, pg_size);
	if (bytes_read < 0) {
		OPAE_ERR("Error reading from %s: %s", path,
			 strerror(errno));
		close(fd);
		return FPGA_EXCEPTION;
	}

	*value = strtoull(buffer, NULL, 0);

	close(fd);
	return FPGA_OK;
}

STATIC int parse_device_vendor_id(sysfs_fpga_device *device)
{
	uint64_t value = 0;
	int res = sysfs_parse_attribute64(device->sysfs_path, "device/device", &value);
	if (res) {
		OPAE_MSG("Error parsing device_id for device: %s",
			 device->sysfs_path);
		return res;
	}
	device->device_id = value;

	res = sysfs_parse_attribute64(device->sysfs_path, "device/vendor", &value);

	if (res) {
		OPAE_ERR("Error parsing vendor_id for device: %s",
			 device->sysfs_path);
		return res;
	}
	device->vendor_id = value;

	return FPGA_OK;
}

STATIC sysfs_fpga_region *make_region(sysfs_fpga_device *device, char *name,
				      int num, fpga_objtype type)
{
	sysfs_fpga_region *region = malloc(sizeof(sysfs_fpga_region));
	if (region == NULL) {
		OPAE_ERR("error creating region");
		return NULL;
	}
	region->device = device;
	region->type = type;
	region->number = num;
	// sysfs path of region is sysfs path of device + / + name
	if (snprintf_s_ss(region->sysfs_path, sizeof(region->sysfs_path),
			  "%s/%s", device->sysfs_path, name)
	    < 0) {
		OPAE_ERR("error formatting path");
		free(region);
		return NULL;
	}

	if (strcpy_s(region->sysfs_name, sizeof(region->sysfs_name),  name)) {
		OPAE_ERR("Error copying sysfs name");
		free(region);
		return NULL;
	}

	return region;
}

/**
 * @brief Match a device node given a format pattern
 *
 * @param fmt A regex pattern for the device node
 * @param inpstr A sysfs path to a potential device node
 * @param(out) prefix[] A prefix string for the device node
 * @param prefix_len capacity of prefix (max length)
 * @param(out) num The sysfs number encoded in the name
 *
 * @note fmt is expected to be a regex pattern in our sysfs_format_table
 *       Matching input strings could could look like:
 *       * region0 where 'region' is the prefix and 0 is the num
 *       * intel-fpga-dev.0 where 'intel-fpga-dev.' is the prefix and 0 is the
 *       num
 *
 *
 * @return FPGA_OK if a match is found, FPGA_NOT_FOUND it no match is found,
 *         FPGA_EXCEPTION if an error is encountered
 */
STATIC fpga_result re_match_device(const char *fmt, char *inpstr, char prefix[],
				   size_t prefix_len, int *num)
{
	int reg_res = 0;
	fpga_result res = FPGA_EXCEPTION;
	regmatch_t matches[RE_DEVICE_GROUPS];
	char err[128];
	char *ptr = NULL;
	char *end = NULL;
	regex_t re;

	ASSERT_NOT_NULL(fmt);
	ASSERT_NOT_NULL(inpstr);
	ASSERT_NOT_NULL(prefix);
	ASSERT_NOT_NULL(num);
	reg_res = regcomp(&re, fmt, REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &re, err, sizeof(err));
		OPAE_ERR("Error compiling regex: %s", err);
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, inpstr, RE_DEVICE_GROUPS, matches, 0);
	if (reg_res) {
		return FPGA_NOT_FOUND;
	}

	ptr = inpstr + matches[RE_DEVICE_GROUP_PREFIX].rm_so;
	end = inpstr + matches[RE_DEVICE_GROUP_PREFIX].rm_eo;
	if (strncpy_s(prefix, prefix_len, ptr, end - ptr)) {
		OPAE_ERR("Error copying prefix from string: %s", inpstr);
		goto out_free;
	}
	*(prefix + (end - ptr)) = '\0';
	ptr = inpstr + matches[RE_DEVICE_GROUP_NUM].rm_so;
	errno = 0;
	*num = strtoul(ptr, NULL, 10);
	if (errno) {
		OPAE_ERR("Error parsing number: %s", inpstr);
		goto out_free;
	}
	res = FPGA_OK;
out_free:
	regfree(&re);
	return res;
}

/**
 * @brief Match a device node given a format pattern
 *
 * @param fmt A regex pattern for the device node
 * @param inpstr A sysfs path to a potential device node
 * @param(out) type[] A type string for the device node
 * @param type_len capacity of type (max length)
 * @param(out) num The sysfs number encoded in the name
 *
 * @note fmt is expected to be a regex pattern in our sysfs_format_table
 *       Matching input strings could could look like:
 *       * dfl-fme.0 where 'fme' is the type and 0 is the num
 *       * dfl-port.1 where 'port' is the type and 1 is the num
 *       * intel-fpga-fme.0 where 'fme' is the type and 0 is the num
 *       * intel-fpga-port.1 where 'port' is the type and 1 is the num
 *
 *
 * @return FPGA_OK if a match is found, FPGA_NOT_FOUND it no match is found,
 *         FPGA_EXCEPTION if an error is encountered
 */
STATIC fpga_result re_match_region(const char *fmt, char *inpstr, char type[],
				   size_t type_len, int *num)
{
	int reg_res = 0;
	fpga_result res = FPGA_EXCEPTION;
	regmatch_t matches[RE_REGION_GROUPS];
	char err[128];
	char *ptr = NULL;
	char *end = NULL;
	regex_t re;

	ASSERT_NOT_NULL(fmt);
	ASSERT_NOT_NULL(inpstr);
	ASSERT_NOT_NULL(type);
	ASSERT_NOT_NULL(num);
	reg_res = regcomp(&re, fmt, REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &re, err, sizeof(err));
		OPAE_ERR("Error compiling regex: %s", err);
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, inpstr, RE_REGION_GROUPS, matches, 0);
	if (reg_res) {
		res = FPGA_NOT_FOUND;
		goto out_free;
	}

	ptr = inpstr + matches[RE_REGION_GROUP_TYPE].rm_so;
	end = inpstr + matches[RE_REGION_GROUP_TYPE].rm_eo;
	if (strncpy_s(type, type_len, ptr, end - ptr)) {
		OPAE_ERR("Error copying type from string: %s", inpstr);
		goto out_free;
	}
	*(type + (end - ptr)) = '\0';
	ptr = inpstr + matches[RE_REGION_GROUP_NUM].rm_so;
	errno = 0;
	*num = strtoul(ptr, NULL, 10);
	if (errno) {
		OPAE_ERR("Error parsing number: %s", inpstr);
		goto out_free;
	}
	res = FPGA_OK;
out_free:
	regfree(&re);
	return res;
}


STATIC int find_regions(sysfs_fpga_device *device)
{
	int num = -1;
	char type[8];
	fpga_result res = FPGA_OK;
	fpga_result match_res = FPGA_NOT_FOUND;
	fpga_objtype region_type = FPGA_DEVICE;
	sysfs_fpga_region **region_ptr = NULL;
	struct dirent *dirent = NULL;
	DIR *dir = opendir(device->sysfs_path);
	if (!dir) {
		OPAE_ERR("failed to open device path: %s", device->sysfs_path);
		return FPGA_EXCEPTION;
	}

	while ((dirent = readdir(dir)) != NULL) {
		res = FPGA_OK;
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		match_res = re_match_region(SYSFS_FORMAT(sysfs_region_fmt),
					    dirent->d_name, type, sizeof(type),
					    &num);
		if (match_res == FPGA_OK) {
			if (!strncmp(FPGA_SYSFS_FME, type,
				     FPGA_SYSFS_FME_LEN)) {
				region_type = FPGA_DEVICE;
				region_ptr = &device->fme;
			} else if (!strncmp(FPGA_SYSFS_PORT, type,
					    FPGA_SYSFS_PORT_LEN)) {
				region_type = FPGA_ACCELERATOR;
				region_ptr = &device->port;
			}
			*region_ptr = make_region(device, dirent->d_name, num,
						  region_type);
		} else if (match_res != FPGA_NOT_FOUND) {
			res = match_res;
			break;
		}
	}

	if (dir)
		closedir(dir);
	if (!device->fme && !device->port) {
		OPAE_ERR("did not find fme/port in device: %s", device->sysfs_path);
		return FPGA_NOT_FOUND;
	}

	return res;
}


STATIC int make_device(sysfs_fpga_device *device, const char *sysfs_class_fpga,
		       char *dir_name, int num)
{
	int res = FPGA_OK;
	char buffer[SYSFS_PATH_MAX] = {0};
	ssize_t sym_link_len = 0;
	if (snprintf_s_ss(device->sysfs_path, SYSFS_PATH_MAX, "%s/%s",
			  sysfs_class_fpga, dir_name)
	    < 0) {
		OPAE_ERR("Error formatting sysfs paths");
		return FPGA_EXCEPTION;
	}

	if (snprintf_s_s(device->sysfs_name, SYSFS_PATH_MAX, "%s", dir_name) < 0) {
		OPAE_ERR("Error formatting sysfs name");
		return FPGA_EXCEPTION;
	}

	sym_link_len = readlink(device->sysfs_path, buffer, SYSFS_PATH_MAX);
	if (sym_link_len < 0) {
		OPAE_ERR("Error reading sysfs link: %s", device->sysfs_path);
		return FPGA_EXCEPTION;
	}

	device->number = num;
	res = parse_pcie_info(device, buffer);

	if (res) {
		OPAE_ERR("Could not parse symlink");
		return res;
	}

	res = parse_device_vendor_id(device);
	if (res) {
		OPAE_MSG("Could not parse vendor/device id");
		return res;
	}

	return find_regions(device);
}



STATIC int sysfs_device_destroy(sysfs_fpga_device *device)
{
	ASSERT_NOT_NULL(device);
	if (device->fme) {
		free(device->fme);
		device->fme = NULL;
	}
	if (device->port) {
		free(device->port);
		device->port = NULL;
	}
	return FPGA_OK;
}

int sysfs_device_count(void)
{
	int res = 0, count = 0;
	if (!opae_mutex_lock(res, &_sysfs_device_lock)) {
		count = _sysfs_device_count;
	}

	if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
		count = 0;
	}

	return count;
}

fpga_result sysfs_foreach_device(device_cb cb, void *context)
{
	uint32_t i = 0;
	int res = 0;
	fpga_result result = FPGA_OK;
	if (opae_mutex_lock(res, &_sysfs_device_lock)) {
		return FPGA_EXCEPTION;
	}

	result = sysfs_finalize();
	if (result) {
		goto out_unlock;
	}
	result = sysfs_initialize();
	if (result) {
		goto out_unlock;
	}
	for (; i < _sysfs_device_count; ++i) {
		result = cb(&_devices[i], context);
		if (result) {
			goto out_unlock;
		}
	}

out_unlock:
	opae_mutex_unlock(res, &_sysfs_device_lock);

	return result;
}

int sysfs_initialize(void)
{
	int stat_res = -1;
	int res = FPGA_OK;
	uint32_t i = 0;
	struct stat st;
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	int num = -1;
	char prefix[64] = {0};

	for (i = 0; i < OPAE_KERNEL_DRIVERS; ++i) {
		errno = 0;
		stat_res = stat(sysfs_path_table[i].sysfs_class_path, &st);
		if (!stat_res) {
			_sysfs_format_ptr = &sysfs_path_table[i];
			break;
		}
		if (errno != ENOENT) {
			OPAE_ERR("Error while inspecting sysfs: %s",
				 strerror(errno));
			return FPGA_EXCEPTION;
		}
	}
	if (i == OPAE_KERNEL_DRIVERS) {
		OPAE_ERR(
			"No valid sysfs class files found - a suitable driver may not be loaded");
		return FPGA_NO_DRIVER;
	}

	_sysfs_device_count = 0;

	const char *sysfs_class_fpga = SYSFS_FORMAT(sysfs_class_path);
	if (!sysfs_class_fpga) {
		OPAE_ERR("Invalid fpga class path: %s", sysfs_class_fpga);
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	// open the root sysfs class directory
	// look in the directory and get device objects
	dir = opendir(sysfs_class_fpga);
	if (!dir) {
		OPAE_MSG("failed to open device path: %s", sysfs_class_fpga);
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		res = re_match_device(SYSFS_FORMAT(sysfs_device_fmt),
				      dirent->d_name, prefix, sizeof(prefix),
				      &num);
		if (res == FPGA_OK) {
			// increment our device count after filling out details
			// of the discovered device in our _devices array
			if (opae_mutex_lock(res, &_sysfs_device_lock)) {
				goto out_free;
			}
			if (make_device(&_devices[_sysfs_device_count++],
					sysfs_class_fpga, dirent->d_name,
					num)) {
				OPAE_MSG("Error processing device: %s",
					 dirent->d_name);
				_sysfs_device_count--;
			}
			if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
				goto out_free;
			}
		}
	}

	if (!_sysfs_device_count) {
		OPAE_ERR("Error discovering fpga devices");
		res = FPGA_NO_DRIVER;
	}
out_free:
	if (dir)
		closedir(dir);
	return res;
}

int sysfs_finalize(void)
{
	uint32_t i = 0;
	int res = 0;
	if (opae_mutex_lock(res, &_sysfs_device_lock)) {
		OPAE_ERR("Error locking mutex");
		return FPGA_EXCEPTION;
	}
	for (; i < _sysfs_device_count; ++i) {
		sysfs_device_destroy(&_devices[i]);
	}
	_sysfs_device_count = 0;
	_sysfs_format_ptr = NULL;
	if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
		OPAE_ERR("Error unlocking mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

const sysfs_fpga_device *sysfs_get_device(size_t num)
{
	const sysfs_fpga_device *ptr = NULL;
	int res = 0;
	if (!opae_mutex_lock(res, &_sysfs_device_lock)) {
		if (num >= _sysfs_device_count) {
			OPAE_ERR("No such device with index: %d", num);
		} else {
			ptr = &_devices[num];
		}
		if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
			ptr = NULL;
		}
	}

	return ptr;
}

fpga_result sysfs_get_interface_id(fpga_token token, fpga_guid guid)
{
	fpga_result res = FPGA_OK;
	char path[SYSFS_PATH_MAX];
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);
	res = cat_token_sysfs_path(path, token, SYSFS_FORMAT(sysfs_compat_id));
	if (res) {
		return res;
	}
	res = opae_glob_path(path);
	if (res) {
		return res;
	}
	return sysfs_read_guid(path, guid);
}



fpga_result sysfs_get_fme_pwr_path(fpga_token token, char *sysfs_pwr)
{
	fpga_result res = FPGA_OK;
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);

	if (sysfs_pwr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	res = cat_token_sysfs_path(sysfs_pwr, token, SYSFS_FORMAT(sysfs_fme_pwr_glob));
	if (res != FPGA_OK) {
		return res;
	}

	// check for path is valid
	res = check_sysfs_path_is_valid(sysfs_pwr);
	if (res != FPGA_OK) {
		OPAE_MSG("Invalid path %s", sysfs_pwr);
		return res;
	}

	return res;
}

fpga_result sysfs_get_fme_temp_path(fpga_token token, char *sysfs_temp)
{
	fpga_result res = FPGA_OK;
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);

	if (sysfs_temp == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = cat_token_sysfs_path(sysfs_temp, token, SYSFS_FORMAT(sysfs_fme_temp_glob));
	if (res != FPGA_OK) {
		return res;
	}

	// check for path is valid
	res = check_sysfs_path_is_valid(sysfs_temp);
	if (res != FPGA_OK) {
		OPAE_MSG("Invalid path %s", sysfs_temp);
		return res;
	}

	return res;
}

fpga_result sysfs_get_fme_perf_path(fpga_token token, char *sysfs_perf)
{
	fpga_result res = FPGA_OK;
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);

	if (sysfs_perf == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = cat_token_sysfs_path(sysfs_perf, token, SYSFS_FORMAT(sysfs_fme_perf_glob));
	if (res != FPGA_OK) {
		return res;
	}

	// check for path is valid
	res = check_sysfs_path_is_valid(sysfs_perf);
	if (res != FPGA_OK) {
		OPAE_MSG("Invalid path %s", sysfs_perf);
		return res;
	}

	return res;
}

fpga_result sysfs_get_port_error_path(fpga_handle handle, char *sysfs_port_error)
{
	fpga_result result = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX] = { 0 };

	if (sysfs_port_error == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	result = get_port_sysfs(handle, sysfs_path);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get port syfs path");
		return result;
	}

	int len = snprintf_s_ss(sysfs_port_error, SYSFS_PATH_MAX,
		"%s/%s", sysfs_path, SYSFS_FORMAT(sysfs_port_err));

	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			sysfs_path, "port error");
		return FPGA_EXCEPTION;
	}

	return result;
}

fpga_result sysfs_get_port_error_clear_path(fpga_handle handle, char *sysfs_port_error_clear)
{
	fpga_result result = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX] = { 0 };

	if (sysfs_port_error_clear == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	result = get_port_sysfs(handle, sysfs_path);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get port syfs path");
		return result;
	}

	int len = snprintf_s_ss(sysfs_port_error_clear, SYSFS_PATH_MAX,
		"%s/%s", sysfs_path, SYSFS_FORMAT(sysfs_port_err_clear));
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			sysfs_path, "port clear error");
		return FPGA_EXCEPTION;
	}

	return result;
}

fpga_result sysfs_get_bmc_path(fpga_token token, char *sysfs_bmc)
{
	fpga_result res = FPGA_OK;
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);

	if (sysfs_bmc == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = cat_token_sysfs_path(sysfs_bmc, token, SYSFS_FORMAT(sysfs_bmc_glob));
	if (res != FPGA_OK) {
		return res;
	}

	return opae_glob_path(sysfs_bmc);
}

fpga_result sysfs_get_max10_path(fpga_token token, char *sysfs_max10)
{
	fpga_result res = FPGA_OK;
	struct _fpga_token *_token = (struct _fpga_token *)token;
	ASSERT_NOT_NULL(_token);

	if (sysfs_max10 == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = cat_token_sysfs_path(sysfs_max10, token, SYSFS_FORMAT(sysfs_max10_glob));
	if (res != FPGA_OK) {
		return res;
	}

	return opae_glob_path(sysfs_max10);
}

fpga_result sysfs_get_fme_pr_interface_id(const char *sysfs_sysfs_path, fpga_guid guid)
{
	fpga_result res = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX];

	int len = snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/%s",
		sysfs_sysfs_path, SYSFS_FORMAT(sysfs_compat_id));
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			sysfs_sysfs_path, sysfs_path);
		return FPGA_EXCEPTION;
	}

	res = opae_glob_path(sysfs_path);
	if (res) {
		return res;
	}
	return sysfs_read_guid(sysfs_path, guid);
}

fpga_result sysfs_get_guid(fpga_token token, const char *sysfspath, fpga_guid guid)
{
	fpga_result res = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX];
	struct _fpga_token *_token = (struct _fpga_token *)token;

	if (_token == NULL || sysfspath == NULL)
		return FPGA_EXCEPTION;

	int len = snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/%s",
		_token->sysfspath, sysfspath);
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			_token->sysfspath, sysfs_path);
		return FPGA_EXCEPTION;
	}
	res = opae_glob_path(sysfs_path);
	if (res) {
		return res;
	}
	return sysfs_read_guid(sysfs_path, guid);
}

int sysfs_filter(const struct dirent *de)
{
	return de->d_name[0] != '.';
}


/**
 * @brief Get a path to an fme node given a path to a port node
 *
 * @param sysfs_port sysfs path to a port node
 * @param(out) sysfs_fme realpath to an fme node in sysfs
 *
 * @return FPGA_OK if able to find the path to the fme
 *         FPGA_EXCEPTION if errors encountered during copying,
 *         formatting strings
 *         FPGA_NOT_FOUND if unable to find fme path or any relevant paths
 */
fpga_result sysfs_get_fme_path(const char *sysfs_port, char *sysfs_fme)
{
	fpga_result result = FPGA_EXCEPTION;
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	char fpga_path[SYSFS_PATH_MAX]    = {0};
	// subdir candidates to look for when locating "fpga*" node in sysfs
	// order is important here because a physfn node is the exception
	// (will only exist when a port is on a VF) and will be used to point
	// to the PF that the FME is on
	const char *fpga_globs[] = {"device/physfn/fpga*", "device/fpga*", NULL};
	int i = 0;

	// now try globbing fme resource sysfs path + a candidate
	// sysfs_port is expected to be the sysfs path to a port
	for (; fpga_globs[i]; ++i) {
		if (snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/../%s",
				  sysfs_port, fpga_globs[i])
		    < 0) {
			OPAE_ERR("Error formatting sysfs path");
			return FPGA_EXCEPTION;
		}
		result = opae_glob_path(sysfs_path);
		if (result == FPGA_OK) {
			// we've found a path to the "fpga*" node
			break;
		} else if (result != FPGA_NOT_FOUND) {
			return result;
		}
	}

	if (!fpga_globs[i]) {
		OPAE_ERR("Could not find path to port device/fpga*");
		return FPGA_NOT_FOUND;
	}


	// format a string to look for in the subdirectory of the "fpga*" node
	// this subdirectory should include glob patterns for the current
	// driver
	// -- intel-fpga-dev.*/intel-fpga-fme.*
	// -- region*/dfl-fme.*
	if (snprintf_s_ss(fpga_path, SYSFS_PATH_MAX, "/%s/%s",
			  SYSFS_FORMAT(sysfs_device_glob),
			  SYSFS_FORMAT(sysfs_fme_glob))
	    < 0) {
		OPAE_ERR("Error formatting sysfs path");
		return FPGA_EXCEPTION;
	}

	// now concatenate the subdirectory to the "fpga*" node
	if (strcat_s(sysfs_path, sizeof(sysfs_path), fpga_path)) {
		OPAE_ERR("Error concatenating path to fpga node");
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(sysfs_path);
	if (result) {
		return result;
	}

	// copy the assembled and verified path to the output param
	if (!realpath(sysfs_path, sysfs_fme)) {
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

//
// sysfs access (read/write) functions
//

fpga_result sysfs_read_int(const char *path, int *i)
{
	int fd;
	int res;
	char buf[SYSFS_PATH_MAX];
	int b;

	if (path == NULL) {
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

	b = 0;

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

	*i = atoi(buf);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

fpga_result sysfs_read_u32(const char *path, uint32_t *u)
{
	int fd;
	int res;
	char buf[SYSFS_PATH_MAX];
	int b;

	if (path == NULL) {
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

	b = 0;

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

	*u = strtoul(buf, NULL, 0);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

// read tuple separated by 'sep' character
fpga_result sysfs_read_u32_pair(const char *path, uint32_t *u1, uint32_t *u2,
				char sep)
{
	int fd;
	int res;
	char buf[SYSFS_PATH_MAX];
	int b;
	char *c;
	uint32_t x1, x2;

	if (sep == '\0') {
		OPAE_MSG("invalid separation character");
		return FPGA_INVALID_PARAM;
	}

	if (path == NULL) {
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

	b = 0;

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

	// read first value
	x1 = strtoul(buf, &c, 0);
	if (*c != sep) {
		OPAE_MSG("couldn't find separation character '%c' in '%s'", sep,
			 path);
		goto out_close;
	}
	// read second value
	x2 = strtoul(c + 1, &c, 0);
	if (*c != '\0') {
		OPAE_MSG("unexpected character '%c' in '%s'", *c, path);
		goto out_close;
	}

	*u1 = x1;
	*u2 = x2;

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

fpga_result sysfs_read_u64(const char *path, uint64_t *u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;

	if (path == NULL) {
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

fpga_result sysfs_write_u64(const char *path, uint64_t u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;
	int len;

	if (path == NULL) {
		OPAE_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		OPAE_MSG("open(%s) failed: %s", path, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		OPAE_MSG("seek: %s", strerror(errno));
		goto out_close;
	}

	len = snprintf_s_l(buf, sizeof(buf), "0x%lx\n", u);

	do {
		res = write(fd, buf + b, len - b);
		if (res <= 0) {
			OPAE_ERR("Failed to write");
			goto out_close;
		}
		b += res;

		if (b > len || b <= 0) {
			OPAE_MSG("Unexpected size writing to %s", path);
			goto out_close;
		}

	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0'
		 && b < len);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}


fpga_result sysfs_write_u64_decimal(const char *path, uint64_t u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;
	int len;

	if (path == NULL) {
		OPAE_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		OPAE_MSG("open(%s) failed: %s", path, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		OPAE_MSG("seek: %s", strerror(errno));
		goto out_close;
	}

	len = snprintf_s_l(buf, sizeof(buf), "%ld\n", u);

	do {
		res = write(fd, buf + b, len - b);
		if (res <= 0) {
			OPAE_ERR("Failed to write");
			goto out_close;
		}
		b += res;

		if (b > len || b <= 0) {
			OPAE_MSG("Unexpected size writing to %s", path);
			goto out_close;
		}

	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0'
		 && b < len);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

fpga_result sysfs_read_guid(const char *path, fpga_guid guid)
{
	int fd;
	int res;
	char buf[SYSFS_PATH_MAX];
	int b;

	int i;
	char tmp;
	unsigned octet;

	if (path == NULL) {
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

	b = 0;

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

	for (i = 0; i < 32; i += 2) {
		tmp = buf[i + 2];
		buf[i + 2] = 0;

		octet = 0;
		sscanf_s_u(&buf[i], "%x", &octet);
		guid[i / 2] = (uint8_t)octet;

		buf[i + 2] = tmp;
	}

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

fpga_result check_sysfs_path_is_valid(const char *sysfs_path)
{
	fpga_result result = FPGA_OK;
	char path[SYSFS_PATH_MAX] = { 0 };
	struct stat stats;
	if (!sysfs_path) {
		OPAE_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	if (strcpy_s(path, SYSFS_PATH_MAX, sysfs_path)) {
		OPAE_ERR("Could not copy globbed path");
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(path);
	if (result) {
		return result;
	}

	if (stat(path, &stats) != 0) {
		OPAE_ERR("stat failed: %s", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (S_ISDIR(stats.st_mode) || S_ISREG(stats.st_mode)) {
		return FPGA_OK;
	}

	return FPGA_EXCEPTION;
}


fpga_result sysfs_path_is_valid(const char *root, const char *attr_path)
{
	char path[SYSFS_PATH_MAX]    = {0};
	fpga_result result          = FPGA_OK;
	struct stat stats;

	int len = snprintf_s_ss(path, SYSFS_PATH_MAX, "%s/%s",
		root, attr_path);
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			root, attr_path);
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(path);
	if (result) {
		return result;
	}

	if (stat(path, &stats) != 0) {
		OPAE_ERR("stat failed: %s", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (S_ISDIR(stats.st_mode) || S_ISREG(stats.st_mode)) {
		return FPGA_OK;
	}

	return FPGA_EXCEPTION;
}

//
// sysfs convenience functions to access device components by device number
//

fpga_result sysfs_get_socket_id(int dev, int subdev, uint8_t *socket_id)
{
	fpga_result result;
	char spath[SYSFS_PATH_MAX];
	int i;

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_FME_PATH_FMT
		      "/" FPGA_SYSFS_SOCKET_ID,
		      dev, subdev);

	i = 0;
	result = sysfs_read_int(spath, &i);
	if (FPGA_OK != result)
		return result;

	*socket_id = (uint8_t)i;

	return FPGA_OK;
}

fpga_result sysfs_get_afu_id(int dev, int subdev, fpga_guid guid)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT
		      "/" FPGA_SYSFS_AFU_GUID,
		      dev, subdev);

	return sysfs_read_guid(spath, guid);
}

fpga_result sysfs_get_pr_id(int dev, int subdev, fpga_guid guid)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_FME_PATH_FMT
		      "/" FPGA_SYSFS_FME_INTERFACE_ID,
		      dev, subdev);

	return sysfs_read_guid(spath, guid);
}

fpga_result sysfs_get_slots(int dev, int subdev, uint32_t *slots)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_FME_PATH_FMT
		      "/" FPGA_SYSFS_NUM_SLOTS,
		      dev, subdev);

	return sysfs_read_u32(spath, slots);
}

fpga_result sysfs_get_bitstream_id(int dev, int subdev, uint64_t *id)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_FME_PATH_FMT
		      "/" FPGA_SYSFS_BITSTREAM_ID,
		      dev, subdev);

	return sysfs_read_u64(spath, id);
}

/**
 * @brief Get a path to a port node given a handle to an resource
 *
 * @param handle Open handle to an fme resource (FPGA_DEVICE)
 * @param(out) sysfs_port realpath to a port node in sysfs
 *
 * @return FPGA_OK if able to find the path to the port
 *         FPGA_EXCEPTION if errors encountered during copying,
 *         formatting strings
 *         FPGA_NOT_FOUND if unable to find fme path or any relevant paths
 */
fpga_result get_port_sysfs(fpga_handle handle, char *sysfs_port)
{

	struct _fpga_token *_token;
	struct _fpga_handle *_handle      = (struct _fpga_handle *)handle;
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	char fpga_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                = FPGA_OK;
	int i = 0;
	// subdir candidates to look for when locating "fpga*" node in sysfs
	// order is important here because a virtfn* node is the exception
	// (will only exist when a port is on a VF) and will be used to point
	// to the VF that the port is on
	const char *fpga_globs[] = {"device/virtfn*/fpga*", "device/fpga*", NULL};
	if (sysfs_port == NULL) {
		OPAE_ERR("Invalid output pointer");
		return FPGA_INVALID_PARAM;
	}

	if (_handle == NULL) {
		OPAE_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	if (!strstr(_token->sysfspath, FPGA_SYSFS_FME)) {
		OPAE_ERR("Invalid sysfspath in token");
		return FPGA_INVALID_PARAM;
	}

	// now try globbing fme token's sysfs path + a candidate
	for (; fpga_globs[i]; ++i) {
		if (snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/../%s",
				  _token->sysfspath, fpga_globs[i])
		    < 0) {
			OPAE_ERR("Error formatting sysfs path");
			return FPGA_EXCEPTION;
		}
		result = opae_glob_path(sysfs_path);
		if (result == FPGA_OK) {
			// we've found a path to the "fpga*" node
			break;
		} else if (result != FPGA_NOT_FOUND) {
			return result;
		}
	}

	if (!fpga_globs[i]) {
		OPAE_ERR("Could not find path to port device/fpga");
		return FPGA_EXCEPTION;
	}

	// format a string to look for in the subdirectory of the "fpga*" node
	// this subdirectory should include glob patterns for the current
	// driver
	// -- intel-fgga-dev.*/intel-fpga-port.*
	// -- region*/dfl-port.*
	if (snprintf_s_ss(fpga_path, SYSFS_PATH_MAX, "/%s/%s",
			  SYSFS_FORMAT(sysfs_device_glob),
			  SYSFS_FORMAT(sysfs_port_glob))
	    < 0) {
		OPAE_ERR("Error formatting sysfs path");
		return FPGA_EXCEPTION;
	}

	// now concatenate the subdirectory to the "fpga*" node
	if (strcat_s(sysfs_path, sizeof(sysfs_path), fpga_path)) {
		OPAE_ERR("Error concatenating path to fpga node");
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(sysfs_path);
	if (result) {
		return result;
	}


	// copy the assembled and verified path to the output param
	if (!realpath(sysfs_path, sysfs_port)) {
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

enum fpga_hw_type opae_id_to_hw_type(uint16_t vendor_id, uint16_t device_id)
{
	enum fpga_hw_type hw_type = FPGA_HW_UNKNOWN;

	if (vendor_id == 0x8086) {

		switch (device_id) {
		case 0xbcbc:
		case 0xbcbd:
		case 0xbcbe:
		case 0xbcbf:
		case 0xbcc0:
		case 0xbcc1:
		case 0x09cb:
			hw_type = FPGA_HW_MCP;
		break;

		case 0x09c4:
		case 0x09c5:
			hw_type = FPGA_HW_DCP_RC;
		break;

		case 0x0b2b:
		case 0x0b2c:
			hw_type = FPGA_HW_DCP_DC;
		break;

		case 0x0b30:
		case 0x0b31:
			hw_type = FPGA_HW_DCP_VC;
		break;

		default:
			OPAE_ERR("unknown device id: 0x%04x", device_id);
		}

	} else {
		OPAE_ERR("unknown vendor id: 0x%04x", vendor_id);
	}

	return hw_type;
}

// get fpga hardware type from handle
fpga_result get_fpga_hw_type(fpga_handle handle, enum fpga_hw_type *hw_type)
{
	struct _fpga_token *_token = NULL;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	char sysfs_path[SYSFS_PATH_MAX] = {0};
	fpga_result result = FPGA_OK;
	int err = 0;
	uint64_t vendor_id = 0;
	uint64_t device_id = 0;

	if (_handle == NULL) {
		OPAE_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	if (hw_type == NULL) {
		OPAE_ERR("Invalid input Parameters");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&_handle->lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Token not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	snprintf_s_s(sysfs_path, SYSFS_PATH_MAX, "%s/../device/vendor",
		_token->sysfspath);

	result = sysfs_read_u64(sysfs_path, &vendor_id);
	if (result != 0) {
		OPAE_ERR("Failed to read vendor ID");
		goto out_unlock;
	}

	snprintf_s_s(sysfs_path, SYSFS_PATH_MAX, "%s/../device/device",
		_token->sysfspath);

	result = sysfs_read_u64(sysfs_path, &device_id);
	if (result != 0) {
		OPAE_ERR("Failed to read device ID");
		goto out_unlock;
	}

	*hw_type = opae_id_to_hw_type((uint16_t)vendor_id,
				      (uint16_t)device_id);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

/*
 * The rlpath path is assumed to be of the form:
 * ../../devices/pci0000:5e/0000:5e:00.0/fpga/intel-fpga-dev.0
 */
fpga_result sysfs_sbdf_from_path(const char *sysfspath, int *s, int *b, int *d,
				 int *f)
{
	int res;
	char rlpath[SYSFS_PATH_MAX];
	char *p;

	res = readlink(sysfspath, rlpath, sizeof(rlpath));
	if (-1 == res) {
		OPAE_MSG("Can't read link %s (no driver?)", sysfspath);
		return FPGA_NO_DRIVER;
	}

	// Find the BDF from the link path.
	rlpath[res] = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		OPAE_MSG("Invalid link %s (no driver?)", rlpath);
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		OPAE_MSG("Invalid link %s (no driver?)", rlpath);
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		OPAE_MSG("Invalid link %s (no driver?)", rlpath);
		return FPGA_NO_DRIVER;
	}
	++p;

	//           11
	// 012345678901
	// ssss:bb:dd.f
	*f = (int)strtoul(p + 11, NULL, 16);
	*(p + 10) = 0;

	*d = (int)strtoul(p + 8, NULL, 16);
	*(p + 7) = 0;

	*b = (int)strtoul(p + 5, NULL, 16);
	*(p + 4) = 0;

	*s = (int)strtoul(p, NULL, 16);

	return FPGA_OK;
}

fpga_result sysfs_objectid_from_path(const char *sysfspath, uint64_t *object_id)
{
	char sdevpath[SYSFS_PATH_MAX];
	uint32_t major = 0;
	uint32_t minor = 0;
	fpga_result result;

	snprintf_s_s(sdevpath, SYSFS_PATH_MAX, "%s/dev", sysfspath);

	result = sysfs_read_u32_pair(sdevpath, &major, &minor, ':');
	if (FPGA_OK != result)
		return result;

	*object_id = ((major & 0xFFF) << 20) | (minor & 0xFFFFF);

	return FPGA_OK;
}

ssize_t eintr_read(int fd, void *buf, size_t count)
{
	ssize_t bytes_read = 0, total_read = 0;
	char *ptr = buf;
	while (total_read < (ssize_t)count) {
		bytes_read = read(fd, ptr + total_read, count - total_read);

		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
			return bytes_read;
		} else if (bytes_read == 0) {
			return lseek(fd, 0, SEEK_CUR);
		} else {
			total_read += bytes_read;
		}
	}
	return total_read;
}

ssize_t eintr_write(int fd, void *buf, size_t count)
{
	ssize_t bytes_written = 0, total_written = 0;
	char *ptr = buf;

	if (!buf) {
		return -1;
	}

	while (total_written < (ssize_t)count) {
		bytes_written =
			write(fd, ptr + total_written, count - total_written);
		if (bytes_written < 0) {
			if (errno == EINTR) {
				continue;
			}
			return bytes_written;
		}
		total_written += bytes_written;
	}
	return total_written;
}

fpga_result cat_token_sysfs_path(char *dest, fpga_token token, const char *path)
{
	if (!dest) {
		OPAE_ERR("destination str is NULL");
		return FPGA_EXCEPTION;
	}
	struct _fpga_token *_token = (struct _fpga_token *)token;
	int len = snprintf_s_ss(dest, SYSFS_PATH_MAX, "%s/%s",
				_token->sysfspath, path);
	if (len < 0) {
		OPAE_ERR("error concatenating strings (%s, %s)",
			 _token->sysfspath, path);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}


fpga_result cat_sysfs_path(char *dest, const char *path)
{
	errno_t err;

	err = strcat_s(dest, SYSFS_PATH_MAX, path);
	switch (err) {
	case EOK:
		return FPGA_OK;
	case ESNULLP:
		OPAE_ERR("NULL pointer in name");
		return FPGA_INVALID_PARAM;
		break;
	case ESZEROL:
		OPAE_ERR("Zero length");
		break;
	case ESLEMAX:
		OPAE_ERR("Length exceeds max");
		break;
	case ESUNTERM:
		OPAE_ERR("Destination not termindated");
		break;
	};

	return FPGA_EXCEPTION;
}

fpga_result cat_handle_sysfs_path(char *dest, fpga_handle handle,
				  const char *path)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)(handle);
	return cat_token_sysfs_path(dest, _handle->token, path);
}

STATIC char *cstr_dup(const char *str)
{
	size_t s = strlen(str);
	char *p = malloc(s+1);
	if (!p) {
		OPAE_ERR("malloc failed");
		return NULL;
	}
	if (strncpy_s(p, s+1, str, s)) {
		OPAE_ERR("Error copying string");
		free(p);
		return NULL;
	}
	p[s] = '\0';
	return p;
}

struct _fpga_object *alloc_fpga_object(const char *sysfspath, const char *name)
{
	struct _fpga_object *obj = calloc(1, sizeof(struct _fpga_object));
	if (obj) {
		pthread_mutexattr_t mattr;
		if (pthread_mutexattr_init(&mattr)) {
			OPAE_ERR("pthread_mutexattr_init() failed");
			goto out_err;
		}
		if (pthread_mutexattr_settype(&mattr,
					      PTHREAD_MUTEX_RECURSIVE)) {
			OPAE_ERR("pthread_mutexattr_settype() failed");
			pthread_mutexattr_destroy(&mattr);
			goto out_err;
		}
		if (pthread_mutex_init(&obj->lock, &mattr)) {
			OPAE_ERR("pthread_mutex_init() failed");
			pthread_mutexattr_destroy(&mattr);
			goto out_err;
		}

		pthread_mutexattr_destroy(&mattr);
		obj->handle = NULL;
		obj->path = cstr_dup(sysfspath);
		obj->name = cstr_dup(name);
		obj->perm = 0;
		obj->size = 0;
		obj->max_size = 0;
		obj->buffer = NULL;
		obj->objects = NULL;
	}
	return obj;
out_err:
	if (obj) {
		free(obj);
		obj = NULL;
	}
	return obj;
}

fpga_result destroy_fpga_object(struct _fpga_object *obj)
{
	fpga_result res = FPGA_OK;
	FREE_IF(obj->path);
	FREE_IF(obj->name);
	FREE_IF(obj->buffer);
	while (obj->size && obj->objects) {
		res = destroy_fpga_object(
			(struct _fpga_object *)obj->objects[--obj->size]);
		if (res) {
			OPAE_ERR("Error freeing subobject");
			return res;
		}
	}
	FREE_IF(obj->objects);

	if (pthread_mutex_unlock(&obj->lock)) {
		OPAE_MSG("pthread_mutex_unlock() failed");
	}

	if (pthread_mutex_destroy(&obj->lock)) {
		OPAE_ERR("Error destroying mutex");
		res = FPGA_EXCEPTION;
	}
	free(obj);
	return res;
}

fpga_result opae_glob_path(char *path)
{
	fpga_result res = FPGA_OK;
	glob_t pglob;
	pglob.gl_pathc = 0;
	pglob.gl_pathv = NULL;
	int globres = glob(path, 0, NULL, &pglob);
	if (!globres) {
		if (pglob.gl_pathc > 1) {
			OPAE_MSG("Ambiguous object key - using first one");
		}
		if (strcpy_s(path, FILENAME_MAX, pglob.gl_pathv[0])) {
			OPAE_ERR("Could not copy globbed path");
			res = FPGA_EXCEPTION;
		}
		globfree(&pglob);
	} else {
		switch (globres) {
		case GLOB_NOSPACE:
			res = FPGA_NO_MEMORY;
			break;
		case GLOB_NOMATCH:
			res = FPGA_NOT_FOUND;
			break;
		default:
			res = FPGA_EXCEPTION;
		}
		if (pglob.gl_pathv) {
			globfree(&pglob);
		}
	}
	return res;
}


fpga_result opae_glob_paths(const char *path, size_t found_max, char *found[],
			    size_t *num_found)
{
	fpga_result res = FPGA_OK;
	glob_t pglob;
	pglob.gl_pathc = 0;
	pglob.gl_pathv = NULL;
	int globres = glob(path, 0, NULL, &pglob);
	size_t i = 0;
	size_t to_copy = 0;

	if (!globres) {
		*num_found = pglob.gl_pathc;
		to_copy = *num_found < found_max ? *num_found : found_max;
		while (found && i < to_copy) {
			found[i] = cstr_dup(pglob.gl_pathv[i]);
			if (!found[i]) {
				// we had an error duplicating the string
				// undo what we've duplicated so far
				while (i) {
					free(found[--i]);
					found[i] = NULL;
				}
				OPAE_ERR("Could not copy globbed path");
				res = FPGA_EXCEPTION;
				goto out_free;
			}
			i++;
		}

	} else {
		switch (globres) {
		case GLOB_NOSPACE:
			res = FPGA_NO_MEMORY;
			break;
		case GLOB_NOMATCH:
			res = FPGA_NOT_FOUND;
			break;
		default:
			res = FPGA_EXCEPTION;
		}
	}
out_free:
	if (pglob.gl_pathv) {
		globfree(&pglob);
	}
	return res;
}

fpga_result sync_object(fpga_object obj)
{
	struct _fpga_object *_obj;
	int fd = -1;
	ssize_t bytes_read = 0;
	ASSERT_NOT_NULL(obj);
	_obj = (struct _fpga_object *)obj;
	fd = open(_obj->path, _obj->perm);
	if (fd < 0) {
		OPAE_ERR("Error opening %s: %s", _obj->path, strerror(errno));
		return FPGA_EXCEPTION;
	}
	bytes_read = eintr_read(fd, _obj->buffer, _obj->max_size);
	if (bytes_read < 0) {
		close(fd);
		return FPGA_EXCEPTION;
	}
	_obj->size = bytes_read;
	close(fd);
	return FPGA_OK;
}

fpga_result make_sysfs_group(char *sysfspath, const char *name,
			     fpga_object *object, int flags, fpga_handle handle)
{
	struct dirent **namelist;
	int n;
	size_t pathlen = strlen(sysfspath);
	char *ptr = NULL;
	errno_t err;
	fpga_object subobj;
	fpga_result res = FPGA_OK;
	struct _fpga_object *group;

	if (flags & FPGA_OBJECT_GLOB) {
		res = opae_glob_path(sysfspath);
	}
	if (res != FPGA_OK) {
		return res;
	}


	n = scandir(sysfspath, &namelist, sysfs_filter, alphasort);
	if (n < 0) {
		OPAE_ERR("Error calling scandir: %s", strerror(errno));
		switch (errno) {
		case ENOMEM:
			return FPGA_NO_MEMORY;
		case ENOENT:
			return FPGA_NOT_FOUND;
		}
		return FPGA_EXCEPTION;
	}

	if (n == 0) {
		OPAE_ERR("Group is empty");
		return FPGA_EXCEPTION;
	}

	group = alloc_fpga_object(sysfspath, name);
	if (!group) {
		res = FPGA_NO_MEMORY;
		goto out_free_namelist;
	}

	group->handle = handle;
	group->type = FPGA_SYSFS_DIR;
	if (flags & FPGA_OBJECT_RECURSE_ONE
	    || flags & FPGA_OBJECT_RECURSE_ALL) {
		ptr = sysfspath + pathlen;
		*ptr++ = '/';
		group->objects = calloc(n, sizeof(fpga_object));
		if (!group->objects) {
			res = FPGA_NO_MEMORY;
			goto out_free_group;
		}
		group->size = 0;
		while (n--) {
			err = strcpy_s(ptr, SYSFS_PATH_MAX - pathlen + 1,
				       namelist[n]->d_name);
			if (err == EOK) {
				if (flags & FPGA_OBJECT_RECURSE_ONE) {
					flags &= ~FPGA_OBJECT_RECURSE_ONE;
				}
				if (!make_sysfs_object(
					    sysfspath, namelist[n]->d_name,
					    &subobj, flags, handle)) {
					group->objects[group->size++] = subobj;
				}
			}
			free(namelist[n]);
		}
		free(namelist);
	} else {
		while (n--) {
			free(namelist[n]);
		}
		free(namelist);
	}

	*object = (fpga_object)group;
	return FPGA_OK;

out_free_group:
	if (destroy_fpga_object(group)) {
		OPAE_ERR("Error destroying object");
	}

out_free_namelist:
	while (n--)
		free(namelist[n]);
	free(namelist);

	return res;
}


fpga_result make_sysfs_array(char *sysfspath, const char *name,
			     fpga_object *object, int flags, fpga_handle handle,
			     char *objects[], size_t num_objects)
{
	fpga_result res = FPGA_OK;
	size_t i = 0;
	struct _fpga_object *array = alloc_fpga_object(sysfspath, name);
	char *oname = NULL;
	if (!array) {
		OPAE_ERR(
			"Error allocating memory for container of fpga_objects");
		return FPGA_NO_MEMORY;
	}
	array->objects = calloc(num_objects, sizeof(fpga_object));
	if (!array->objects) {
		OPAE_ERR("Error allocating memory for array of fpga_objects");
		destroy_fpga_object(array);
		return FPGA_NO_MEMORY;
	}

	array->handle = handle;
	array->type = FPGA_SYSFS_LIST;
	array->size = num_objects;
	for (i = 0; i < num_objects; ++i) {
		oname = strrchr(objects[i], '/');
		if (!oname) {
			OPAE_ERR("Error with sysfs path: %s", objects[i]);
			res = FPGA_EXCEPTION;
			goto out_err;
		}
		res = make_sysfs_object(objects[i], oname+1, &array->objects[i],
					flags & ~FPGA_OBJECT_GLOB, handle);
		if (res) {
			goto out_err;
		}
	}
	*object = (fpga_object)array;
	return res;
out_err:
	if (destroy_fpga_object(array)) {
		OPAE_ERR("Error destroying object");
	}
	return res;
}


#define MAX_SYSOBJECT_GLOB 128
fpga_result make_sysfs_object(char *sysfspath, const char *name,
			      fpga_object *object, int flags,
			      fpga_handle handle)
{
	uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
	struct _fpga_object *obj = NULL;
	struct stat objstat;
	int statres;
	fpga_result res = FPGA_OK;
	char *object_paths[MAX_SYSOBJECT_GLOB] = { NULL };
	size_t found = 0;
	if (flags & FPGA_OBJECT_GLOB) {
		res = opae_glob_paths(sysfspath, MAX_SYSOBJECT_GLOB,
				      object_paths, &found);
		if (res) {
			return res;
		}
		if (found == 1) {
			if (strncpy_s(sysfspath, SYSFS_PATH_MAX,
				      object_paths[0], PATH_MAX)) {
				OPAE_ERR("error copying glob result");
				res = FPGA_EXCEPTION;
				goto out_free;
			}
			res = make_sysfs_object(sysfspath, name, object,
						flags & ~FPGA_OBJECT_GLOB,
						handle);
		} else {
			res = make_sysfs_array(sysfspath, name, object, flags,
					       handle, object_paths, found);
		}
		// opae_glob_paths allocates memory for each path found
		// let's free it here since we don't need it any longer
		while (found) {
			free(object_paths[--found]);
		}
		return res;
	}
	statres = stat(sysfspath, &objstat);
	if (statres < 0) {
		OPAE_MSG("Error accessing %s: %s", sysfspath, strerror(errno));
		switch (errno) {
		case ENOENT:
			res = FPGA_NOT_FOUND;
			goto out_free;
		case ENOMEM:
			res = FPGA_NO_MEMORY;
			goto out_free;
		case EACCES:
			res = FPGA_NO_ACCESS;
			goto out_free;
		}
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (S_ISDIR(objstat.st_mode)) {
		return make_sysfs_group(sysfspath, name, object, flags, handle);
	}
	obj = alloc_fpga_object(sysfspath, name);
	if (!obj) {
		return FPGA_NO_MEMORY;
	}
	obj->handle = handle;
	obj->type = FPGA_SYSFS_FILE;
	obj->buffer = calloc(pg_size, sizeof(uint8_t));
	obj->max_size = pg_size;
	if (handle && (objstat.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))) {
		if ((objstat.st_mode & (S_IRUSR | S_IRGRP | S_IROTH))) {
			obj->perm = O_RDWR;
		} else {
			obj->perm = O_WRONLY;
		}
	} else {
		obj->perm = O_RDONLY;
	}
	*object = (fpga_object)obj;
	if (obj->perm == O_RDONLY || obj->perm == O_RDWR) {
		return sync_object((fpga_object)obj);
	}

	return FPGA_OK;
out_free:


	free(obj);
	return res;
}
