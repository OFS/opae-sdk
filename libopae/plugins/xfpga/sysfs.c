// Copyright(c) 2017-2018, Intel Corporation
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

static struct {
	const char *sysfs_class_path;
	const char *sysfs_region_fmt;
	const char *sysfs_resource_fmt;
	const char *sysfs_compat_id;
} sysfs_path_table[OPAE_KERNEL_DRIVERS] = {
	// upstream driver sysfs formats
	{"/sys/class/fpga_region", "region([0-9])+",
	 "dfl-(fme|port)\\.([0-9]+)", "/dfl-fme-region.*/fpga_region/region*/compat_id"},
	// intel driver sysfs formats
	{"/sys/class/fpga", "intel-fpga-dev\\.([0-9]+)",
	 "intel-fpga-(fme|port)\\.([0-9]+)", "pr/interface_id"} };

static uint32_t _sysfs_format_index;
static uint32_t _sysfs_region_count;
/* mutex to protect sysfs region data structures */
pthread_mutex_t _sysfs_region_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define SYSFS_FORMAT(s) sysfs_path_table[_sysfs_format_index].s


#define SYSFS_MAX_REGIONS 128
static sysfs_fpga_region _regions[SYSFS_MAX_REGIONS];

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])/fpga"
#define PCIE_PATH_PATTERN_GROUPS 5

#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			FPGA_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0);

STATIC int parse_pcie_info(sysfs_fpga_region *region, char *buffer)
{
	char err[128] = {0};
	regex_t re;
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };
	int res = FPGA_EXCEPTION;

	int reg_res = regcomp(&re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);
	if (reg_res) {
		FPGA_ERR("Error compling regex");
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, buffer, PCIE_PATH_PATTERN_GROUPS, matches, 0);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		FPGA_ERR("Error executing regex: %s", err);
		res = FPGA_EXCEPTION;
		goto out;
	} else {
		PARSE_MATCH_INT(buffer, matches[1], region->segment, 16, out);
		PARSE_MATCH_INT(buffer, matches[2], region->bus, 16, out);
		PARSE_MATCH_INT(buffer, matches[3], region->device, 16, out);
		PARSE_MATCH_INT(buffer, matches[4], region->function, 10, out);
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
		FPGA_ERR("error concatenating strings (%s, %s)",
			 root, attr_path);
		return FPGA_EXCEPTION;
	}
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_ERR("Error opening %s: %s", path, strerror(errno));
		return FPGA_EXCEPTION;
	}
	bytes_read = eintr_read(fd, buffer, pg_size);
	if (bytes_read < 0) {
		FPGA_ERR("Error reading from %s: %s", path,
			 strerror(errno));
		close(fd);
		return FPGA_EXCEPTION;
	}

	*value = strtoull(buffer, NULL, 0);

	close(fd);
	return FPGA_OK;
}

STATIC int parse_device_vendor_id(sysfs_fpga_region *region)
{
	uint64_t value = 0;
	int res = sysfs_parse_attribute64(region->region_path, "device/device", &value);
	if (res) {
		FPGA_ERR("Error parsing device_id for region: %s",
			 region->region_path);
		return res;
	}
	region->device_id = value;

	res = sysfs_parse_attribute64(region->region_path, "device/vendor", &value);

	if (res) {
		FPGA_ERR("Error parsing vendor_id for region: %s",
			 region->region_path);
		return res;
	}
	region->vendor_id = value;

	return FPGA_OK;
}

STATIC int make_region(sysfs_fpga_region *region, const char *sysfs_class_fpga,
		       char *dir_name, int num)
{
	int res = FPGA_OK;
	char buffer[SYSFS_PATH_MAX] = {0};
	ssize_t sym_link_len = 0;
	if (snprintf_s_ss(region->region_path, SYSFS_PATH_MAX, "%s/%s",
			  sysfs_class_fpga, dir_name)
	    < 0) {
		FPGA_ERR("Error formatting sysfs paths");
		return FPGA_EXCEPTION;
	}

	if (snprintf_s_s(region->region_name, SYSFS_PATH_MAX, "%s", dir_name) < 0) {
		FPGA_ERR("Error formatting sysfs name");
		return FPGA_EXCEPTION;
	}

	sym_link_len = readlink(region->region_path, buffer, SYSFS_PATH_MAX);
	if (sym_link_len < 0) {
		FPGA_ERR("Error reading sysfs link: %s", region->region_path);
		return FPGA_EXCEPTION;
	}

	region->number = num;
	res = parse_pcie_info(region, buffer);

	if (res) {
		FPGA_ERR("Could not parse symlink");
		return res;
	}

	return parse_device_vendor_id(region);
}

STATIC sysfs_fpga_resource *make_resource(sysfs_fpga_region *region, char *name,
					  int num, fpga_objtype type)
{
	sysfs_fpga_resource *resource = malloc(sizeof(sysfs_fpga_resource));
	if (resource == NULL) {
		FPGA_ERR("error creating resource");
		return NULL;
	}
	resource->region = region;
	resource->type = type;
	resource->num = num;
	// copy the full path to the parent region object
	strcpy_s(resource->res_path, SYSFS_PATH_MAX, region->region_path);
	// add a trailing path seperator '/'
	int len = strlen(resource->res_path);
	char *ptr = resource->res_path + len;
	*ptr = '/';
	ptr++;
	*ptr = '\0';
	// append the name to get the full path to the resource
	if (cat_sysfs_path(resource->res_path, name)) {
		FPGA_ERR("error concatenating path");
		free(resource);
		return NULL;
	}

	if (snprintf_s_s(resource->res_name, SYSFS_PATH_MAX, "%s", name) < 0) {
		FPGA_ERR("Error formatting sysfs name");
		free(resource);
		return NULL;
	}

	return resource;
}


STATIC void find_resources(sysfs_fpga_region *region)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	regex_t re;
	int reg_res = -1;
	int num = -1;
	char err[128] = {0};
	regmatch_t matches[SYSFS_MAX_RESOURCES];
	reg_res = regcomp(&re, SYSFS_FORMAT(sysfs_resource_fmt), REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		FPGA_MSG("Error compiling regex: %s", err);
	}

	dir = opendir(region->region_path);
	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		reg_res = regexec(&re, dirent->d_name, SYSFS_MAX_RESOURCES,
				  matches, 0);
		if (!reg_res) {
			int type_beg = matches[1].rm_so;
			// int type_end = matches[1].rm_eo;
			int num_beg = matches[2].rm_so;
			// int num_end = matches[2].rm_eo;
			if (type_beg < 1 || num_beg < 1) {
				FPGA_MSG("Invalid sysfs resource format");
				continue;
			}
			num = strtoul(dirent->d_name + num_beg, NULL, 10);
			if (!strncmp(FPGA_SYSFS_FME, dirent->d_name + type_beg,
				     FPGA_SYSFS_FME_LEN)) {
				region->fme = make_resource(
					region, dirent->d_name, num, FPGA_DEVICE);
			} else if (!strncmp(FPGA_SYSFS_PORT,
					    dirent->d_name + type_beg,
					    FPGA_SYSFS_PORT_LEN)) {
				region->port =
					make_resource(region, dirent->d_name,
						      num, FPGA_ACCELERATOR);
			}
		}
	}
	regfree(&re);
	closedir(dir);
}

STATIC int sysfs_region_destroy(sysfs_fpga_region *region)
{
	ASSERT_NOT_NULL(region);
	if (region->fme) {
		free(region->fme);
		region->fme = NULL;
	}
	if (region->port) {
		free(region->port);
		region->port = NULL;
	}
	return FPGA_OK;
}

int sysfs_region_count(void)
{
	int res = 0, count = 0;
	if (!opae_mutex_lock(res, &_sysfs_region_lock)) {
		count = _sysfs_region_count;
	}

	if (opae_mutex_unlock(res, &_sysfs_region_lock)) {
		count = 0;
	}

	return count;
}

void sysfs_foreach_region(region_cb cb, void *context)
{
	uint32_t i = 0;
	int res = 0;
	if (!opae_mutex_lock(res, &_sysfs_region_lock)) {
		for ( ; i < _sysfs_region_count; ++i) {
			cb(&_regions[i], context);
		}

		opae_mutex_unlock(res, &_sysfs_region_lock);
	}
}

int sysfs_initialize(void)
{
	int stat_res = -1;
	int reg_res = -1;
	int res = FPGA_OK;
	uint32_t i = 0;
	struct stat st;
	DIR *dir = NULL;
	char err[128] = {0};
	struct dirent *dirent = NULL;
	regex_t region_re;
	regmatch_t matches[SYSFS_MAX_REGIONS];

	for (i = 0; i < OPAE_KERNEL_DRIVERS; ++i) {
		errno = 0;
		stat_res = stat(sysfs_path_table[i].sysfs_class_path, &st);
		if (!stat_res) {
			_sysfs_format_index = i;
			break;
		}
		if (errno != ENOENT) {
			FPGA_ERR("Error while inspecting sysfs: %s",
				 strerror(errno));
			return FPGA_EXCEPTION;
		}
	}
	if (i == OPAE_KERNEL_DRIVERS) {
		FPGA_ERR(
			"No valid sysfs class files found - a suitable driver may not be loaded");
		return FPGA_NO_DRIVER;
	}

	_sysfs_region_count = 0;
	reg_res = regcomp(&region_re, SYSFS_FORMAT(sysfs_region_fmt),
			  REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &region_re, err, 128);
		FPGA_ERR("Error compling regex: %s", err);
		return FPGA_EXCEPTION;
	};

	const char *sysfs_class_fpga = SYSFS_FORMAT(sysfs_class_path);
	// open the root sysfs class directory
	// look in the directory and get region (device) objects
	dir = opendir(sysfs_class_fpga);
	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		// if the current directory matches the region (device) regex
		reg_res = regexec(&region_re, dirent->d_name, SYSFS_MAX_REGIONS,
				  matches, 0);
		if (!reg_res) {
			int num_begin = matches[1].rm_so;
			if (num_begin < 0) {
				FPGA_ERR("sysfs format invalid: %s", dirent->d_name);
				continue;
			}
			int num = strtoul(dirent->d_name + num_begin, NULL, 10);
			// increment our region count after filling out details
			// of the discovered region in our _regions array
			if (opae_mutex_lock(res, &_sysfs_region_lock)) {
				goto out_free;
			}
			if (make_region(&_regions[_sysfs_region_count++],
					sysfs_class_fpga, dirent->d_name,
					num)) {
				FPGA_MSG("Error processing region: %s",
					 dirent->d_name);
				_sysfs_region_count--;
			}
			if (opae_mutex_unlock(res, &_sysfs_region_lock)) {
				goto out_free;
			}
		}
	}

	if (opae_mutex_lock(res, &_sysfs_region_lock)) {
		goto out_free;
	} else if (!_sysfs_region_count) {
		FPGA_ERR("Error discovering fpga regions");
		res = FPGA_NO_DRIVER;
		goto out_unlock;
	}

	// now, for each discovered region, look inside for resources
	// (fme|port)
	for (i = 0; i < _sysfs_region_count; ++i) {
		find_resources(&_regions[i]);
	}

out_unlock:
	if (pthread_mutex_unlock(&_sysfs_region_lock)) {
		FPGA_MSG("error unlocking sysfs region mutex");
		res = FPGA_EXCEPTION;
	}
out_free:
	regfree(&region_re);
	closedir(dir);
	return res;
}

int sysfs_finalize(void)
{
	uint32_t i = 0;
	for (; i < _sysfs_region_count; ++i) {
		sysfs_region_destroy(&_regions[i]);
	}
	_sysfs_region_count = 0;
	return FPGA_OK;
}

const sysfs_fpga_region *sysfs_get_region(size_t num)
{
	const sysfs_fpga_region *ptr = NULL;
	int res = 0;
	if (!opae_mutex_lock(res, &_sysfs_region_lock)) {
		if (num >= _sysfs_region_count) {
			FPGA_ERR("No such region with index: %d", num);
		} else {
			ptr = &_regions[num];
		}
		if (opae_mutex_unlock(res, &_sysfs_region_lock)) {
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


fpga_result sysfs_get_fme_pr_interface_id(const char *sysfs_res_path, fpga_guid guid)
{
	fpga_result res = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX];

	int len = snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/%s",
		sysfs_res_path, SYSFS_FORMAT(sysfs_compat_id));
	if (len < 0) {
		FPGA_ERR("error concatenating strings (%s, %s)",
			sysfs_res_path, sysfs_path);
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
		FPGA_ERR("error concatenating strings (%s, %s)",
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

fpga_result sysfs_get_fme_path(int dev, int subdev, char *path)
{
	fpga_result result = FPGA_OK;
	char spath[SYSFS_PATH_MAX];
	char sysfs_path[SYSFS_PATH_MAX];
	errno_t e;

	int len = snprintf_s_ss(sysfs_path, SYSFS_PATH_MAX, "%s/%s",
		SYSFS_FORMAT(sysfs_class_path), SYSFS_FME_PATH);
	if (len < 0) {
		FPGA_ERR("Error formatting sysfs path");
		return FPGA_EXCEPTION;
	}

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		sysfs_path, dev, subdev);

	result = opae_glob_path(spath);
	if (result) {
		return result;
	}

	e = strncpy_s(path, SYSFS_PATH_MAX,
		spath, SYSFS_PATH_MAX);
	if (EOK != e) {
		return FPGA_EXCEPTION;
	}

	return result;
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
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	b = 0;

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
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
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	b = 0;

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
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
		FPGA_MSG("invalid separation character");
		return FPGA_INVALID_PARAM;
	}

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	b = 0;

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0'
		 && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b - 1] = 0;

	// read first value
	x1 = strtoul(buf, &c, 0);
	if (*c != sep) {
		FPGA_MSG("couldn't find separation character '%c' in '%s'", sep,
			 path);
		goto out_close;
	}
	// read second value
	x2 = strtoul(c + 1, &c, 0);
	if (*c != '\0') {
		FPGA_MSG("unexpected character '%c' in '%s'", *c, path);
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

fpga_result __FIXME_MAKE_VISIBLE__ sysfs_read_u64(const char *path, uint64_t *u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
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

fpga_result __FIXME_MAKE_VISIBLE__ sysfs_write_u64(const char *path, uint64_t u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;
	int len;

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed: %s", path, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek: %s", strerror(errno));
		goto out_close;
	}

	len = snprintf_s_l(buf, sizeof(buf), "0x%lx\n", u);

	do {
		res = write(fd, buf + b, len - b);
		if (res <= 0) {
			FPGA_ERR("Failed to write");
			goto out_close;
		}
		b += res;

		if (b > len || b <= 0) {
			FPGA_MSG("Unexpected size writing to %s", path);
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


fpga_result __FIXME_MAKE_VISIBLE__ sysfs_write_u64_decimal(const char *path, uint64_t u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = {0};
	int b = 0;
	int len;

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed: %s", path, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek: %s", strerror(errno));
		goto out_close;
	}

	len = snprintf_s_l(buf, sizeof(buf), "%ld\n", u);

	do {
		res = write(fd, buf + b, len - b);
		if (res <= 0) {
			FPGA_ERR("Failed to write");
			goto out_close;
		}
		b += res;

		if (b > len || b <= 0) {
			FPGA_MSG("Unexpected size writing to %s", path);
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
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	b = 0;

	do {
		res = read(fd, buf + b, sizeof(buf) - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
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

fpga_result sysfs_path_isvalid(const char *root, const char *attr_path)
{
	char path[SYSFS_PATH_MAX]    = {0};
	fpga_result result          = FPGA_OK;
	struct stat stats;

	int len = snprintf_s_ss(path, SYSFS_PATH_MAX, "%s/%s",
		root, attr_path);
	if (len < 0) {
		FPGA_ERR("error concatenating strings (%s, %s)",
			root, attr_path);
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(path);
	if (result) {
		return result;
	}

	if (stat(path, &stats) != 0) {
		FPGA_ERR("stat failed: %s", strerror(errno));
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

// Get port syfs path
fpga_result get_port_sysfs(fpga_handle handle, char *sysfs_port)
{

	struct _fpga_token *_token;
	struct _fpga_handle *_handle      = (struct _fpga_handle *)handle;
	char *p                           = 0;
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;
	errno_t e;

	if (sysfs_port == NULL) {
		FPGA_ERR("Invalid output pointer");
		return FPGA_INVALID_PARAM;
	}

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		return FPGA_INVALID_PARAM;
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL == p) {
		FPGA_ERR("Invalid sysfspath in token");
		return FPGA_INVALID_PARAM;
	}

	int len  = snprintf_s_s(sysfs_path, SYSFS_PATH_MAX, "%s/../*-port.*",
		_token->sysfspath);
	if (len < 0) {
		FPGA_ERR("Error formatting sysfs path");
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(sysfs_path);
	if (result) {
		return result;
	}

	e = strncpy_s(sysfs_port, SYSFS_PATH_MAX,
		sysfs_path, SYSFS_PATH_MAX);
	if (EOK != e) {
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

// get fpga device id
fpga_result get_fpga_deviceid(fpga_handle handle, uint64_t *deviceid)
{
	struct _fpga_token *_token = NULL;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	char sysfs_path[SYSFS_PATH_MAX] = {0};
	char *p = NULL;
	fpga_result result = FPGA_OK;
	int err = 0;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	if (deviceid == NULL) {
		FPGA_ERR("Invalid input Parameters");
		return FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_lock(&_handle->lock)) {
		FPGA_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Token not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (p == NULL) {
		FPGA_ERR("Failed to read sysfs path");
		result = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	snprintf_s_s(sysfs_path, SYSFS_PATH_MAX, "%s/../device/device",
		_token->sysfspath);

	result = sysfs_read_u64(sysfs_path, deviceid);
	if (result != 0) {
		FPGA_ERR("Failed to read device ID");
		goto out_unlock;
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

// get fpga device id using the sysfs path
fpga_result sysfs_deviceid_from_path(const char *sysfspath, uint64_t *deviceid)
{
	char sysfs_path[SYSFS_PATH_MAX] = {0};
	char *p = NULL;
	int device_instance = 0;
	fpga_result result = FPGA_OK;

	if (deviceid == NULL) {
		FPGA_ERR("Invalid input Parameters");
		return FPGA_INVALID_PARAM;
	}

	p = strstr(sysfspath, FPGA_SYSFS_FME);
	if (p == NULL) {
		FPGA_ERR("Failed to read sysfs path");
		return FPGA_NOT_SUPPORTED;
	}

	p = strchr(sysfspath, '.');
	if (p == NULL) {
		FPGA_ERR("Failed to read sysfs path");
		return FPGA_NOT_SUPPORTED;
	}

	device_instance = atoi(p + 1);

	snprintf_s_is(sysfs_path, SYSFS_PATH_MAX,
		      SYSFS_FPGA_CLASS_PATH SYSFS_FPGA_FMT "/%s",
		      device_instance, FPGA_SYSFS_DEVICEID);

	result = sysfs_read_u64(sysfs_path, deviceid);
	if (result != 0)
		FPGA_ERR("Failed to read device ID");

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
		FPGA_MSG("Can't read link %s (no driver?)", sysfspath);
		return FPGA_NO_DRIVER;
	}

	// Find the BDF from the link path.
	rlpath[res] = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		FPGA_MSG("Invalid link %s (no driver?)", rlpath);
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		FPGA_MSG("Invalid link %s (no driver?)", rlpath);
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(rlpath, '/');
	if (!p) {
		FPGA_MSG("Invalid link %s (no driver?)", rlpath);
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
		FPGA_ERR("destination str is NULL");
		return FPGA_EXCEPTION;
	}
	struct _fpga_token *_token = (struct _fpga_token *)token;
	int len = snprintf_s_ss(dest, SYSFS_PATH_MAX, "%s/%s",
				_token->sysfspath, path);
	if (len < 0) {
		FPGA_ERR("error concatenating strings (%s, %s)",
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
		FPGA_ERR("NULL pointer in name");
		return FPGA_INVALID_PARAM;
		break;
	case ESZEROL:
		FPGA_ERR("Zero length");
		break;
	case ESLEMAX:
		FPGA_ERR("Length exceeds max");
		break;
	case ESUNTERM:
		FPGA_ERR("Destination not termindated");
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
	if (strncpy_s(p, s+1, str, s)) {
		FPGA_ERR("Error copying string");
		return NULL;
	}
	p[s] = '\0';
	return p;
}

struct _fpga_object *alloc_fpga_object(const char *sysfspath, const char *name)
{
	struct _fpga_object *obj = calloc(1, sizeof(struct _fpga_object));
	if (obj) {
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
			FPGA_MSG("Ambiguous object key - using first one");
		}
		if (strcpy_s(path, FILENAME_MAX, pglob.gl_pathv[0])) {
			FPGA_ERR("Could not copy globbed path");
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

fpga_result sync_object(fpga_object obj)
{
	struct _fpga_object *_obj;
	int fd = -1;
	ssize_t bytes_read = 0;
	ASSERT_NOT_NULL(obj);
	_obj = (struct _fpga_object *)obj;
	fd = open(_obj->path, _obj->perm);
	if (fd < 0) {
		FPGA_ERR("Error opening %s: %s", _obj->path, strerror(errno));
		return FPGA_EXCEPTION;
	}
	bytes_read = eintr_read(fd, _obj->buffer, _obj->max_size);
	if (bytes_read < 0) {
		FPGA_ERR("Error reading from %s: %s", _obj->path,
			 strerror(errno));
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
		FPGA_ERR("Error calling scandir: %s", strerror(errno));
		switch (errno) {
		case ENOMEM:
			return FPGA_NO_MEMORY;
		case ENOENT:
			return FPGA_NOT_FOUND;
		}
		return FPGA_EXCEPTION;
	}

	if (n == 0) {
		FPGA_ERR("Group is empty");
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
	if (group->path)
		free(group->path);
	if (group->name)
		free(group->name);
	free(group);

out_free_namelist:
	while (n--)
		free(namelist[n]);
	free(namelist);

	return res;
}

fpga_result make_sysfs_object(char *sysfspath, const char *name,
			      fpga_object *object, int flags,
			      fpga_handle handle)
{
	uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
	struct _fpga_object *obj = NULL;
	struct stat objstat;
	int statres;
	fpga_result res = FPGA_OK;
	if (flags & FPGA_OBJECT_GLOB) {
		res = opae_glob_path(sysfspath);
	}
	statres = stat(sysfspath, &objstat);
	if (statres < 0) {
		FPGA_MSG("Error accessing %s: %s", sysfspath, strerror(errno));
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
