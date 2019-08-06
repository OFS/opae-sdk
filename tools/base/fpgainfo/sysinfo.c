// Copyright(c) 2018, Intel Corporation
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

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>

#include "safe_string/safe_string.h"
#include "opae/fpga.h"
#include "fpgainfo.h"
#include "sysinfo.h"

static fpga_result read_string(char *buf, unsigned int buf_size, const char *path)
{
	int b = 0;
	int res = 0;
	int fd;

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
		res = read(fd, buf + b, buf_size - b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > buf_size) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0'
		 && (unsigned)b < buf_size);

	// erase \n
	buf[b - 1] = 0;

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_OK;
}

fpga_result fpgainfo_sysfs_read_guid(const char *path, fpga_guid guid)
{
	char buf[SYSFS_PATH_MAX];
	fpga_result fres = FPGA_OK;

	int i;
	char tmp;
	unsigned octet;

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fres = read_string(buf, sizeof(buf), path);
	if (FPGA_OK != fres) {
		return fres;
	}

	for (i = 0; i < 32; i += 2) {
		tmp = buf[i + 2];
		buf[i + 2] = 0;

		octet = 0;
		sscanf_s_u(&buf[i], "%x", &octet);
		guid[i / 2] = (uint8_t)octet;

		buf[i + 2] = tmp;
	}

	return FPGA_OK;
}

fpga_result fpgainfo_sysfs_read_int(const char *path, int *i)
{
	fpga_result fres = FPGA_OK;
	char buf[SYSFS_PATH_MAX];

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fres = read_string(buf, sizeof(buf), path);
	if (FPGA_OK != fres) {
		return fres;
	}

	*i = atoi(buf);

	return FPGA_OK;
}

fpga_result fpgainfo_sysfs_read_u32(const char *path, uint32_t *u)
{
	fpga_result fres = FPGA_OK;
	char buf[SYSFS_PATH_MAX];

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fres = read_string(buf, sizeof(buf), path);
	if (FPGA_OK != fres) {
		return fres;
	}

	*u = strtoul(buf, NULL, 0);

	return FPGA_OK;
}

fpga_result fpgainfo_sysfs_read_u64(const char *path, uint64_t *u)
{
	fpga_result fres = FPGA_OK;
	char buf[SYSFS_PATH_MAX] = {0};

	if (path == NULL) {
		FPGA_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fres = read_string(buf, sizeof(buf), path);
	if (FPGA_OK != fres) {
		return fres;
	}

	*u = strtoull(buf, NULL, 0);

	return FPGA_OK;
}

static struct dev_list *fpgainfo_add_dev(const char *sysfspath,
					 const char *devpath,
					 struct dev_list *parent)
{
	struct dev_list *pdev;
	errno_t e;

	pdev = (struct dev_list *)malloc(sizeof(*pdev));
	if (NULL == pdev)
		return NULL;

	e = strncpy_s(pdev->sysfspath, sizeof(pdev->sysfspath), sysfspath,
		      SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(pdev->devpath, sizeof(pdev->devpath), devpath,
		      DEV_PATH_MAX);
	if (EOK != e)
		goto out_free;

	pdev->next = parent->next;
	parent->next = pdev;

	pdev->parent = parent;

	// printf("pdev paths: sysfs:'%s' dev:'%s'\n", pdev->sysfspath,
	//       pdev->devpath);

	return pdev;

out_free:
	free(pdev);
	return NULL;
}

static fpga_result fpgainfo_enum_fme(const char *sysfspath, const char *name,
				     struct dev_list *parent)
{
	fpga_result result;
	struct stat stats;
	struct dev_list *pdev;
	char spath[SYSFS_PATH_MAX];
	char dpath[DEV_PATH_MAX];

	// Make sure it's a directory.
	if (stat(sysfspath, &stats) != 0) {
		FPGA_MSG("stat failed: %s", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (!S_ISDIR(stats.st_mode))
		return FPGA_OK;
	int socket_id = 0;

	snprintf_s_s(dpath, sizeof(dpath), FPGA_DEV_PATH "/%s", name);

	// printf("fpgainfo_add_dev for %s\n", dpath);

	pdev = fpgainfo_add_dev(sysfspath, dpath, parent);
	if (!pdev) {
		FPGA_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->objtype = FPGA_DEVICE;

	pdev->segment = parent->segment;
	pdev->bus = parent->bus;
	pdev->device = parent->device;
	pdev->function = parent->function;
	pdev->vendor_id = parent->vendor_id;
	pdev->device_id = parent->device_id;

	// Discover the FME GUID from sysfs (pr/interface_id)
	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_FME_INTERFACE_ID,
		     sysfspath);

	result = fpgainfo_sysfs_read_guid(spath, pdev->guid);
	if (FPGA_OK != result)
		return result;

	// Discover the socket id from the FME's sysfs entry.
	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_SOCKET_ID,
		     sysfspath);

	result = fpgainfo_sysfs_read_int(spath, &socket_id);
	if (FPGA_OK != result)
		return result;

	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_NUM_SLOTS,
		     sysfspath);
	result = fpgainfo_sysfs_read_u32(spath, &pdev->fpga_num_slots);
	if (FPGA_OK != result)
		return result;

	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_BITSTREAM_ID,
		     sysfspath);
	result = fpgainfo_sysfs_read_u64(spath, &pdev->fpga_bitstream_id);
	if (FPGA_OK != result)
		return result;

	pdev->fpga_bbs_version.major =
		FPGA_BBS_VER_MAJOR(pdev->fpga_bitstream_id);
	pdev->fpga_bbs_version.minor =
		FPGA_BBS_VER_MINOR(pdev->fpga_bitstream_id);
	pdev->fpga_bbs_version.patch =
		FPGA_BBS_VER_PATCH(pdev->fpga_bitstream_id);

	parent->socket_id = socket_id;
	parent->fme = pdev;
	return FPGA_OK;
}

static fpga_result fpgainfo_enum_top_dev(const char *sysfspath,
					 struct dev_list *list)
{
	fpga_result result = FPGA_NOT_FOUND;
	struct stat stats;

	struct dev_list *pdev;

	DIR *dir;
	struct dirent *dirent;
	char spath[SYSFS_PATH_MAX];
	int res;
	char *p;
	int f;
	unsigned s, b, d;

	// Make sure it's a directory.
	if (stat(sysfspath, &stats) != 0) {
		FPGA_MSG("stat failed: %s", strerror(errno));
		return FPGA_NO_DRIVER;
	}

	if (!S_ISDIR(stats.st_mode))
		return FPGA_OK;

	res = readlink(sysfspath, spath, sizeof(spath));
	if (-1 == res) {
		FPGA_MSG("Can't read link");
		return FPGA_NO_DRIVER;
	}

	pdev = fpgainfo_add_dev(sysfspath, "", list);
	if (!pdev) {
		FPGA_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	// Find the BDF from the link path.
	spath[res] = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	++p;

	//           11
	// 012345678901
	// ssss:bb:dd.f
	f = 0;
	sscanf_s_i(p + 11, "%d", &f);

	pdev->function = (uint8_t)f;
	*(p + 10) = 0;

	d = 0;
	sscanf_s_u(p + 8, "%x", &d);

	pdev->device = (uint8_t)d;
	*(p + 7) = 0;

	b = 0;
	sscanf_s_u(p + 5, "%x", &b);

	pdev->bus = (uint8_t)b;
	*(p + 4) = 0;

	s = 0;
	sscanf_s_u(p, "%x", &s);
	pdev->segment = (uint16_t)s;

	// read the vendor and device ID from the 'device' path
	uint32_t x = 0;
	char vendorpath[SYSFS_PATH_MAX];
	snprintf_s_s(vendorpath, SYSFS_PATH_MAX, "%s/device/vendor", sysfspath);
	result = fpgainfo_sysfs_read_u32(vendorpath, &x);
	if (result != FPGA_OK)
		return result;
	pdev->vendor_id = (uint16_t)x;

	char devicepath[SYSFS_PATH_MAX];
	snprintf_s_s(devicepath, SYSFS_PATH_MAX, "%s/device/device", sysfspath);
	result = fpgainfo_sysfs_read_u32(devicepath, &x);
	if (result != FPGA_OK)
		return result;
	pdev->device_id = (uint16_t)x;

	// Find the FME devices.
	dir = opendir(sysfspath);
	if (NULL == dir) {
		FPGA_MSG("Can't open directory: %s", sysfspath);
		return FPGA_NO_DRIVER;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		snprintf_s_ss(spath, sizeof(spath), "%s/%s", sysfspath,
			      dirent->d_name);

		// printf("Considering %s\n", spath);

		if (strstr(dirent->d_name, FPGA_SYSFS_FME)) {
			// printf("Enumerating FME\n");
			result = fpgainfo_enum_fme(spath, dirent->d_name, pdev);
			if (result != FPGA_OK)
				break;
		}
	}

	closedir(dir);

	return result;
}

fpga_result fpgainfo_enumerate_devices(struct dev_list *head)
{
	fpga_result result = FPGA_NOT_FOUND;

	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char sysfspath[SYSFS_PATH_MAX];
	struct dev_list *lptr;

	if (NULL == head) {
		FPGA_MSG("head is NULL");
		return FPGA_INVALID_PARAM;
	}

	memset_s(head, sizeof(head), 0);

	// Find the top-level FPGA devices.
	dir = opendir(SYSFS_FPGA_CLASS_PATH);
	if (NULL == dir) {
		FPGA_MSG("can't find %s (no driver?)", SYSFS_FPGA_CLASS_PATH);
		return FPGA_NO_DRIVER;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s",
			      SYSFS_FPGA_CLASS_PATH, dirent->d_name);

		result = fpgainfo_enum_top_dev(sysfspath, head);
		if (result != FPGA_OK)
			break;
	}

	closedir(dir);

	if (result != FPGA_OK) {
		FPGA_MSG("No FPGA resources found");
		return result;
	}

	/* create and populate token data structures */
	for (lptr = head->next; NULL != lptr; lptr = lptr->next) {
		if (!strnlen_s(lptr->devpath, sizeof(lptr->devpath)))
			continue;

		// propagate the socket_id field.
		lptr->socket_id = lptr->parent->socket_id;
		lptr->fme = lptr->parent->fme;
	}

	return result;
}

static struct dev_list *head;

const char *get_sysfs_path(fpga_properties props, fpga_objtype type,
			   struct dev_list **item)
{
	fpga_result res = FPGA_OK;
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	struct dev_list *lptr = head;

	if (NULL == head) {
		head = (struct dev_list *)calloc(1, sizeof(struct dev_list));
		res = fpgainfo_enumerate_devices(head);
		fpgainfo_print_err("enumerating devices for sysfspath", res);
		if (FPGA_OK != res) {
			return "** Failed to enumerate **";
		}
	}

	res = fpgaPropertiesGetSegment(props, &segment);
	fpgainfo_print_err("reading segment from properties", res);

	res = fpgaPropertiesGetBus(props, &bus);
	fpgainfo_print_err("reading bus from properties", res);

	res = fpgaPropertiesGetDevice(props, &device);
	fpgainfo_print_err("reading device from properties", res);

	res = fpgaPropertiesGetFunction(props, &function);
	fpgainfo_print_err("reading function from properties", res);

	for (lptr = head->next; NULL != lptr; lptr = lptr->next) {
		if ((lptr->bus == bus) && (lptr->device == device)
		    && (lptr->function == function)
		    && (lptr->segment == segment) && (lptr->objtype == type)) {
			break;
		}
	}

	if (item) {
		*item = lptr;
	}

	return lptr ? (const char *)lptr->fme->sysfspath : NULL;
}

fpga_result glob_sysfs_path(char *path)
{
	glob_t pglob;
	pglob.gl_pathc = 0;
	pglob.gl_pathv = NULL;
	fpga_result ret = FPGA_OK;

	if (glob(path, 0, NULL, &pglob) == 0) {
		if (pglob.gl_pathc > 0) {
			/* using first one */
			if (strcpy_s(path, PATH_MAX, pglob.gl_pathv[0]))
				ret = FPGA_EXCEPTION;
		}
		globfree(&pglob);
	} else {
		if (pglob.gl_pathv)
			globfree(&pglob);
		ret = FPGA_NOT_FOUND;
	}
	return ret;
}
