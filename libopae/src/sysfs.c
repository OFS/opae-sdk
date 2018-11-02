// Copyright(c) 2017, Intel Corporation
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

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <opae/types.h>

#include "safe_string/safe_string.h"

#include "types_int.h"
#include "sysfs_int.h"
#include "log_int.h"
#include "common_int.h"

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
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

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
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

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
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

	// read first value
	x1 = strtoul(buf, &c, 0);
	if (*c != sep) {
		FPGA_MSG("couldn't find separation character '%c' in '%s'", sep, path);
		goto out_close;
	}
	// read second value
	x2 = strtoul(c+1, &c, 0);
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
	int fd                     = -1;
	int res                    = 0;
	char buf[SYSFS_PATH_MAX]   = {0};
	int b                      = 0;

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
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

	*u = strtoull(buf, NULL, 0);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

fpga_result __FIXME_MAKE_VISIBLE__ sysfs_write_u64(const char *path, uint64_t u)
{
	int fd                     = -1;
	int res                    = 0;
	char buf[SYSFS_PATH_MAX]   = {0};
	int b                      = 0;

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

	snprintf_s_l(buf, sizeof(buf), "0x%lx", u);

	do {
		res = write(fd, buf + b, sizeof(buf) -b);
		if (res <= 0) {
			FPGA_ERR("Failed to write");
			goto out_close;
		}
		b += res;

		if ((unsigned)b > sizeof(buf) || b <= 0) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}

	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0' && (unsigned)b < sizeof(buf));

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
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

	for (i = 0 ; i < 32 ; i += 2) {
		tmp = buf[i+2];
		buf[i+2] = 0;

		octet = 0;
		sscanf_s_u(&buf[i], "%x", &octet);
		guid[i/2] = (uint8_t) octet;

		buf[i+2] = tmp;
	}

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}

//
// sysfs convenience functions to access device components by device number
//

// FIXME: uses same number for device and FME (may not be true in future)
fpga_result sysfs_get_socket_id(int dev, int subdev, uint8_t *socket_id)
{
	fpga_result result;
	char spath[SYSFS_PATH_MAX];
	int i;

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH
		 SYSFS_FME_PATH_FMT "/"
		 FPGA_SYSFS_SOCKET_ID,
		 dev, subdev);

	i = 0;
	result = sysfs_read_int(spath, &i);
	if (FPGA_OK != result)
		return result;

	*socket_id = (uint8_t) i;

	return FPGA_OK;
}

// FIXME: uses same number for device and PORT (may not be true in future)
fpga_result sysfs_get_afu_id(int dev, int subdev, fpga_guid guid)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH
		 SYSFS_AFU_PATH_FMT "/"
		 FPGA_SYSFS_AFU_GUID,
		 dev, subdev);

	return sysfs_read_guid(spath, guid);
}

fpga_result sysfs_get_pr_id(int dev, int subdev, fpga_guid guid)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH
		 SYSFS_FME_PATH_FMT "/"
		 FPGA_SYSFS_FME_INTERFACE_ID,
		 dev, subdev);

	return sysfs_read_guid(spath, guid);
}

// FIXME: uses same number for device and FME (may not be true in future)
fpga_result sysfs_get_slots(int dev, int subdev, uint32_t *slots)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH
		 SYSFS_FME_PATH_FMT "/"
		 FPGA_SYSFS_NUM_SLOTS,
		 dev, subdev);

	return sysfs_read_u32(spath, slots);
}

// FIXME: uses same number for device and FME (may not be true in future)
fpga_result sysfs_get_bitstream_id(int dev, int subdev, uint64_t *id)
{
	char spath[SYSFS_PATH_MAX];

	snprintf_s_ii(spath, SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH
		 SYSFS_FME_PATH_FMT "/"
		 FPGA_SYSFS_BITSTREAM_ID,
		 dev, subdev);

	return sysfs_read_u64(spath, id);
}

// Get port syfs path
fpga_result get_port_sysfs(fpga_handle handle,
				char *sysfs_port)
{

	struct _fpga_token  *_token;
	struct _fpga_handle *_handle  = (struct _fpga_handle *)handle;
	char *p                       = 0;

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

	snprintf_s_ii(sysfs_port, SYSFS_PATH_MAX,
		SYSFS_FPGA_CLASS_PATH SYSFS_AFU_PATH_FMT,
		_token->device_instance, _token->subdev_instance);

	return FPGA_OK;
}

// get fpga device id
fpga_result get_fpga_deviceid(fpga_handle handle,
				uint64_t *deviceid)
{
	struct _fpga_token  *_token      = NULL;
	struct _fpga_handle  *_handle    = (struct _fpga_handle *)handle;
	char sysfs_path[SYSFS_PATH_MAX]  = {0};
	char *p                          = NULL;
	fpga_result result               = FPGA_OK;
	int err                          = 0;

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

	snprintf_s_is(sysfs_path,
		 SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH SYSFS_FPGA_FMT "/%s",
		 _token->device_instance,
		 FPGA_SYSFS_DEVICEID);

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
fpga_result sysfs_deviceid_from_path(const char *sysfspath,
				uint64_t *deviceid)
{
	char sysfs_path[SYSFS_PATH_MAX]  = {0};
	char *p                          = NULL;
	int device_instance              = 0;
	fpga_result result               = FPGA_OK;

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

	snprintf_s_is(sysfs_path,
		 SYSFS_PATH_MAX,
		 SYSFS_FPGA_CLASS_PATH SYSFS_FPGA_FMT "/%s",
		 device_instance,
		 FPGA_SYSFS_DEVICEID);

	result = sysfs_read_u64(sysfs_path, deviceid);
	if (result != 0)
		FPGA_ERR("Failed to read device ID");

	return result;
}

/*
 * The rlpath path is assumed to be of the form:
 * ../../devices/pci0000:5e/0000:5e:00.0/fpga/intel-fpga-dev.0
 */
fpga_result sysfs_sbdf_from_path(const char *sysfspath, int *s, int *b, int *d, int *f)
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
	*f = (int) strtoul(p+11, NULL, 16);
	*(p + 10) = 0;

	*d = (int) strtoul(p+8, NULL, 16);
	*(p + 7) = 0;

	*b = (int) strtoul(p+5, NULL, 16);
	*(p + 4) = 0;

	*s = (int) strtoul(p, NULL, 16);

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
