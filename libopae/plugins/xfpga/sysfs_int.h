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

#ifndef __FPGA_SYSFS_INT_H__
#define __FPGA_SYSFS_INT_H__

#include <opae/types.h>
#include <stdint.h>
#include <unistd.h>

#define SYSFS_PATH_MAX 256
#define SYSFS_FPGA_CLASS_PATH "/sys/class/fpga"

#define SYSFS_AFU_PATH_FMT "/intel-fpga-dev.%d/intel-fpga-port.%d"
#define SYSFS_FME_PATH_FMT "/intel-fpga-dev.%d/intel-fpga-fme.%d"

// substring that identifies a sysfs directory as the FME device.
#define FPGA_SYSFS_FME "fme"
// substring that identifies a sysfs directory as the AFU device.
#define FPGA_SYSFS_AFU "port"
// name of the FME interface ID (GUID) sysfs node.
#define FPGA_SYSFS_FME_INTERFACE_ID "pr/interface_id"
// name of the AFU GUID sysfs node.
#define FPGA_SYSFS_AFU_GUID "afu_id"
// name of the socket id sysfs node.
#define FPGA_SYSFS_SOCKET_ID "socket_id"
// name of the number of slots sysfs node.
#define FPGA_SYSFS_NUM_SLOTS "ports_num"
// name of the bitstream id sysfs node.
#define FPGA_SYSFS_BITSTREAM_ID "bitstream_id"

#ifdef __cplusplus
extern "C" {
#endif

int sysfs_initialize(void);
int sysfs_finalize(void);
int sysfs_region_count(void);

/**
 * @brief Get BBS interface id
 *
 * @param handle
 * @parm  Interface id low pointer
 * @parm  Interface id high pointer
 *
 * @return
 */
fpga_result get_interface_id(fpga_handle handle, uint64_t *id_l, uint64_t *id_h);

/*
 * sysfs utility functions.
 */
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
fpga_result get_fpga_deviceid(fpga_handle handle, uint64_t *deviceid);
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
fpga_result sync_object(fpga_object object);
fpga_result make_sysfs_group(char *sysfspath, const char *name,
			     fpga_object *object, int flags, fpga_handle handle);
fpga_result make_sysfs_object(char *sysfspath, const char *name,
			      fpga_object *object, int flags, fpga_handle handle);

fpga_result sysfs_write_u64_decimal(const char *path, uint64_t u);

#ifdef __cplusplus
}
#endif
#endif // ___FPGA_SYSFS_INT_H__
