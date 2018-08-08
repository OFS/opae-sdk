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

#include <sys/types.h>
#include <sys/stat.h>

#include "common_int.h"
#include "log_int.h"
#include "sysfs_int.h"
#include "types_int.h"
#include "safe_string/safe_string.h"
#include <opae/types_enum.h>
#include <opae/sysobject.h>

#define NULL_CHECK(var)                                                        \
	do {                                                                   \
		if (var == NULL) {                                             \
			FPGA_MSG(#var " is NULL");                             \
		}                                                              \
	} while (false);

#define FREE_IF(var)                                                           \
	do {                                                                   \
		if (var) {                                                     \
			free(var);                                             \
			var = NULL;                                            \
		}                                                              \
	} while (false);



fpga_result __FPGA_API__ fpgaReadObject32(fpga_token token, const char *key,
					  uint32_t *value)
{
	NULL_CHECK(token);
	NULL_CHECK(key);
	NULL_CHECK(value);

	char objpath[SYSFS_PATH_MAX];

	fpga_result res = cat_token_sysfs_path(objpath, token, key);
	if (res) {
		return res;
	}

	return sysfs_read_u32(objpath, value);
}

fpga_result __FPGA_API__ fpgaReadObject64(fpga_token token, const char *key,
					  uint64_t *value)
{
	NULL_CHECK(token);
	NULL_CHECK(key);
	NULL_CHECK(value);

	char objpath[SYSFS_PATH_MAX];
	fpga_result res = cat_token_sysfs_path(objpath, token, key);
	if (res) {
		return res;
	}

	return sysfs_read_u64(objpath, value);
}


fpga_result __FPGA_API__ fpgaWriteObject64(fpga_handle handle, const char *key,
					   uint64_t value)
{
	NULL_CHECK(handle);
	NULL_CHECK(key);
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = cat_handle_sysfs_path(objpath, handle, key);
	if (res) {
		return res;
	}

	return sysfs_write_u64(objpath, value);
}

fpga_result __FPGA_API__ fpgaReadObjectBytes(fpga_token token, const char *key,
					     uint8_t *buffer, size_t offset,
					     size_t *len)
{
	int fd = -1, fd_stat = 0;
	char objpath[SYSFS_PATH_MAX];
	ssize_t count = 0;
	fpga_result res = FPGA_EXCEPTION;
	struct stat objstat;

	NULL_CHECK(token);
	NULL_CHECK(key);
	NULL_CHECK(len);

	res = cat_token_sysfs_path(objpath, token, key);
	if (res) {
		return res;
	}

	fd_stat = stat(objpath, &objstat);
	if (fd_stat < 0) {
		FPGA_ERR("Error with object path: %s", strerror(errno));
		if (errno == EACCES) {
			return FPGA_NO_ACCESS;
		}
		return FPGA_EXCEPTION;
	}

	if (!buffer) {
		*len = objstat.st_size - offset;
		return FPGA_OK;
	}


	fd = open(objpath, O_RDONLY);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	count = eintr_read(fd, buffer, *len);
	if (count < (ssize_t)*len) {
		if (count < 0) {
			FPGA_ERR("Error with pread operation: %s",
				 strerror(errno));
			res = FPGA_EXCEPTION;
		} else {
			FPGA_MSG("Bytes read (%d) is less that requested (%d)",
				 count, *len);
			res = count == 0 ? FPGA_EXCEPTION : FPGA_OK;
		}
	} else {
		res = FPGA_OK;
	}
	*len = count;
	close(fd);

	return res;
}


fpga_result fpgaGetTokenObject(fpga_token token, const char *name,
			       fpga_object *object, int flags)
{
	(void)flags;
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = FPGA_EXCEPTION;

	NULL_CHECK(token);
	NULL_CHECK(name);
	res = cat_token_sysfs_path(objpath, token, name);
	if (res) {
		return res;
	}

	return make_sysfs_object(objpath, name, object);
}


fpga_result fpgaDestroyObject(fpga_object *obj)
{
	if (NULL == obj || NULL == *obj) {
		FPGA_MSG("Invalid object pointer");
		return FPGA_INVALID_PARAM;
	}
	struct _fpga_object *_obj = (struct _fpga_object *)*obj;

	FREE_IF(_obj->path);
	FREE_IF(_obj->name);
	FREE_IF(_obj->buffer);
	while (_obj->size && _obj->objects) {
		if (fpgaDestroyObject(&_obj->objects[--_obj->size])) {
			FPGA_ERR("Error freeing subobject");
		}
	}
	FREE_IF(_obj->objects);
	if (_obj->fd > 0) {
		close(_obj->fd);
		_obj->fd = -1;
	}
	free(_obj);
	*obj = NULL;
	return FPGA_OK;
}

fpga_result fpgaObjectRead(fpga_object obj, uint8_t *buffer, size_t offset,
			   size_t len, int flags)
{
	NULL_CHECK(obj);
	NULL_CHECK(buffer);
	size_t bytes_read = 0;
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	if (offset + len > _obj->size) {
		return FPGA_INVALID_PARAM;
	}

	if (flags & FPGA_OBJECT_SYNC) {
		bytes_read = eintr_read(_obj->fd, _obj->buffer, _obj->size);
		if (bytes_read != _obj->size) {
			FPGA_ERR("Object size changed");
			return FPGA_EXCEPTION;
		}
	}

	memcpy_s(buffer, 4096, _obj->buffer + offset, len);

	return FPGA_OK;
}
