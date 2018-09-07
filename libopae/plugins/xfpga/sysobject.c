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

#define FREE_IF(var)                                                           \
	do {                                                                   \
		if (var) {                                                     \
			free(var);                                             \
			var = NULL;                                            \
		}                                                              \
	} while (false);


fpga_result __FPGA_API__ fpgaReadObjectBytes(fpga_token token, const char *key,
					     uint8_t *buffer, size_t offset,
					     size_t *len)
{
	int fd = -1, fd_stat = 0;
	char objpath[SYSFS_PATH_MAX];
	ssize_t count = 0;
	fpga_result res = FPGA_EXCEPTION;
	struct stat objstat;

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(key);
	ASSERT_NOT_NULL(len);

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


fpga_result fpgaTokenGetObject(fpga_token token, const char *name,
			       fpga_object *object, int flags)
{
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = FPGA_EXCEPTION;

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(name);
	res = cat_token_sysfs_path(objpath, token, name);
	if (res) {
		return res;
	}

	return make_sysfs_object(objpath, name, object, flags, NULL);
}


fpga_result fpgaHandleGetObject(fpga_token handle, const char *name,
				fpga_object *object, int flags)
{
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = FPGA_EXCEPTION;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(name);
	res = cat_handle_sysfs_path(objpath, handle, name);
	if (res) {
		return res;
	}

	return make_sysfs_object(objpath, name, object, flags, handle);
}

fpga_result fpgaObjectGetObject(fpga_object parent, fpga_handle handle,
				const char *name, fpga_object *object,
				int flags)
{
	char objpath[SYSFS_PATH_MAX] = {0};
	fpga_result res = FPGA_EXCEPTION;
	ASSERT_NOT_NULL(parent);
	ASSERT_NOT_NULL(name);
	struct _fpga_object *_obj = (struct _fpga_object *)parent;
	if (_obj->type == FPGA_SYSFS_FILE) {
		return FPGA_INVALID_PARAM;
	}
	res = cat_sysfs_path(objpath, _obj->path);
	if (res) {
		return res;
	}
	res = cat_sysfs_path(objpath, "/");
	if (res) {
		return res;
	}

	res = cat_sysfs_path(objpath, name);
	if (res) {
		return res;
	}



	return make_sysfs_object(objpath, name, object, flags, handle);
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

fpga_result fpgaObjectRead64(fpga_object obj, uint64_t *value, int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	if (flags & FPGA_OBJECT_SYNC) {
		lseek(_obj->fd, 0, SEEK_SET);
		_obj->size = eintr_read(_obj->fd, _obj->buffer, _obj->max_size);
	}
	if (flags & FPGA_OBJECT_TEXT) {
		*value = strtoull((char *)_obj->buffer, NULL, 0);
	} else {
		*value = *(uint64_t*)_obj->buffer;
	}
	return FPGA_OK;
}

fpga_result fpgaObjectRead(fpga_object obj, uint8_t *buffer, size_t offset,
			   size_t len, int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(buffer);
	if (offset + len > _obj->size) {
		return FPGA_INVALID_PARAM;
	}

	if (flags & FPGA_OBJECT_SYNC) {
		lseek(_obj->fd, 0, SEEK_SET);
		_obj->size = eintr_read(_obj->fd, _obj->buffer, _obj->max_size);
	}
	if (offset + len > _obj->size) {
		FPGA_ERR("Bytes requested exceed object size");
		return FPGA_INVALID_PARAM;
	}
	memcpy_s(buffer, _obj->max_size, _obj->buffer + offset, len);

	return FPGA_OK;
}

fpga_result fpgaObjectWrite64(fpga_object obj, uint64_t value, int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	size_t bytes_written = 0;
	fpga_result res;
	errno_t err;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(_obj->handle);
	res = handle_check_and_lock(_obj->handle);
	if (res != FPGA_OK) {
		return res;
	}
	memset32_s((uint32_t *)_obj->buffer, _obj->size, 0);
	if (flags & FPGA_OBJECT_TEXT) {
		snprintf_s_l((char *)_obj->buffer, _obj->max_size, "%lux", value);
		_obj->size = (size_t)strlen((const char*)_obj->buffer);
	} else {
		_obj->size = sizeof(uint64_t);
		*(uint64_t*)_obj->buffer = value;
	}
	lseek(_obj->fd, 0, SEEK_SET);
	bytes_written = eintr_write(_obj->fd, _obj->buffer, _obj->size);
	if (bytes_written != _obj->size) {
		FPGA_ERR("Did not write 64-bit value");
		res = FPGA_EXCEPTION;
	}
	if ((err = pthread_mutex_unlock(&((struct _fpga_handle*)_obj->handle)->lock))) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(errno));
	}
	return res;
}
