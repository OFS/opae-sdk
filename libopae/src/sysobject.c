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
#include <unistd.h>

#include "common_int.h"
#include "log_int.h"
#include "sysfs_int.h"
#include "types_int.h"
#include "safe_string/safe_string.h"

#define NULL_CHECK(var)                                                        \
	do {                                                                   \
		if (var == NULL) {                                             \
			FPGA_MSG(#var " is NULL");                             \
		}                                                              \
	} while (false);

fpga_result cat_token_sysfs_path(char *dest, fpga_token token, const char *path)
{
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

fpga_result cat_handle_sysfs_path(char *dest, fpga_handle handle,
				  const char *path)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)(handle);
	return cat_token_sysfs_path(dest, _handle->token, path);
}

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

fpga_result __FPGA_API__ fpgaReadObjectBytes(fpga_token token, const char *key,
					     uint8_t *buffer, size_t offset,
					     size_t *len)
{
	int fd = -1;
	char objpath[SYSFS_PATH_MAX];
	ssize_t count = 0;
	fpga_result res = FPGA_EXCEPTION;

	NULL_CHECK(token);
	NULL_CHECK(key);
	NULL_CHECK(buffer);
	NULL_CHECK(len);

	res = cat_token_sysfs_path(objpath, token, key);
	if (res) {
		return res;
	}

	fd = open(objpath, O_RDONLY);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	count = pread(fd, buffer, *len, offset);
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

fpga_result __FPGA_API__ fpgaWriteObject32(fpga_handle handle, const char *key,
					   uint32_t value)
{
	NULL_CHECK(handle);
	NULL_CHECK(key);
	char objpath[SYSFS_PATH_MAX];
	fpga_result res = cat_handle_sysfs_path(objpath, handle, key);
	if (res) {
		return res;
	}

	return sysfs_write_u32(objpath, value);
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

fpga_result __FPGA_API__ fpgaWriteObjectBytes(fpga_handle handle,
					      const char *key, uint8_t *buffer,
					      size_t offset, size_t *len)
{
	char objpath[SYSFS_PATH_MAX];
	int fd = -1;
	ssize_t count = 0;
	fpga_result res = FPGA_EXCEPTION;

	NULL_CHECK(handle);
	NULL_CHECK(key);
	NULL_CHECK(buffer);
	NULL_CHECK(len);

	res = cat_handle_sysfs_path(objpath, handle, key);
	if (res) {
		return res;
	}
	fd = open(objpath, O_WRONLY);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	count = pwrite(fd, buffer, *len, offset);
	if (count < (ssize_t)*len) {
		if (count < 0) {
			FPGA_ERR("Error with pwrite operation: %s",
				 strerror(errno));
			res = FPGA_EXCEPTION;
		} else {
			FPGA_MSG(
				"Bytes written (%d) is less that requested (%d)",
				count, *len);
		}
		res = count == 0 ? FPGA_EXCEPTION : FPGA_OK;
	}
	*len = count;
	close(fd);
	return res;
}
