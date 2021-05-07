// Copyright(c) 2021, Intel Corporation
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
#ifndef __FPGA_OPAE_UDEV_H__
#define __FPGA_OPAE_UDEV_H__
#include <libudev.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _dfl_error
{
	const char *attr;
	struct _dfl_error *next;
} dfl_error;

typedef struct _dfl_device
{
	struct udev_device *dev;
	struct udev_device *region;
	struct udev_device *pci;
	fpga_objtype type;
	uint32_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint32_t device_id;
	uint32_t vendor_id;
	uint32_t object_id;
	uint32_t numa_node;
	uint32_t num_errors;
	dfl_error *errors;
	struct _dfl_device *next;
} dfl_device;

int dfl_parse_guid(const char *guid_str, fpga_guid guid);
dfl_device *dfl_device_enum();
dfl_device *dfl_device_clone(dfl_device *e);
void dfl_device_destroy(dfl_device *e);
const char *dfl_device_get_pci_addr(dfl_device *e);
dfl_device *dfl_device_get_parent(dfl_device *e);
dev_t dfl_device_get_devnum(dfl_device *dev);
int dfl_device_get_afu_id(dfl_device *d, fpga_guid);
int dfl_device_get_compat_id(dfl_device *d, fpga_guid);
int dfl_device_get_ports_num(dfl_device *dev, uint32_t *value);
int dfl_device_get_bbs_id(dfl_device *dev, uint64_t *bbs_id);
int dfl_device_guid_match(dfl_device *dev, fpga_guid guid);
const char *dfl_device_get_attr(dfl_device *e, const char *attr);
int dfl_device_set_attr(dfl_device *e, const char *attr, const char *value);
int dfl_device_read_attr64(dfl_device *dev, const char *attr, uint64_t *value);
int dfl_device_write_attr64(dfl_device *dev, const char *attr, uint64_t value);

void udev_print_list(struct udev_list_entry *le);
#ifdef __cplusplus
}
#endif
#endif // __FPGA_OPAE_UDEV_H__ 
