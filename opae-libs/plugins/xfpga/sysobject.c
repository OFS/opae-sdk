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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "common_int.h"
#include "sysfs_int.h"
#include "types_int.h"
#include <opae/types_enum.h>
#include <opae/sysobject.h>
#include <opae/log.h>


#define VALIDATE_NAME(_N)                                       \
	do {                                                        \
		if (_N[0] == '.' || _N[0] == '/' || strstr(_N, "..")) { \
			OPAE_MSG("%s is not a valid input", _N);            \
			return FPGA_INVALID_PARAM;                          \
		}                                                       \
	} while (false);

fpga_result __XFPGA_API__ xfpga_fpgaTokenGetObject(fpga_token token, const char *name,
						  fpga_object *object, int flags)
{
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = FPGA_EXCEPTION;

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(name);
	VALIDATE_NAME(name);
	res = cat_token_sysfs_path(objpath, token, name);
	if (res) {
		return res;
	}

	return make_sysfs_object(objpath, name, object, flags, NULL);
}

fpga_result __XFPGA_API__
xfpga_fpgaHandleGetObject(fpga_token handle, const char *name,
						   fpga_object *object, int flags)
{
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = FPGA_EXCEPTION;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(name);
	VALIDATE_NAME(name);
	res = cat_handle_sysfs_path(objpath, handle, name);
	if (res) {
		return res;
	}

	return make_sysfs_object(objpath, name, object, flags, handle);
}

fpga_result __XFPGA_API__
xfpga_fpgaObjectGetObject(fpga_object parent, const char *name,
						   fpga_object *object, int flags)
{
	char objpath[SYSFS_PATH_MAX] = {0};
	fpga_result res = FPGA_EXCEPTION;
	ASSERT_NOT_NULL(parent);
	ASSERT_NOT_NULL(name);
	ASSERT_NOT_NULL(object);
	VALIDATE_NAME(name);
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


	return make_sysfs_object(objpath, name, object, flags, _obj->handle);
}

fpga_result __XFPGA_API__
xfpga_fpgaCloneObject(fpga_object src, fpga_object *dst)
{
	size_t i = 0;
	fpga_result res = FPGA_OK;
	ASSERT_NOT_NULL(src);
	ASSERT_NOT_NULL(dst);
	struct _fpga_object *_src = (struct _fpga_object *)src;
	struct _fpga_object *_dst = alloc_fpga_object(_src->path, _src->name);
	if (!_dst) {
		return FPGA_NO_MEMORY;
	}
	_dst->handle = _src->handle;
	_dst->perm = _src->perm;
	_dst->size = _src->size;
	_dst->type = _src->type;
	_dst->max_size = _src->max_size;
	if (_src->type == FPGA_SYSFS_FILE) {
		_dst->buffer = calloc(_dst->max_size, sizeof(uint8_t));
		memcpy(_dst->buffer, _src->buffer, _src->max_size);
	} else {
		_dst->buffer = NULL;
		_dst->objects = calloc(_src->size, sizeof(fpga_object));
		if (!_dst->objects) {
			res = FPGA_NO_MEMORY;
			goto out_err;
		}
		for (i = 0; i < _src->size; ++i) {
			res = xfpga_fpgaCloneObject(_src->objects[i],
						    &_dst->objects[i]);
			if (res) {
				_dst->size = i;
				goto out_err;
			}
		}
	}
	*dst = _dst;
	return res;
out_err:
	destroy_fpga_object(_dst);
	*dst = NULL;
	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectGetObjectAt(fpga_object parent,
						     size_t idx,
						     fpga_object *object)
{
	fpga_result res = FPGA_OK;
	ASSERT_NOT_NULL(parent);
	ASSERT_NOT_NULL(object);
	struct _fpga_object *_obj = (struct _fpga_object *)parent;
	if (pthread_mutex_lock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_lock() failed");
		return FPGA_EXCEPTION;
	}

	if (_obj->type == FPGA_SYSFS_FILE) {
		return FPGA_INVALID_PARAM;
	}
	if (idx >= _obj->size) {
		return FPGA_INVALID_PARAM;
	}
	res = xfpga_fpgaCloneObject(_obj->objects[idx], object);
	if (pthread_mutex_unlock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_unlock() failed");
	}
	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaDestroyObject(fpga_object *obj)
{
	fpga_result res = FPGA_OK;
	if (NULL == obj || NULL == *obj) {
		OPAE_MSG("Invalid object pointer");
		return FPGA_INVALID_PARAM;
	}
	struct _fpga_object *_obj = (struct _fpga_object *)*obj;
	if (pthread_mutex_lock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_lock() failed");
	}

	res = destroy_fpga_object(_obj);
	*obj = NULL;
	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectGetSize(fpga_object obj,
						 uint32_t *size,
						 int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	fpga_result res = FPGA_OK;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(size);
	if (flags & FPGA_OBJECT_SYNC && _obj->type == FPGA_SYSFS_FILE) {
		res = sync_object(obj);
		if (res) {
			return res;
		}
	}
	*size = _obj->size;
	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectRead64(fpga_object obj,
						uint64_t *value,
						int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	fpga_result res = FPGA_OK;
	if (_obj->type != FPGA_SYSFS_FILE) {
		return FPGA_INVALID_PARAM;
	}
	if (flags & FPGA_OBJECT_SYNC) {
		res = sync_object(obj);
	}
	if (res) {
		return res;
	}
	if (flags & FPGA_OBJECT_RAW) {
		*value = *(uint64_t *)_obj->buffer;
	} else {
		*value = strtoull((char *)_obj->buffer, NULL, 0);
	}
	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectRead(fpga_object obj,
					      uint8_t *buffer,
					      size_t offset,
					      size_t len,
					      int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	fpga_result res = FPGA_OK;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(buffer);
	if (_obj->type != FPGA_SYSFS_FILE) {
		return FPGA_INVALID_PARAM;
	}
	if (offset + len > _obj->size) {
		return FPGA_INVALID_PARAM;
	}

	if (flags & FPGA_OBJECT_SYNC) {
		res = sync_object(obj);
		if (res) {
			return res;
		}
	}
	if (offset + len > _obj->size) {
		OPAE_ERR("Bytes requested exceed object size");
		return FPGA_INVALID_PARAM;
	}
	memcpy(buffer, _obj->buffer + offset, len);

	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectWrite64(fpga_object obj,
						 uint64_t value,
						 int flags)
{
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	size_t bytes_written = 0;
	int fd = -1;
	fpga_result res = FPGA_OK;
	int err;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(_obj->handle);
	if (_obj->type != FPGA_SYSFS_FILE) {
		return FPGA_INVALID_PARAM;
	}
	res = handle_check_and_lock(_obj->handle);
	if (res != FPGA_OK) {
		return res;
	}
	if (_obj->max_size) {
		memset(_obj->buffer, 0, _obj->max_size);
	}
	if (flags & FPGA_OBJECT_RAW) {
		_obj->size = sizeof(uint64_t);
		*(uint64_t *)_obj->buffer = value;
	} else {
		snprintf((char *)_obj->buffer, _obj->max_size, "0x%" PRIx64,
			     value);
		_obj->size = (size_t)strlen((const char *)_obj->buffer);
	}
	fd = open(_obj->path, _obj->perm);
	if (fd < 0) {
		OPAE_ERR("Error opening %s: %s", _obj->path, strerror(errno));
		res = FPGA_EXCEPTION;
		goto out_unlock;
	}
	lseek(fd, 0, SEEK_SET);
	bytes_written = eintr_write(fd, _obj->buffer, _obj->size);
	if (bytes_written != _obj->size) {
		OPAE_ERR("Did not write 64-bit value: %s", strerror(errno));
		res = FPGA_EXCEPTION;
	}
out_unlock:
	if (fd >= 0)
		close(fd);
	err = pthread_mutex_unlock(
		&((struct _fpga_handle *)_obj->handle)->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(errno));
		res = FPGA_EXCEPTION;
	}
	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectGetType(fpga_object obj,
						 enum fpga_sysobject_type *type)
{
	fpga_result res = FPGA_OK;
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(type);
	if (pthread_mutex_lock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_lock() failed");
		return FPGA_EXCEPTION;
	}

	switch (_obj->type) {
	case FPGA_SYSFS_DIR:
	case FPGA_SYSFS_LIST:
		*type = FPGA_OBJECT_CONTAINER;
		break;
	case FPGA_SYSFS_FILE:
		*type = FPGA_OBJECT_ATTRIBUTE;
		break;
	default:
		res = FPGA_INVALID_PARAM;
	}

	if (pthread_mutex_unlock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_unlock() failed");
	}

	return res;
}

fpga_result __XFPGA_API__ xfpga_fpgaObjectGetName(fpga_object obj, char *name,
						 size_t max_len)
{
	fpga_result res = FPGA_OK;
	struct _fpga_object *_obj = (struct _fpga_object *)obj;
	size_t len;

	ASSERT_NOT_NULL(obj);
	ASSERT_NOT_NULL(name);

	if (pthread_mutex_lock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_lock() failed");
		return FPGA_EXCEPTION;
	}

	if (!_obj->name) {
		pthread_mutex_unlock(&_obj->lock);
		return FPGA_INVALID_PARAM;
	}

	len = strnlen(_obj->name, max_len - 1);
	strncpy(name, _obj->name, len + 1);

	if (pthread_mutex_unlock(&_obj->lock)) {
		OPAE_ERR("pthread_mutex_unlock() failed");
	}

	return res;
}
