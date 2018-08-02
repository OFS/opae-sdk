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

static inline ssize_t eintr_pread(int fd, void *buf, size_t count, size_t offset)
{
	ssize_t bytes_read = 0, total_read = 0;
	char *ptr = buf;
	while (total_read < (ssize_t)count) {
		bytes_read = pread(fd, ptr + total_read, count, offset);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			}
		}
		total_read += bytes_read;
	}
	return total_read;
}

static inline ssize_t eintr_write(int fd, void *buf, size_t count)
{
	ssize_t bytes_written = 0, total_written = 0;
	char *ptr = buf;
	while (total_written < (ssize_t)count) {
		bytes_written = write(fd, ptr + total_written, count);
		if (bytes_written < 0) {
			if (errno == EINTR) {
				continue;
			}
		}
		total_written += bytes_written;
	}
	return total_written;
}

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
	if (fd_stat < 0){
		FPGA_ERR("Error with object path: %s", strerror(errno));
		if (errno == EACCES){
			return FPGA_NO_ACCESS;
		}
		return FPGA_EXCEPTION;
	}

	if (!buffer){
		*len = objstat.st_size - offset;
		return FPGA_OK;
	}


	fd = open(objpath, O_RDONLY);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	count = eintr_pread(fd, buffer, *len, offset);
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
					      size_t offset, size_t len)
{
	char objpath[SYSFS_PATH_MAX];
	int fd = -1;
	off_t fsize = 0;
	ssize_t bytes_written = 0, bytes_read = 0;
	fpga_result res = FPGA_EXCEPTION;
	char *rw_buffer = NULL;

	NULL_CHECK(handle);
	NULL_CHECK(key);
	NULL_CHECK(buffer);

	res = cat_handle_sysfs_path(objpath, handle, key);
	if (res) {
		return res;
	}
	fd = open(objpath, O_RDWR);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	// According to kernel docs on sysfs:
	//
	// When writing sysfs files, userspace processes should first read the
	// entire file, modify the values it wishes to change, then write the
	// entire buffer back.

	// Get the file size and allocate a buffer to read its contents into
	fsize = lseek(fd, 0, SEEK_END);
	if (fsize < 0) {
		FPGA_ERR("Error with lseek operation: %s", strerror(errno));
		res = FPGA_EXCEPTION;
		goto out_close;
	}

	// rewind the offset to 0
	if (lseek(fd, 0, SEEK_SET) < 0) {
		FPGA_ERR("Error with lseek operation: %s", strerror(errno));
		res = FPGA_EXCEPTION;
		goto out_close;
	}

	// check if this operation would go out of bounds
	if (offset + len > (size_t)fsize) {
		FPGA_ERR("Bytes to write exceed file size");
		res = FPGA_EXCEPTION;
		goto out_close;
	}


	rw_buffer = calloc(fsize, sizeof(char));
	if (rw_buffer == NULL) {
		res = FPGA_NO_MEMORY;
		goto out_close;
	}

	// read the contents of the sysfs object
	bytes_read = eintr_pread(fd, rw_buffer, fsize, 0);
	if (bytes_read < fsize) {
		res = FPGA_EXCEPTION;
		if (bytes_read < 0) {
			FPGA_ERR("Error with read operation: %s",
				 strerror(errno));
		} else {
			FPGA_ERR("Bytes read (%d) < object size (%d)",
				 bytes_read, fsize);
		}
		goto out_free;
	}

	// copy bytes to write to rw_write buffer
	memcpy_s(rw_buffer + offset, fsize - offset, buffer, len);

	// write modified buffer to sysfs object
	bytes_written = eintr_write(fd, rw_buffer, fsize);
	if (bytes_written < fsize) {
		res = FPGA_EXCEPTION;
		if (bytes_written < 0) {
			FPGA_ERR("Error with write operation: %s",
				 strerror(errno));
		} else {
			FPGA_MSG(
				"Bytes written (%d) is less that object size (%d)",
				bytes_written, fsize);
		}
		goto out_free;
	}

	res = FPGA_OK;

out_free:
	free(rw_buffer);

out_close:
	close(fd);
	return res;
}
