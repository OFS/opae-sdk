// Copyright(c) 2017-2019, Intel Corporation
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

#ifndef __FPGA_SYSFS_INT_H__
#define __FPGA_SYSFS_INT_H__

#include <opae/types.h>
#include <stdint.h>
#include <unistd.h>

#include "types_int.h"

#define SYSFS_PATH_MAX 256


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _sysfs_fpga_device sysfs_fpga_device;

typedef struct _sysfs_fpga_region {
	sysfs_fpga_device *device;
	char sysfs_path[SYSFS_PATH_MAX];
	char sysfs_name[SYSFS_PATH_MAX];
	fpga_objtype type;
	int number;
} sysfs_fpga_region;

#define SYSFS_MAX_RESOURCES 4
typedef struct _sysfs_fpga_device {
	char sysfs_path[SYSFS_PATH_MAX];
	char sysfs_name[SYSFS_PATH_MAX];
	int number;
	sysfs_fpga_region *fme;
	sysfs_fpga_region *port;
  uint32_t segment;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint32_t device_id;
  uint32_t vendor_id;
} sysfs_fpga_device;

int sysfs_initialize(void);
int sysfs_finalize(void);
int sysfs_device_count(void);

typedef fpga_result (*device_cb)(const sysfs_fpga_device *device, void *context);
fpga_result sysfs_foreach_device(device_cb cb, void *context);

const sysfs_fpga_device *sysfs_get_device(size_t num);
int sysfs_parse_attribute64(const char *root, const char *attr_path, uint64_t *value);

fpga_result sysfs_get_fme_pr_interface_id(const char *sysfs_res_path, fpga_guid guid);

fpga_result sysfs_get_guid(fpga_token token, const char *sysfspath, fpga_guid guid);

fpga_result sysfs_get_fme_path(const char *rpath, char *fpath);

fpga_result sysfs_path_is_valid(const char *root, const char *attr_path);

/**
 * @brief Get BBS interface id
 *
 * @param handle
 * @parm  Interface id low pointer
 * @parm  Interface id high pointer
 *
 * @return
 */
fpga_result sysfs_get_interface_id(fpga_token token, fpga_guid guid);

/*
 * sysfs utility functions.
 */

fpga_result opae_glob_path(char *path);
fpga_result sysfs_sbdf_from_path(const char *sysfspath, int *s, int *b, int *d, int *f);
fpga_result sysfs_read_int(const char *path, int *i);
fpga_result sysfs_read_u32(const char *path, uint32_t *u);
fpga_result sysfs_read_u32_pair(const char *path, uint32_t *u1, uint32_t *u2,
				 char sep);
fpga_result sysfs_read_u64(const char *path, uint64_t *u);
fpga_result sysfs_write_u64(const char *path, uint64_t u);
fpga_result sysfs_read_guid(const char *path, fpga_guid guid);
fpga_result sysfs_get_socket_id(int dev, int subdev, uint8_t *socket_id);
fpga_result sysfs_get_afu_id(int dev, int subdev, fpga_guid guid);
fpga_result sysfs_get_pr_id(int dev, int subdev, fpga_guid guid);
fpga_result sysfs_get_slots(int dev, int subdev, uint32_t *slots);
fpga_result sysfs_get_bitstream_id(int dev, int subdev, uint64_t *id);
fpga_result get_port_sysfs(fpga_handle handle, char *sysfs_port);
enum fpga_hw_type opae_id_to_hw_type(uint16_t vendor_id, uint16_t device_id);
fpga_result get_fpga_hw_type(fpga_handle handle, enum fpga_hw_type *hw_type);
fpga_result sysfs_deviceid_from_path(const char *sysfspath,
				uint64_t *deviceid);
fpga_result sysfs_objectid_from_path(const char *sysfspath,
				     uint64_t *object_id);
ssize_t eintr_read(int fd, void *buf, size_t count);
ssize_t eintr_write(int fd, void *buf, size_t count);
fpga_result cat_token_sysfs_path(char *dest, fpga_token token,
				 const char *path);
fpga_result cat_sysfs_path(char *dest, const char *path);
fpga_result cat_handle_sysfs_path(char *dest, fpga_handle handle,
				  const char *path);
struct _fpga_object *alloc_fpga_object(const char *sysfspath, const char *name);
fpga_result destroy_fpga_object(struct _fpga_object *obj);
fpga_result sync_object(fpga_object object);
fpga_result make_sysfs_group(char *sysfspath, const char *name,
			     fpga_object *object, int flags, fpga_handle handle);
fpga_result make_sysfs_object(char *sysfspath, const char *name,
			      fpga_object *object, int flags, fpga_handle handle);

fpga_result sysfs_write_u64_decimal(const char *path, uint64_t u);

fpga_result sysfs_get_port_error_path(fpga_handle handle, char *sysfs_port_error);
fpga_result sysfs_get_port_clr_err_path(fpga_handle handle, char *sysfs_port_clrerr);
fpga_result sysfs_get_fme_pwr_path(fpga_token token, char *sysfs_pwr);
fpga_result sysfs_get_fme_temp_path(fpga_token token, char *sysfs_temp);
fpga_result sysfs_get_fme_perf_path(fpga_token token, char *sysfs_perf);
#ifdef __cplusplus
}
#endif
#endif // ___FPGA_SYSFS_INT_H__
